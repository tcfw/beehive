package virtio

import (
	"encoding/binary"
	"math"
	"sync"
	"unsafe"

	"github.com/tcfw/kernel/services/go/fs/drivers"
	"github.com/tcfw/kernel/services/go/fs/drivers/block"
)

const (
	virtioHeaderMagic   = uint32(0x74726976)
	virtioBlockDeviceID = uint32(2)
)

type VirtioDeviceStatus uint32

const (
	VirtioDeviceStatusAcknowledge      VirtioDeviceStatus = 1
	VirtioDeviceStatusDriver           VirtioDeviceStatus = 2
	VirtioDeviceStatusFailed           VirtioDeviceStatus = 128
	VirtioDeviceStatusFeaturesOK       VirtioDeviceStatus = 8
	VirtioDeviceStatusDriverOK         VirtioDeviceStatus = 4
	VirtioDeviceStatusDeviceNeedsReset VirtioDeviceStatus = 64
)

type VirtioDeviceFeature uint32

const (
	VirtioBlockDeviceFeatureBarrier      VirtioDeviceFeature = 1 << 0  //(Legacy) Device supports request barriers.
	VirtioBlockDeviceFeatureSize_Max     VirtioDeviceFeature = 1 << 1  //Maximum size of any single segment is in size_max.
	VirtioBlockDeviceFeatureSeg_Max      VirtioDeviceFeature = 1 << 2  //Maximum number of segments in a request is in seg_max.
	VirtioBlockDeviceFeatureGeometry     VirtioDeviceFeature = 1 << 4  //Disk-style geometry specified in geometry.
	VirtioBlockDeviceFeatureRo           VirtioDeviceFeature = 1 << 5  //Device is read-only.
	VirtioBlockDeviceFeatureBlk_Size     VirtioDeviceFeature = 1 << 6  //Block size of disk is in blk_size.
	VirtioBlockDeviceFeatureScsi         VirtioDeviceFeature = 1 << 7  //(Legacy) Device supports scsi packet commands.
	VirtioBlockDeviceFeatureFlush        VirtioDeviceFeature = 1 << 9  //Cache flush command support.
	VirtioBlockDeviceFeatureTopology     VirtioDeviceFeature = 1 << 10 //Device exports information on optimal I/O alignment.
	VirtioBlockDeviceFeatureConfig_Wce   VirtioDeviceFeature = 1 << 11 //Device can toggle its cache between writeback and writethrough modes.
	VirtioBlockDeviceFeatureDiscard      VirtioDeviceFeature = 1 << 13 //Device can support discard command, maximum discard sectors size in max_discard_sectors and maximum discard segment number in max_discard_seg.
	VirtioBlockDeviceFeatureWrite_Zeroes VirtioDeviceFeature = 1 << 14 //Device can support write zeroes command, maximum write zeroes sectors size in max_write_zeroes_sectors and maximum write zeroes segment number in max_write_zeroes_seg.

	VirtioDeviceFreatureRing_Indirect_Desc VirtioDeviceFeature = 1 << 28 //Negotiating this feature indicates that the driver can use descriptors with the VIRTQ_DESC_F_INDIRECT flag set, as described in 2.6.5.3 Indirect Descriptors and 2.7.7 Indirect Flag: Scatter-Gather Support.
	VirtioDeviceFreatureRing_Event_Idx     VirtioDeviceFeature = 1 << 29 //This feature enables the used_event and the avail_event fields as described in 2.6.7, 2.6.8 and 2.7.10.

	//FeatureSel 0x1 required:
	VirtioDeviceFreatureVersion_1         VirtioDeviceFeature = 1 << (32 - 32) //This indicates compliance with this specification, giving a simple way to detect legacy devices or drivers.
	VirtioDeviceFreatureAccess_Platform   VirtioDeviceFeature = 1 << (33 - 32) //This feature indicates that the device can be used on a platform where device access to data in memory is limited and/or translated. E.g. this is the case if the device can be located behind an IOMMU that translates bus addresses from the device into physical addresses in memory, if the device can be limited to only access certain memory addresses or if special commands such as a cache flush can be needed to synchronise data in memory with the device. Whether accesses are actually limited or translated is described by platform-specific means. If this feature bit is set to 0, then the device has same access to memory addresses supplied to it as the driver has. In particular, the device will always use physical addresses matching addresses used by the driver (typically meaning physical addresses used by the CPU) and not translated further, and can access any address supplied to it by the driver. When clear, this overrides any platform-specific description of whether device access is limited or translated in any way, e.g. whether an IOMMU may be present.
	VirtioDeviceFreatureRing_Packed       VirtioDeviceFeature = 1 << (34 - 32) //This feature indicates support for the packed virtqueue layout as described in 2.7 Packed Virtqueues.
	VirtioDeviceFreatureIn_Order          VirtioDeviceFeature = 1 << (35 - 32) //This feature indicates that all buffers are used by the device in the same order in which they have been made available.
	VirtioDeviceFreatureOrder_Platform    VirtioDeviceFeature = 1 << (36 - 32) //This feature indicates that memory accesses by the driver and the device are ordered in a way described by the platform.
	VirtioDeviceFreatureSr_Iov            VirtioDeviceFeature = 1 << (37 - 32) //This feature indicates that the device supports Single Root I/O Virtualization. Currently only PCI devices support this feature.
	VirtioDeviceFreatureNotification_Data VirtioDeviceFeature = 1 << (38 - 32) //This feature indicates that the driver passes extra data (besides identifying the virtqueue) in its device notifications. See 2.7.23 Driver notifications.
)

