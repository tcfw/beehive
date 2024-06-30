package network

type Dot1Q uint32

func (d Dot1Q) VLAN() uint16 {
	return uint16(d & 0x1FFF)
}

func (d Dot1Q) DropEligible() bool {
	return d&0x2000 > 0
}

func (d Dot1Q) Priority() uint8 {
	return uint8(d & 0xE000 >> 13)
}
