package main

import (
	"github.com/tcfw/kernel/services/go/fs"

	_ "github.com/tcfw/kernel/services/go/fs/drivers/virtio"
)

func main() {
	if err := fs.Discover(); err != nil {
		print(err.Error())
		panic(err)
	}

}
