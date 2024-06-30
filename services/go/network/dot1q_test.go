package network

import "testing"

func TestDot1QVLAN(t *testing.T) {
	d := Dot1Q(0x81000001)

	if d.VLAN() != 1 {
		t.Fatal("unexpected vlan")
	}
}
