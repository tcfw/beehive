package network

import "sync"

var (
	defaultNetSpace NetSpace
)

type NetSpace struct {
	ID       int
	FIB      ForwardingTrieNode
	FIBCache map[string]ForwardingRule
	FIBCacheMu sync.RWMutex

	Neighbours        *NeighbourCache
	NeighbourProbes   map[string]*ProbeRequest
	NeighbourProbesMu sync.Mutex
}

func init() {
	defaultNetSpace.Neighbours = NewNeighbourCache(defaultNeighbourTimeout)
	defaultNetSpace.NeighbourProbes = make(map[string]*ProbeRequest)
	defaultNetSpace.FIBCache = make(map[string]ForwardingRule)
}

func DefaultNetSpace() *NetSpace {
	return &defaultNetSpace
}
