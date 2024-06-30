package network

import (
	"net"
	"testing"
)

func TestFibInsert(t *testing.T) {
	root := ForwardingTrieNode{}
	root.Insert(ForwardingRule{Weight: 100, Family: AddressFamilyIPv4, Prefix: net.IPNet{IP: net.IP{0, 0, 0, 0}, Mask: net.IPMask{0, 0, 0, 0}}, NextHop: net.IP{192, 168, 1, 1}, Scope: FRSUniverse})

	if len(root.rules) != 1 {
		t.Fatal("expected root rules to have 1 rule")
	}
	if len(root.next) != 0 {
		t.Fatal("expected root children to be empty")
	}

	root.Insert(ForwardingRule{Weight: 100, Family: AddressFamilyIPv4, Prefix: net.IPNet{IP: net.IP{1, 0, 0, 0}, Mask: net.IPMask{0, 0, 0, 0}}, NextHop: net.IP{192, 168, 1, 1}, Scope: FRSUniverse})
	if len(root.next) != 1 {
		t.Fatal("expected root children to have 1 child")
	}
	if len(root.next[1].rules) != 0 {
		t.Fatal("expected first children to have no rules")
	}
	if len(root.next[1].next) != 1 {
		t.Fatal("expected first children to have 1 child")
	}
	if len(root.next[1].next[0].rules) != 0 {
		t.Fatal("expected l2 children to have no rules")
	}
	if len(root.next[1].next[0].next) != 1 {
		t.Fatal("expected l2 children to have 1 child")
	}
	if len(root.next[1].next[0].next[0].rules) != 0 {
		t.Fatal("expected l3 children to have no rules")
	}
	if len(root.next[1].next[0].next[0].next) != 1 {
		t.Fatal("expected l3 children to have 1 child")
	}
	if len(root.next[1].next[0].next[0].next[0].rules) != 1 {
		t.Fatal("expected l4 child to have 1 rules")
	}
	if len(root.next[1].next[0].next[0].next[0].next) != 0 {
		t.Fatal("expected l4 children to have 0 child")
	}
}

func TestFibLookup(t *testing.T) {
	root := ForwardingTrieNode{}

	root.Insert(ForwardingRule{Weight: 100, Family: AddressFamilyIPv4, Prefix: net.IPNet{IP: net.IP{0, 0, 0, 0}, Mask: net.IPMask{0, 0, 0, 0}}, NextHop: net.IP{192, 168, 1, 1}, Scope: FRSUniverse})
	root.Insert(ForwardingRule{Weight: 100, Family: AddressFamilyIPv4, Prefix: net.IPNet{IP: net.IP{1, 1, 0, 0}, Mask: net.IPMask{255, 255, 255, 0}}, NextHop: net.IP{192, 168, 1, 2}, Scope: FRSUniverse})
	root.Insert(ForwardingRule{Weight: 100, Family: AddressFamilyIPv4, Prefix: net.IPNet{IP: net.IP{159, 196, 46, 32}, Mask: net.IPMask{255, 255, 255, 252}}, Scope: FRSOnInterface})
	root.Insert(ForwardingRule{Weight: 100, Family: AddressFamilyIPv6, Prefix: net.IPNet{IP: net.IPv6linklocalallnodes, Mask: net.IPMask{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}}, Scope: FRSOnInterface})

	prefix, rules := root.LongestPrefixMatch(net.IP{159, 196, 46, 33}, AddressFamilyIPv4)
	if prefix == nil {
		t.Fatal("no prefix found")
	}
	if rules == nil {
		t.Fatal("should have gotten a rule")
	}
	if rules[0].Scope != FRSOnInterface {
		t.Fatal("should have gotten a on interface rule")
	}

	prefix, rules = root.LongestPrefixMatch(net.IP{1, 1, 0, 33}, AddressFamilyIPv4)
	if prefix == nil {
		t.Fatal("no prefix found")
	}
	if rules == nil {
		t.Fatal("should have gotten a rule")
	}
	if rules[0].Scope != FRSUniverse {
		t.Fatal("should have gotten a on interface rule")
	}
	if !rules[0].NextHop.Equal(net.IP{192, 168, 1, 2}) {
		t.Fatal("unexpected next hop for 1.1.1.33")
	}

	prefix, rules = root.LongestPrefixMatch(net.IP{172, 1, 1, 33}, AddressFamilyIPv4)
	if prefix == nil {
		t.Fatal("no default prefix found")
	}
	if rules == nil {
		t.Fatal("should have gotten a rule")
	}
	if rules[0].Scope != FRSUniverse {
		t.Fatal("should have gotten a on interface rule")
	}
	if !rules[0].NextHop.Equal(net.IP{192, 168, 1, 1}) {
		t.Fatal("unexpected next hop for 172.1.1.33")
	}

	prefix, rules = root.LongestPrefixMatch(net.IPv6linklocalallnodes, AddressFamilyIPv6)
	if prefix == nil {
		t.Fatal("no prefix found")
	}
	if rules == nil {
		t.Fatal("should have gotten a rule")
	}
	if rules[0].Scope != FRSOnInterface {
		t.Fatal("should have gotten a on interface rule")
	}
}
