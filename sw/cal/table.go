package main

import (
	"fmt"
	"slices"
)

const tabRows = 32

type Table struct {
	keys []int32
	vals []uint16
}

func (t *Table) Insert(key int32, val uint16) error {
	if len(t.keys) >= tabRows {
		return fmt.Errorf("too many rows")
	}

	i, ok := slices.BinarySearch(t.keys, key)
	if ok {
		return ErrDupKey{key}
	}
	t.keys = slices.Insert(t.keys, i, key)
	t.vals = slices.Insert(t.vals, i, val)

	return nil
}
