package block

type IORequestType uint

const (
	IORequestTypeRead IORequestType = iota
	IORequestTypeWrite
	IORequestTypeFlush
	IORequestTypeTrim
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
	Err error
}
