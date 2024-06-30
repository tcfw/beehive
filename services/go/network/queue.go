package network

type QueueDiscipline uint

const (
	QueueDisciplineNone QueueDiscipline = iota
	QueueDisciplineFIFO
)
