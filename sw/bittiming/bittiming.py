from argparse import ArgumentParser
from math import floor, ceil

MAX_BRP = 2**6

MIN_TQ_PER_BIT = 8
MAX_TQ_PER_BIT = 25

MIN_PROP_SEG = 1
MAX_PROP_SEG = 8

MIN_PS1 = 1
MAX_PS1 = 8

MIN_PS2 = 2
MAX_PS2 = 8

MIN_SJW = 1
MAX_SJW = 4

CNF2_BTLMODE = 1 << 7

def prescaler(f_osc: int, bitrate: int, tq_per_bit: int) -> int | None:
	brp: float = f_osc / (2.0 * bitrate * tq_per_bit)
	if brp.is_integer():
		return int(brp)
	else:
		return None

def is_valid_tq_per_bit(f_osc: int, bitrate: int, tq_per_bit: int) -> bool:
	brp = prescaler(f_osc, bitrate, tq_per_bit)
	return (brp is not None) and (brp <= MAX_BRP)

def valid_tqs_per_bit(f_osc: int, bitrate: int) -> list[int]:
	return list(filter(
		lambda tq_per_bit: is_valid_tq_per_bit(f_osc, bitrate, tq_per_bit),
		range(MIN_TQ_PER_BIT, MAX_TQ_PER_BIT+1)))

def prop_plus_ps1(tq_per_bit: int, samplepoint: float) -> float:
	sync: int = 1
	return samplepoint * tq_per_bit - sync

def prop_ps1(tq_per_bit, samplepoint: float) -> (int, int):
	sync: int = 1
	prop_plus_ps1: float = samplepoint * tq_per_bit - sync

	prop: int = floor(prop_plus_ps1 / 2)
	ps1: int = ceil(prop_plus_ps1 / 2)
	return (prop, ps1)

def ps2(tq_per_bit: int, prop: int, ps1: int) -> int:
	sync: int = 1
	return tq_per_bit - sync - prop - ps1

def parse_int(s: str) -> int:
	n: float = float(s)
	if n.is_integer():
		return int(n)
	else:
		raise argparse.ArgumentTypeError(f"invalid int: {s}")

def unzip(lst: list[tuple[any, any]]) -> (list[any], list[any]):
	return zip(*lst)

def fmt_freq(f: int) -> str:
	if f > 1e6:
		return f"{f/1e6}MHz"
	elif f > 1e3:
		return f"{f/1e3}kHz"
	else:
		return f"{f}Hz"

def fmt_bitrate(br: int) -> str:
	if br > 1e6:
		return f"{br/1e6}Mbps"
	elif br > 1e3:
		return f"{br/1e3}kbps"
	else:
		return f"{br}bps"

class Bittiming:
	sync: int = 1

	def __init__(self, fosc: int, bitrate: int, prop: int, ps1: int, ps2: int):
		self.fosc = fosc
		self.bitrate = bitrate
		self.prop = prop
		self.ps1 = ps1
		self.ps2 = ps2

	def samplepoint(self) -> float:
		return (self.sync + self.prop + self.ps1) / self.tq_per_bit()

	def tq_per_bit(self) -> int:
		return self.sync + self.prop + self.ps1 + self.ps2

	def prescaler(self) -> int:
		brp = self.fosc / (2 * self.bitrate * self.tq_per_bit())
		assert brp.is_integer()
		return int(brp)

	def max_sjw(self) -> int:
		return min(MAX_SJW, self.ps1, self.ps2)

	def cnf1(self) -> int:
		return (((self.max_sjw()-1) & 0x3) << 6) | (self.prescaler() & 0x3F)

	def cnf2(self) -> int:
		phseg: int = (self.ps1-1) & 0x7
		prseg: int = (self.prop-1) & 0x7
		return CNF2_BTLMODE | (phseg << 3) | prseg

	def cnf3(self) -> int:
		return (self.ps2-1) & 0x7

	def __str__(self) -> str:
		return f"fosc={fmt_freq(self.fosc)}, bitrate={fmt_bitrate(self.bitrate)}, Tq/bit={self.tq_per_bit()}, BRP={self.prescaler()}, sync={self.sync}, prop={self.prop}, PS1={self.ps1}, PS2={self.ps2}, SP={self.samplepoint()*100.0:.1f}%, SJW_max={self.max_sjw()}, (CNF0, CNF1, CNF2)=(0x{self.cnf1():02X}, 0x{self.cnf2():02X}, 0x{self.cnf3():02X})"

if __name__ == "__main__":
	argparser = ArgumentParser(description="Calculate CAN bit-timing parameters for the MCP2515")
	argparser.add_argument("-fosc", type=parse_int, required=True, help="Oscillator frequency (Hz)")
	argparser.add_argument("-bitrate", type=parse_int, required=True, help="Bitrate (bps)")
	argparser.add_argument("-samplepoint", type=float, required=True, help="Sample point (%)")
	args = argparser.parse_args()

	# Find valid numbers of time quanta per bit
	tqs_per_bit: list[int] = valid_tqs_per_bit(args.fosc, args.bitrate)

	# Calculate Propogation Segment and Phase Segment 1 for each Tq/bit value
	props: list[int]
	ps1s: list[int]
	props, ps1s = unzip(map(
		lambda tq_per_bit: prop_ps1(tq_per_bit, args.samplepoint/100.0),
		tqs_per_bit))

	# Calculate Phase Segment 2 for each Tq/bit value
	ps2s: list[int] = list(map(
		lambda tq_per_bit, prop, ps1: ps2(tq_per_bit, prop, ps1),
		tqs_per_bit, props, ps1s))

	# Print valid bit-timings and MCP2515 config register values
	bittimings: list[Bittiming] = list(map(
		lambda prop, ps1, ps2: Bittiming(args.fosc, args.bitrate, prop, ps1, ps2),
		props, ps1s, ps2s))
	for bt in bittimings:
		print(bt)
