package main

import (
	"cmp"
	bin "encoding/binary"
	"encoding/csv"
	"fmt"
	"io"
	"os"
	"slices"
	"strconv"

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
	sigIndex, rowIndex uint8
	key                int32
	val                uint16
}

func parseTable(filename string, sigIndex uint8) (Table, error) {
	f, err := os.Open(filename)
	if err != nil {
		eprintf("%v\n", err)
	}
	defer f.Close()

	tbl := Table{sigIndex, nil}
	rdr := csv.NewReader(f)
	for {
		err := parseRow(rdr, &tbl)
		if err == io.EOF {
			return tbl, nil
		} else if err != nil {
			return Table{}, fmt.Errorf("%s:%v", filename, err)
		}
	}
}

func parseRow(rdr *csv.Reader, tbl *Table) error {
	row, err := rdr.Read()
	if err != nil {
		return err
	}
	if len(row) != 2 {
		line, _ := rdr.FieldPos(0)
		return fmt.Errorf("%d: malformed row", line)
	}
	key, err := strconv.ParseInt(row[0], 10, 32)
	if err != nil {
		line, col := rdr.FieldPos(0)
		return fmt.Errorf("%d:%d: %v", line, col, err)
	}
	val, err := strconv.ParseUint(row[1], 10, 16)
	if err != nil {
		line, col := rdr.FieldPos(1)
		return fmt.Errorf("%d:%d: %v", line, col, err)
	}
	if err := tbl.Insert(int32(key), uint16(val)); err != nil {
		line, col := rdr.FieldPos(0)
		return fmt.Errorf("%d:%d: %v", line, col, err)
	}
	return nil
}

func (tbl *Table) Insert(key int32, val uint16) error {
	if len(tbl.rows) >= maxTabRows {
		return fmt.Errorf("too many rows")
	}

	i, ok := slices.BinarySearchFunc(tbl.rows, key, cmpRowKey)
	if ok {
		return ErrDupKey{key}
	}
	tbl.rows = slices.Insert(tbl.rows, i, Row{tbl.sigIndex, uint8(i), key, val})

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
		if err := tbl.rows[i].Send(bus); err != nil {
			return err
		}
	}

	// Fill rest of table with last row
	lastRow := tbl.rows[len(tbl.rows)-1]
	for ; i < maxTabRows; i++ {
		lastRow.rowIndex = uint8(i)
		if err := lastRow.Send(bus); err != nil {
			return err
		}
	}

	return nil
}

// Transmit a Table Control frame containing one row of a table.
func (row Row) Send(bus canbus.Bus) error {
	req := TableControlRequest{row.sigIndex, row.rowIndex}
	reply := &Row{}
	isReply := func(reply *Row) bool {
		return reply.sigIndex == row.sigIndex && reply.rowIndex == row.rowIndex
	}
	return sendCtrlFrame(row, req, reply, bus, isReply, verifyTblCtrlReply)
}

// Verify that the response to a Table Control REMOTE REQUEST
// is the same as what was commanded to be written.
func verifyTblCtrlReply(cmd Row, reply *Row) bool {
	return *reply == cmd
}

func (row Row) MarshalFrame() (can.Frame, error) {
	var data [8]byte
	if _, err := bin.Encode(data[0:4], bin.BigEndian, row.key); err != nil {
		return can.Frame{}, err
	}
	if _, err := bin.Encode(data[4:6], bin.BigEndian, row.val); err != nil {
		return can.Frame{}, err
	}
	return can.Frame{
		ID:         uint32(tblCtrlId) | uint32((row.sigIndex<<5)&0xE0) | uint32(row.rowIndex&0x1F),
		Length:     6,
		Data:       data,
		IsExtended: true,
	}, nil
}

func (row *Row) UnmarshalFrame(frame can.Frame) error {
	if !frame.IsExtended || frame.ID&tblCtrlMask != tblCtrlId {
		return errWrongId
	}
	if frame.Length != 6 {
		return fmt.Errorf("wrong DLC for Table Control frame: %d", frame.Length)
	}
	row.sigIndex = uint8((frame.ID & 0xE0) >> 5)
	row.rowIndex = uint8(frame.ID & 0x1F)
	if _, err := bin.Decode(frame.Data[0:4], bin.BigEndian, &row.key); err != nil {
		return err
	}
	if _, err := bin.Decode(frame.Data[4:6], bin.BigEndian, &row.val); err != nil {
		return err
	}
	return nil
}

// TableControlRequest is a Table Control REMOTE REQUEST frame.
type TableControlRequest struct {
	sigIndex, rowIndex uint8
}

func (r TableControlRequest) MarshalFrame() (can.Frame, error) {
	return can.Frame{
		ID:         tblCtrlId | uint32((r.sigIndex<<5)&0xE0) | uint32(r.rowIndex&0x1F),
		IsRemote:   true,
		IsExtended: true,
	}, nil
}
