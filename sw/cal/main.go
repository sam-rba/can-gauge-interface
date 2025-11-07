package main

import (
	"flag"
	"fmt"
	"os"

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

	// Parse signals in DBC file
	signals, err := parseSignals(*dbcFilename)
	if err != nil {
		eprintf("Error parsing %s: %v\n", *dbcFilename, err)
	}

	fmt.Println(signals)
}

// Parse signals in the DBC file.
func parseSignals(filename string) (Signals, error) {
	msgDefs, err := parseDbcFile(filename)
	if err != nil {
		return Signals{}, err
	}

	// Search for signals in messages
	var signals Signals
	for _, msg := range msgDefs {
		for _, sig := range msg.Signals {
			if err := tryAssignSignal(&signals.tach, sig, *tachSig); err != nil {
				return signals, err
			}
			if err := tryAssignSignal(&signals.speed, sig, *speedSig); err != nil {
				return signals, err
			}
			if err := tryAssignSignal(&signals.an1, sig, *an1Sig); err != nil {
				return signals, err
			}
			if err := tryAssignSignal(&signals.an2, sig, *an2Sig); err != nil {
				return signals, err
			}
			if err := tryAssignSignal(&signals.an3, sig, *an3Sig); err != nil {
				return signals, err
			}
			if err := tryAssignSignal(&signals.an4, sig, *an4Sig); err != nil {
				return signals, err
			}
		}
	}

	// Check all signals provided on command line are present in the DBC file
	if *tachSig != "" && signals.tach == nil {
		return signals, ErrNoSig{filename, *tachSig}
	}
	if *speedSig != "" && signals.speed == nil {
		return signals, ErrNoSig{filename, *speedSig}
	}
	if *an1Sig != "" && signals.an1 == nil {
		return signals, ErrNoSig{filename, *an1Sig}
	}
	if *an2Sig != "" && signals.an2 == nil {
		return signals, ErrNoSig{filename, *an2Sig}
	}
	if *an3Sig != "" && signals.an3 == nil {
		return signals, ErrNoSig{filename, *an3Sig}
	}
	if *an4Sig != "" && signals.an4 == nil {
		return signals, ErrNoSig{filename, *an4Sig}
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
