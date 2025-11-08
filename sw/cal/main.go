package main

import (
	"flag"
	"fmt"
	"os"

	"go.einride.tech/can/pkg/dbc"
	"go.einride.tech/can/pkg/socketcan"
)

type Signals struct {
	tach, speed, an1, an2, an3, an4 *dbc.SignalDef
}

// Flags
var (
	// DBC file
	dbcFilenameFlag = "dbc"
	dbcFilename     = flag.String(dbcFilenameFlag, "", "DBC file name")

	// SocketCAN device
	canDevFlag = "can"
	canDev     = flag.String(canDevFlag, "can0", "SocketCAN device")

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
	tachTblFlag  = "tachTbl"
	speedTblFlag = "speedTbl"
	an1TblFlag   = "an1Tbl"
	an2TblFlag   = "an2Tbl"
	an3TblFlag   = "an3Tbl"
	an4TblFlag   = "an4Tbl"
	tachTbl      = flag.String(tachTblFlag, "", "tachometer calibration CSV file")
	speedTbl     = flag.String(speedTblFlag, "", "speedometer calibration CSV file")
	an1Tbl       = flag.String(an1TblFlag, "", "analog channel 1 calibration CSV file")
	an2Tbl       = flag.String(an2TblFlag, "", "analog channel 2 calibration CSV file")
	an3Tbl       = flag.String(an3TblFlag, "", "analog channel 3 calibration CSV file")
	an4Tbl       = flag.String(an4TblFlag, "", "analog channel 4 calibration CSV file")
)

func main() {
	// Parse command line args
	flag.Parse()
	if *dbcFilename == "" {
		weprintf("Missing '%s' flag.\n", dbcFilenameFlag)
		flag.Usage()
		os.Exit(1)
	}
	if err := checkTablesProvided(); err != nil {
		eprintf("%v\n", err)
	}
	sigNames := nonEmpty(*tachSig, *speedSig, *an1Sig, *an2Sig, *an3Sig, *an4Sig)
	tblFilenames := nonEmpty(*tachTbl, *speedTbl, *an1Tbl, *an2Tbl, *an3Tbl, *an4Tbl)

	// Parse signals in DBC file
	fmt.Println("Parsing", *dbcFilename)
	sigDefs, err := parseSignals(*dbcFilename, sigNames)
	if err != nil {
		eprintf("%v\n", err)
	}

	// Open CAN connection
	conn, err := socketcan.Dial("can", *canDev)
	if err != nil {
		eprintf("%v\n", err)
	}
	defer conn.Close()
	tx := socketcan.NewTransmitter(conn)
	defer tx.Close()

	// Write calibration tables to EEPROM
	for k, filename := range tblFilenames {
		fmt.Println("Parsing", filename)
		tbl, err := parseTable(filename)
		if err != nil {
			eprintf("%v\n", err)
		}

		fmt.Println("Transmitting", filename)
		if err := writeTable(tx, tbl, k); err != nil {
			eprintf("%v\n", err)
		}
	}

	fmt.Println(sigDefs) // TODO
}

func nonEmpty(ss ...string) map[int]string {
	m := make(map[int]string)
	for i := range ss {
		if ss[i] != "" {
			m[i] = ss[i]
		}
	}
	return m
}

// Check that a calibration  table was provided for each given signal.
func checkTablesProvided() error {
	signals := []string{*tachSig, *speedSig, *an1Sig, *an2Sig, *an3Sig, *an4Sig}
	tables := []string{*tachTbl, *speedTbl, *an1Tbl, *an2Tbl, *an3Tbl, *an4Tbl}
	tableFlags := []string{tachTblFlag, speedTblFlag, an1TblFlag, an2TblFlag, an3TblFlag, an4TblFlag}
	for i := range signals {
		if signals[i] != "" && tables[i] == "" {
			return fmt.Errorf("Missing flag -%s", tableFlags[i])
		}
	}
	return nil
}
