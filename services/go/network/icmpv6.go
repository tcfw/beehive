package network

import (
	"encoding/binary"
	"net"
)

var (
	icmpv6AdditionalHandlers = map[ICMPv6Type]ProtocolHandler{}
)

type ICMPv6Header []byte

type ICMPv6Type uint8

const (
	ICMPv6TypeDestinationUnreachable         ICMPv6Type = 1
	ICMPv6TypePacketTooBig                   ICMPv6Type = 2
	ICMPv6TypeTimeExceeded                   ICMPv6Type = 3
	ICMPv6TypeParameterProblem               ICMPv6Type = 4
	ICMPv6TypeEchoRequest                    ICMPv6Type = 128
	ICMPv6TypeEchoReply                      ICMPv6Type = 129
	ICMPv6TypeMulticastListenerQuery         ICMPv6Type = 130
	ICMPv6TypeMulticastListenerReport        ICMPv6Type = 131
	ICMPv6TypeMulticastListenerDone          ICMPv6Type = 132
	ICMPv6TypeRouterSolicitation             ICMPv6Type = 133
	ICMPv6TypeRouterAdvertisement            ICMPv6Type = 134
	ICMPv6TypeNeighborSolicitation           ICMPv6Type = 135
	ICMPv6TypeNeighborAdvertisement          ICMPv6Type = 136
	ICMPv6TypeRedirectMessage                ICMPv6Type = 137
	ICMPv6TypeCertificationPathSolicitation  ICMPv6Type = 148
	ICMPv6TypeCertificationPathAdvertisement ICMPv6Type = 149

	icmpv6MinHeaderSize = 4
)

func (i ICMPv6Header) Type() ICMPv6Type {
	return ICMPv6Type(i[0])
}

func (i ICMPv6Header) SetType(t ICMPv6Type) {
	i[0] = byte(t)
}

func (i ICMPv6Header) Code() uint8 {
	return i[1]
}

func (i ICMPv6Header) SetCode(c uint8) {
	i[1] = c
}

func (i ICMPv6Header) Checksum() uint16 {
	return binary.BigEndian.Uint16(i[2:])
}

func (i ICMPv6Header) SetChecksum(csum uint16) {
	binary.LittleEndian.PutUint16(i[2:], csum)
}

func (i ICMPv6Header) MessageBody() []byte {
	return i[4:]
}

func (i ICMPv6Header) SetMessageBody(b []byte) {
	copy(i[4:], b)
}

const (
	ICMPv6NDPRouterSolicitationSize    = 4
	ICMPv6NDPRouterAdvertisementSize   = 12
	ICMPv6NDPNeighborSolicitationSize  = 20
	ICMPv6NDPNeighborAdvertisementSize = 20
	ICMPv6NDPRedirectSize              = 36
)

// ICMPv6NDPRouterSolicitation only contains a 32-bit
// reserved field
type ICMPv6NDPRouterSolicitation []byte

func (rs ICMPv6NDPRouterSolicitation) Options() []byte {
	return rs[ICMPv6NDPRouterSolicitationSize:]
}

type ICMPv6NDPRouterAdvertisement []byte

func (ra ICMPv6NDPRouterAdvertisement) CurrentHopLimit() uint8 {
	return ra[0]
}

func (ra ICMPv6NDPRouterAdvertisement) SetCurrentHopLimit(hl uint8) {
	ra[0] = hl
}

const (
	// "Managed address configuration" flag
	//
	// just use DHCPv6 for prefix, dns, domains etc.
	ICMPv6NDPRouterAdvertisementFlagM = 1 << 7

	// "Other configuration" flag
	//
	// use some config from DHCPv6 e.g. dns, domains
	// but not prefixes
	ICMPv6NDPRouterAdvertisementFlagO = 1 << 6
)

func (ra ICMPv6NDPRouterAdvertisement) Flags() uint8 {
	return ra[1] & 0xC0
}

func (ra ICMPv6NDPRouterAdvertisement) SetFlags(flags uint8) {
	ra[1] = flags & 0xC0
}

func (ra ICMPv6NDPRouterAdvertisement) RouterLifetime() uint16 {
	return binary.BigEndian.Uint16(ra[2:])
}

func (ra ICMPv6NDPRouterAdvertisement) SetRouterLifetime(rlt uint16) {
	binary.BigEndian.PutUint16(ra[2:], rlt)
}

