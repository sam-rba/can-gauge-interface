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
