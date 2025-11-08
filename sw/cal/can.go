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

// Write a calibration table to the EEPROM.
func writeTable(tx *socketcan.Transmitter, tbl Table, sig int) error {
	var i int
	for i = 0; i < len(tbl.keys); i++ {
		err := writeRow(tx, tbl.keys[i], tbl.vals[i], sig, i)
		if err != nil {
			return err
		}
	}
	for ; i < tabRows; i++ {
		err := writeRow(tx, tbl.keys[len(tbl.keys)-1], tbl.vals[len(tbl.keys)-1], sig, i)
		if err != nil {
			return err
		}
	}
	return nil
}

func writeRow(tx *socketcan.Transmitter, key int32, val uint16, sig int, row int) error {
	var data [8]byte
	_, err := bin.Encode(data[0:4], bin.BigEndian, key)
	if err != nil {
		return err
	}
	_, err = bin.Encode(data[4:6], bin.BigEndian, val)
	if err != nil {
		return err
	}

	frame := can.Frame{
		ID:         uint32(tblCtrlId) | uint32((sig<<5)&0xE0) | uint32(row&0x1F),
		Length:     6,
		Data:       data,
		IsExtended: true,
	}
	ctx, _ := context.WithTimeout(context.Background(), timeout)
	return transmit(tx, frame, ctx)

}

func transmit(tx *socketcan.Transmitter, frame can.Frame, ctx context.Context) error {
	for {
		err := tx.TransmitFrame(ctx, frame)
		if err == nil {
			return nil
		} else if strings.HasSuffix(err.Error(), "no buffer space available") {
			continue
		} else {
			return err
		}
	}
}
