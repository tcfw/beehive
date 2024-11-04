package partition

import (
	"encoding/binary"
	"errors"
	"time"

	"github.com/tcfw/kernel/services/go/fs/devices"
	"github.com/tcfw/kernel/services/go/fs/drivers/block"
	"github.com/tcfw/kernel/services/go/fs/filesystems"
	"github.com/tcfw/kernel/services/go/fs/utils"
)

const (
	mbrSignature1 = 0x55
	mbrSignature2 = 0xAA

	partition1Offset = 0x01BE
	partition2Offset = 0x01CE
	partition3Offset = 0x01DE
	partition4Offset = 0x01EE
)

func IsMBRTable(d []byte) bool {
	end2 := d[len(d)-2:]
	return end2[0] == mbrSignature1 && end2[1] == mbrSignature2
}

type MBR [512]byte

func (h MBR) DiskSignature() uint32 {
	return binary.LittleEndian.Uint32(h[0x01BC:])
}

func (h MBR) Partition1() MBRPartition {
	return MBRPartition(h[partition1Offset:])
}

func (h MBR) Partition2() MBRPartition {
	return MBRPartition(h[partition2Offset:])
}

func (h MBR) Partition3() MBRPartition {
	return MBRPartition(h[partition3Offset:])
}

func (h MBR) Partition4() MBRPartition {
	return MBRPartition(h[partition4Offset:])
}

type MBRPartitionAttributes uint8

const (
	MBRPartitionAttributesValid MBRPartitionAttributes = 1 << 7
)

type MBRPartition []byte

func (p MBRPartition) Attributes() MBRPartitionAttributes {
	return MBRPartitionAttributes(p[0])
}

func (p MBRPartition) FirstSectorCHS() utils.CHSAddress {
	return utils.CHSAddress(p[1:])
}

func (p MBRPartition) PartitionType() uint8 {
	return p[4]
}

func (p MBRPartition) LastSectorCHS() utils.CHSAddress {
	return utils.CHSAddress(p[5:])
}

func (p MBRPartition) LBAStart() uint32 {
	return binary.LittleEndian.Uint32(p[8:])
}

func (p MBRPartition) SectorCount() uint32 {
	return binary.LittleEndian.Uint32(p[12:])
}

func DiscoverMBRParitions(d *devices.Device) error {
	if d.DeviceType != devices.DeviceTypeBlock {
		return errors.New("wrong device type for MBR")
	}

	bdev := d.BlockDevice

	blockCh := make(chan block.IOResponse)

	mbr := MBR{}

	//Read first partition for MBR
	req := block.IORequest{
		RequestType: block.IORequestTypeRead,
		Data:        mbr[:],
		Offset:      0,
	}
	bdev.Enqueue([]block.IORequest{req}, blockCh)

	select {
	case resp := <-blockCh:
		if err := resp.Err; err != nil {
			return err
		}
	case <-time.After(1 * time.Second):
		return ErrorTimeout
	}
	p1 := mbr.Partition1()
	p2 := mbr.Partition2()
	p3 := mbr.Partition3()
	p4 := mbr.Partition4()

	if p1.Attributes()&MBRPartitionAttributesValid != 0 {
		//register partition1
		devices.RegisterDevice(&devices.Device{
			Name:       d.Name + ".0",
			DeviceType: devices.DeviceTypeBlockPartition,
			BlockPartition: &Partition{
				BlockQueuer: d.BlockDevice,
				Type:        filesystems.MBRPartitionTypeToFS(p1.PartitionType()),
				StartOffset: uint64(p1.LBAStart()) * d.BlockDevice.BlockSize(),
				EndOffset:   uint64(p1.LBAStart())*d.BlockDevice.BlockSize() + uint64(p1.SectorCount())*d.BlockDevice.BlockSize(),
			},
		})
	}

	if p2.Attributes()&MBRPartitionAttributesValid != 0 {
		//register partition2
		devices.RegisterDevice(&devices.Device{
			Name:       d.Name + ".1",
			DeviceType: devices.DeviceTypeBlockPartition,
			BlockPartition: &Partition{
				BlockQueuer: d.BlockDevice,
				Type:        filesystems.MBRPartitionTypeToFS(p2.PartitionType()),
				StartOffset: uint64(p2.LBAStart()) * d.BlockDevice.BlockSize(),
				EndOffset:   uint64(p2.LBAStart())*d.BlockDevice.BlockSize() + uint64(p2.SectorCount())*d.BlockDevice.BlockSize(),
			},
		})
	}

	if p3.Attributes()&MBRPartitionAttributesValid != 0 {
		//register partition3
		devices.RegisterDevice(&devices.Device{
			Name:       d.Name + ".2",
			DeviceType: devices.DeviceTypeBlockPartition,
			BlockPartition: &Partition{
				BlockQueuer: d.BlockDevice,
				Type:        filesystems.MBRPartitionTypeToFS(p3.PartitionType()),
				StartOffset: uint64(p3.LBAStart()) * d.BlockDevice.BlockSize(),
				EndOffset:   uint64(p3.LBAStart())*d.BlockDevice.BlockSize() + uint64(p3.SectorCount())*d.BlockDevice.BlockSize(),
			},
		})
	}

	if p4.Attributes()&MBRPartitionAttributesValid != 0 {
		//register partition4
		devices.RegisterDevice(&devices.Device{
			Name:       d.Name + ".3",
			DeviceType: devices.DeviceTypeBlockPartition,
			BlockPartition: &Partition{
				BlockQueuer: d.BlockDevice,
				Type:        filesystems.MBRPartitionTypeToFS(p4.PartitionType()),
				StartOffset: uint64(p4.LBAStart()) * d.BlockDevice.BlockSize(),
				EndOffset:   uint64(p4.LBAStart())*d.BlockDevice.BlockSize() + uint64(p4.SectorCount())*d.BlockDevice.BlockSize(),
			},
		})
	}

	return nil
}
