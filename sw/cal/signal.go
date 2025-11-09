package main

import (
	"context"
	bin "encoding/binary"
	"fmt"
	"math"

	"go.einride.tech/can"
	"go.einride.tech/can/pkg/dbc"

	"git.samanthony.xyz/can_gauge_interface/sw/cal/canbus"
)

const (
	sigCtrlId   uint32 = 0x1272100
	sigCtrlMask uint32 = 0x1FFFF00

	exide = 1 << 31
)

type SignalDef struct {
	index       uint8
	id          uint32
	isExtended  bool
	name        string
	start, size uint8
	isBigEndian bool
	isSigned    bool
}

func NewSignalDef(index uint8, msg *dbc.MessageDef, sig dbc.SignalDef) (SignalDef, error) {
	if sig.StartBit > math.MaxUint8 {
		return SignalDef{}, fmt.Errorf("%s: start bit out of range: %d", sig.Name, sig.StartBit)
	}
	if sig.Size > math.MaxUint8 {
		return SignalDef{}, fmt.Errorf("%s: size out of range: %d", sig.Name, sig.Size)
	}

	return SignalDef{
		index,
		uint32(msg.MessageID) & extMask,
		msg.MessageID.IsExtended(),
		string(sig.Name),
		uint8(sig.StartBit),
		uint8(sig.Size),
		sig.IsBigEndian,
		sig.IsSigned,
	}, nil
}

// Transmit a signal's encoding in a Signal Control frame so the Interface can store it in its EEPROM.
func (sig SignalDef) SendEncoding(bus canbus.Bus) error {
	frame, err := sig.MarshalFrame()
	if err != nil {
		return err
	}

	var retry int
	for retry = 0; retry < maxRetries; retry++ {
		// Write to EEPROM
		ctx, _ := context.WithTimeout(context.Background(), timeout)
		if err := bus.Send(ctx, frame); err != nil {
			return err
		}

		// Read back
		request := sigCtrlRequest(sig.index)
		ctx, _ = context.WithTimeout(context.Background(), timeout)
		if err := bus.Send(ctx, request); err != nil {
			return err
		}
		ctx, _ = context.WithTimeout(context.Background(), timeout)
		reply, err := bus.Receive(ctx)
		if err != nil {
			return err
		}
		var rsig SignalDef
		err = rsig.UnmarshalFrame(reply)
		if err == errWrongId {
			continue
		} else if err != nil {
			return err
		}

		// Verify
		rsig.index = sig.index
		rsig.name = sig.name
		fmt.Println("Received signal encoding", rsig)
		if rsig == sig {
			fmt.Printf("Signal %d encoding OK\n", sig.index)
			return nil // success
		} else {
			weprintf("Warning: signal %d verification failed; rewriting...\n", sig.index)
			continue
		}
	}
	// Max retries exceeded
	return fmt.Errorf("signal %d verification failed", sig.index)
}

// Construct a Signal Control REMOTE REQUEST frame.
func sigCtrlRequest(index uint8) can.Frame {
	return can.Frame{
		ID:         sigCtrlId | uint32(index&0xF),
		IsRemote:   true,
		IsExtended: true,
	}
}

func (sig SignalDef) MarshalFrame() (can.Frame, error) {
	var data [8]byte
	if _, err := bin.Encode(data[0:4], bin.BigEndian, uint32(sig.id)&extMask); err != nil {
		return can.Frame{}, err
	}
	if sig.isExtended {
		data[0] |= 0x80 // EXIDE
	}
	data[4] = uint8(sig.start)
	data[5] = uint8(sig.size)
	if !sig.isBigEndian {
		data[6] |= 0x80
	}
	if sig.isSigned {
		data[6] |= 0x40
	}
	return can.Frame{
		ID:         sigCtrlId | uint32(sig.index&0xF),
		Length:     7,
		Data:       data,
		IsExtended: true,
	}, nil
}

func (sig *SignalDef) UnmarshalFrame(frame can.Frame) error {
	if !frame.IsExtended || frame.ID&sigCtrlMask != sigCtrlId {
		return errWrongId
	}
	if frame.Length != 7 {
		return fmt.Errorf("Signal Control frame has wrong DLC: %d", frame.Length)
	}
	sig.index = uint8(frame.ID & 0xF)
	var id uint32
	if _, err := bin.Decode(frame.Data[0:4], bin.BigEndian, &id); err != nil {
		return err
	}
	sig.id = id & extMask
	sig.isExtended = id&exide != 0
	sig.start = frame.Data[4]
	sig.size = frame.Data[5]
	sig.isBigEndian = frame.Data[6]&0x80 == 0
	sig.isSigned = frame.Data[6]&0x40 != 0
	return nil
}