func disableFeature(en *VirtioDeviceFeature, df VirtioDeviceFeature) {
	*en &= ^df
}

type VirtioBlkConfig struct {
	Capacity uint64
	SizeMax  uint32
	SegMax   uint32
	Geometry struct {
		Cylinders uint16
		Heads     uint8
		Sectors   uint8
	}
	BlkSize  uint32
	Topology struct {
		// # of logical blocks per physical block (log2)
		PhysicalBlockExp uint8
		// offset of first aligned logical block
		AlignmentOffset uint8
		// suggested minimum I/O size in blocks
		MinIoSize uint16
		// optimal (suggested maximum) I/O size in blocks
		OptIoSize uint32
	}
	Writeback              uint8
	_unused0               [3]uint8
	MaxDiscardSectors      uint32
	MaxDiscardSeg          uint32
	DiscardSectorAlignment uint32
	MaxWriteZeroesSectors  uint32
	MaxWriteZeroesSeg      uint32
	WriteZeroesMayUnmap    uint8
	_unused1               [3]uint8
}

func init() {
	drivers.RegisterDriver("virtio,mmio,block-device", InitVirtioMMIODevice)
}

type VirtioHeader []byte

func (v VirtioHeader) Magic() uint32 {
	return binary.LittleEndian.Uint32(v) //Offset=0x0
}

func (v VirtioHeader) Version() uint32 {
	return binary.LittleEndian.Uint32(v[0x4:]) //Offset=0x04
}

func (v VirtioHeader) SubsystemDeviceID() uint32 {
	return binary.LittleEndian.Uint32(v[0x08:]) //Offset=0x08
}

func (v VirtioHeader) SubsystemVendorID() uint32 {
	return binary.LittleEndian.Uint32(v[0x0c:]) //Offset=0x0c
}

func (v VirtioHeader) DeviceFeatures() VirtioDeviceFeature {
	return VirtioDeviceFeature(binary.LittleEndian.Uint32(v[0x10:])) //Offset=0x10
}

func (v VirtioHeader) DeviceFeaturesSel(d VirtioDeviceFeature) {
	binary.LittleEndian.PutUint32(v[0x14:], uint32(d)) //Offset=0x14
}

func (v VirtioHeader) DriverFeatures(d VirtioDeviceFeature) {
	binary.LittleEndian.PutUint32(v[0x20:], uint32(d)) //Offset=0x20
}