func (ra ICMPv6NDPRouterAdvertisement) ReachableTime() uint32 {
	return binary.BigEndian.Uint32(ra[4:])
}

func (ra ICMPv6NDPRouterAdvertisement) SetReachableTime(rt uint32) {
	binary.BigEndian.PutUint32(ra[4:], rt)
}

func (ra ICMPv6NDPRouterAdvertisement) RetransTimer() uint32 {
	return binary.BigEndian.Uint32(ra[8:])
}

func (ra ICMPv6NDPRouterAdvertisement) SetRetransTimer(rt uint32) {
	binary.BigEndian.PutUint32(ra[8:], rt)
}

func (rs ICMPv6NDPRouterAdvertisement) Options() []byte {
	return rs[ICMPv6NDPRouterAdvertisementSize:]
}

// ICMPv6NDPNeighborSolicitation contains a 32-bit reserved field
type ICMPv6NDPNeighborSolicitation []byte

func (ns ICMPv6NDPNeighborSolicitation) TargetAddress() net.IP {
	return net.IP(ns[4:20])
}

func (ns ICMPv6NDPNeighborSolicitation) SetTargetAddress(ip net.IP) {
	copy(ns[4:20], ip)
}

func (rs ICMPv6NDPNeighborSolicitation) Options() []byte {
	return rs[ICMPv6NDPNeighborSolicitationSize:]
}

type ICMPv6NDPNeighborAdvertisement []byte

const (
	// Router flag
	//
	// Sent from a router
	ICMPv6NDPNeighborAdvertisementFlagR = 1 << 31

	// Solicited flag
	//
	// Sent in response to a
	// Neighbor Solicitation
	ICMPv6NDPNeighborAdvertisementFlagS = 1 << 30

	// Override flag
	//
	// The advertisement should
	// override an existing cache entry and update
	// the cached link-layer address.
	ICMPv6NDPNeighborAdvertisementFlagO = 1 << 29
)

func (na ICMPv6NDPNeighborAdvertisement) Flags() uint32 {
	return binary.BigEndian.Uint32(na[0:]) & 0xE0000000
}
func (na ICMPv6NDPNeighborAdvertisement) SetFlags(flags uint32) {
	binary.BigEndian.PutUint32(na[0:], flags&0xE0000000)
}

func (ns ICMPv6NDPNeighborAdvertisement) TargetAddress() net.IP {
	return net.IP(ns[4:20])
}

func (ns ICMPv6NDPNeighborAdvertisement) SetTargetAddress(ip net.IP) {
	copy(ns[4:20], ip)
}

func (rs ICMPv6NDPNeighborAdvertisement) Options() []byte {
	return rs[ICMPv6NDPNeighborAdvertisementSize:]
}

// ICMPv6NDPRedirect contains a 32-bit reserved field
type ICMPv6NDPRedirect []byte

func (ns ICMPv6NDPRedirect) TargetAddress() net.IP {
	return net.IP(ns[4:20])
}

func (ns ICMPv6NDPRedirect) SetTargetAddress(ip net.IP) {
	copy(ns[4:20], ip)
}

func (ns ICMPv6NDPRedirect) DestinationAddress() net.IP {
	return net.IP(ns[4:20])
}

func (ns ICMPv6NDPRedirect) SetDestinationAddress(ip net.IP) {
	copy(ns[4:20], ip)
}

func (rs ICMPv6NDPRedirect) Options() []byte {
	return rs[ICMPv6NDPRedirectSize:]
}

const (
	ICMPv6NDPOptionTypeSourceLinkLayerAddress uint8 = 1
	ICMPv6NDPOptionTypeTargetLinkLayerAddress uint8 = 2
	ICMPv6NDPOptionTypePrefixInformation      uint8 = 3
	ICMPv6NDPOptionTypeRedirectedHeader       uint8 = 4
	ICMPv6NDPOptionTypeMTU                    uint8 = 5

	ICMPv6NDPOptionSourceTargetLinkLayerAddressSize = 2 + 6 //assuming ethernet
	ICMPv6NDPOptionPrefixInformationSize            = 32
	ICMPv6NDPOptionRedirectedHeaderSize             = 8 //not including original packet
	ICMPv6NDPOptionMTUSize                          = 8
)

type ICMPv6NDPOption interface {
	Type() uint8
	SetType(uint8)
	Length() uint8
	SetLength(uint8)
}

