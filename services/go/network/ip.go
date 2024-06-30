package network

import (
	"encoding/binary"
	"net"
	"unsafe"
)

var (
	ipv6AdditionalHandlers = map[uint8]ProtocolHandler{}
	ipv4AdditionalHandlers = map[uint8]ProtocolHandler{}
)

const (
	ipv6HeaderOffsetLength     = 4
	ipv6HeaderOffsetNextHeader = 6
	ipv6HeaderOffsetHopLimit   = 7
	ipv6HeaderOffsetSrcAddress = 8
	ipv6HeaderOffsetDstAddress = 24

	ipv6ExtensionNextHeaderHopbyHop     = 0
	ipv6ExtensionNextHeaderRouting      = 43
	ipv6ExtensionNextHeaderFragment     = 44
	ipv6ExtensionNextHeaderESP          = 50
	ipv6ExtensionNextHeaderAuthHeader   = 51
	ipv6ExtensionNextHeaderDestOption   = 60
	ipv6ExtensionNextHeaderMobility     = 135
	ipv6ExtensionNextHeaderHIP          = 139
	ipv6ExtensionNextHeaderShim6        = 140
	ipv6ExtensionNextHeaderNoNextHeader = 59

	ipv6MinHeaderSize = 40
)

const (
	IPProtoICMP   = 0x01
	IPProtoICMPv6 = 0x3A
	IPProtoTCP    = 0x06
	IPProtoUDP    = 0x11
)

type IPv6Header []byte

func (i IPv6Header) Version() uint8 {
	return i[0] >> 4
}

func (i IPv6Header) SetVersion() {
	i[0] = (i[0] & 0x0F) | 0x60
}

func (i IPv6Header) TrafficClass() uint8 {
	return uint8(binary.BigEndian.Uint16(i[0:]) >> 4)
}

func (i IPv6Header) SetTrafficClass(tc uint8) {
	i[0] = (i[0] & 0xF0) | (tc >> 4)
	i[1] = (tc << 4) | (i[1] & 0x0F)
}

func (i IPv6Header) FlowLabel() uint32 {
	return uint32(binary.BigEndian.Uint32(i[0:]) & 0x0FFFFF)
}

func (i IPv6Header) SetFlowLabel(fl uint32) {
	vt := binary.BigEndian.Uint32(i[0:]) & 0xFFF00000
	binary.BigEndian.PutUint32(i[0:], vt|(fl&0x000FFFFF))
}

func (i IPv6Header) Length() uint16 {
	return binary.BigEndian.Uint16(i[ipv6HeaderOffsetLength:])
}

func (i IPv6Header) SetLength(l uint16) {
	binary.BigEndian.PutUint16(i[ipv6HeaderOffsetLength:], l)
}

func (i IPv6Header) NextHeader() uint8 {
	return uint8(i[ipv6HeaderOffsetNextHeader])
}

func (i IPv6Header) SetNextHeader(nh uint8) {
	i[ipv6HeaderOffsetNextHeader] = nh
}

func (i IPv6Header) HopLimit() uint8 {
	return uint8(i[ipv6HeaderOffsetHopLimit])
}

func (i IPv6Header) SetHopLimit(hl uint8) {
	i[ipv6HeaderOffsetHopLimit] = hl
}

func (i IPv6Header) SrcAddress() net.IP {
	return net.IP(i[ipv6HeaderOffsetSrcAddress : ipv6HeaderOffsetSrcAddress+16])
}

func (i IPv6Header) SetSrcAdress(ip net.IP) {
	copy(i[ipv6HeaderOffsetSrcAddress:], ip)
}

func (i IPv6Header) DstAddress() net.IP {
	return net.IP(i[ipv6HeaderOffsetDstAddress : ipv6HeaderOffsetDstAddress+16])
}

func (i IPv6Header) SetDstAdress(ip net.IP) {
	copy(i[ipv6HeaderOffsetDstAddress:], ip)
}