func (v VirtioHeader) DriverFeaturesSel(d uint32) {
	binary.LittleEndian.PutUint32(v[0x24:], d) //Offset=0x24
}

func (v VirtioHeader) QueueSel(d uint32) {
	binary.LittleEndian.PutUint32(v[0x30:], d) //Offset=0x30
}

func (v VirtioHeader) QueueNumMax() uint32 {
	return binary.LittleEndian.Uint32(v[0x24:]) //Offset=0x24
}

func (v VirtioHeader) SetQueueNum(d uint32) {
	binary.LittleEndian.PutUint32(v[0x38:], d) //Offset=0x38
}

func (v VirtioHeader) QueueReady() uint32 {
	return binary.LittleEndian.Uint32(v[0x44:]) //Offset=0x44
}

func (v VirtioHeader) SetQueueReady(d uint32) {
	binary.LittleEndian.PutUint32(v[0x44:], d) //Offset=0x44
}

func (v VirtioHeader) QueueNotify(d uint32) {
	binary.LittleEndian.PutUint32(v[0x50:], d) //Offset=0x50
}

func (v VirtioHeader) InterruptStatus() uint32 {
	return binary.LittleEndian.Uint32(v[0x60:]) //Offset=0x60
}

func (v VirtioHeader) InterruptACK(d uint32) {
	binary.LittleEndian.PutUint32(v[0x64:], d) //Offset=0x64
}

func (v VirtioHeader) Status() VirtioDeviceStatus {
	return VirtioDeviceStatus(binary.LittleEndian.Uint32(v[0x70:])) //Offset=0x70
}

func (v VirtioHeader) SetStatus(d VirtioDeviceStatus) {
	binary.LittleEndian.PutUint32(v[0x70:], uint32(d)) //Offset=0x70
}
func (v VirtioHeader) SetQueueDesc(dd uint64) {
	v.SetQueueDescLow(uint32(dd))
	v.SetQueueDescHigh(uint32(dd >> 32))
}

func (v VirtioHeader) SetQueueDescLow(d uint32) {
	binary.LittleEndian.PutUint32(v[0x80:], d) //Offset=0x80
}

func (v VirtioHeader) SetQueueDescHigh(d uint32) {
	binary.LittleEndian.PutUint32(v[0x84:], d) //Offset=0x84
}
func (v VirtioHeader) SetQueueAvail(dd uint64) {
	v.SetQueueAvailLow(uint32(dd))
	v.SetQueueAvailHigh(uint32(dd >> 32))
}

func (v VirtioHeader) SetQueueAvailLow(d uint32) {
	binary.LittleEndian.PutUint32(v[0x90:], d) //Offset=0x90
}

func (v VirtioHeader) SetQueueAvailHigh(d uint32) {
	binary.LittleEndian.PutUint32(v[0x94:], d) //Offset=0x94
}
func (v VirtioHeader) SetQueueUsed(dd uint64) {
	v.SetQueueUsedLow(uint32(dd))
	v.SetQueueUsedHigh(uint32(dd >> 32))
}

func (v VirtioHeader) SetQueueUsedLow(d uint32) {
	binary.LittleEndian.PutUint32(v[0xa0:], d) //Offset=0xa0
}

func (v VirtioHeader) SetQueueUsedHigh(d uint32) {
	binary.LittleEndian.PutUint32(v[0xa4:], d) //Offset=0xa4
}

func (v VirtioHeader) ConfigGeneration() uint32 {
	return binary.LittleEndian.Uint32(v[0xfc:]) //Offset=0xfc
}

// Legacy

type VirtioLegacyHeader []byte

func (v VirtioLegacyHeader) Magic() uint32 {
	return binary.LittleEndian.Uint32(v) //Offset=0x0
}

func (v VirtioLegacyHeader) Version() uint32 {
	return binary.LittleEndian.Uint32(v[0x4:]) //Offset=0x04
}

func (v VirtioLegacyHeader) SubsystemDeviceID() uint32 {
	return binary.LittleEndian.Uint32(v[0x08:]) //Offset=0x08
}

