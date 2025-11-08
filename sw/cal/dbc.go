package main

import (
	"fmt"
	"os"
	"slices"

	"go.einride.tech/can/pkg/dbc"
)

// Extract signals from the DBC file.
func parseSignals(filename string, names map[uint8]string) ([]SignalDef, error) {
	// Parse DBC file
	msgDefs, err := parseDbcFile(filename)
	if err != nil {
		return nil, err
	}

	// Search for signals
	signals := make([]SignalDef, 0, len(names))
	for _, msg := range msgDefs {
		for k, name := range names {
			i := slices.IndexFunc(msg.Signals, func(sig dbc.SignalDef) bool { return sig.Name == dbc.Identifier(name) })
			if i < 0 {
				continue
			}
			fmt.Printf("Found signal %v\n", msg.Signals[i])
			sig, err := NewSignalDef(k, msg, msg.Signals[i])
			if err != nil {
				return nil, err
			}
			signals = append(signals, sig)
		}
	}

	// Check all signals are present
	if len(signals) != len(names) {
		for i := 0; i < len(signals); i++ {
			if names[uint8(i)] != signals[i].name {
				return nil, ErrNoSig{filename, names[uint8(i)]}
			}
		}
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
