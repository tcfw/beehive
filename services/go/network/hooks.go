package network

type HookAction uint

const (
	HookActionNOOP HookAction = iota
	HookActionDROP
	HookActionFORWARD
)

type HookFnAction struct {
	Action        HookAction
	NextInterface *Interface
}

type HookFn func(*Packet) HookFnAction

var (
	HookEthernetPacketRX HookFn
	HookIPv6PacketRX     HookFn
	HookIPv4PacketRX     HookFn
	HookICMPv6PacketRX   HookFn
)

func init() {
	defaultHook := func(p *Packet) HookFnAction { return HookFnAction{HookActionNOOP, nil} }

	HookEthernetPacketRX = defaultHook
	HookIPv6PacketRX = defaultHook
	HookIPv4PacketRX = defaultHook
	HookICMPv6PacketRX = defaultHook
}

func ChainHookBefore(org HookFn, fn HookFn) HookFn {
	return func(p *Packet) HookFnAction {
		res := fn(p)

		if res.Action == HookActionNOOP {
			return org(p)
		}

		return res
	}
}

func ChainHookAfter(org HookFn, fn HookFn) HookFn {
	return func(p *Packet) HookFnAction {
		res := org(p)

		if res.Action == HookActionNOOP {
			return fn(p)
		}

		return res
	}
}
