package fs

import (
	"fmt"
	"time"

	"github.com/pkg/errors"
	"github.com/tcfw/kernel/services/go/fs/drivers"
	"github.com/tcfw/kernel/services/go/fs/drivers/block"
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

	print(fmt.Sprintf("success - mapped: %d\n", len(devices)))

	return nil
}

func partitionDiscover() error {
	waitCh := make(chan block.BlockRequestIOResponse, 2)

	for _, d := range devices {
		if d.DeviceType != DeviceTypeBlock {
			continue
		}

		req := block.BlockDeviceIORequest{
			RequestType: block.IORequestTypeRead,
			ID:          0,
			Offset:      0,
			Data:        make([]byte, d.BlockDriver.BlockSize()),
		}
		req2 := block.BlockDeviceIORequest{
			RequestType: block.IORequestTypeRead,
			ID:          0,
			Offset:      32256,
			Data:        make([]byte, d.BlockDriver.BlockSize()),
		}

		var resp block.BlockRequestIOResponse

	retry1:
		for {
			d.BlockDriver.Enqueue([]block.BlockDeviceIORequest{req}, waitCh)

			select {
			case resp = <-waitCh:
				break retry1
			case <-time.After(2 * time.Millisecond):
				print("retrying")
			}
		}
		if resp.Err != nil {
			print(fmt.Sprintf("got block data: %X", req.Data))
			return resp.Err
		}

		print(fmt.Sprintf("got bytes: %X\n", resp.Req.Data))

	retry2:
		for {
			d.BlockDriver.Enqueue([]block.BlockDeviceIORequest{req2}, waitCh)

			select {
			case resp = <-waitCh:
				break retry2
			case <-time.After(2 * time.Millisecond):
				print("retrying")
			}
		}

		if resp.Err != nil {
			print(fmt.Sprintf("got block data: %X", req.Data))
			return resp.Err
		}

		print(fmt.Sprintf("got bytes: %X\n", resp.Req.Data))
	}

	return nil
}
