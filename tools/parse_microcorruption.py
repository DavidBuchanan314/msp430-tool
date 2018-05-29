import sys
import re

if len(sys.argv) != 2:
	exit("USAGE: {} objdump.asm".format(sys.argv[0]))

filename = sys.argv[1]
if filename == "-":
	filename = "input"
	instream = sys.stdin
else:
	instream = open(sys.argv[1], "r")

memory = bytearray([0]*0x10000)
symbols = []

for line in instream.readlines():
	line = line.strip()
	if not line:
		continue
	address = int(line[:4], 0x10)
	if line[4] == ":": # instruction/data
		line = line.split(":")[1].strip()
		if line[0] == '"': # a string
			value = eval("b"+line)
			memory[address:address+len(value)] = value
		else: # an instruction
			data = []
			instructions = line.split(" ")
			for inst in instructions:
				if not re.match(r"[0-9a-f]{4}", inst):
					break
				data += [int(inst[:2], 0x10), int(inst[2:], 0x10)]
			memory[address:address+len(data)] = data
	else: # a symbol
		sym = line[5:].strip()
		if sym.startswith("<") and sym.endswith(">"):
			sym = sym[1:-1]
		elif sym.endswith(":"):
			sym = sym[:-1]
		symbols.append((address, sym))

with open(filename+".bin", "wb") as outfile:
	outfile.write(memory)

with open(filename+".symbols", "w") as outfile:
	for addr, sym in sorted(symbols):
		outfile.write("0x{:04x} {}\n".format(addr, sym))
