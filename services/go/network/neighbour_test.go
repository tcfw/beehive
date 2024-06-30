package network

import (
	"net"
	"testing"
	"time"
)

func BenchmarkNeighbourSet(b *testing.B) {
	neigh := NewNeighbourCache(10 * time.Second)
	ip := net.IP{1, 1, 1, 1}
	mac := MacAddress{1, 2, 3, 4, 5, 6}

	for n := 0; n < b.N; n++ {
		neigh.Set(ip, mac, nil, 0)
	}
}

func BenchmarkNeighbourGet(b *testing.B) {
	neigh := NewNeighbourCache(10 * time.Second)
	for i := uint(0); i < 1000; i++ {
		neigh.Set(net.IP{byte(i / 1024 % 255), byte(i / 512 % 255), byte(i / 256 % 255), byte(i % 255)}, MacAddress{1, 2, 3, 4, 5, 6}, nil, 0)
	}

	b.ResetTimer()

	for n := 0; n < b.N; n++ {
		_, ok := neigh.Get(net.IP{0, 0, 0, 1})
		if !ok {
			b.Fatal("should have received a neighbour")
		}
	}
}

func BenchmarkNeighbourGetNotFound(b *testing.B) {
	neigh := NewNeighbourCache(10 * time.Second)
	for i := uint(1); i < 1000; i++ {
		neigh.Set(net.IP{byte(i / 1024 % 255), byte(i / 512 % 255), byte(i / 256 % 255), byte(i % 255)}, MacAddress{1, 2, 3, 4, 5, 6}, nil, 0)
	}

	b.ResetTimer()

	for n := 0; n < b.N; n++ {
		_, ok := neigh.Get(net.IP{0, 0, 0, 0, 0, 1})
		if ok {
			b.Fatal("should not have received a neighbour")
		}
	}
}

func TestSolicitedNodeMulticastFromIP(t *testing.T) {
	tests := []struct {
		ip       net.IP
		expected net.IP
	}{
		{
			ip:       net.ParseIP("2001:db8::2aa:ff:fe28:9c5a"),
			expected: net.ParseIP("ff02::1:ff28:9c5a"),
		},
		{
			ip:       net.ParseIP("2001:db8::1"),
			expected: net.ParseIP("ff02::1:ff00:1"),
		},
	}

	for _, test := range tests {
		actual := make(net.IP, net.IPv6len)
		SolicitedNodeMulticastFromIP(test.ip, actual)
		if !test.expected.Equal(actual) {
			t.Fatalf("solicited node multicast ip was wrong, expected %s got %s", test.expected, actual)
		}
	}
}
