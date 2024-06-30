package network

import (
	"bytes"
	"context"
	"encoding/binary"
	"runtime/trace"
)

var (
	ethernetAdditionalHandlers = map[EtherType]ProtocolHandler{}
)

func HandleEthernetFrame(p *Packet) {
	span := trace.StartRegion(context.Background(), "HandleEthernetFrame")

	eth := Ethernet(p.Payload)
	p.Payload = eth.Payload()
	p.HandledBy = append(p.HandledBy, PacketProtocolSlice{ProtocolIdentifierEthernet, 0})

	if act := HookEthernetPacketRX(p); act.Action != HookActionNOOP {
		switch act.Action {
		case HookActionFORWARD:
			if act.NextInterface != nil {
				act.NextInterface.Handlers.Enqueue(p)
			} else {
				DropPacket(p)
			}
			return
		case HookActionDROP:
			DropPacket(p)
			return
		}
	}

	/*
		TODO(tcfw):
		- dst check
		- forwarding
	*/

	switch etherType := eth.EtherType(); etherType {
	case EtherType_ARP:
		HandleNeighbourARPPacket(p)
	case EtherType_IPv4:
		HandleIPv4Packet(p)
	case EtherType_IPv6:
		span.End()
		HandleIPv6Packet(p)
	default:
		handler, ok := ethernetAdditionalHandlers[etherType]
		if !ok {
			DropPacket(p)
			return
		}

		handler(p)
	}
}

//
// 48-bit Mac Address
//

const (
	MacAddressLength = 6
)

var (
	BroadcastMacAddress = MacAddress{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
	EmptyMacAddress     = MacAddress{0, 0, 0, 0, 0, 0}
)

type MacAddress []byte

func (m MacAddress) IsMcast() bool {
	//ipv4 & ipv6
	return m[0] == 0x1 || (m[0] == 0x33 && m[1] == 0x33)
}

func (m MacAddress) IsBcast() bool {
	return bytes.Equal(m, BroadcastMacAddress)
}

func (m MacAddress) Equals(addr MacAddress) bool {
	return bytes.Equal(m, addr)
}

func (m MacAddress) IsUnicast() bool {
	//fast check for first multicast or broadcast byte
	if m[0] != 0x1 && m[0] != 0x33 && m[0] != 0xff {
		return true
	}

	if m[0] == 0xff && m[1] != 0xff {
		return true
	}

	return !m.IsBcast()
}

//
// EtherType
//

type EtherType uint16

const (
	EtherType_IPv4  EtherType = 0x0800
	EtherType_ARP   EtherType = 0x0806
	EtherType_Dot1Q EtherType = 0x8100
	EtherType_IPv6  EtherType = 0x86DD
)

//
// Ethernet Frame
//

const (
	EthernetFrameMinSize = MacAddressLength + MacAddressLength + 2
)

type Ethernet []byte

func (e Ethernet) DstMacAddress() MacAddress {
	return MacAddress(e[0:6])
}

func (e Ethernet) SetDstMacAddress(addr MacAddress) {
	copy(e[0:], addr)
}

func (e Ethernet) SrcMacAddress() MacAddress {
	return MacAddress(e[6:12])
}

func (e Ethernet) SetSrcMacAddress(addr MacAddress) {
	copy(e[6:], addr)
}

func (e Ethernet) Dot1Q() Dot1Q {
	if EtherType(binary.BigEndian.Uint16(e[12:])) == EtherType_Dot1Q {
		return Dot1Q(binary.BigEndian.Uint32(e[12:]))
	}

	return 0
}

func (e Ethernet) SetDot1Q(tag Dot1Q) {
	//set ethertype and tag
	binary.BigEndian.PutUint32(e[12:], uint32(tag))
}

func (e Ethernet) EtherType() EtherType {
	etherType := EtherType(binary.BigEndian.Uint16(e[12:]))
	if etherType == EtherType_Dot1Q {
		etherType = EtherType(binary.BigEndian.Uint16(e[16:]))
	}

	return etherType
}

func (e Ethernet) SetEtherType(t EtherType) {
	//if we're already vlan tagged
	if EtherType(binary.BigEndian.Uint16(e[12:])) == EtherType_Dot1Q {
		binary.BigEndian.PutUint16(e[16:], uint16(t))
		return
	}

	binary.BigEndian.PutUint16(e[12:], uint16(t))
}

func (e Ethernet) Payload() []byte {
	if e.EtherType() == EtherType_Dot1Q {
		return e[18:]
	}

	return e[14:]
}

func (e Ethernet) SetPayload(d []byte) {
	if e.EtherType() == EtherType_Dot1Q {
		copy(e[18:], d)
	}

	copy(e[14:], d)
}
