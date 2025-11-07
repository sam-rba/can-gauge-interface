package main

import (
	"fmt"
	"os"
	"slices"

	"go.einride.tech/can/pkg/dbc"
)

// Extract signals from the DBC file.
func parseSignals(filename string, names []string) ([]dbc.SignalDef, error) {
	// Parse DBC file
	msgDefs, err := parseDbcFile(filename)
	if err != nil {
		return nil, err
	}

	// Search for signals
	sigPtrs := make([]*dbc.SignalDef, len(names))
	for _, msg := range msgDefs {
		for i := range names {
			j := slices.IndexFunc(msg.Signals, func(sig dbc.SignalDef) bool { return sig.Name == dbc.Identifier(names[i]) })
			if j < 0 {
				continue
			}
			if sigPtrs[i] != nil {
				return nil, ErrDupSig{msg.Signals[j]}
			}
			sigPtrs[i] = &msg.Signals[j]
		}
	}

	// Check all signals are present
	if i := slices.IndexFunc(sigPtrs, func(sp *dbc.SignalDef) bool { return sp == nil }); i >= 0 {
		return nil, ErrNoSig{filename, names[i]}
	}

	// Dereference
	signals := make([]dbc.SignalDef, len(sigPtrs))
	for i := range sigPtrs {
		signals[i] = *sigPtrs[i]
	}

	return signals, nil
}

func parseDbcFile(filename string) ([]*dbc.MessageDef, error) {
	// Read file
	buf, err := os.ReadFile(filename)
	if err != nil {
		return nil, err
	}

	// Parse file
	parser := dbc.NewParser(filename, buf)
	if err := parser.Parse(); err != nil {
		return nil, err
	}

	// Filter message definitions
	defs := parser.Defs()
	msgDefs := make([]*dbc.MessageDef, 0, len(defs))
	for _, def := range defs {
		if msg, ok := def.(*dbc.MessageDef); ok {
			msgDefs = append(msgDefs, msg)
		}
	}
	return msgDefs, nil
}

func tryAssignSignal(dst **dbc.SignalDef, sig dbc.SignalDef, targetName string) error {
	if targetName != "" && string(sig.Name) == targetName {
		if *dst != nil {
			return fmt.Errorf("%v: duplicate signal '%s'", sig.Pos, sig.Name)
		}
		*dst = &sig
	}
	return nil

}
