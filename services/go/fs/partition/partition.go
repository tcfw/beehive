package partition

import "github.com/tcfw/kernel/services/go/fs/filesystems"

type PartitionAttributes uint64

const (
	PartitionAttributeFirmware PartitionAttributes = iota + 1
)

type Partition struct {
	Type        filesystems.FileSystem
	StartOffset uint64
	EndOffset   uint64
	Attributes  uint64
}
