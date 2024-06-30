package utils

import (
	"runtime"
	"sync"
	"sync/atomic"
	"syscall"
	"unsafe"
)

const (
	RingMinSliceSize = 5 * unsafe.Sizeof(uint64(0))
)

// Ring is a MPSC circular queue
type Ring struct {
	Len        *uint64
	ObjectSize *uint64
	Tail       *uint64
	Head       *uint64
	Reserved   *uint64
	Data       []byte

	zero []byte
	mu   sync.Mutex
}

func NewRing(objectSize uint64, len uint64) *Ring {
	var tail, head, reserved uint64
	return &Ring{
		Len:        &len,
		ObjectSize: &objectSize,
		Tail:       &tail,
		Head:       &head,
		Reserved:   &reserved,
		Data:       make([]byte, len*objectSize),
		zero:       make([]byte, objectSize),
	}
}

// NewRingFromPointer makes a new ring using the given
// offset for data location. This should only to be used
// within shared memory regions.
// The data is assumed to all be in arch native endianess
func NewRingFromPointer(p unsafe.Pointer) *Ring {
	r := &Ring{
		Len:        (*uint64)(unsafe.Add(p, 0)),
		ObjectSize: (*uint64)(unsafe.Add(p, 8)),
		Tail:       (*uint64)(unsafe.Add(p, 16)),
		Head:       (*uint64)(unsafe.Add(p, 24)),
		Reserved:   (*uint64)(unsafe.Add(p, 32)),
	}

	r.Data = unsafe.Slice((*byte)(unsafe.Add(p, RingMinSliceSize)), *r.Len**r.ObjectSize)

	r.zero = make([]byte, *r.ObjectSize)

	return r
}

// NewRingFromSlice makes a Ring using the slice data.
// See NewRingFromPointer for specifics
func NewRingFromSlice(d []byte) *Ring {
	return NewRingFromPointer(unsafe.Pointer(unsafe.SliceData(d)))
}

// reserve bumps the reserved index forward, but keeps
// the Head index in place
func (r *Ring) reserve() []byte {
	resv := atomic.LoadUint64(r.Reserved)
	//if ring would be full
	if (resv+1)-atomic.LoadUint64(r.Tail) > *r.Len {
		return nil
	}

	if !atomic.CompareAndSwapUint64(r.Reserved, resv, resv+1) {
		return nil
	}

	index := resv
	from := (index % *r.Len) * *r.ObjectSize
	to := (((index + 1) % *r.Len) * *r.ObjectSize)
	if to < from && to == 0 {
		to = uint64(len(r.Data))
	}
	return r.Data[from : to : from+*r.ObjectSize]
}

// pushReserved bumps the Head index by delta positions.
// Care must be taken when reserving multiple blocks for
// premature pushes
func (r *Ring) pushReserved(delta uint64) uint64 {
	return atomic.AddUint64(r.Head, delta)
}

// Push copies byte slice d to the highest unreserved position
func (r *Ring) Push(d []byte) bool {
	ret := r.PushNoWake(d)

	if runtime.GOOS == "beehive" {
		syscall.RawSyscall(65, uintptr(unsafe.Pointer(r.Head)), 1, 1) //SYS_FUTEX, &r.Head, FUTEX_WAKE, 1
	}

	return ret
}

// PushNoWake bumps the Head index by delta positions
// with the slice, but will not signal other threads
// to wake up with the change to the Head index
func (r *Ring) PushNoWake(d []byte) bool {
	r.mu.Lock()
	defer r.mu.Unlock()

	if uint64(len(d)) > *r.ObjectSize {
		panic("object larger than Ring object size")
	}

	b := r.reserve()
	if b == nil {
		return false
	}

	copy(b, d)
	if len(d) < len(b) {
		//zero out remaining data
		copy(b[len(d):], r.zero)
	}

	r.pushReserved(1)

	return true
}

// Pull provides a slice to the latest Tail index and bump the
// tail index
func (r *Ring) Pull() []byte {
	tail := atomic.LoadUint64(r.Tail)
	// if there's something to read
	if (atomic.LoadUint64(r.Head)) == tail {
		return nil
	}

	if !atomic.CompareAndSwapUint64(r.Tail, tail, tail+1) {
		return nil
	}

	index := tail + 1
	from := ((index - 1) % *r.Len) * *r.ObjectSize
	to := (index % *r.Len) * *r.ObjectSize
	if to < from && to == 0 {
		to = uint64(len(r.Data))
	}
	return r.Data[from : to : from+*r.ObjectSize]
}

// PullWait attempts to pull an available slice from the ring
// However if there's nothing to read will attmept to enter
// into a Futex sleep if on beehiveOS
func (r *Ring) PullWait() []byte {
	iter := 0
	for {
		d := r.Pull()
		if d != nil {
			return d
		}
		iter++

		if iter > 20 {
			if runtime.GOOS == "beehive" {
				syscall.Syscall6(65, uintptr(unsafe.Pointer(r.Head)), 2, 0, 100000000, 0, 0) //SYS_FUTEX, &r.Head, FUTEX_SLEEP, 0, 100ms
			}
		}
	}
}

// Pull provides a slice to the latest Tail index, but does
// not bump the read index
func (r *Ring) Peek() []byte {
	tail := atomic.LoadUint64(r.Tail)
	// if there's something to read
	if (atomic.LoadUint64(r.Head)) == tail {
		return nil
	}

	from := (tail % *r.Len) * *r.ObjectSize
	to := ((tail + 1) % *r.Len) * *r.ObjectSize
	if to < from && to == 0 {
		to = uint64(len(r.Data))
	}
	return r.Data[from : to : from+*r.ObjectSize]
}

// PullWait attempts to pull an available slice from the ring
// However if there's nothing to read will attmept to enter
// into a Futex sleep if on beehiveOS
func (r *Ring) PullToWait(d []byte) {
	iter := 0
	for {
		if r.PullTo(d) {
			return
		}
		iter++

		if iter > 20 {
			if runtime.GOOS == "beehive" {
				syscall.Syscall6(65, uintptr(unsafe.Pointer(r.Head)), 2, 0, 100000000, 0, 0) //SYS_FUTEX, &r.Head, FUTEX_SLEEP, 0, 100ms
			}
		}
	}
}

// PullTo copies the next unread block to the destination byte slice
func (r *Ring) PullTo(d []byte) bool {
	tail := atomic.LoadUint64(r.Tail)
	// if there's something to read
	if (atomic.LoadUint64(r.Head)) == tail {
		return false
	}

	from := (tail % *r.Len) * *r.ObjectSize
	to := ((tail + 1) % *r.Len) * *r.ObjectSize
	if to < from && to == 0 {
		to = uint64(len(r.Data))
	}
	copy(d, r.Data[from:to:from+*r.ObjectSize])
	return atomic.CompareAndSwapUint64(r.Tail, tail, tail+1)
}
