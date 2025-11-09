package main

import (
	"cmp"
	"context"
	bin "encoding/binary"
	"fmt"
	"slices"

	"go.einride.tech/can"

	"git.samanthony.xyz/can_gauge_interface/sw/cal/canbus"
)

const (
	tblCtrlId   uint32 = 0x1272000
	tblCtrlMask uint32 = 0x1FFFF00

	maxTabRows = 32
)

type Table struct {
	sigIndex uint8
	rows     []Row
}

type Row struct {
	key int32
	val uint16
}

func (t *Table) Insert(key int32, val uint16) error {
	if len(t.rows) >= maxTabRows {
		return fmt.Errorf("too many rows")
	}

	i, ok := slices.BinarySearchFunc(t.rows, key, cmpRowKey)
	if ok {
		return ErrDupKey{key}
	}
	t.rows = slices.Insert(t.rows, i, Row{key, val})

	return nil
}

func cmpRowKey(row Row, key int32) int {
	return cmp.Compare(row.key, key)
}

// Transmit a table in Table Control frames so the Interface can store it in its EEPROM.
func (tbl Table) Send(bus canbus.Bus) error {
	// Send populated rows
	var i int
	for i = 0; i < len(tbl.rows); i++ {
		if err := sendRow(tbl.sigIndex, uint8(i), tbl.rows[i], bus); err != nil {
			return err
		}
	}

	// Fill rest of table with last row
	lastRow := tbl.rows[len(tbl.rows)-1]
	for ; i < maxTabRows; i++ {
		if err := sendRow(tbl.sigIndex, uint8(i), lastRow, bus); err != nil {
			return err
		}
	}

	return nil
}

// Transmit a Table Control frame containing one row of a table.
func sendRow(sigIndex, rowIndex uint8, row Row, bus canbus.Bus) error {
	frame, err := marshalTblCtrlFrame(sigIndex, rowIndex, row)
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
		request := tblCtrlRequest(sigIndex, rowIndex)
		ctx, _ = context.WithTimeout(context.Background(), timeout)
		if err := bus.Send(ctx, request); err != nil {
			return err
		}
		ctx, _ = context.WithTimeout(context.Background(), timeout)
		reply, err := bus.Receive(ctx)
		if err != nil {
			return err
		}
		rRow, rSigIndex, rRowIndex, err := unmarshalTblCtrlFrame(reply)
		if err == errWrongId || rSigIndex != sigIndex || rRowIndex != rowIndex {
			continue
		} else if err != nil {
			return err
		}

		// Verify
		if rRow == row {
			fmt.Printf("Table %d row %d OK\n", sigIndex, rowIndex)
			return nil // success
		} else {
			weprintf("Warning: table %d row %d verification failed; rewriting...\n", sigIndex, rowIndex)
			continue
		}
	}
	// Max retries exceeded
	return fmt.Errorf("table %d row %d verification failed", sigIndex, rowIndex)
}

func marshalTblCtrlFrame(sigIndex, rowIndex uint8, row Row) (can.Frame, error) {
	var data [8]byte
	if _, err := bin.Encode(data[0:4], bin.BigEndian, row.key); err != nil {
		return can.Frame{}, err
	}
	if _, err := bin.Encode(data[4:6], bin.BigEndian, row.val); err != nil {
		return can.Frame{}, err
	}
	return can.Frame{
		ID:         uint32(tblCtrlId) | uint32((sigIndex<<5)&0xE0) | uint32(rowIndex&0x1F),
		Length:     6,
		Data:       data,
		IsExtended: true,
	}, nil
}

func unmarshalTblCtrlFrame(frame can.Frame) (row Row, sigIndex, rowIndex uint8, err error) {
	if !frame.IsExtended || frame.ID&tblCtrlMask != tblCtrlId {
		err = errWrongId
		return
	}
	if frame.Length != 6 {
		err = fmt.Errorf("Table Control frame has wrong DLC: %d", frame.Length)
		return
	}
	sigIndex = uint8((frame.ID & 0xE0) >> 5)
	rowIndex = uint8(frame.ID & 0x1F)
	_, err = bin.Decode(frame.Data[0:4], bin.BigEndian, &row.key)
	if err != nil {
		return
	}
	_, err = bin.Decode(frame.Data[4:6], bin.BigEndian, &row.val)
	return
}

// Construct a Table Control REMOTE REQUEST frame.
func tblCtrlRequest(sigIndex, rowIndex uint8) can.Frame {
	return can.Frame{
		ID:         tblCtrlId | uint32((sigIndex<<5)&0xE0) | uint32(rowIndex&0x1F),
		IsRemote:   true,
		IsExtended: true,
	}
}
