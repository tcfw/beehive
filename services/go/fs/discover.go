package fs

import (
	"fmt"

	"github.com/pkg/errors"
	"github.com/tcfw/kernel/services/go/fs/drivers"
	"github.com/tcfw/kernel/services/go/utils"
)

func Discover() error {
	if err := mmioDiscover(); err != nil {
		return errors.Wrap(err, "discovering devices via mmio")
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
