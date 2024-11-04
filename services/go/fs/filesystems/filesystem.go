package filesystems

import "io/fs"

type FileSystemType uint

const (
	FileSystemUnknown FileSystemType = iota
	FileSystemFAT16
	FileSystemFAT32
	FileSystemExt4
)

func MBRPartitionTypeToFS(mbr uint8) FileSystemType {
	switch mbr {
	case 0x6:
		return FileSystemFAT16
	case 0xc:
		return FileSystemFAT32
	case 0x83:
		return FileSystemExt4
	default:
		return FileSystemUnknown
	}
}

type INode uint64

type Stat struct{}

type Mode fs.FileMode

type FSReplyType int

const (
	FSReplyError FSReplyType = iota + 1
	FSReplyEntry
	FSReplyCreate
	FSReplyAttr
	FSReplyReadLink
	FSReplyOpen
	FSReplyWrite
	FSReplyBuf
	FSReplyData
	FSReplyStatfs
	FSReplyXattr
	FSReplyLock
	FSReplyBmap
	FSReplyIOCtl
	FSReplyPoll
	FSReplyLSeek
)

type FSReply interface {
	FSReplyType() FSReplyType
	IncRefCount()
	DecRefCount()
}

type FSRequestAttributes uint64

type FSRequest struct {
	ID     uint64
	RespCh chan<- FSReply
	Attrs  FSRequestAttributes
	Owner  uint64
}

type FileSystemXAttr interface {
	SetXAttr(req *FSRequest, ino INode, name string, value string)
	GetXAttr(req *FSRequest, ino INode, name string)
	ListXAttr(req *FSRequest, ino INode, size uint64)
}

type FileSystemLocker interface {
	GetLock(req *FSRequest, ino INode)
	SetLock(req *FSRequest, ino INode, owner uint64)
}

type FileSystemIOCtrl interface {
	IOCtl(req *FSRequest, ino INode, cmd int32, arg []byte, flags int32, inBuf []byte, outBufSize uint64)
}

type FileSystemRangeCopier interface {
	CopyFileRange(req *FSRequest, src_ino INode, src_offset uint64, dst_ino INode, dst_offset uint64, size uint64)
}

type FileSystem interface {
	Type() FileSystemType

	Init() error
	Destroy() error

	Lookup(req *FSRequest, parent INode, name string)
	Forget(req *FSRequest, ino INode, delta uint64)
	GetAttr(req *FSRequest, ino INode)
	SetAttr(req *FSRequest, ino INode, attr *Stat, mask uint64)
	ReadLink(req *FSRequest, ino INode)

	MkDir(req *FSRequest, parent INode, name string)

	Unlink(req *FSRequest, parent INode, name string)
	RmDir(req *FSRequest, parent INode, name string)
	SymLink(req *FSRequest, link string, parent INode, name string)
	Rename(req *FSRequest, parent INode, name string, newParent INode, newName string)
	Link(req *FSRequest, ino INode, newParent INode, newName string)

	Open(req *FSRequest, ino INode)
	Read(req *FSRequest, ino INode, size uint64, offset uint64)
	Write(req *FSRequest, ino INode, buf []byte, size uint64, offset uint64)
	Flush(req *FSRequest, ino INode)
	Release(req *FSRequest, ino INode)
	FSync(req *FSRequest, ino INode, dataOnly bool)

	OpenDir(req *FSRequest, ino INode)
	ReadDir(req *FSRequest, ino INode, size uint64, offset uint64)
	ReleaseDir(req *FSRequest, ino INode)
	FsyncDir(req *FSRequest)

	StatFS(req *FSRequest, ino INode)

	Create(req *FSRequest, parent INode, name string, mode Mode)

	Poll(req *FSRequest, ino INode)

	FAllocate(req *FSRequest, ino INode, mode Mode, size uint64, offset uint64)

	LSeek(req *FSRequest)
}
