package main

import (
	"context"
	"fmt"
	"io"
	"os"

	"golang.org/x/sync/errgroup"

	"git.samanthony.xyz/can_gauge_interface/sw/usbcom/usb"
)

const bufSize = 512

func main() {
	exitCode := make(chan int)
	go exit(exitCode)
	defer close(exitCode)

	dev, err := usb.Connect()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error connecting to device: %v\n", err)
		exitCode <- 1
		return
	}
	defer func() {
		if err := dev.Close(); err != nil {
			fmt.Fprintf(os.Stderr, "Error :%v\n", err)
			exitCode <- 1
		}
	}()

	var group errgroup.Group
	ctx, cancel := context.WithCancel(context.Background())

	// Host->Device
	group.Go(func() error {
		err := hostToDevice(dev)
		cancel() // device->host routine
		return err
	})

	// Device->Host
	group.Go(func() error {
		return deviceToHost(ctx, dev)
	})

	if err = group.Wait(); err != nil {
		fmt.Fprintf(os.Stderr, "Error: %v\n", err)
		exitCode <- 1
	}
}

func exit(code <-chan int) {
	worst := 0
	for c := range code {
		if c > worst {
			worst = c
		}
	}
	os.Exit(worst)
}

func hostToDevice(dev *usb.Device) error {
	_, err := io.Copy(dev, os.Stdin)
	return err
}

func deviceToHost(ctx context.Context, dev *usb.Device) error {
	buf := make([]byte, bufSize)
	for {
		select {
		case <-ctx.Done():
			return nil
		default:
		}

		n, err := dev.Read(buf)
		if err != nil {
			return err
		}
		fmt.Print(string(buf[:n]))
	}
}
