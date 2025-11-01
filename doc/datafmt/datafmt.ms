.TL
Data Frame Format for the CAN Gauge Interface
.AU
Sam Anthony
.AI
.LP
The Gauge Interface receives
.I signals
.[
dbc07
.]
from other nodes in the vehicle via the CAN bus.
It also accepts frames from a host PC for calibration.
The format of these calibration, or
.I control ,
frames are the subject of this document.
.NH 1
Signals
.LP
The Interface listens for 6 signals on the bus:
.nr step -1 1
.IP \n+[step].
Tachometer
.IP \n+[step].
Speedometer
.IP \n+[step].
Analog 1
.IP \n+[step].
Analog 2
.IP \n+[step].
Analog 3
.IP \n+[step].
Analog 4
.LP
Each of these signals has an associated CAN ID and a calibration table; thus, there are 6 IDs and 6 tables.
The Interface listens for these IDs and uses the tables to lookup output signal values to drive he gauges.
Each table has 32 rows, each of which contains a key and a value.
The keys and values are each 32-bit unsigned integers.
Numbers are transmitted in big-endian order in frames' DATA FIELD.
.NH 1
Frames
.LP
In addition to the signal-carrying frames, there are also two types of
.I control
frame:
.B "Table Control" ,
and
.B "ID Control" .
.NH 2
Table Control Frame
.LP
The Table Control Frame is used to read and write rows of the calibration tables.
It is an extended frame with DLC=8.
It may be either a DATA FRAME: to write to a table\(emor a REMOTE FRAME: to read from a table.
.PP
The Table Control Frame has extended ID
.B 12720XXh .
The LSB,
.I X ,
inticates which table, and which row of the table, to read or modify.
The upper 3 bits of
.I X
indicate one of  the 6 tables [0, 5].
The lower 5 bits of
.I X
indicate one of the table's 32 rows [0, 31].
.begin dformat
style bitwid 0.15
Table Control ID
	28-8 12720h
	7-5 Table
	4-0 Row
.end
.LP
The DATA FIELD contains a row of the table.
The row is composed of two 32-bit unsigned integers: a key and a value.
They are transmitted in big-endian order.
.begin dformat
style bitwid 0.07
style recspread 0
Table Control DATA FIELD
	7-0 D0
	7-0 D1
	7-0 D2
	7-0 D3
	7-0 D4
	7-0 D5
	7-0 D6
	7-0 D7
noname
	31-0 Key
	31-0 Val
.end
.LP
Upon receiving a Table Control DATA FRAME, the Interface will write the key and value to the row of the table specified in the ID.
.PP
In the case of a REMOTE FRAME, the Interface will read the row of the table specified in the ID, and respond with a DATA FRAME containing the key and value of the row.
.NH 2
ID Control Frame
.LP
The ID Control Frame is used to read or modify the CAN ID associated with one of the 6 signals.
It is an extended frame with DLC=8.
It may be either a DATA FRAME: to modify\(emor a REMOTE FRAME: to read.
.PP
The ID Control Frame has extended ID
.B 127210Xh .
The least significant nibble,
.I X ,
indicates one of the 6 signals [0, 5].
.begin dformat
style bitwid 0.15
ID Control ID
	28-4 127210h
	3-0 X
.end
.LP
The DATA FIELD contains the CAN ID of the signal specified in
.I X .
It may be either a standard or an extended ID.
In the case of a standard 11-bit ID, DLC=2 and the ID is sent as a 16-bit number.
In the case of an extended 29-bit ID, DLC=4 and the ID is sent as a 32-bit number.
The numbers are sent big-endian in both cases.
DLC can be used to determine whether the ID is standard or extended.
.begin dformat
style bitwid 0.08
style recspread 0
ID Control DATA FIELD (11-bit Std. ID)
	7-0 D0
	7-0 D1
noname
	15-11
	10-0 Std. ID
.end
.begin dformat
style bitwid 0.08
style recspread 0
ID Control DATA FIELD (29-bit Ext. ID)
	7-0 D0
	7-0 D1
	7-0 D2
	7-0 D3
noname
	31-29
	28-0 Ext. ID
.end
