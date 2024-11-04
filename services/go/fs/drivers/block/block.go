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
	ErrBlockReqOutOfBounds        = errors.New("out of bounds")
	ErrBlockReqMisaligned         = errors.New("request not aligned")
)

type IORequest struct {
	RequestType IORequestType
	ID          uint64
	Offset      uint64
	Length      uint64 //only used for Trim
	Data        []byte
	Ctx         any
}

type IOResponse struct {
	Req *IORequest
	Err error
}
