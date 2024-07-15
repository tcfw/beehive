package filesystems

type FileSystem uint

const (
	FileSystemUnknown FileSystem = iota + 1
	FileSystemFAT
	FileSystemFAT64
	FileSystemExt4
)
