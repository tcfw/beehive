package partition

import (
	"bytes"
)

const (
	gptSignature = "EFI PART"
)

func IsGPTTable(d []byte) bool {
	gptSignatureBytes := []byte(gptSignature)
	return bytes.Equal(d[:len(gptSignatureBytes)], gptSignatureBytes)
}
