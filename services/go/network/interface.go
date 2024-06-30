package network

import "net"

type DeviceClass int

const (
	//Virtual
	DeviceClassLoopback DeviceClass = iota + 1
	DeviceClassPointToPoint
	DeviceClassBridge

	//Physical
	DeviceClassEthernet
	DeviceClassBluetooth
)

type DeviceFlags uint

const (
	DeviceFlagUp        DeviceFlags = 1 << iota // interface is administratively up
	DeviceFlagLowerUp                           //interface has a carrier signal
	DeviceFlagBroadcast                         // interface supports broadcast access capability
	DeviceFlagMulticast                         // interface supports multicast access capability
	DeviceFlagRunning                           // interface is in running state
)

type InterfaceCapabilities uint64

const (
	InterfaceCapabilitiesHWCSUM InterfaceCapabilities = 1 << iota
	InterfaceCapabilitiesIPCSUM
	InterfaceCapabilitiesIPV6CSUM
)

type IPFlags uint

const (
	IPFlagTemporary IPFlags = 1 << iota
	IPFlagDynamic
	IPFlagStatic
)

type IP struct {
	net.IPNet

	Flags IPFlags
}

type Interface struct {
	NetSpace *NetSpace

	Index           int
	Class           DeviceClass
	MTU             int
	DefaultHopLimit uint8
	Name            string
	Flags           DeviceFlags
	Capabilities    InterfaceCapabilities

	HardwareAddr      MacAddress
	IPAddrs           []IP // global unicast, link-local, loopback, unique local, etc.
	SubscribedIPAddrs []IP // multicast addresses e.g. solicited-node multicast

	Handlers InterfaceHandlers
	Stats    InterfaceStatistics
}

type InterfaceStatistics struct {
	TXPackets uint64
	TXErr     uint64
	TXDrop    uint64

	RXPackets uint64
	RXErr     uint64
	RXDrop    uint64
}

type InterfaceHandlers interface {
	QueueDisc() QueueDiscipline //the type of queue the interface uses to handle packets
	Len() uint                  // size of the queue, if applicable
	Poll() error                // check if there is packets to dequeue

	// Enqueue (write) a packet to the interface.
	// if the queue is full, the handler should error rather than blocking
	// to prevent blocking other networking activities (packet dropped)
	// on successful enqueuing of the packet, the bool should be true
	// if enqueuing is successful, the handler must acknowledge the packet by calling Done
	// not the caller
	Enqueue(*Packet) (error, bool)

	// Dequeue (read) a packet from the interface
	// If no packets are available to be read, error should be nil and the result be false
	Dequeue() (*Packet, error, bool)
}
