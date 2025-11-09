package main

import (
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

// Transmit a signal's encoding in a Signal Control frame
// so the Interface can store it in its EEPROM.
func (sig SignalDef) SendEncoding(bus canbus.Bus) error {
	req := SignalControlRequest{sig.index}
	reply := &SignalDef{}
	isReply := func(reply *SignalDef) bool { return true }
	return sendCtrlFrame(sig, req, reply, bus, isReply, verifySigCtrlReply)
}

// Verify that the response to a Signal Control REMOTE REQUEST
// is the same as what was commanded to be written.
func verifySigCtrlReply(cmd SignalDef, reply *SignalDef) bool {
	reply.index = cmd.index // ignore index and name of reply
	reply.name = cmd.name
	return *reply == cmd
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
		return fmt.Errorf("wrong DLC for Signal Control frame: %d", frame.Length)
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

// SignalControlRequest is a Signal Control REMOTE REQUEST frame.
type SignalControlRequest struct {
	sigIndex uint8
}

func (r SignalControlRequest) MarshalFrame() (can.Frame, error) {
	return can.Frame{
		ID:         sigCtrlId | uint32(r.sigIndex&0xF),
		IsRemote:   true,
		IsExtended: true,
	}, nil
}