func (i IPv6Header) Payload() []byte {
	curNextHop := uint8(i[ipv6HeaderOffsetNextHeader])
	curoff := uint8(ipv6MinHeaderSize)

	for extN := 0; extN < 12; extN++ {
		switch curNextHop {
		case ipv6ExtensionNextHeaderHopbyHop,
			ipv6ExtensionNextHeaderRouting,
			ipv6ExtensionNextHeaderDestOption:
			curNextHop = uint8(i[curoff])
			curoff += i[curoff+i[curoff+1]]
		case ipv6ExtensionNextHeaderFragment:
			curNextHop = uint8(i[curoff])
			curoff += 8
		case ipv6ExtensionNextHeaderNoNextHeader:
			return i[len(i)-1:]
		default:
			return i[ipv6MinHeaderSize:]
		}
	}

	// fallback to just after ipv6 standard header size
	return i[ipv6MinHeaderSize:]
}

func MulticastMACAddressFromIP(ip net.IP, mac MacAddress) {
	if len(ip) != net.IPv6len {
		return
	}

	mac[0] = 0x33
	mac[1] = 0x33
	mac[2] = ip[12]
	mac[3] = ip[13]
	mac[4] = ip[14]
	mac[5] = ip[15]
}

func HandleIPv6Packet(p *Packet) {
	AppendHandledByCurrentOffset(p, ProtocolIdentifierIPv6)
	iph := IPv6Header(p.Payload)
	p.Payload = iph.Payload()

	if act := HookIPv6PacketRX(p); act.Action != HookActionNOOP {
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

	destCheck := false

	// global unicast, link-local, loopback, unique local,
	// etc. IP addresses
	for _, ifaceIP := range p.SrcDevice.IPAddrs {
		if ifaceIP.IP.Equal(net.IP(iph.DstAddress())) {
			destCheck = true
			break
		}
	}

	// multicast addresses
	if !destCheck {
		for _, ifaceIP := range p.SrcDevice.SubscribedIPAddrs {
			if ifaceIP.IP.Equal(net.IP(iph.DstAddress())) {
				destCheck = true
				break
			}
		}
	}

	if !destCheck {
		DropPacket(p)
		return
	}

	switch nextProto := iph.NextHeader(); nextProto {
	case IPProtoICMPv6:
		HandleICMPv6Packet(p)
	// case IPProtoICMP:
	// case IPProtoTCP:
	// case IPProtoUDP:
	default:
		handler, ok := ipv6AdditionalHandlers[nextProto]
		if !ok {
			DropPacket(p)
			return
		}

		handler(p)
	}
}

func IPv6HeaderFromSlice(d []byte) *IPv6Header {
	return (*IPv6Header)(unsafe.Pointer(unsafe.SliceData(d)))
}

type IPv4Header []byte

func (i IPv4Header) Version() uint8 {
	return i[0] >> 4
}

func (i IPv4Header) IHL() uint8 {
	return i[0] & 0xF
}

func (i IPv4Header) DSCP() uint8 {
	return i[1] >> 2
}

func (i IPv4Header) ECN() uint8 {
	return i[1] & 0x3
}

func (i IPv4Header) Length() uint16 {
	return binary.BigEndian.Uint16(i[2:])
}

func (i IPv4Header) Identification() uint16 {
	return binary.BigEndian.Uint16(i[4:])
}

func (i IPv4Header) Flags() byte {
	return i[6] >> 5
}

func (i IPv4Header) FragmentOffset() uint16 {
	return binary.BigEndian.Uint16(i[6:]) & 0x1FFF
}

func (i IPv4Header) TTL() uint8 {
	return i[8]
}

func (i IPv4Header) Protocol() uint8 {
	return i[9]
}

func (i IPv4Header) Checksum() uint16 {
	return binary.BigEndian.Uint16(i[10:])
}

func (i IPv4Header) SrcAddress() net.IP {
	return net.IP(i[12:16])
}

func (i IPv4Header) DstAddress() net.IP {
	return net.IP(i[16:20])
}

func HandleIPv4Packet(p *Packet) {}
