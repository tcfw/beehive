package virtio

import (
	"errors"
	"fmt"
	"strconv"
	"sync/atomic"
	"syscall"
	"time"
	"unsafe"

	"github.com/tcfw/kernel/services/go/fs"
	"github.com/tcfw/kernel/services/go/fs/drivers/block"
	"github.com/tcfw/kernel/services/go/utils"
)

func InitVirtioMMIODevice(info utils.DevInfo) error {
	name := fs.ReservedName("virtio")

	dev := &MMIODriver{
		devInfo: info,
	}

	fs.RegisterDevice(&fs.FSDevice{
		Name:        name,
		DeviceType:  fs.DeviceTypeBlock,
		BlockDriver: dev,
	})

	if err := dev.init(); err != nil {
		return err
	}

	return nil
}

type MMIODriver struct {
	devInfo      utils.DevInfo
	header       VirtioHeader
	isLegacy     bool
	legacyHeader VirtioLegacyHeader

	queues      []queue
	nextQueueID uint32
	stop        bool
}

type queue interface {
	isFull() bool
	size() uint32
	id() uint32
	enqueue(req *block.BlockDeviceIORequest, comp chan<- block.BlockRequestIOResponse, m *MMIODriver) error
	pickup() error
}

func (m *MMIODriver) init() error {
	if err := m.mapBar(); err != nil {
		return fmt.Errorf("loading BAR: %w", err)
	}

	if m.header.Magic() != virtioHeaderMagic {
		return errors.New("invalid virtio header magic")
	}

	if m.header.Version() == 0x1 {
		print("virtio legacy header\n")
		m.isLegacy = true
		m.legacyHeader = VirtioLegacyHeader(m.header)
		return m.legacyInit()
	} else if m.header.Version() != 0x2 {
		return errors.New("unsupported virtio version " + strconv.Itoa(int(m.header.Version())))
	}

	if m.header.SubsystemDeviceID() != virtioBlockDeviceID {
		return errors.New("virtio device was no a block device")
	}

	m.header.SetStatus(0)
	utils.MemoryBarrier()
	m.header.SetStatus(VirtioDeviceStatusAcknowledge) //ack device
	utils.MemoryBarrier()
	m.header.SetStatus(m.header.Status() | VirtioDeviceStatusDriver) //device supported
	utils.MemoryBarrier()

	m.header.DeviceFeaturesSel(0)
	m.header.DriverFeaturesSel(0)
	utils.MemoryBarrier()

	deviceFeatures := m.header.DeviceFeatures()
	disableFeature(&deviceFeatures, VirtioDeviceFreatureRing_Indirect_Desc)

	m.header.DriverFeatures(deviceFeatures)
	m.header.SetStatus(m.header.Status() | VirtioDeviceStatusFeaturesOK) //features supported
	utils.MemoryBarrier()

	if m.header.Status()|VirtioDeviceStatusFeaturesOK == 0 {
		return errors.New("device did not accetp features")
	}

	if err := m.addQueue(); err != nil {
		return fmt.Errorf("failed to add queue to device: %w", err)
	}

	m.header.SetStatus(m.header.Status() | VirtioDeviceStatusDriverOK) //ready for use
	utils.MemoryBarrier()

	go m.Watch()

	return nil
}

func (m *MMIODriver) StopWatch() {
	m.stop = true
}

func (m *MMIODriver) Watch() error {
	m.stop = false
	var status uint32

	for {
		for {
			if m.stop {
				return nil
			}

			//TODO(tcfw) this would usually be handled as a signal
			utils.MemoryBarrier()
			status = m.header.InterruptStatus()
			if status != 0 || m.queues[0].(*VirtioQueue128).Used.Idx != m.queues[0].(*VirtioQueue128).TailUsed {
				break
			}

			time.Sleep(250 * time.Microsecond)
		}

		m.header.InterruptACK(status)
		utils.MemoryBarrier()
		syscall.Syscall(syscall.SYS_DEV_IRQ_ACK, 0x4F, 0, 0)

		if status == 0x3 {
			//status change
			m.checkStatus()
			continue
		}

		for _, q := range m.queues {
			if err := q.pickup(); err != nil {
				return fmt.Errorf("picking up responses from queue %d: %w", q.id(), err)
			}
		}
	}

	return nil
}

func (m *MMIODriver) checkStatus() {
	status := m.header.Status()

	if status|VirtioDeviceStatusDeviceNeedsReset != 0 {
		print(fmt.Sprintf("device needs resetting!\n"))
		m.stop = true
	}
}

