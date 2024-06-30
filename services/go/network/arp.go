package network

import (
	"encoding/binary"
	"net"
)

const (
	ARPOperationRequest     = uint16(1)
	ARPOperationReply       = uint16(2)
	ARPHardwareTypeEthernet = uint16(1)

	ARPFrameMinSize = 28
)

type ARP []byte

func (a ARP) HardwareType() uint16 {
	return binary.BigEndian.Uint16(a[0:])
}

func (a ARP) SetHardwareType(t uint16) {
	binary.BigEndian.PutUint16(a[0:], t)
}

func (a ARP) ProtocolType() uint16 {
	return binary.BigEndian.Uint16(a[2:])
}

func (a ARP) SetProtocolType(t uint16) {
	binary.BigEndian.PutUint16(a[2:], t)
}

func (a ARP) HardwareAddrLen() uint8 {
	return a[4]
}

func (a ARP) SetHardwareAddrLen(l uint8) {
	a[4] = l
}

func (a ARP) ProtocolAddrLen() uint8 {
	return a[5]
}

func (a ARP) SetProtocolAddrLen(l uint8) {
	a[5] = l
}

func (a ARP) Operation() uint16 {
	return binary.BigEndian.Uint16(a[6:])
}

func (a ARP) SetOperation(o uint16) {
	binary.BigEndian.PutUint16(a[6:], o)
}

func (a ARP) SenderHardwareAddress() MacAddress {
	return MacAddress(a[8:14])
}

func (a ARP) SetSenderHardwareAddress(m MacAddress) {
	copy(a[8:14], m)
}

func (a ARP) SenderProtocolAddress() net.IP {
	return net.IP(a[14:18])
}

func (a ARP) SetSenderProtocolAddress(ip net.IP) {
	copy(a[14:18], ip)
}

func (a ARP) TargetHardwareAddress() MacAddress {
	return MacAddress(a[18:24])
}

func (a ARP) SetTargetHardwareAddress(m MacAddress) {
	copy(a[18:24], m)
}

func (a ARP) TargetProtocolAddress() net.IP {
	return net.IP(a[24:28])
}

func (a ARP) SetTargetProtocolAddress(ip net.IP) {
	copy(a[24:28], ip)
}
