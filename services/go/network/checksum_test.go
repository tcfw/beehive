package network

import (
	"testing"
)

func TestHash16OnesComplement(t *testing.T) {
	d := []byte{0x01, 0x00, 0xF2, 0x03, 0xF4, 0xF5, 0xF6, 0xF7, 0x00, 0x00}

	csum := Checksum16OnesComplement(d)
	if csum != 0x0E21 {
		t.Fatalf("unexpected 16 One's completement sum, expected 0x210E, got 0x%X", csum)
	}
}

func BenchmarkHash16OnesComplement(b *testing.B) {
	d := []byte{0x01, 0x00, 0xF2, 0x03, 0xF4, 0xF5, 0xF6, 0xF7, 0x00, 0x00}

	for n := 0; n < b.N; n++ {
		Checksum16OnesComplement(d)
	}
}
