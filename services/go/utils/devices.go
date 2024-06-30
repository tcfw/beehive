package utils

import (
	"syscall"
	"unsafe"
)

type DevInfo struct {
	ID         uint32
	name       [64]byte
	dtype      [64]byte
	PhyBar     uint64
	PhyBarSize uint64
	Interrupts [5]uint64
}

func (d *DevInfo) Name() string {
	return string(d.name[:])
}

func (d *DevInfo) Type() string {
	return string(d.dtype[:])
}

func DeviceCount() (uint32, error) {
	devCount, _, err := syscall.RawSyscall(syscall.SYS_DEV_COUNT, 0, 0, 0)
	if err != 0 {
		return 0, err
	}

	return uint32(devCount), nil
}

func DeviceInfo(id uint32) (DevInfo, error) {
	var di DevInfo

	_, _, err := syscall.RawSyscall(syscall.SYS_DEV_INFO, uintptr(id), uintptr(unsafe.Pointer(&di)), 0)
	if err != 0 {
		return di, err
	}

	return di, nil
}

func DeviceProp(id uint32, prop string) ([]byte, error) {
	propValue := make([]byte, 1024)

	n, _, err := syscall.RawSyscall6(
		syscall.SYS_DEV_PROP,
		uintptr(id),
		uintptr(unsafe.Pointer(unsafe.StringData(prop))),
		uintptr(len(prop)),
		uintptr(unsafe.Pointer(unsafe.SliceData(propValue))),
		uintptr(len(propValue)),
		0,
	)
	if err != 0 {
		return nil, err
	}

	return propValue[:n], nil
}
