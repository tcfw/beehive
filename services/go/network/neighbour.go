package network

import (
	"net"
	"sync"
	"time"
)

const (
	defaultNeighbourTimeout = 30 * time.Second

	maxProbeWaiters    = 20
	maxProbeRetries    = 5
	probeRetryInterval = 5 * time.Second
)

type NeighbourFlags uint

const (
	NeighbourFlagPermanent NeighbourFlags = 1 << iota
	NeighbourFlagIncomplete
	NeighbourFlagProbe
	NeighbourFlagStale
)

type NeighbourEntry struct {
	IP       net.IP
	MAC      MacAddress
	Device   *Interface
	LastSeen time.Time
	Flags    NeighbourFlags
}

type NeighbourCache struct {
	mutex   sync.RWMutex
	entries map[string]NeighbourEntry
	timeout time.Duration
}

// NewNeighbourCache creates a new NeighbourCache instance
func NewNeighbourCache(timeout time.Duration) *NeighbourCache {
	return &NeighbourCache{
		entries: make(map[string]NeighbourEntry, 200),
		timeout: timeout,
	}
}

// Get retrieves a neighbour entry from the cache by IP address
func (c *NeighbourCache) Get(ip net.IP) (NeighbourEntry, bool) {
	c.mutex.RLock()
	defer c.mutex.RUnlock()

	entry, ok := c.entries[string(ip)]
	if ok {
		if time.Since(entry.LastSeen) <= c.timeout {
			entry.Flags |= NeighbourFlagStale
		}

		return entry, true
	}

	return NeighbourEntry{}, false
}

// Set adds or updates a neighbour entry in the cache
func (c *NeighbourCache) Set(ip net.IP, mac MacAddress, iface *Interface, flags NeighbourFlags) NeighbourEntry {
	c.mutex.Lock()
	defer c.mutex.Unlock()

	macc := make(MacAddress, len(mac))
	copy(macc, mac)

	ipc := make(net.IP, len(ip))
	copy(ipc, ip)

	entry := NeighbourEntry{
		IP:       ipc,
		MAC:      macc,
		Device:   iface,
		Flags:    flags,
		LastSeen: time.Now(),
	}
	c.entries[string(ip)] = entry

	return entry
}

// cleanup removes expired entries from the cache
func (c *NeighbourCache) cleanup() {
	time.Sleep(c.timeout)
	c.mutex.Lock()
	defer c.mutex.Unlock()
	for key, entry := range c.entries {
		if time.Since(entry.LastSeen) > c.timeout {
			delete(c.entries, key)
		}
	}
}

func HandleNeighbourARPPacket(p *Packet) {

}

type ProbeResponseState uint

const (
	ProbePending ProbeResponseState = iota
	ProbeRetriesExceeded
	ProbeSuccessful
	ProbePacketAged //for when we wait to tell a waiter their packet is too old
)

type ProbeResponse struct {
	State ProbeResponseState
	Entry *NeighbourEntry
}

type ProbeRequest struct {
	ns         *NetSpace
	neighbour  net.IP
	iface      *Interface
	mu         sync.Mutex
	waiters    chan chan ProbeResponse
	retryCount int
	retryTimer *time.Timer
	entry      NeighbourEntry
	state      ProbeResponseState
}

func (pr *ProbeRequest) startProbe() {
	pr.sendProbe()

	for range pr.retryTimer.C {
		pr.retryCount--
		if pr.retryCount < 0 {
			pr.completeProbe(ProbeRetriesExceeded)
		}
	}
}

func (pr *ProbeRequest) completeProbe(state ProbeResponseState) {
	pr.retryTimer.Stop()

	pr.mu.Lock()
	defer pr.mu.Unlock()

	pr.state = state

	for waiter := range pr.waiters {
		select {
		case waiter <- ProbeResponse{
			State: state,
			Entry: &pr.entry,
		}:
		default:
		}
		close(waiter)
	}
}

func (pr *ProbeRequest) sendProbe() {
	if len(pr.neighbour) == net.IPv4len {
		pr.sendARPProbe()
	} else {
		pr.sendNDProbe()
	}
}

func (pr *ProbeRequest) receivedProbeResponse(p *Packet) {
	eth := Ethernet(p.Frame[GetOffsetProtoHandler(p, ProtocolIdentifierEthernet):])
	if eth.EtherType() == EtherType_ARP {
		pr.handleReceivedARP(p)
	} else {
		pr.handleReceivedNA(p)
	}
}

