package virtio

import (
	"errors"
	"fmt"
	"strconv"
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

	queues       []*VirtioQueue128
	legacyQueues []*VirtioLegacyQueue128
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

	arenaIdx := <-m.queues[0].ArenaIdxCh
	descIdx1 := <-m.queues[0].DescIdxCh
	descIdx2 := <-m.queues[0].DescIdxCh
	descIdx3 := <-m.queues[0].DescIdxCh

	m.queues[0].Arena[arenaIdx].Type = VirtioBlkReqTypeIn
	m.queues[0].Arena[arenaIdx].Sector = 63

	m.queues[0].Desc[descIdx1].Addr = uint64(m.queues[0].ArenaPhy + uintptr(arenaIdx*VirtioBlkReqSize))
	m.queues[0].Desc[descIdx1].Len = uint32(VirtioBlkReqHeaderSize)
	m.queues[0].Desc[descIdx1].Flags = VirtqDescFlagNext

	m.queues[0].Desc[descIdx2].Addr = m.queues[0].Desc[descIdx1].Addr + VirtioBlkReqHeaderSize
	m.queues[0].Desc[descIdx2].Len = 512
	m.queues[0].Desc[descIdx2].Flags = VirtqDescFlagWrite | VirtqDescFlagNext

	m.queues[0].Desc[descIdx3].Addr = m.queues[0].Desc[descIdx2].Addr + 512
	m.queues[0].Desc[descIdx3].Len = uint32(VirtioBlkReqFooterSize)
	m.queues[0].Desc[descIdx3].Flags = VirtqDescFlagWrite

	m.queues[0].Desc[descIdx1].Next = uint16(descIdx2)
	m.queues[0].Desc[descIdx2].Next = uint16(descIdx3)

	m.queues[0].Avail.Ring[0] = uint16(descIdx1)
	m.queues[0].Avail.Idx++

	utils.MemoryBarrier()
	m.header.QueueNotify(0)
	utils.MemoryBarrier()

	for {
		//TODO(tcfw) this would usually be handled as a signal
		status := m.header.InterruptStatus()
		if status != 0 {
			break
		}
		utils.MemoryBarrier()
	}

	m.header.InterruptACK(m.header.InterruptStatus())
	utils.MemoryBarrier()

	print(fmt.Sprintf("buf=%X status=0x%X config=%+v\n", m.queues[0].Arena[arenaIdx].Data, m.header.Status(), *m.getConfig()))

	return nil
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
	}

	//fill idx chans
	for i := 0; i < size; i++ {
		queue.ArenaIdxCh <- i
		queue.DescIdxCh <- i
	}

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
	m.legacyQueues = append(m.legacyQueues, queue)

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
	return true
}

func (m *MMIODriver) QueueSize() uint64 {
	maxSize := uint64(0)

	for _, q := range m.queues {
		maxSize += uint64(q.Num / 3)
	}

	for _, q := range m.legacyQueues {
		maxSize += uint64(len(q.Desc) / 3)
	}

	return maxSize
}

func (m *MMIODriver) Enqueue(reqs []block.BlockDeviceIORequest, comp chan<- *block.BlockDeviceIORequest) error {
	return nil
}
