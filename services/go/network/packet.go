package network

import (
	"math"
	"sync"
	"sync/atomic"

	"github.com/tcfw/kernel/services/go/utils"
)

const (
	PacketHeaderPadding = 50 //ipv6
)

var (
	getPacketCount      = uint64(0)
	returnedPacketCount = uint64(0)
)

func PacketFromRing(r *utils.Ring) *Packet {
	p := GetPacket()
	p.Frame = p.Frame[:*r.ObjectSize]
	r.PullToWait(p.Frame[:*r.ObjectSize])
	p.Payload = p.Frame[0:]

	return p
}

type Packet struct {
	Frame     []byte
	Payload   []byte
	SrcDevice *Interface

	HandledBy []PacketProtocolSlice

	//Done a callback set by the originating device to mark
	//when the lower frame can be reused (e.g. added back to a pool or ring buffer)
	Done func()
}

func (p *Packet) reset() {
	p.Payload = p.Frame[:0]
	p.Frame = p.Frame[:0]
	p.HandledBy = p.HandledBy[:0]
}

type PacketProtocolSlice struct {
	Proto  ProtocolIdentifier
	Offset uint16
}

var (
	zeroArray = make([]byte, 1500)
)

var packetPool = sync.Pool{
	New: func() any {
		p := new(Packet)
		p.Frame = make([]byte, 0, 1500)
		p.HandledBy = make([]PacketProtocolSlice, 6)

		return p
	},
}

func GetPacket() *Packet {
	p := packetPool.Get().(*Packet)
	p.Done = func() {
		packetPool.Put(p)
		atomic.AddUint64(&returnedPacketCount, 1)
	}

	// p.Frame = p.Frame[:cap(p.Frame)]
	// if len(zeroArray) != len(p.Frame) {
	// 	//grow the zero array
	// 	zeroArray = make([]byte, len(p.Frame))
	// }
	// copy(p.Frame, zeroArray)
	p.reset()

	atomic.AddUint64(&getPacketCount, 1)

	return p
}

func DropPacket(p *Packet) {
	if p.SrcDevice != nil {
		p.SrcDevice.Stats.RXDrop++
	}

	p.Done()
}

func AppendHandledByCurrentOffset(p *Packet, proto ProtocolIdentifier) {
	AppendHandlerAtOffset(p, proto, uint16(cap(p.Frame)-cap(p.Payload)))
}

func AppendHandlerAtOffset(p *Packet, proto ProtocolIdentifier, offset uint16) {
	p.HandledBy = append(p.HandledBy, PacketProtocolSlice{proto, offset})
}

func GetOffsetProtoHandler(p *Packet, proto ProtocolIdentifier) uint16 {
	for _, h := range p.HandledBy {
		if h.Proto == proto {
			return h.Offset
		}
	}

	return math.MaxUint16
}
