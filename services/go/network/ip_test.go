package network

import (
	"bytes"
	"net"
	"testing"
)

func TestMulticastMACAddressFromIP(t *testing.T) {
	tests := []struct {
		ip       net.IP
		expected MacAddress
	}{
		{
			net.ParseIP("ff02::1:ff28:9c5a"),
			MacAddress{0x33, 0x33, 0xff, 0x28, 0x9c, 0x5a},
		},
		{
			net.ParseIP("ff02::1"),
			MacAddress{0x33, 0x33, 0x00, 0x00, 0x00, 0x1},
		},
	}

	for _, test := range tests {
		actual := make(MacAddress, 6)
		MulticastMACAddressFromIP(test.ip, actual)
		if !bytes.Equal(actual, test.expected) {
			t.Fatalf("multicast mac address was wrong, expected %X, got %X", test.expected, actual)
		}
	}
}

func TestIPv6FromSlice(t *testing.T) {
	hdr := IPv6Header{
		0x60, 0x1F, 0xEF, 0xFF,

		0x01, 0x23, 0x11, 0x3d,

		0x24, 0x04, 0x68, 0x00, 0x40, 0x15, 0x08, 0x03,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0a,

		0x24, 0x03, 0x58, 0x06, 0xf1, 0xe9, 0x00, 0x00,
		0xa4, 0x67, 0x5c, 0x0b, 0xc5, 0x29, 0xb2, 0xff,
	}

	if hdr.Version() != 0x6 {
		t.Fatalf("unexpected ipv6 version header, expected 0x6, got 0x%x", hdr.Version())
	}

	if hdr.TrafficClass() != 1 {
		t.Fatalf("unexpected ipv6 version header, expected 1, got 0x%x", hdr.TrafficClass())
	}

	if hdr.FlowLabel() != 0x0FEFFF {
		t.Fatalf("unexpected ipv6 flow label, expected 0x0FEFFF, got 0x%X", hdr.FlowLabel())
	}

	if hdr.Length() != 291 {
		t.Fatalf("unexpected ipv6 payload length, expected 291, got %d", hdr.Length())
	}

	if hdr.NextHeader() != 17 {
		t.Fatalf("unexpected ipv6 payload next header, expected 17 (UDP), got %d", hdr.NextHeader())
	}
}
