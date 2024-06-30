package network

type ProtocolHandler func(*Packet)

type ProtocolIdentifier uint

const (
	ProtocolIdentifierUnknown ProtocolIdentifier = iota
	ProtocolIdentifierEthernet
	ProtocolIdentifierIPv6
	ProtocolIdentifierIPv4
	ProtocolIdentifierICMPv6
	ProtocolIdentifierARP
	ProtocolIdentifierNDP
)
