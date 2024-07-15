package utils

import (
	"syscall"
	"unsafe"
)

const (
	MMAP_READ   int64 = 1 << 0
	MMAP_WRITE  int64 = 1 << 1
	MMAP_DEVICE int64 = 1 << 2

	SYSINFO_OP_FIELD_NCPU      uintptr = 1
	SYSINFO_OP_FIELD_PAGE_SIZE uintptr = 2
)

func MemMap(paddr uintptr, vaddr uintptr, n uint64, flags int64) ([]byte, error) {
	d, _, err := syscall.RawSyscall6(syscall.SYS_MEM_MAP, paddr, vaddr, uintptr(n), uintptr(flags), 0, 0)
	if err != 0 {
		return nil, err
	}

	ds := unsafe.Slice((*byte)(unsafe.Pointer(d)), n)
	return ds, nil
}

func DevPhyAddr(vaddr uintptr) (uintptr, error) {
	d, _, err := syscall.RawSyscall6(syscall.SYS_DEV_PHY_ADDR, vaddr, 0, 0, 0, 0, 0)
	if err != 0 {
		return 0, err
	}

	return d, nil
}

func PageSize() (uint, error) {
	d, _, err := syscall.RawSyscall6(syscall.SYS_SYSINFO, SYSINFO_OP_FIELD_PAGE_SIZE, 0, 0, 0, 0, 0)
	if err != 0 {
		return 0, err
	}

	return uint(d), nil
}

func MemoryBarrier()
