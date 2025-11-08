package main

import (
	"context"
	bin "encoding/binary"
	"strings"
	"time"

	"go.einride.tech/can"
	"go.einride.tech/can/pkg/socketcan"
)

const (
	tblCtrlId = 0x1272000
	sigCtrlId = 0x1272100

	timeout = 5 * time.Second
)

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
	}

	return nil
}

// Transmit a Table Control frame containing one row of a table.
func sendRow(tx *socketcan.Transmitter, key int32, val uint16, sig int, row int) error {
	// Serialize DATA FIELD
	var data [8]byte
	_, err := bin.Encode(data[0:4], bin.BigEndian, key)
	if err != nil {
		return err
	}
	_, err = bin.Encode(data[4:6], bin.BigEndian, val)
	if err != nil {
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
