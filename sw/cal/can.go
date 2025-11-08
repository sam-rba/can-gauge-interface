package main

import (
	"context"
	bin "encoding/binary"
	"fmt"
	"math"
	"strings"
	"time"

	"go.einride.tech/can"
	"go.einride.tech/can/pkg/socketcan"
)

const (
	tblCtrlId uint32 = 0x1272000
	sigCtrlId uint32 = 0x1272100

	stdMask = 0x7FF
	extMask = 0x1FFFFFFF

	timeout          = 1 * time.Second
	eepromWriteDelay = 5 * time.Millisecond
)

// Transmit a signal's encoding in a Signal Control frame so the Interface can store it in its EEPROM.
func sendEncoding(def SignalDef, sig int, tx *socketcan.Transmitter) error {
	// Serialize DATA FIELD
	var data [8]byte
	if err := serializeEncodingData(def, &data); err != nil {
		return err
	}
	fmt.Println(data)

	// Construct ID and send frame
	frame := can.Frame{
		ID:         sigCtrlId | uint32(sig&0xF),
		Length:     7,
		Data:       data,
		IsExtended: true,
	}
	ctx, _ := context.WithTimeout(context.Background(), timeout)
	return transmit(tx, frame, ctx)
}

// Serialize the DATA FIELD of a Signal Control frame.
func serializeEncodingData(def SignalDef, data *[8]byte) error {
	// SigId field
	if _, err := bin.Encode(data[0:4], bin.BigEndian, uint32(def.id)&extMask); err != nil {
		return err
	}
	if def.id.IsExtended() {
		data[0] |= 0x80 // EXIDE
	}

	// Start field
	if def.start > math.MaxUint8 {
		return fmt.Errorf("%s: start bit out of range: %d>%d", def.name, def.start, math.MaxUint8)
	}
	data[4] = uint8(def.start)

	// Size field
	if def.size > math.MaxUint8 {
		return fmt.Errorf("%s: size out of range: %d>%d", def.name, def.size, math.MaxUint8)
	}
	data[5] = uint8(def.size)

	// Order and sign flag bits
	if !def.isBigEndian {
		data[6] |= 0x80
	}
	if def.isSigned {
		data[6] |= 0x40
	}

	fmt.Println(data)

	return nil
}

// Transmit a table in Table Control frames so the Interface can store it in its EEPROM.
func sendTable(tx *socketcan.Transmitter, tbl Table, sig int) error {
	// Send populated rows
	var i int
	for i = 0; i < len(tbl.keys); i++ {
		err := sendRow(tx, tbl.keys[i], tbl.vals[i], sig, i)
		if err != nil {
			return err
		}
	}

	// Fill rest of table with last row
	for ; i < tabRows; i++ {
		err := sendRow(tx, tbl.keys[len(tbl.keys)-1], tbl.vals[len(tbl.keys)-1], sig, i)
		if err != nil {
			return err
		}
		time.Sleep(eepromWriteDelay)
	}

	return nil
}

// Transmit a Table Control frame containing one row of a table.
func sendRow(tx *socketcan.Transmitter, key int32, val uint16, sig int, row int) error {
	// Serialize DATA FIELD
	var data [8]byte
	if _, err := bin.Encode(data[0:4], bin.BigEndian, key); err != nil {
		return err
	}
	if _, err := bin.Encode(data[4:6], bin.BigEndian, val); err != nil {
		return err
	}

	// Construct ID and send frame
	frame := can.Frame{
		ID:         uint32(tblCtrlId) | uint32((sig<<5)&0xE0) | uint32(row&0x1F),
		Length:     6,
		Data:       data,
		IsExtended: true,
	}
	ctx, _ := context.WithTimeout(context.Background(), timeout)
	return transmit(tx, frame, ctx)
}

// Transmit a CAN frame to the bus, retrying if the buffer is full.
func transmit(tx *socketcan.Transmitter, frame can.Frame, ctx context.Context) error {
	for {
		err := tx.TransmitFrame(ctx, frame)
		if err == nil {
			return nil // success
		} else if strings.HasSuffix(err.Error(), "no buffer space available") {
			continue // retry
		} else {
			return err // error, abort
		}
	}
}
