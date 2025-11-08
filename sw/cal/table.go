package main

import (
	"cmp"
	"context"
	bin "encoding/binary"
	"fmt"
	"slices"
	"time"

	"go.einride.tech/can"

	"git.samanthony.xyz/can_gauge_interface/sw/cal/canbus"
)

const (
	tblCtrlId  uint32 = 0x1272000
	maxTabRows        = 32
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
		time.Sleep(eepromWriteDelay)
	}

	// Fill rest of table with last row
	lastRow := tbl.rows[len(tbl.rows)-1]
	for ; i < maxTabRows; i++ {
		if err := sendRow(tbl.sigIndex, uint8(i), lastRow, bus); err != nil {
			return err
		}
		time.Sleep(eepromWriteDelay)
	}

	return nil
}

// Transmit a Table Control frame containing one row of a table.
func sendRow(sigIndex, rowIndex uint8, row Row, bus canbus.Bus) error {
	// Serialize DATA FIELD
	var data [8]byte
	if _, err := bin.Encode(data[0:4], bin.BigEndian, row.key); err != nil {
		return err
	}
	if _, err := bin.Encode(data[4:6], bin.BigEndian, row.val); err != nil {
		return err
	}

	// Construct ID and send frame
	frame := can.Frame{
		ID:         uint32(tblCtrlId) | uint32((sigIndex<<5)&0xE0) | uint32(rowIndex&0x1F),
		Length:     6,
		Data:       data,
		IsExtended: true,
	}
	ctx, _ := context.WithTimeout(context.Background(), timeout)
	return bus.Send(ctx, frame)
}
