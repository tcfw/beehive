package network

import (
	"bytes"
	"net"
	"testing"
)

func TestARPConstruct(t *testing.T) {
	arp := make(ARP, ARPFrameMinSize)
	arp.SetHardwareType(ARPHardwareTypeEthernet)
	arp.SetProtocolType(uint16(EtherType_IPv4))
	arp.SetHardwareAddrLen(6)
	arp.SetProtocolAddrLen(4)
	arp.SetOperation(ARPOperationRequest)
	arp.SetSenderHardwareAddress(MacAddress{1, 2, 3, 4, 5, 6})
	arp.SetSenderProtocolAddress(net.IP{1, 2, 3, 4})
	arp.SetTargetHardwareAddress(EmptyMacAddress)
	arp.SetTargetProtocolAddress(net.IP{4, 3, 2, 1})

	expected := []byte{
		0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x01, 0x02, 0x03, 0x04,
		0x05, 0x06, 0x01, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x04, 0x03, 0x02, 0x01,
	}

	if !bytes.Equal(arp, expected) {
		t.Fatalf("arp construct failed, expected %X, got %X", expected, []byte(arp))
	}
}
