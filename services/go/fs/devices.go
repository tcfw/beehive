package fs

import (
	"strconv"
	"sync"
	"sync/atomic"

	"github.com/tcfw/kernel/services/go/fs/drivers"
)

type DeviceType uint

const (
	DeviceTypeUnknown DeviceType = iota
	DeviceTypeBlock
)

var (
	devices   = []*FSDevice{}
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

type FSDevice struct {
	Name        string
	DeviceType  DeviceType
	BlockDriver drivers.BlockDriver
}

func RegisterDevice(dev *FSDevice) {
	devicesMu.Lock()
	defer devicesMu.Unlock()

	devices = append(devices, dev)
	print("registered device: ", dev.Name, "\n")
}
