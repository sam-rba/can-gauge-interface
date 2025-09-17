package usb

import (
	"fmt"

	"github.com/google/gousb"
)

const (
	vendorId  gousb.ID = 0x04d8
	productId gousb.ID = 0x000a

	inEndpointAddr  = 0x82
	outEndpointAddr = 0x02
)

type Device struct {
	ctx       *gousb.Context
	dev       *gousb.Device
	closeIntf func()
	in        *gousb.ReadStream  // IN endpoint (device->host)
	out       *gousb.WriteStream // OUT endpoint (host->device)
}

func Connect() (*Device, error) {
	ctx := gousb.NewContext()

	dev, err := ctx.OpenDeviceWithVIDPID(vendorId, productId)
	if err != nil {
		ctx.Close()
		return nil, err
	}

	intf, closeIntf, err := interfaceByClass(dev, gousb.ClassData)
	if err != nil {
		dev.Close()
		ctx.Close()
		return nil, err
	}

	inEp, err := intf.InEndpoint(inEndpointAddr)
	if err != nil {
		closeIntf()
		dev.Close()
		ctx.Close()
		return nil, err
	}

	inEpDesc, ok := intf.Setting.Endpoints[inEndpointAddr]
	if !ok {
		closeIntf()
		dev.Close()
		ctx.Close()
		return nil, fmt.Errorf("no such endpoint: %x", inEndpointAddr)
	}

	in, err := inEp.NewStream(inEpDesc.MaxPacketSize, 1)
	if err != nil {
		closeIntf()
		dev.Close()
		ctx.Close()
		return nil, err
	}

	outEp, err := intf.OutEndpoint(outEndpointAddr)
	if err != nil {
		closeIntf()
		dev.Close()
		ctx.Close()
		return nil, err
	}

	outEpDesc, ok := intf.Setting.Endpoints[outEndpointAddr]
	if !ok {
		closeIntf()
		dev.Close()
		ctx.Close()
		return nil, fmt.Errorf("no such endpoint: %x", outEndpointAddr)
	}

	out, err := outEp.NewStream(outEpDesc.MaxPacketSize, 1)
	if err != nil {
		closeIntf()
		dev.Close()
		ctx.Close()
		return nil, err
	}

	return &Device{
		ctx,
		dev,
		closeIntf,
		in, out,
	}, nil
}

func interfaceByClass(dev *gousb.Device, class gousb.Class) (intf *gousb.Interface, close func(), err error) {
	cfgNum, err := dev.ActiveConfigNum()
	if err != nil {
		return nil, nil, fmt.Errorf("failed to get active config number of device %s: %v", dev, err)
	}
	cfg, err := dev.Config(cfgNum)
	if err != nil {
		return nil, nil, fmt.Errorf("failed to claim config %d of device %s: %v", cfgNum, dev, err)
	}

	intfNum, altNum, ok := func() (int, int, bool) {
		for i, intf := range cfg.Desc.Interfaces {
			for a, alt := range intf.AltSettings {
				if alt.Class == class {
					return i, a, true
				}
			}
		}
		return -1, -1, false
	}()
	if !ok {
		cfg.Close()
		return nil, nil, fmt.Errorf("failed to get %s interface of device %s: %v", class, dev, err)
	}

	intf, err = cfg.Interface(intfNum, altNum)
	if err != nil {
		cfg.Close()
		return nil, nil, fmt.Errorf("failed to select interface (%d,%d) of config %d of device %s: %v", intfNum, altNum, cfgNum, dev, err)
	}

	return intf, func() {
		intf.Close()
		cfg.Close()
	}, nil
}

func (dev *Device) Close() error {
	outErr := dev.out.Close()
	inErr := dev.in.Close()
	dev.closeIntf()
	devErr := dev.dev.Close()
	ctxErr := dev.ctx.Close()

	return nonNil(outErr, inErr, devErr, ctxErr)
}

func nonNil(errs ...error) error {
	for _, err := range errs {
		if err != nil {
			return err
		}
	}
	return nil
}

// Read from the IN endpoint (device->host)
func (dev *Device) Read(p []byte) (n int, err error) {
	return dev.in.Read(p)
}

// Write to the OUT endpoint (host->device)
func (dev *Device) Write(p []byte) (n int, err error) {
	return dev.out.Write(p)
}
