package main

import (
	"fmt"
	"os"

	"go.einride.tech/can/pkg/dbc"
)

func eprintf(format string, a ...any) {
	weprintf(format, a...)
	os.Exit(1)
}

func weprintf(format string, a ...any) {
	fmt.Fprintf(os.Stderr, format, a...)
}

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

type ErrDupKey struct {
	key int32
}

func (e ErrDupKey) Error() string {
	return fmt.Sprintf("duplicate key %d", e.key)
}
