package main

import (
	"fmt"

	"go.einride.tech/can/pkg/dbc"
)

type ErrDupSig struct {
	sig dbc.SignalDef
}

func (e ErrDupSig) Error() string {
	return fmt.Sprintf("%v: duplicate signal '%s'", e.sig.Pos, e.sig.Name)
}

type ErrNoSig struct {
	filename string // DBC file
	signal   string // signal name
}

func (e ErrNoSig) Error() string {
	return fmt.Sprintf("%s: no such signal '%s'", e.filename, e.signal)
}