func (v VirtioLegacyHeader) SubsystemVendorID() uint32 {
	return binary.LittleEndian.Uint32(v[0x0c:]) //Offset=0x0c
}

func (v VirtioLegacyHeader) DeviceFeatures() uint32 {
	return binary.LittleEndian.Uint32(v[0x10:]) //Offset=0x10
}

func (v VirtioLegacyHeader) DeviceFeaturesSel(d uint32) {
	binary.LittleEndian.PutUint32(v[0x14:], d) //Offset=0x14
}

func (v VirtioLegacyHeader) DriverFeatures(d uint32) {
	binary.LittleEndian.PutUint32(v[0x20:], d) //Offset=0x20
}

func (v VirtioLegacyHeader) DriverFeaturesSel(d uint32) {
	binary.LittleEndian.PutUint32(v[0x24:], d) //Offset=0x24
}

func (v VirtioLegacyHeader) GuestPageSize(d uint32) {
	binary.LittleEndian.PutUint32(v[0x028:], d) //Offset=0x028
}

func (v VirtioLegacyHeader) QueueSel(d uint32) {
	binary.LittleEndian.PutUint32(v[0x030:], d) //Offset=0x030
}

func (v VirtioLegacyHeader) QueueNumMax() uint32 {
	return binary.LittleEndian.Uint32(v[0x034:]) //Offset=0x034
}

func (v VirtioLegacyHeader) QueueNum(d uint32) {
	binary.LittleEndian.PutUint32(v[0x038:], d) //Offset=0x038
}

func (v VirtioLegacyHeader) QueueAlign(d uint32) {
	binary.LittleEndian.PutUint32(v[0x03c:], d) //Offset=0x03c
}

func (v VirtioLegacyHeader) QueuePFN() uint32 {
	return binary.LittleEndian.Uint32(v[0x040:]) //Offset=0x040
}

func (v VirtioLegacyHeader) SetQueuePFN(d uint32) {
	binary.LittleEndian.PutUint32(v[0x040:], d) //Offset=0x040
}

func (v VirtioLegacyHeader) QueueNotify(d uint32) {
	binary.LittleEndian.PutUint32(v[0x050:], d) //Offset=0x050
}

func (v VirtioLegacyHeader) InterruptStatus() uint32 {
	return binary.LittleEndian.Uint32(v[0x60:]) //Offset=0x60
}

func (v VirtioLegacyHeader) InterruptACK(d uint32) {
	binary.LittleEndian.PutUint32(v[0x064:], d) //Offset=0x064
}

func (v VirtioLegacyHeader) Status() VirtioDeviceStatus {
	return VirtioDeviceStatus(binary.LittleEndian.Uint32(v[0x070:])) //Offset=0x070
}

func (v VirtioLegacyHeader) SetStatus(d VirtioDeviceStatus) {
	binary.LittleEndian.PutUint32(v[0x070:], uint32(d)) //Offset=0x070
}

type VirtioQueue[AA any, AT any, UT any] struct {
	Num uint32
	ID  uint32

	Desc  []VirtqDesc
	Avail *VirtqAvail[AT]
	Used  *VirtqUsed[UT]

	TailAvil uint16
	TailUsed uint16

	Arena      AA
	ArenaPhy   uintptr
	ArenaIdxCh chan int
	DescIdxCh  chan int
	Mu         sync.Mutex
}

const (
	VirtioLegacyQueue128Size = int(unsafe.Sizeof(VirtioLegacyQueue128{}))
)

type VirtioLegacyQueue128 struct {
	Desc  [128]VirtqDesc
	Avail VirtqAvail[[128]uint16]
	_     [3066]uint8
	Used  VirtqUsed[[128]VirtqUsedElem]

	Aidx uint16
	Uidx uint16
	ID   uint32
}

func (q *VirtioLegacyQueue128) isFull() bool {
	return q.Aidx-q.Avail.Idx >= uint16(len(q.Desc))
}

func (q *VirtioLegacyQueue128) size() uint32 {
	return 128
}