type ICMPv6NDPOptionLinkLayerAddress []byte

func (lla ICMPv6NDPOptionLinkLayerAddress) Type() uint8 {
	return lla[0]
}

func (lla ICMPv6NDPOptionLinkLayerAddress) SetType(t uint8) {
	lla[0] = t
}

func (lla ICMPv6NDPOptionLinkLayerAddress) Length() uint8 {
	return lla[1]
}

func (lla ICMPv6NDPOptionLinkLayerAddress) SetLength(l uint8) {
	lla[1] = l
}

func (lla ICMPv6NDPOptionLinkLayerAddress) LinkLayerAddress() MacAddress {
	return MacAddress(lla[2 : 2+(lla.Length()*8)])[:6]
}

func (lla ICMPv6NDPOptionLinkLayerAddress) SetLinkLayerAddress(mac MacAddress) {
	copy(lla[3:], mac)
}

type ICMPv6NDPOptionPrefixInformation []byte

func (pi ICMPv6NDPOptionPrefixInformation) Type() uint8 {
	return pi[0]
}

func (pi ICMPv6NDPOptionPrefixInformation) SetType(t uint8) {
	pi[0] = t
}

func (pi ICMPv6NDPOptionPrefixInformation) Length() uint8 {
	return pi[1]
}

func (pi ICMPv6NDPOptionPrefixInformation) SetLength(l uint8) {
	pi[1] = l
}

func (pi ICMPv6NDPOptionPrefixInformation) PrefixLength() uint8 {
	return pi[2]
}

func (pi ICMPv6NDPOptionPrefixInformation) SetPrefixLength(pl uint8) {
	pi[2] = pl
}

const (
	// On-link flag
	ICMPv6NDPOptionPrefixInformationFlagL = 1 << 7

	// Autonomous address-configuration flag
	ICMPv6NDPOptionPrefixInformationFlagA = 1 << 6
)

func (pi ICMPv6NDPOptionPrefixInformation) Flags() uint8 {
	return pi[3] & 0xC0
}

func (pi ICMPv6NDPOptionPrefixInformation) SetFlags(flags uint8) {
	pi[3] = flags & 0xC0
}

func (pi ICMPv6NDPOptionPrefixInformation) ValidLifetime() uint32 {
	return binary.BigEndian.Uint32(pi[4:])
}

func (pi ICMPv6NDPOptionPrefixInformation) SetValidLifetime(vl uint32) {
	binary.BigEndian.PutUint32(pi[4:], vl)
}

func (pi ICMPv6NDPOptionPrefixInformation) PreferredLifetime() uint32 {
	return binary.BigEndian.Uint32(pi[8:])
}

func (pi ICMPv6NDPOptionPrefixInformation) SetPreferredLifetime(pl uint32) {
	binary.BigEndian.PutUint32(pi[8:], pl)
}

func (pi ICMPv6NDPOptionPrefixInformation) Prefix() net.IP {
	return net.IP(pi[16:32])
}

func (pi ICMPv6NDPOptionPrefixInformation) SetPrefix(p net.IP) {
	copy(pi[16:32], p)
}

type ICMPv6NDPOptionRedirected []byte

func (r ICMPv6NDPOptionRedirected) Type() uint8 {
	return r[0]
}

func (r ICMPv6NDPOptionRedirected) SetType(t uint8) {
	r[0] = t
}

func (r ICMPv6NDPOptionRedirected) Length() uint8 {
	return r[1]
}

func (r ICMPv6NDPOptionRedirected) SetLength(l uint8) {
	r[1] = l
}

func (r ICMPv6NDPOptionRedirected) Data() []byte {
	return r[8:]
}

func (r ICMPv6NDPOptionRedirected) SetData(d []byte) {
	copy(r[8:], d)
}

type ICMPv6NDPOptionMTU []byte

func (mtu ICMPv6NDPOptionMTU) Type() uint8 {
	return mtu[0]
}

func (mtu ICMPv6NDPOptionMTU) SetType(t uint8) {
	mtu[0] = t
}

func (mtu ICMPv6NDPOptionMTU) Length() uint8 {
	return mtu[1]
}

func (mtu ICMPv6NDPOptionMTU) SetLength(l uint8) {
	mtu[1] = l
}