func (m *MMIODriver) legacyInit() error {
	if m.legacyHeader.SubsystemDeviceID() != virtioBlockDeviceID {
		return errors.New("virtio device was no a block device")
	}

	m.legacyHeader.SetStatus(0)                                                        //reset
	m.legacyHeader.SetStatus(VirtioDeviceStatusAcknowledge)                            //ack device
	m.legacyHeader.SetStatus(VirtioDeviceStatusAcknowledge | VirtioDeviceStatusDriver) //device supported

	m.legacyHeader.GuestPageSize(4096)

	feats := m.legacyHeader.DeviceFeatures()
	m.legacyHeader.DeviceFeaturesSel(feats) //ack all features

	if err := m.addLegacyQueue(); err != nil {
		return fmt.Errorf("adding new queue: %w", err)
	}

	utils.MemoryBarrier()

	m.legacyHeader.SetStatus(m.legacyHeader.Status() | VirtioDeviceStatusDriverOK)

	print(fmt.Sprintf("Status: %+v\n", m.legacyHeader.Status()))

	return nil
}

const size = 128

type VirtioQueue128 VirtioQueue[[]VirtioBlkReq, [size]uint16, [size]VirtqUsedElem]

func (q *VirtioQueue128) isFull() bool {
	return q.TailAvil-q.Used.Idx >= uint16(q.Num)
}

func (q *VirtioQueue128) size() uint32 {
	return q.Num
}

func (q *VirtioQueue128) id() uint32 {
	return q.ID
}

func (m *MMIODriver) addQueue() error {
	if m.isLegacy {
		return m.addLegacyQueue()
	}

	//reserve device areas
	reqBlockRaw, err := utils.MemMap(0, 0, uint64(size*VirtioBlkReqSize), utils.MMAP_DEVICE|utils.MMAP_READ|utils.MMAP_WRITE)
	if err != nil {
		return fmt.Errorf("allocation req area for queue: %w", err)
	}
	reqBlockRawPhy, err := utils.DevPhyAddr(uintptr(unsafe.Pointer(unsafe.SliceData(reqBlockRaw))))
	if err != nil {
		return fmt.Errorf("getting req area phy for queue: %w", err)
	}

	descBlockSize := uint64(size * VirtqDescSize)
	availBlockRawSize := size*VirtqAvailElemSize + VirtqAvailHeaderSize + VirtqAvailFooterSize
	usedBlockRawSize := size*VirtqUsedElemSize + VirtqUsedHeaderSize + VirtqUsedFooterSize

	descBlockRaw, err := utils.MemMap(0, 0, descBlockSize, utils.MMAP_DEVICE|utils.MMAP_READ|utils.MMAP_WRITE)
	if err != nil {
		return fmt.Errorf("allocation desc area for queue: %w", err)
	}

	desc := utils.ContiguousObjectArrayAsSlice[VirtqDesc](utils.ContiguousObjectArray{
		ObjectSize: VirtqDescSize,
		Arena:      descBlockRaw,
	})

	availBlockRaw, err := utils.MemMap(0, 0, uint64(availBlockRawSize), utils.MMAP_DEVICE|utils.MMAP_READ|utils.MMAP_WRITE)
	if err != nil {
		return fmt.Errorf("allocation desc area for queue: %w", err)
	}

	avail := utils.ContiguousObjectArrayAsT[VirtqAvail[[size]uint16]](utils.ContiguousObjectArray{
		Arena: availBlockRaw,
	})

	usedBlockRaw, err := utils.MemMap(0, 0, uint64(usedBlockRawSize), utils.MMAP_DEVICE|utils.MMAP_READ|utils.MMAP_WRITE)
	if err != nil {
		return fmt.Errorf("allocation desc area for queue: %w", err)
	}

	used := utils.ContiguousObjectArrayAsT[VirtqUsed[[size]VirtqUsedElem]](utils.ContiguousObjectArray{
		Arena: usedBlockRaw,
	})

	reqs := utils.ContiguousObjectArrayAsSlice[VirtioBlkReq](utils.ContiguousObjectArray{
		ObjectSize: VirtioBlkReqSize,
		Arena:      reqBlockRaw,
	})

	queue := &VirtioQueue128{
		Num:        size,
		Desc:       desc,
		Avail:      avail,
		Used:       used,
		Arena:      reqs,
		ArenaPhy:   reqBlockRawPhy,
		ArenaIdxCh: make(chan int, size),
		DescIdxCh:  make(chan int, size),
		ID:         atomic.AddUint32(&m.nextQueueID, 1) - 1,
	}

	//fill idx chans
	for i := 0; i < size; i++ {
		queue.ArenaIdxCh <- i
		queue.DescIdxCh <- i
	}

	// queue.Avail.Flags = VirtqAvailFlagNoInterrupt

	m.queues = append(m.queues, queue)

	return m.setupQueue(len(m.queues)-1, queue)
}

