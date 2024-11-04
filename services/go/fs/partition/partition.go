package partition

import (
	"errors"
	"time"

	"github.com/tcfw/kernel/services/go/fs/drivers"
	"github.com/tcfw/kernel/services/go/fs/drivers/block"
	"github.com/tcfw/kernel/services/go/fs/filesystems"
)

type PartitionTableType int

const (
	PartitionTableTypeUnknown PartitionTableType = iota
	PartitionTableTypeMBR
	PartitionTableTypeGPT
)

var (
	ErrorTimeout = errors.New("detect timedout")
)

func IdentifyPartitionTable(bdev drivers.BlockQueuer) (PartitionTableType, error) {
	blockCh := make(chan block.IOResponse)

	detectedType := PartitionTableTypeUnknown
	bSize := bdev.BlockSize()

	//Read first partition for MBR
	req := block.IORequest{
		RequestType: block.IORequestTypeRead,
		Data:        make([]byte, bSize),
		Offset:      0,
	}
	bdev.Enqueue([]block.IORequest{req}, blockCh)

	select {
	case resp := <-blockCh:
		if err := resp.Err; err != nil {
			return detectedType, err
		}
	case <-time.After(1 * time.Second):
		return detectedType, ErrorTimeout
	}

	if IsMBRTable(req.Data) {
		detectedType = PartitionTableTypeMBR
	} else {
		return detectedType, nil
	}

	//Read secton partition for GPT
	req.Offset = 1 * bSize
	bdev.Enqueue([]block.IORequest{req}, blockCh)

	select {
	case resp := <-blockCh:
		if err := resp.Err; err != nil {
			return detectedType, err
		}
	case <-time.After(1 * time.Second):
		return detectedType, ErrorTimeout
	}

	if IsGPTTable(req.Data) {
		detectedType = PartitionTableTypeGPT
	}

	return detectedType, nil
}

type PartitionAttributes uint64

const (
	PartitionAttributeFirmware PartitionAttributes = iota + 1
)

type Partition struct {
	drivers.BlockQueuer

	Type        filesystems.FileSystemType
	StartOffset uint64
	EndOffset   uint64
	Attributes  PartitionAttributes
}

func (p *Partition) Enqueue(reqs []block.IORequest, comp chan<- block.IOResponse) (error, int) {
	//rewrite requests to be at the start LBA offset and bounds check

	reqrr := make([]block.IORequest, 0, len(reqs))
	bsize := p.BlockSize()

	for _, req := range reqs {
		req.Offset += p.StartOffset

		if req.Offset+bsize > p.StartOffset+p.EndOffset {
			comp <- block.IOResponse{
				Req: &req,
				Err: block.ErrBlockReqOutOfBounds,
			}
			continue
		}

		reqrr = append(reqrr, req)
	}

	return p.BlockQueuer.Enqueue(reqrr, comp)
}
