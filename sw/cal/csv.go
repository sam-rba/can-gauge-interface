package main

import (
	"os"
	"fmt"
	"io"
	"strconv"
	"encoding/csv"
)

func parseTable(filename string) (map[int32]uint16, error) {
	f, err := os.Open(filename)
	if err != nil {
		eprintf("%v\n", err)
	}
	defer f.Close()

	tbl := make(map[int32]uint16)
	rdr := csv.NewReader(f)
	for {
		err := parseRow(rdr, tbl)
		if err == io.EOF {
			return tbl, nil
		} else if err != nil {
			return nil, fmt.Errorf("%s:%v", filename, err)
		}
	}
}

func parseRow(rdr *csv.Reader, tbl map[int32]uint16) error {
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
	if _, ok := tbl[int32(key)]; ok {
		line, col := rdr.FieldPos(0)
		return fmt.Errorf("%d: %d: duplicate key %d", line, col, key)
	}
	tbl[int32(key)] = uint16(val)
	return nil
}
