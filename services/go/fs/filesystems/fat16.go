package filesystems

import "encoding/binary"

type FAT16SuperBlock []byte

func (s FAT16SuperBlock) JumpCode() []byte {
	return s[0:3]
}

func (s FAT16SuperBlock) OEMName() []byte {
	return s[3:11]
}

func (s FAT16SuperBlock) BytesPerSector() uint16 {
	return binary.LittleEndian.Uint16(s[11:])
}

func (s FAT16SuperBlock) SectorsPerCluster() uint8 {
	return s[13]
}

func (s FAT16SuperBlock) ReservedSectors() uint16 {
	return binary.LittleEndian.Uint16(s[14:])
}

func (s FAT16SuperBlock) FATCopies() uint8 {
	return s[16]
}

func (s FAT16SuperBlock) NumRootDirs() uint16 {
	return binary.LittleEndian.Uint16(s[17:])
}

func (s FAT16SuperBlock) TotalNumberOfSectors() uint16 {
	return binary.LittleEndian.Uint16(s[19:])
}

func (s FAT16SuperBlock) MediaType() uint8 {
	return s[21]
}

func (s FAT16SuperBlock) SectorsPerFAT() uint16 {
	return binary.LittleEndian.Uint16(s[22:])
}

func (s FAT16SuperBlock) SectorsPerTrack() uint16 {
	return binary.LittleEndian.Uint16(s[24:])
}

func (s FAT16SuperBlock) SectorsPerHeads() uint16 {
	return binary.LittleEndian.Uint16(s[26:])
}

func (s FAT16SuperBlock) NumberOfHiddenSectors() uint32 {
	return binary.LittleEndian.Uint32(s[28:])
}

func (s FAT16SuperBlock) NumberOfSectorsInFileSystem() uint32 {
	return binary.LittleEndian.Uint32(s[32:])
}

func (s FAT16SuperBlock) LogicalDriveNumber() uint8 {
	return s[36]
}

func (s FAT16SuperBlock) ExtendedSignature() uint8 {
	return s[38]
}

func (s FAT16SuperBlock) SerialNumber() uint32 {
	return binary.LittleEndian.Uint32(s[39:])
}

func (s FAT16SuperBlock) VolumeLabel() []byte {
	return s[43:54]
}

func (s FAT16SuperBlock) FileSystemType() []byte {
	return s[54:62]
}

func (s FAT16SuperBlock) Signature() []byte {
	return s[510:512]
}
