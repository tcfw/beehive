package utils

import (
	"unsafe"
)

// ContiguousObjectArray is a funky way to replicate arbitrary arrays with header and footer parameters
type ContiguousObjectArray struct {
	HeaderSize int
	FooterSize int
	ObjectSize int

	Arena []byte
}

func NewContiguousObjectArray[T any, HT any, FT any](n int) *ContiguousObjectArray {
	var v T
	var vh HT
	var vf FT

	objSize := int(unsafe.Sizeof(v))
	hpadding := int(unsafe.Sizeof(vh))
	fpadding := int(unsafe.Sizeof(vf))

	return &ContiguousObjectArray{
		HeaderSize: hpadding,
		FooterSize: fpadding,
		ObjectSize: objSize,

		Arena: make([]byte, (n*objSize)+hpadding+fpadding),
	}
}

func (c ContiguousObjectArray) DataPointer() uintptr {
	return uintptr(unsafe.Add(unsafe.Pointer(unsafe.SliceData(c.Arena)), c.HeaderSize))
}

func ContiguousObjectArrayAsSlice[T any](c ContiguousObjectArray) []T {
	return ([]T)(unsafe.Slice(ContiguousObjectArrayItem[T](c, 0), (len(c.Arena)-c.HeaderSize-c.FooterSize)/c.ObjectSize))[:]
}

func ContiguousObjectArrayHeader[T any](c ContiguousObjectArray) *T {
	return (*T)(unsafe.Pointer(unsafe.SliceData(c.Arena)))
}

func ContiguousObjectArrayFooter[T any](c ContiguousObjectArray) *T {
	footerOffset := len(c.Arena) - c.HeaderSize
	return (*T)(unsafe.Add(unsafe.Pointer(unsafe.SliceData(c.Arena)), footerOffset))
}

func ContiguousObjectArrayAsT[T any](c ContiguousObjectArray) *T {
	return (*T)(unsafe.Pointer(unsafe.SliceData(c.Arena)))
}

func ContiguousObjectArrayItem[T any](c ContiguousObjectArray, nth int) *T {
	offset := c.HeaderSize + (nth * c.ObjectSize)
	return (*T)(unsafe.Add(unsafe.Pointer(unsafe.SliceData(c.Arena)), offset))
}