func (mtu ICMPv6NDPOptionMTU) MTU() uint32 {
	return binary.BigEndian.Uint32(mtu[4:])
}

func (mtu ICMPv6NDPOptionMTU) SetMTU(m uint32) {
	binary.BigEndian.PutUint32(mtu[4:], m)
}

type ICMPEcho []byte

func (i ICMPEcho) Identifier() uint16 {
	return binary.BigEndian.Uint16(i[0:])
}

func (i ICMPEcho) SetIdentifier(id uint16) {
	binary.BigEndian.PutUint16(i[0:], id)
}

func (i ICMPEcho) Sequence() uint16 {
	return binary.BigEndian.Uint16(i[2:])
}

func (i ICMPEcho) SetSequence(seq uint16) {
	binary.BigEndian.PutUint16(i[2:], seq)
}

func (i ICMPEcho) Payload() []byte {
	return i[4:]
}

func (i ICMPEcho) SetPayload(d []byte) {
	copy(i[4:], d)
}

func HandleICMPv6Packet(p *Packet) {
	icmp6Hdr := ICMPv6Header(p.Payload)
	AppendHandledByCurrentOffset(p, ProtocolIdentifierICMPv6)
	p.Payload = icmp6Hdr.MessageBody()

	if (p.SrcDevice.Capabilities | InterfaceCapabilitiesIPV6CSUM) == 0 {
		if icmpv6Checksum(p, icmp6Hdr) != 0 {
			DropPacket(p)
			return
		}
	}

	if act := HookICMPv6PacketRX(p); act.Action != HookActionNOOP {
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

	switch icmpType := icmp6Hdr.Type(); icmpType {
	case ICMPv6TypeEchoRequest:
		handleICMPv6EchoRequest(p)
	case ICMPv6TypeNeighborSolicitation:
		handleICMPv6NeighborSolicitation(p, icmp6Hdr)
	case ICMPv6TypeNeighborAdvertisement:
		handleICMPv6NeighborAdvertisement(p, icmp6Hdr)
	default:
		handler, ok := icmpv6AdditionalHandlers[icmpType]
		if !ok {
			DropPacket(p)
			return
		}

		handler(p)
	}
}

func handleICMPv6NeighborSolicitation(p *Packet, icmp6Hdr ICMPv6Header) {
	p.Payload = p.Payload[icmpv6MinHeaderSize:]
	AppendHandledByCurrentOffset(p, ProtocolIdentifierNDP)
	neighborSolicitation := ICMPv6NDPNeighborSolicitation(p.Payload)
	neighborSolicitation.TargetAddress()
}

func handleICMPv6NeighborAdvertisement(p *Packet, icmp6Hdr ICMPv6Header) {
	ip := IPv6Header(p.Frame[GetOffsetProtoHandler(p, ProtocolIdentifierIPv6):])
	icmpLen := ip.Length()
	if icmp6Hdr.Code() != 0 || icmpLen < 24 {
		DropPacket(p)
		return
	}

	AppendHandledByCurrentOffset(p, ProtocolIdentifierNDP)
	neighborAdvertisement := ICMPv6NDPNeighborAdvertisement(p.Payload)
	target := neighborAdvertisement.TargetAddress()
	flags := neighborAdvertisement.Flags()

	if target.IsMulticast() || (ip.DstAddress().IsMulticast() && (flags|ICMPv6NDPNeighborAdvertisementFlagS) != 0) {
		DropPacket(p)
		return
	}

	nce, wantTarget := p.SrcDevice.NetSpace.NeighbourProbes[string(target)]
	if !wantTarget {
		//For now only record entries we actually want
		DropPacket(p)
		return
	}

	nce.handleReceivedNA(p)
}

func readICMPv6NDPOptions(d []byte) []ICMPv6NDPOption {
	i := 0
	opts := []ICMPv6NDPOption{}

	for i < len(d) {
		if d[i+1] == 0 {
			break
		}

		switch d[i] {
		case ICMPv6NDPOptionTypeSourceLinkLayerAddress, ICMPv6NDPOptionTypeTargetLinkLayerAddress:
			opts = append(opts, ICMPv6NDPOptionLinkLayerAddress(d[i:]))
		case ICMPv6NDPOptionTypePrefixInformation:
			opts = append(opts, ICMPv6NDPOptionPrefixInformation(d[i:]))
		case ICMPv6NDPOptionTypeRedirectedHeader:
			opts = append(opts, ICMPv6NDPOptionRedirected(d[i:]))
		case ICMPv6NDPOptionTypeMTU:
			opts = append(opts, ICMPv6NDPOptionMTU(d[i:]))
		}
		i += int(d[i+1] * 8)
	}

	return opts
}

func handleICMPv6EchoRequest(p *Packet) {
	defer p.Done()

	echoRequest := ICMPEcho(p.Payload)
	ipHdr := IPv6Header(p.Frame[GetOffsetProtoHandler(p, ProtocolIdentifierIPv6):])

	dstIP := ipHdr.SrcAddress()
	srcIP := ipHdr.DstAddress()
	ns := p.SrcDevice.NetSpace

	nextHop, fib, ok := GetNextHop(ns, dstIP, AddressFamilyIPv6)
	if !ok {
		//no idea where to send the packet
		return
	}

	reply := GetPacket()
	reply.Frame = reply.Frame[:EthernetFrameMinSize+ipv6MinHeaderSize+icmpv6MinHeaderSize+len(echoRequest)]

	eth := Ethernet(reply.Frame[0:])
	eth.SetSrcMacAddress(p.SrcDevice.HardwareAddr)
	eth.SetEtherType(EtherType_IPv6)
	AppendHandlerAtOffset(reply, ProtocolIdentifierEthernet, 0)

	ipv6 := IPv6Header(reply.Frame[EthernetFrameMinSize:])
	ipv6.SetVersion()
	ipv6.SetNextHeader(IPProtoICMPv6)
	ipv6.SetHopLimit(fib.Interface.DefaultHopLimit)
	ipv6.SetLength(uint16(icmpv6MinHeaderSize + len(echoRequest)))
	ipv6.SetDstAdress(dstIP)
	ipv6.SetSrcAdress(srcIP)
	AppendHandlerAtOffset(reply, ProtocolIdentifierIPv6, EthernetFrameMinSize)

	icmpv6 := ICMPv6Header(reply.Frame[EthernetFrameMinSize+ipv6MinHeaderSize:])
	icmpv6.SetType(ICMPv6TypeEchoReply)
	icmpv6.SetCode(0)
	icmpv6.SetMessageBody(echoRequest)
	icmpv6.SetChecksum(0)
	if (p.SrcDevice.Capabilities | InterfaceCapabilitiesIPV6CSUM) == 0 {
		icmpv6.SetChecksum(icmpv6Checksum(reply, icmpv6))
	}

	dstMacEntry, ok := GetNextHopMacAddress(ns, nextHop, AddressFamilyIPv6)
	if !ok || dstMacEntry.MAC == nil || (dstMacEntry.Flags&NeighbourFlagIncomplete) > 0 {
		go func(ns *NetSpace, nextHop net.IP, iface *Interface) {
			resp, ok := <-ProbeNeighbour(ns, nextHop, iface)
			if !ok || resp.State != ProbeSuccessful {
				//probe timedout or too many newer packets
				reply.Done()
				return
			}

			eth.SetDstMacAddress(resp.Entry.MAC)

			_, ok = p.SrcDevice.Handlers.Enqueue(reply)
			if !ok {
				reply.Done()
			}
		}(ns, nextHop, p.SrcDevice)
		return
	}
	eth.SetDstMacAddress(dstMacEntry.MAC)

	_, ok = p.SrcDevice.Handlers.Enqueue(reply)
	if !ok {
		reply.Done()
	}
}

const ipv6PseudoHeaderLen = 2*net.IPv6len + 8

func icmpv6Checksum(p *Packet, icmpHdr ICMPv6Header) uint16 {
	ipHdr := IPv6Header(p.Frame[GetOffsetProtoHandler(p, ProtocolIdentifierIPv6):])

	// b := make([]byte, ipv6PseudoHeaderLen, ipv6PseudoHeaderLen+len(icmpHdr))

	tp := GetPacket()
	defer tp.Done()
	b := tp.Frame[:ipv6PseudoHeaderLen+len(icmpHdr)]

	copy(b[0:], ipHdr.SrcAddress())
	copy(b[16:], ipHdr.DstAddress())
	binary.BigEndian.PutUint16(b[34:], ipHdr.Length())
	b[39] = 58
	copy(b[ipv6PseudoHeaderLen:], icmpHdr)

	return Checksum16OnesComplement(b)
}
