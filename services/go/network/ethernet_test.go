package network

import (
	"bytes"
	"fmt"
	"testing"
)

func TestMacAddressIsMcast(t *testing.T) {
	//ipv4
	addr := MacAddress{0x01, 0, 0, 0, 0, 0x1}
	if !addr.IsMcast() {
		t.Fatal("mac address should have been multicast")
	}

	//ipv6
	addr = MacAddress{0x33, 0x33, 0, 0, 0, 0x2}
	if !addr.IsMcast() {
		t.Fatal("mac address should have been multicast")
	}

	//not multicast
	addr = MacAddress{0xd8, 0x34, 0xc5, 0xfe, 0x76, 0x1b}
	if addr.IsMcast() {
		t.Fatal("mac address should *not* have been multicast")
	}
}

func TestMacAddressIsBcast(t *testing.T) {
	addr := MacAddress{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
	if !addr.IsBcast() {
		t.Fatal("mac address should have been multicast")
	}

	//not broadcast
	addr = MacAddress{0xd8, 0x34, 0xc5, 0xfe, 0x76, 0x1b}
	if addr.IsBcast() {
		t.Fatal("mac address should *not* have been multicast")
	}
}

func BenchmarkMacAddressIsUnicast(b *testing.B) {
	b.Run("fast path", func(b *testing.B) {
		addr := MacAddress{0x02, 0, 0, 0, 0, 0x1}

		for n := 0; n < b.N; n++ {
			if !addr.IsUnicast() {
				b.Fatal("mac address should have been multicast")
			}
		}
	})

	b.Run("slow path", func(b *testing.B) {
		addr := MacAddress{0xFF, 0, 0, 0, 0, 0x1}

		for n := 0; n < b.N; n++ {
			if !addr.IsUnicast() {
				b.Fatal("mac address should have been multicast")
			}
		}
	})
}

func TestEthernetRead(t *testing.T) {
	tests := []struct {
		frame     []byte
		dst       MacAddress
		src       MacAddress
		etherType EtherType
		dot1q     Dot1Q
		payload   []byte
	}{
		{
			//ipv4 frame
			frame: []byte{0x7c, 0x2e, 0xbd, 0x2b, 0x70, 0x16, 0x60, 0x3e, 0x5f, 0x39, 0x1b, 0xd2, 0x08, 0x00},
			dst:   MacAddress{0x7c, 0x2e, 0xbd, 0x2b, 0x70, 0x16}, src: MacAddress{0x60, 0x3e, 0x5f, 0x39, 0x1b, 0xd2}, etherType: EtherType_IPv4,
		},
		{
			//ipv6 frame
			frame: []byte{0x60, 0x3e, 0x5f, 0x39, 0x1b, 0xd2, 0x76, 0xac, 0xb9, 0xd9, 0xc7, 0xbe, 0x86, 0xdd},
			dst:   MacAddress{0x60, 0x3e, 0x5f, 0x39, 0x1b, 0xd2}, src: MacAddress{0x76, 0xac, 0xb9, 0xd9, 0xc7, 0xbe}, etherType: EtherType_IPv6,
		},
		{
			//dot1q frame
			frame: []byte{0x60, 0x3e, 0x5f, 0x39, 0x1b, 0xd2, 0x76, 0xac, 0xb9, 0xd9, 0xc7, 0xbe, 0x81, 0x0, 0x0, 0x1, 0x86, 0xdd},
			dst:   MacAddress{0x60, 0x3e, 0x5f, 0x39, 0x1b, 0xd2}, src: MacAddress{0x76, 0xac, 0xb9, 0xd9, 0xc7, 0xbe}, dot1q: Dot1Q(0x81000001), etherType: EtherType_IPv6,
		},
	}

	for n, test := range tests {
		t.Run(fmt.Sprintf("ethernet read %d", n), func(t *testing.T) {
			eth := Ethernet(test.frame)

			if !eth.DstMacAddress().Equals(test.dst) {
				t.Fatalf("dst mac address was not equal, expected %x, got %x", test.dst, eth.DstMacAddress())
			}

			if !eth.SrcMacAddress().Equals(test.src) {
				t.Fatalf("src mac address was not equal, expected %x, got %x", test.src, eth.SrcMacAddress())
			}

			if eth.EtherType() != test.etherType {
				t.Fatalf("ethertype was not equal, expected %d, got %d", test.etherType, eth.EtherType())
			}

			if test.dot1q != 0 {
				if eth.Dot1Q() != test.dot1q {
					t.Fatalf("dot1q was not equal, expected %d, got %d", test.dot1q, eth.Dot1Q())
				}
			}
		})
	}
}

func TestEthernetWrite(t *testing.T) {
	tests := []struct {
		frame     []byte
		dst       MacAddress
		src       MacAddress
		etherType EtherType
		dot1q     Dot1Q
		payload   []byte
	}{
		{
			//ipv4 frame
			dst: MacAddress{0x7c, 0x2e, 0xbd, 0x2b, 0x70, 0x16}, src: MacAddress{0x60, 0x3e, 0x5f, 0x39, 0x1b, 0xd2}, etherType: EtherType_IPv4,
			frame: []byte{0x7c, 0x2e, 0xbd, 0x2b, 0x70, 0x16, 0x60, 0x3e, 0x5f, 0x39, 0x1b, 0xd2, 0x08, 0x00},
		},
		{
			//ipv6 frame
			dst: MacAddress{0x60, 0x3e, 0x5f, 0x39, 0x1b, 0xd2}, src: MacAddress{0x76, 0xac, 0xb9, 0xd9, 0xc7, 0xbe}, etherType: EtherType_IPv6,
			frame: []byte{0x60, 0x3e, 0x5f, 0x39, 0x1b, 0xd2, 0x76, 0xac, 0xb9, 0xd9, 0xc7, 0xbe, 0x86, 0xdd},
		},
		{
			//dot1q frame
			dst: MacAddress{0x60, 0x3e, 0x5f, 0x39, 0x1b, 0xd2}, src: MacAddress{0x76, 0xac, 0xb9, 0xd9, 0xc7, 0xbe}, dot1q: Dot1Q(0x81000001), etherType: EtherType_IPv6,
			frame: []byte{0x60, 0x3e, 0x5f, 0x39, 0x1b, 0xd2, 0x76, 0xac, 0xb9, 0xd9, 0xc7, 0xbe, 0x81, 0x0, 0x0, 0x1, 0x86, 0xdd},
		},
	}

	for n, test := range tests {
		t.Run(fmt.Sprintf("ethernet read %d", n), func(t *testing.T) {
			frame := make(Ethernet, len(test.frame))
			frame.SetDstMacAddress(test.dst)
			frame.SetSrcMacAddress(test.src)
			if test.dot1q != 0 {
				frame.SetDot1Q(test.dot1q)
			}
			frame.SetEtherType(test.etherType)

			if !bytes.Equal(frame, test.frame) {
				t.Fatalf("frame does not match expected frame, expected %X, got %X", test.frame, frame)
			}
		})
	}
}