func (pr *ProbeRequest) sendNDProbe() {
	var srcIP *IP

	//use the first available IPv6 but prefer a
	// static address over dynamic or temporary
	for _, ip := range pr.iface.IPAddrs {
		if len(ip.IP) != net.IPv6len {
			continue
		}

		if srcIP == nil || (srcIP.Flags|IPFlagStatic) == 0 {
			srcIP = &ip
		}
		if (srcIP.Flags | IPFlagStatic) != 0 {
			break
		}
	}
	if srcIP == nil {
		return
	}

	dstL3IP := make(net.IP, 16)
	SolicitedNodeMulticastFromIP(pr.neighbour, dstL3IP)
	dstL2MAC := make(MacAddress, 6)
	MulticastMACAddressFromIP(dstL3IP, dstL2MAC)

	probe := GetPacket()
	probe.Frame = probe.Frame[:EthernetFrameMinSize+ipv6MinHeaderSize+icmpv6MinHeaderSize+ICMPv6NDPNeighborSolicitationSize+ICMPv6NDPOptionSourceTargetLinkLayerAddressSize]

	eth := Ethernet(probe.Frame[0:])
	eth.SetDstMacAddress(dstL2MAC)
	eth.SetSrcMacAddress(pr.iface.HardwareAddr)
	eth.SetEtherType(EtherType_IPv6)
	AppendHandlerAtOffset(probe, ProtocolIdentifierEthernet, 0)

	ipv6 := IPv6Header(probe.Frame[EthernetFrameMinSize:])
	ipv6.SetVersion()
	ipv6.SetNextHeader(IPProtoICMPv6)
	ipv6.SetHopLimit(255)
	ipv6.SetLength(uint16(icmpv6MinHeaderSize + ICMPv6NDPNeighborSolicitationSize + ICMPv6NDPOptionSourceTargetLinkLayerAddressSize))
	ipv6.SetDstAdress(dstL3IP)
	ipv6.SetSrcAdress(srcIP.IP)
	AppendHandlerAtOffset(probe, ProtocolIdentifierIPv6, EthernetFrameMinSize)

	ns := make(ICMPv6NDPNeighborSolicitation, ICMPv6NDPNeighborSolicitationSize)
	ns.SetTargetAddress(pr.neighbour)

	slla := make(ICMPv6NDPOptionLinkLayerAddress, ICMPv6NDPOptionSourceTargetLinkLayerAddressSize)
	slla.SetType(ICMPv6NDPOptionTypeSourceLinkLayerAddress)
	slla.SetLength(1)
	slla.SetLinkLayerAddress(pr.iface.HardwareAddr)

	icmpv6 := ICMPv6Header(probe.Frame[EthernetFrameMinSize+ipv6MinHeaderSize:])
	icmpv6.SetType(ICMPv6TypeNeighborSolicitation)
	icmpv6.SetCode(0)
	icmpv6.SetMessageBody(append(ns, slla...))
	icmpv6.SetChecksum(0)
	icmpv6.SetChecksum(icmpv6Checksum(probe, icmpv6))

	_, ok := pr.iface.Handlers.Enqueue(probe)
	if !ok {
		probe.Done()
	}
}

func (pr *ProbeRequest) sendARPProbe() {
	var srcIP *IP

	//use the first available IPv4
	for _, ip := range pr.iface.IPAddrs {
		if ip.IP.To4() != nil {
			srcIP = &ip
			break
		}
	}
	if srcIP == nil {
		return
	}

	probe := GetPacket()
	probe.Frame = probe.Frame[:EthernetFrameMinSize+ARPFrameMinSize]

	eth := Ethernet(probe.Frame[0:])
	eth.SetDstMacAddress(BroadcastMacAddress)
	eth.SetSrcMacAddress(pr.iface.HardwareAddr)
	eth.SetEtherType(EtherType_ARP)
	AppendHandlerAtOffset(probe, ProtocolIdentifierEthernet, 0)

	arp := ARP(probe.Frame[EthernetFrameMinSize:])
	arp.SetHardwareType(ARPHardwareTypeEthernet)
	arp.SetProtocolType(uint16(EtherType_IPv4))
	arp.SetHardwareAddrLen(6)
	arp.SetProtocolAddrLen(4)
	arp.SetOperation(ARPOperationRequest)
	arp.SetSenderHardwareAddress(pr.iface.HardwareAddr)
	arp.SetSenderProtocolAddress(srcIP.IP)
	arp.SetTargetHardwareAddress(EmptyMacAddress)
	arp.SetTargetProtocolAddress(pr.neighbour.To4())
	AppendHandlerAtOffset(probe, ProtocolIdentifierARP, EthernetFrameMinSize)

	_, ok := pr.iface.Handlers.Enqueue(probe)
	if !ok {
		probe.Done()
	}
}

func (pr *ProbeRequest) handleReceivedARP(p *Packet) {}

