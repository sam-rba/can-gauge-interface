.TL
Frame Format for the CAN Gauge Interface
.AU
Sam Anthony
.AI
.LP
The Gauge Interface receives
.I signals
.[
dbc10
.]
from other nodes via the vehicle's CAN bus.
It also accepts frames from a host PC for calibration.
The formats of these calibration, or
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
Each of these signals has an associated CAN ID, encoding specification, and calibration table.
A signal's CAN ID specifies which message contains the signal.
It's encoding specification defines how the signal is encoded in the message's DATA FIELD.
And it's calibration field defines a mapping from the raw signal value extracted from the message to an output value that is used to generate a waveform to display the signal on a gauge.
.PP
Each table has 32 rows, each of which contains a key and a value.
The keys and values are 32-bit and 16-bit respectively.
Numbers are transmitted in big-endian order in frames' DATA FIELDs.
.NH 1
Frames
.LP
In addition to the signal-carrying frames, there are also two types of
.I control
frame:
.B "Table Control" ,
and
.B "Signal Control" .
.NH 2
Table Control Frame
.LP
The Table Control Frame is used to read and write rows of the calibration tables.
It is an extended frame with DLC=6.
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
The row is composed of a 32-bit integer and a 16-bit integer: a key and a value.
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
noname
	31-0 Key
	15-0 Val
.end
.LP
Upon receiving a Table Control DATA FRAME, the Interface will write the key and value to the row of the table specified in the ID.
.PP
In the case of a REMOTE FRAME, the Interface will read the row of the table specified in the ID, and respond with a DATA FRAME containing the key and value of the row.
.NH 2
Signal Control Frame
.LP
The Signal Control Frame is used to read or modify the CAN ID associated with one of the signals, as well as to read or modify the signal's encoding format.
It is an extended frame with DLC=7.
It may be either a DATA FRAME: to modify\(emor a REMOTE FRAME: to read.
.PP
Upon receiving a Signal Control DATA FRAME, the Interface will write the signal's new ID and encoding to the EEPROM.
Upon receiving a Signal Control REMOTE FRAME, the Interface will respond with a DATA FRAME containing the signal's ID and encoding which were retrieved from the EEPROM.
.PP
The ID Control Frame has extended ID
.B 127210Xh .
The least significant nibble,
.I X ,
indicates one of the 6 signals [0, 5].
.begin dformat
style bitwid 0.15
Signal Control ID
	28-4 127210h
	3-0 X
.end
.LP
The DATA FIELD contains the signal's CAN ID and encoding format.
.begin dformat
style bitwid 0.08
style recspread 0
Signal Control DATA FIELD
	7-0 D0
	7-0 D1
	7-0 D2
	7-0 D3
	7-0 D4
	7-0 D5
	7-0 D6
noname
	31-0 SigId
	23-0 Encoding
noname
	31 EXIDE
	30-29 Empty
	28-0 ID
	7-0 Start
	7-0 Size
	7 Order
	6 Sign
	5-0 Empty
.end
.LP
The signal's CAN ID is sent as a big-endian 32-bit integer in the
.I SigId
field.
The MSb\(embit 31, called
.I EXIDE \(em
indicates whether the ID is standard or extended.
.TS
tab(&);
Ci
L L.
EXIDE
= 0 & Standard identifier
= 1 & Extended identifier
.TE
The lower 11 or 29 bits hold the ID itself, depending on whether it is standard or extended.
In the case of a standard ID,
.I EXIDE =0
and the ID is sent in the lower 11 bits of
.I SigId .
In the case of an extended ID,
.I EXIDE =1
and the ID is sent in the lower 29 bits of
.I SigId .
.PP
The signal encoding defines how the signal's raw value is encoded in the message that carries it.
The encoding is sent in the
.I Encoding
field of the Signal Control Frame's DATA FIELD.
.LP
.I Start
specifies the position of the signal within the DATA FIELD of the message.
It corresponds to
.CW start_bit
in DBC.
.PP
.I Size
specifies the size of the signal in bits.
It corresponds to
.CW signal_size
in DBC.
.PP
.I Order
specifies the signal's byte order.
It corresponds to
.CW byte_order
in DBC.
.TS
tab(&);
Ci
L L.
Order
= 0 & Big-endian
= 1 & Little-endian
.TE
.LP
.I Sign
specifies the signal as being of type unsigned or signed.
It corresponds to
.CW value_type
in DBC.
.TS
tab(&);
Ci
L L.
Sign
= 0 & Unsigned
= 1 & Signed
.TE
