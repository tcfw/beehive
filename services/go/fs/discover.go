package fs

import (
	"fmt"

	"github.com/pkg/errors"
	"github.com/tcfw/kernel/services/go/fs/devices"
	"github.com/tcfw/kernel/services/go/fs/drivers"
	"github.com/tcfw/kernel/services/go/fs/drivers/block"
	"github.com/tcfw/kernel/services/go/fs/filesystems"
	"github.com/tcfw/kernel/services/go/fs/partition"
	"github.com/tcfw/kernel/services/go/utils"
)

func Discover() error {
	if err := mmioDiscover(); err != nil {
		return errors.Wrap(err, "discovering devices via mmio")
	}

	if err := partitionDiscover(); err != nil {
		return errors.Wrap(err, "discovering device partitions")
	}

	return nil
}

func mmioDiscover() error {
	devCount, err := utils.DeviceCount()
	if err != nil {
		return errors.Wrap(err, "getting device count")
	}

	for i := uint32(0); i < devCount; i++ {
		info, err := utils.DeviceInfo(i)
		if err != nil {
			return errors.Wrap(err, "getting device info")
		}

		driver, found := drivers.FindDeviceDriver(info.Type())
		if !found {
			continue
		}

		if err := driver(info); err != nil {
			print(fmt.Sprintf("error setting up driver for MMIO device id=%d: %s", info.ID, err), "\n")
		}
	}

	print(fmt.Sprintf("success - mapped: %d\n", len(devices.GetDevices())))

	return nil
}

func partitionDiscover() error {
	for _, d := range devices.GetDevices() {
		if d.DeviceType != devices.DeviceTypeBlock {
			continue
		}

		print("detecting partitions")

		var tableType partition.PartitionTableType
		for i := 0; i < 3; i++ {
			tt, err := partition.IdentifyPartitionTable(d.BlockDevice)
			if err != nil {
				if err == partition.ErrorTimeout {
					continue
				}
				return err
			}
			tableType = tt
			break
		}

		switch tableType {
		case partition.PartitionTableTypeUnknown:
			print("unknown table type")
		case partition.PartitionTableTypeMBR:
			partition.DiscoverMBRParitions(d)
		case partition.PartitionTableTypeGPT:
			print("GPT table type")
		default:
			print(fmt.Sprintf("table type reference unknown %+v", tableType))
		}
	}

	p1 := devices.GetDevice("virtio0.0")

	respCh := make(chan block.IOResponse)
	req := block.IORequest{
		RequestType: block.IORequestTypeRead,
		Data:        make([]byte, p1.BlockPartition.BlockSize()),
		Offset:      0,
	}
	reqs := []block.IORequest{req}

	err, _ := p1.BlockPartition.Enqueue(reqs, respCh)
	if err != nil {
		return err
	}

	resp := <-respCh
	if err := resp.Err; err != nil {
		print("failed to get part block")
	} else {
		f16sb := filesystems.FAT16SuperBlock(req.Data)
		print(fmt.Sprintf(
			"JumpCode=% x\r\nOEMName=%s\r\nBytesPerSector=%+v\r\nSectorsPerCluster=%+v\r\nReservedSectors=%+v\r\nFATCopies=%+v\r\nNumRootDirs=%+v\r\nTotalNumberOfSectors=%+v\r\nMediaType=%+v\r\nSectorsPerFAT=%+v\r\nSectorsPerTrack=%+v\r\nSectorsPerHeads=%+v\r\nNumberOfHiddenSectors=%+v\r\nNumberOfSectorsInFileSystem=%+v\r\nLogicalDriveNumber=%+v\r\nExtendedSignature=%+v\r\nSerialNumber=%+v\r\nVolumeLabel=%s\r\nFileSystemType=%s\r\nSignature=% x\r\n",
			f16sb.JumpCode(),
			f16sb.OEMName(),
			f16sb.BytesPerSector(),
			f16sb.SectorsPerCluster(),
			f16sb.ReservedSectors(),
			f16sb.FATCopies(),
			f16sb.NumRootDirs(),
			f16sb.TotalNumberOfSectors(),
			f16sb.MediaType(),
			f16sb.SectorsPerFAT(),
			f16sb.SectorsPerTrack(),
			f16sb.SectorsPerHeads(),
			f16sb.NumberOfHiddenSectors(),
			f16sb.NumberOfSectorsInFileSystem(),
			f16sb.LogicalDriveNumber(),
			f16sb.ExtendedSignature(),
			f16sb.SerialNumber(),
			f16sb.VolumeLabel(),
			f16sb.FileSystemType(),
			f16sb.Signature(),
		))

		s := uint64(uint16(f16sb.FATCopies())*f16sb.SectorsPerFAT() + f16sb.ReservedSectors())

		for i := s; i <= s+16; i++ {
			req.Offset = i * p1.BlockPartition.BlockSize()
			reqs = []block.IORequest{req}

			err, _ = p1.BlockPartition.Enqueue(reqs, respCh)
			if err != nil {
				return err
			}

			resp = <-respCh
			if err := resp.Err; err != nil {
				print("failed to get part block")
			} else {
				print(fmt.Sprintf("s%d=% x s=%s", i, req.Data, req.Data))
			}
		}
	}

	return nil
}
