package main

import (
	"flag"
	"fmt"
	"os"
	"slices"

	"go.einride.tech/can/pkg/dbc"
)

const dev = "can0"

type Signals struct {
	tach, speed, an1, an2, an3, an4 *dbc.SignalDef
}

// Flags
var (
	// DBC file
	dbcFilenameFlag = "dbc"
	dbcFilename     = flag.String(dbcFilenameFlag, "", "DBC file name")

	// Signal names
	tachSigFlag  = "tachSig"
	speedSigFlag = "speedSig"
	an1SigFlag   = "an1Sig"
	an2SigFlag   = "an2Sig"
	an3SigFlag   = "an3Sig"
	an4SigFlag   = "an4Sig"
	tachSig      = flag.String(tachSigFlag, "", "tachometer signal name")
	speedSig     = flag.String(speedSigFlag, "", "speedometer signal name")
	an1Sig       = flag.String(an1SigFlag, "", "analog channel 1 signal name")
	an2Sig       = flag.String(an2SigFlag, "", "analog channel 2 signal name")
	an3Sig       = flag.String(an3SigFlag, "", "analog channel 3 signal name")
	an4Sig       = flag.String(an4SigFlag, "", "analog channel 4 signal name")

	// Calibration tables
	// TODO
)

func main() {
	// Parse command line args
	flag.Parse()
	if *dbcFilename == "" {
		weprintf("Missing '%s' flag.\n", dbcFilenameFlag)
		flag.Usage()
		os.Exit(1)
	}
	sigNames := nonEmpty(*tachSig, *speedSig, *an1Sig, *an2Sig, *an3Sig, *an4Sig)

	// Parse signals in DBC file
	sigDefs, err := parseSignals(*dbcFilename, sigNames)
	if err != nil {
		eprintf("Error parsing %s: %v\n", *dbcFilename, err)
	}

	fmt.Println(sigDefs)
}

func nonEmpty(ss ...string) []string {
	r := make([]string, 0, len(ss))
	for _, s := range ss {
		if s != "" {
			r = append(r, s)
		}
	}
	return r
}

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

func eprintf(format string, a ...any) {
	weprintf(format, a...)
	os.Exit(1)
}

func weprintf(format string, a ...any) {
	fmt.Fprintf(os.Stderr, format, a...)
}