func (m *MMIODriver) setupQueue(id int, queue *VirtioQueue128) error {
	m.header.QueueSel(uint32(id))
	utils.MemoryBarrier()

	descPhy, err := utils.DevPhyAddr(uintptr(unsafe.Pointer(unsafe.SliceData(queue.Desc))))
	if err != nil {
		return fmt.Errorf("getting physical address of desc: %w", err)
	}

	availPhy, err := utils.DevPhyAddr(uintptr(unsafe.Pointer(queue.Avail)))
	if err != nil {
		return fmt.Errorf("getting physical address of desc: %w", err)
	}

	usedPhy, err := utils.DevPhyAddr(uintptr(unsafe.Pointer(queue.Used)))
	if err != nil {
		return fmt.Errorf("getting physical address of desc: %w", err)
	}

	m.header.SetQueueNum(queue.Num)
	m.header.SetQueueDesc(uint64(descPhy))
	m.header.SetQueueAvail(uint64(availPhy))
	m.header.SetQueueUsed(uint64(usedPhy))

	utils.MemoryBarrier()
	m.header.SetQueueReady(1)
	utils.MemoryBarrier()

	return nil
}

func (m *MMIODriver) addLegacyQueue() error {
	arena, err := utils.MemMap(0, 0, uint64(VirtioLegacyQueue128Size), utils.MMAP_DEVICE|utils.MMAP_READ|utils.MMAP_WRITE)
	if err != nil {
		return fmt.Errorf("allocating region for legacy queue", err)
	}

	queue := (*VirtioLegacyQueue128)(unsafe.Pointer(unsafe.SliceData(arena)))
	queue.ID = atomic.AddUint32(&m.nextQueueID, 1) - 1
	queue.Aidx = 0
	queue.Uidx = 0

	m.queues = append(m.queues, queue)

	if err := m.setupLegacyQueue(len(m.queues)-1, queue); err != nil {
		return fmt.Errorf("failed to setup legacy queue: %w", err)
	}

	return nil
}

func (m *MMIODriver) setupLegacyQueue(id int, queue *VirtioLegacyQueue128) error {
	addr, err := utils.DevPhyAddr(uintptr(unsafe.Pointer(queue)))
	if err != nil {
		return fmt.Errorf("failed to get phy addr of memmap: %w", err)
	}

	m.legacyHeader.QueueSel(uint32(id))
	m.legacyHeader.QueueNum(128)
	m.legacyHeader.QueueAlign(4096)

	m.legacyHeader.SetQueuePFN(uint32(addr))

	return nil
}

func (m *MMIODriver) getConfig() *VirtioBlkConfig {
	return (*VirtioBlkConfig)(unsafe.Add(unsafe.Pointer(unsafe.SliceData(m.header)), 0x100))
}

func (m *MMIODriver) mapBar() error {
	pageSize, err := utils.PageSize()
	if err != nil {
		return err
	}

	phybar := uintptr(m.devInfo.PhyBar)
	if phybar%uintptr(pageSize) != 0 {
		//round down
		phybar -= phybar % uintptr(pageSize)
	}

	size := m.devInfo.PhyBarSize
	if size < uint64(pageSize) {
		size = uint64(pageSize)
	}
	if size%uint64(pageSize) != 0 {
		//round up
		size += uint64(pageSize) - size%uint64(pageSize)
	}

	block, err := utils.MemMap(phybar, 0, size, utils.MMAP_DEVICE|utils.MMAP_WRITE|utils.MMAP_READ)
	if err != nil {
		return err
	}

	m.header = VirtioHeader(block[m.devInfo.PhyBar-uint64(phybar) : size : size])

	return nil
}

func (m *MMIODriver) BlockSize() uint64 {
	return uint64(m.getConfig().BlkSize)
}

func (m *MMIODriver) IsBusy() bool {
	utils.MemoryBarrier()

	for _, q := range m.queues {
		if q.isFull() {
			return true
		}
	}

	return false
}

func (m *MMIODriver) QueueSize() uint64 {
	maxSize := uint64(0)

	for _, q := range m.queues {
		maxSize += uint64(q.size() / 3)
	}

	return maxSize
}

func (m *MMIODriver) hasQueues() bool {
	return len(m.queues) != 0
}

func (m *MMIODriver) Enqueue(reqs []block.BlockDeviceIORequest, comp chan<- block.BlockRequestIOResponse) (error, int) {
	if !m.hasQueues() {
		return errors.New("device not ready"), 0
	}

	processed := 0

reqs:
	for _, r := range reqs {
		for _, q := range m.queues {
			if !q.isFull() {
				if err := q.enqueue(&r, comp, m); err != nil {
					return err, processed
				}

				processed++
				continue reqs
			}
		}
	}

	return nil, processed
}

