package utils

import (
	"bytes"
	"encoding/binary"
	"testing"
)

func TestRingPush(t *testing.T) {
	ring := NewRing(1, 10)
	ring.Push([]byte{1})

	if *ring.Head != 1 {
		t.Fatal("head index did not move")
	}
}

func TestRingPull(t *testing.T) {
	ring := NewRing(1, 10)
	ring.Push([]byte{1})

	d := ring.Pull()
	if *ring.Tail != *ring.Head {
		t.Fatal("tail should now equal head")
	}
	if !bytes.Equal(d, []byte{1}) {
		t.Fatal("unexpected data pulled from ring")
	}
}

func TestRingPeek(t *testing.T) {
	ring := NewRing(1, 10)
	ring.Push([]byte{1})

	pre := ring.Tail
	d := ring.Peek()
	if ring.Tail != pre {
		t.Fatal("tail should not have moved yet")
	}
	if !bytes.Equal(d, []byte{1}) {
		t.Fatal("unexpected data pulled from ring")
	}
}

func TestPullTo(t *testing.T) {
	ring := NewRing(1, 10)
	ring.Push([]byte{1})

	b := make([]byte, 1)
	if !ring.PullTo(b) {
		t.Fatal("failed to PullTo slice")
	}

	if !bytes.Equal(b, []byte{1}) {
		t.Fatal("unexpected byte in slice")
	}
}

func TestRingFull(t *testing.T) {
	ring := NewRing(2, 3)
	ring.Push([]byte{0, 1})
	ring.Push([]byte{2, 3})
	ring.Push([]byte{4, 5})

	if ring.Push([]byte{5, 6}) {
		t.Fatal("expected queue full error")
	}

	if !bytes.Equal(ring.Data, []byte{0, 1, 2, 3, 4, 5}) {
		t.Fatal("ring in data corrupted by pushes")
	}
}

func TestRingPushSmallerData(t *testing.T) {
	ring := NewRing(2, 2)
	copy(ring.Data, []byte{2, 2, 2, 2})
	ring.Push([]byte{1})

	if !bytes.Equal(ring.Data, []byte{1, 0, 2, 2}) {
		t.Fatal("ring data contains unexpected byte values")
	}
}

func TestRingFromSlice(t *testing.T) {
	b := make([]byte, RingMinSliceSize+10)

	//Set up ring to contain 1 object in the queue

	binary.NativeEndian.PutUint64(b[0:], 10) //len
	binary.NativeEndian.PutUint64(b[8:], 1)  //objectSize
	binary.NativeEndian.PutUint64(b[16:], 0) //Tail
	binary.NativeEndian.PutUint64(b[24:], 1) //Head
	//Data
	b[RingMinSliceSize] = 1

	r := NewRingFromSlice(b)

	if *r.Len != 10 {
		t.Fatal("mapping len failed")
	}

	if !bytes.Equal(r.Pull(), []byte{1}) {
		t.Fatal("pull failed to produce stored value")
	}
}

func BenchmarkRingPushPull(b *testing.B) {
	ring := NewRing(1500, 2)

	b.SetBytes(int64(*ring.ObjectSize))

	for n := 0; n < b.N; n++ {
		if !ring.Push([]byte{1, 2}) {
			b.Fatal("push failed")
		}
		if ring.Pull() == nil {
			b.Fatal("pull failed")
		}
	}
}
