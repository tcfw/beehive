package network

import (
	"context"
	"runtime"
	"runtime/trace"
)

var (
	nWorkers = runtime.NumCPU()
	workCh   = make(chan *Packet, 200)
)

func init() {
	// startPacketWorkers()
}

func startPacketWorkers() {
	for i := 0; i < nWorkers; i++ {
		go doPacketWork(i)
	}
}

func doPacketWork(id int) {
	for p := range workCh {
		trace.WithRegion(context.Background(), "handlePacket", func() {
			HandleEthernetFrame(p)
		})

	}
}

func handlePacket(p *Packet) {
	workCh <- p
}