func (q *VirtioQueue128) enqueue(req *block.BlockDeviceIORequest, comp chan<- block.BlockRequestIOResponse, m *MMIODriver) error {
	if req.Offset%m.BlockSize() != 0 {
		return errors.New("offset not aligned to block size")
	}

	if req.Offset+req.Length > m.getConfig().Capacity {
		return errors.New("req overflows device capacity")
	}

	arenaIdx := <-q.ArenaIdxCh
	arena := &q.Arena[arenaIdx]

	switch req.RequestType {
	case block.IORequestTypeRead:
		arena.Type = VirtioBlkReqTypeIn
		break
	case block.IORequestTypeWrite:
		arena.Type = VirtioBlkReqTypeOut
		break
	case block.IORequestTypeFlush:
		arena.Type = VirtioBlkReqTypeFlush
	case block.IORequestTypeTrim:
		arena.Type = VirtioBlkReqTypeWrite_Zeroes
	default:
		return fmt.Errorf("unknown request type")
	}

	arena.Sector = req.Offset / m.BlockSize()
	arena.waiter = comp
	arena.req = req
	arena.Status = 0

	reqLen := req.Length
	if reqLen == 0 {
		reqLen = m.BlockSize()
	}

	descIdx1 := <-q.DescIdxCh
	descIdx2 := <-q.DescIdxCh
	descIdx3 := <-q.DescIdxCh

	q.Desc[descIdx1].Addr = uint64(q.ArenaPhy + uintptr(arenaIdx*VirtioBlkReqSize))
	q.Desc[descIdx1].Len = uint32(VirtioBlkReqHeaderSize)
	q.Desc[descIdx1].Flags = VirtqDescFlagNext

	q.Desc[descIdx2].Addr = q.Desc[descIdx1].Addr + VirtioBlkReqHeaderSize
	q.Desc[descIdx2].Len = uint32(reqLen)
	q.Desc[descIdx2].Flags = VirtqDescFlagNext

	if arena.Type == VirtioBlkReqTypeIn {
		q.Desc[descIdx2].Flags |= VirtqDescFlagWrite
	}

	q.Desc[descIdx3].Addr = q.Desc[descIdx2].Addr + 512
	q.Desc[descIdx3].Len = uint32(VirtioBlkReqFooterSize)
	q.Desc[descIdx3].Flags = VirtqDescFlagWrite

	q.Desc[descIdx1].Next = uint16(descIdx2)
	q.Desc[descIdx2].Next = uint16(descIdx3)

	q.Avail.Ring[q.Avail.Idx%uint16(q.Num)] = uint16(descIdx1)
	q.Avail.Idx++

	utils.MemoryBarrier()
	m.header.QueueNotify(q.id())
	utils.MemoryBarrier()

	return nil
}

func (q *VirtioQueue128) pickup() error {
	if q.TailUsed == q.Used.Idx {
		// print(fmt.Sprintf("nothing found in queue tail=0x%X head=0x%X\n", q.TailUsed, q.Used.Idx))
		return nil
	}

	for q.TailUsed != q.Used.Idx {
		descRef := q.Used.Ring[q.TailUsed%uint16(q.Num)]
		desc := q.Desc[descRef.Id]

		arenaOffset := int(desc.Addr-uint64(q.ArenaPhy)) / VirtioBlkReqSize
		arena := q.Arena[arenaOffset]
		resp := block.BlockRequestIOResponse{
			Req: arena.req,
		}

		switch arena.Status {
		case VirtioBlkStatusUnsupp:
			resp.Err = block.ErrBlockOperationNotSupported
			break
		case VirtioBlkStatusIOErr:
			resp.Err = block.ErrBlockIOError
			break
		case VirtioBlkStatusOk:
			copy(resp.Req.Data, arena.Data[:])
			resp.Err = nil
		default:
			resp.Err = block.ErrBlockUnknownResponse
		}

		if arena.waiter != nil {
			arena.waiter <- resp
		}

		q.ArenaIdxCh <- arenaOffset

		if desc.Next != 0 && desc.Flags|VirtqDescFlagNext != 0 {
			desc1 := q.Desc[desc.Next]
			if desc1.Flags|VirtqDescFlagNext != 0 && desc1.Next != 0 {
				q.DescIdxCh <- int(desc1.Next)
			}
			q.DescIdxCh <- int(desc.Next)
		}
		q.DescIdxCh <- int(descRef.Id)

		q.TailUsed++
	}

	return nil
}

func (queue *VirtioLegacyQueue128) enqueue(req *block.BlockDeviceIORequest, comp chan<- block.BlockRequestIOResponse, m *MMIODriver) error {
	return errors.New("not implemented")
}

func (queue *VirtioLegacyQueue128) pickup() error {
	return errors.New("not implemented")

}