func (q *VirtioLegacyQueue128) id() uint32 {
	return q.ID
}

const (
	VirtioBlkReqSize       = int(unsafe.Sizeof(VirtioBlkReq{}))
	VirtioBlkReqHeaderSize = 16
	VirtioBlkReqFooterSize = 1
	VirtqDescSize          = int(unsafe.Sizeof(VirtqDesc{}))
)

type VirtqDescFlag uint16

const (
	VirtqDescFlagNext     VirtqDescFlag = 1 // This marks a buffer as continuing via the next field.
	VirtqDescFlagWrite    VirtqDescFlag = 2 // This marks a buffer as device write-only (otherwise device read-only).
	VirtqDescFlagIndirect VirtqDescFlag = 4 // This means the buffer contains a list of buffer descriptors.
)

type VirtqDesc struct {
	/* Address (guest-physical). */
	Addr uint64
	/* Length. */
	Len uint32
	/* The flags as indicated above. */
	Flags VirtqDescFlag
	/* Next field if flags & NEXT */
	Next uint16
}

type VirtqUsedFlag uint16

const (
	VirtqUsedFlagNoNotify VirtqUsedFlag = 1

	VirtqUsedHeaderSize = 2 * 2 //sizeof(uint16)*2
	VirtqUsedFooterSize = 1 * 2 //sizeof(uint16)*1
	VirtqUsedElemSize   = int(unsafe.Sizeof(VirtqUsedElem{}))
)

type VirtqUsed[T any] struct {
	Flags      VirtqUsedFlag
	Idx        uint16
	Ring       T
	AvailEvent uint16 /* Only if VIRTIO_F_EVENT_IDX */
}

/* uint32 is used here for ids for padding reasons. */
type VirtqUsedElem struct {
	/* Index of start of used descriptor chain. */
	Id uint32
	/* Total length of the descriptor chain which was used (written to) */
	Len uint32
}

type VirtqAvailFlag uint16

const (
	VirtqAvailFlagNoInterrupt VirtqAvailFlag = 1

	VirtqAvailHeaderSize = 2 * 2 //sizeof(uint16)*2
	VirtqAvailFooterSize = 1 * 2 //sizeof(uint16)*1
	VirtqAvailElemSize   = int(unsafe.Sizeof(uint16(0)))
)

type VirtqAvail[T any] struct {
	Flags     VirtqAvailFlag
	Idx       uint16
	Ring      T
	UsedEvent uint16 /* Only if VIRTIO_F_EVENT_IDX */
}

type VirtioBlkReqType uint32

const (
	VirtioBlkReqTypeIn           VirtioBlkReqType = 0
	VirtioBlkReqTypeOut          VirtioBlkReqType = 1
	VirtioBlkReqTypeFlush        VirtioBlkReqType = 4
	VirtioBlkReqTypeDiscard      VirtioBlkReqType = 11
	VirtioBlkReqTypeWrite_Zeroes VirtioBlkReqType = 13
)

type VirtioBlkReqStatus uint32

const (
	VirtioBlkStatusOk      VirtioBlkReqStatus = 0
	VirtioBlkStatusIOErr   VirtioBlkReqStatus = 1
	VirtioBlkStatusUnsupp  VirtioBlkReqStatus = 2
	VirtioBlkStatusUnknown VirtioBlkReqStatus = math.MaxUint32
)

type VirtioBlkReq struct {
	Type     VirtioBlkReqType
	Reserved uint32
	Sector   uint64
	Data     [512]uint8
	Status   VirtioBlkReqStatus

	req    *block.BlockDeviceIORequest
	waiter chan<- block.BlockRequestIOResponse
}

type VirtioBlkDiscardWriteZeroesFlag uint32

const (
	VirtioBlkDiscardWriteZeroesFlagUnmap VirtioBlkDiscardWriteZeroesFlag = 1 << 0
)

type VirtioBlkDiscardWriteZeroes struct {
	Sector     uint64
	NumSectors uint32
	flags      VirtioBlkDiscardWriteZeroesFlag // unmap:1; reserved:31;
}
