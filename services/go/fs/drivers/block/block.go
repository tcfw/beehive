package block

import "errors"

type IORequestType uint

const (
	IORequestTypeRead IORequestType = iota
	IORequestTypeWrite
	IORequestTypeFlush
	IORequestTypeTrim
)

var (
	ErrBlockOperationNotSupported = errors.New("operation not supported")
	ErrBlockIOError               = errors.New("io error")
	ErrBlockUnknownResponse       = errors.New("unknown device response")
)

type BlockDeviceIORequest struct {
	RequestType IORequestType
	ID          uint64
	Offset      uint64
	Length      uint64 //only used for Trim
	Data        []byte
	Ctx         any
}

type BlockRequestIOResponse struct {
	Req *BlockDeviceIORequest
	Err error
}