func (pr *ProbeRequest) handleReceivedNA(p *Packet) {
	defer p.Done()

	ip := IPv6Header(p.Frame[GetOffsetProtoHandler(p, ProtocolIdentifierIPv6):])
	icmpLen := ip.Length()

	ndp := ICMPv6NDPNeighborAdvertisement(p.Frame[GetOffsetProtoHandler(p, ProtocolIdentifierNDP):])

	if !ndp.TargetAddress().Equal(pr.neighbour) {
		return
	}

	options := []ICMPv6NDPOption{}

	if icmpLen > ICMPv6NDPNeighborAdvertisementSize+icmpv6MinHeaderSize {
		options = readICMPv6NDPOptions(ndp.Options())
	}

	var targetMac MacAddress

	for _, opt := range options {
		if opt.Type() == ICMPv6NDPOptionTypeTargetLinkLayerAddress {
			targetMac = opt.(ICMPv6NDPOptionLinkLayerAddress).LinkLayerAddress()
			break
		}
	}

	if targetMac == nil {
		return
	}

	pr.entry = pr.ns.Neighbours.Set(ndp.TargetAddress(), targetMac, pr.iface, 0)
	go pr.completeProbe(ProbeSuccessful)
}

func ProbeNeighbour(ns *NetSpace, neighbour net.IP, iface *Interface) <-chan ProbeResponse {
	ns.NeighbourProbesMu.Lock()

	probeKey := string(neighbour)

	activeProbe, ok := ns.NeighbourProbes[probeKey]
	if !ok {
		entry := ns.Neighbours.Set(neighbour, nil, iface, NeighbourFlagIncomplete)
		activeProbe = &ProbeRequest{
			ns:         ns,
			neighbour:  neighbour,
			iface:      iface,
			waiters:    make(chan chan ProbeResponse, maxProbeWaiters),
			retryCount: maxProbeRetries,
			retryTimer: time.NewTimer(probeRetryInterval),
			entry:      entry,
			state:      ProbePending,
		}
		go activeProbe.startProbe()
		ns.NeighbourProbes[probeKey] = activeProbe
	}

	activeProbe.mu.Lock()
	ns.NeighbourProbesMu.Unlock()
	defer activeProbe.mu.Unlock()

	waiter := make(chan ProbeResponse)
	if activeProbe.state != ProbePending {
		go func() {
			waiter <- ProbeResponse{State: activeProbe.state, Entry: &activeProbe.entry}
			close(waiter)
		}()
		return waiter
	}

	for {
		select {
		case activeProbe.waiters <- waiter:
			return waiter
		default:
			//too many active waiters, pop a waiter and try again
			oldWaiter := <-activeProbe.waiters
			select {
			case oldWaiter <- ProbeResponse{State: ProbePacketAged, Entry: &activeProbe.entry}:
			default:
			}
			close(oldWaiter)
		}
	}
}

func GetNextHop(ns *NetSpace, dst net.IP, af AddressFamily) (net.IP, ForwardingRule, bool) {
	ns.FIBCacheMu.RLock()
	fr, ok := ns.FIBCache[string(dst)]
	if ok {
		ns.FIBCacheMu.RUnlock()
		nextHop := dst
		if fr.Scope == FRSUniverse {
			nextHop = fr.NextHop
		}
		return nextHop, fr, true
	}
	ns.FIBCacheMu.RUnlock()

	var fibRule ForwardingRule
	weight := 0

	_, dstRules := ns.FIB.LongestPrefixMatch(dst, af)
	if len(dstRules) == 0 {
		return nil, ForwardingRule{}, false
	}

	for _, r := range dstRules {
		if r.Weight < weight {
			continue
		}
		weight = r.Weight
		fibRule = r
	}

	dstIP := make(net.IP, len(dst))
	copy(dstIP, dst)

	var nextHop = dstIP
	if fibRule.Scope == FRSUniverse {
		nextHop = fibRule.NextHop
	}

	ns.FIBCacheMu.Lock()
	defer ns.FIBCacheMu.Unlock()
	ns.FIBCache[string(dst)] = fibRule

	return nextHop, fibRule, true
}

func GetNextHopMacAddress(ns *NetSpace, dst net.IP, af AddressFamily) (NeighbourEntry, bool) {
	nextHop, _, ok := GetNextHop(ns, dst, af)
	if !ok {
		return NeighbourEntry{}, false
	}

	return ns.Neighbours.Get(nextHop)
}

func SolicitedNodeMulticastFromIP(ip net.IP, dest net.IP) {
	if len(ip) != net.IPv6len {
		return
	}

	dest[0] = 0xff
	dest[1] = 0x2
	dest[2] = 0
	dest[3] = 0
	dest[4] = 0
	dest[5] = 0
	dest[6] = 0
	dest[7] = 0
	dest[8] = 0
	dest[9] = 0
	dest[10] = 0
	dest[11] = 1
	dest[12] = 0xff
	dest[13] = ip[13]
	dest[14] = ip[14]
	dest[15] = ip[15]
}
