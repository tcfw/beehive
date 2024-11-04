package devices

import (
	"fmt"
	"strconv"
	"sync"
	"sync/atomic"

	"github.com/tcfw/kernel/services/go/fs/drivers"
)

type DeviceType uint

const (
	DeviceTypeUnknown DeviceType = iota
	DeviceTypeBlock
	DeviceTypeBlockPartition
)

var (
	devices   = []*Device{}
	devicesMu sync.Mutex

	deviceNameCount = map[string]*atomic.Uint32{}
)

func ReservedName(devType string) string {
	lastCount, ok := deviceNameCount[devType]
	if !ok {
		lastCount = &atomic.Uint32{}
		deviceNameCount[devType] = lastCount
	}

	n := lastCount.Add(1)

	return devType + strconv.Itoa(int(n-1))
}

type Device struct {
	Name           string
	DeviceType     DeviceType
	BlockDevice    drivers.BlockDriver
	BlockPartition drivers.BlockQueuer
}

func GetDevices() []*Device {
	return devices
}

func GetDevice(name string) *Device {
	devicesMu.Lock()
	defer devicesMu.Unlock()

	for _, dev := range devices {
		if dev.Name == name {
			return dev
		}
	}

	return nil
}

func RegisterDevice(dev *Device) {
	devicesMu.Lock()
	defer devicesMu.Unlock()

	devices = append(devices, dev)
	print(fmt.Sprintf("registered device: %s", dev.Name))
}
