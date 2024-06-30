package network

import "net"

type AddressFamily uint

const (
	AddressFamilyIPv4 AddressFamily = iota + 1
	AddressFamilyIPv6
)

type ForwardingTrieNode struct {
	next  map[byte]*ForwardingTrieNode
	rules []ForwardingRule
}

func (t *ForwardingTrieNode) Insert(rule ForwardingRule) {
	if rule.Prefix.IP.IsUnspecified() {
		rule.Prefix.IP = net.IP{}
	}

	currentNode := t
	for _, b := range rule.Prefix.IP {
		if currentNode.next == nil {
			currentNode.next = make(map[byte]*ForwardingTrieNode)
		}
		child, ok := currentNode.next[b]
		if !ok {
			child = &ForwardingTrieNode{}
			currentNode.next[b] = child
		}
		currentNode = child
	}
	currentNode.rules = append(currentNode.rules, rule)
}

func (t *ForwardingTrieNode) LongestPrefixMatch(addr net.IP, family AddressFamily) (*net.IPNet, []ForwardingRule) {
	currentNode := t
	var bestPrefix *net.IPNet
	var bestRuleSet []ForwardingRule

	//populate default
	for _, rule := range currentNode.rules {
		if rule.Family != family {
			continue
		}
		bestPrefix = &currentNode.rules[0].Prefix
		bestRuleSet = currentNode.rules
	}

	for _, b := range addr {
		if currentNode.next == nil {
			break
		}
		childNode, ok := currentNode.next[b]
		if !ok {
			for b != 0 {
				b--
				child, ok := currentNode.next[b]
				if ok {
					childNode = child
					break
				}
			}
			if childNode == nil {
				break
			}
		}

		if len(childNode.rules) > 0 && childNode.rules[0].Family == family {
			if childNode.rules[0].Prefix.Contains(addr) {
				bestPrefix = &childNode.rules[0].Prefix
				bestRuleSet = childNode.rules
			}
		}
		currentNode = childNode
	}
	return bestPrefix, bestRuleSet
}

type ForwardingRouteScope uint

const (
	FRSLocal ForwardingRouteScope = iota
	FRSOnInterface
	FRSUniverse
)

type ForwardingRule struct {
	Weight    int
	Family    AddressFamily
	Prefix    net.IPNet
	NextHop   net.IP
	Interface *Interface
	Scope     ForwardingRouteScope
}
