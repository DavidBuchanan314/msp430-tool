#ifndef DISAS_H
#define DISAS_H

#include <stdint.h>
#include <stdbool.h>

#define BITFIELD(value, start, len) (((value) >> ((start)+1-(len))) & ((1<<(len))-1))
#define MEMBYTE(mem, offset) (((uint8_t*)mem)[(offset)&0xFFFF])
#define MEMWORD(mem, offset) (MEMBYTE(mem,offset)|(MEMBYTE(mem,(offset)+1)<<8))

/* http://mspgcc.sourceforge.net/manual/x147.html */
typedef enum AddrMode
{
	AM_DIR = 0, /* 00: Rn         : Register direct */
	AM_IDX = 1, /* 01: offset(Rn) : Register indexed */
	AM_IND = 2, /* 10: @Rn        : Register indirect */
	AM_INC = 3, /* 11: @Rn+       : Register indirect with post-increment */
	AM_ANY = -1, /* see emulated_insn_t */
} addrmode_t;

/* http://mspgcc.sourceforge.net/manual/x223.html */
typedef enum InsnFmt
{
	/*    bit: |15|14|13|12|11|10|9 |8 |7 |6 |5 |4 |3 |2 |1 |0 | */
	IF_1OP, /* |0 |0 |0 |1 |0 |0 | Opcode |BW| Ad  | Dest Reg  | */
	IF_JMP, /* |0 |0 |1 | Cond.  |          PC Offset          | */
	IF_2OP, /* |  Opcode   |  Src Reg  |Ad|BW| As  | Dest Reg  | */
	IF_NUM, /* number of instruction formats */
} insnfmt_t;

static const uint16_t IF_1OP_MASK = 0xfc00; /* hex(0b1111110000000000) */
static const uint16_t IF_1OP_BITS = 0x1000; /* hex(0b0001000000000000) */
static const uint16_t IF_JMP_MASK = 0xe000; /* hex(0b1110000000000000) */
static const uint16_t IF_JMP_BITS = 0x2000; /* hex(0b0010000000000000) */

typedef enum Register
{
	REG_PC = 0,
	REG_SP = 1,
	REG_SR = 2,
	REG_CG = 3,
	/* I could put all the other regs here, but they never get referenced directly */
	REG_ANY = -1, /* see emulated_insn_t */
} register_t;

static const char* const REGNAME[0x10] = {
	"pc",  "sp",  "sr",  "cg",
	"r4",  "r5",  "r6",  "r7",
	"r8",  "r9",  "r10", "r11",
	"r12", "r13", "r14", "r15",
};

static const char* const REG_CONSTANTS[][4] = {
	[REG_SR] = {REGNAME[REG_SR], "&0x%04x", "#0x4", "#0x8"},
	[REG_CG] = {"#0x0", "#0x1", "#0x2", "#-0x1"}
};

typedef enum Opcode
{
	/* Single operand opcodes */
	OP_RRC  = 0,
	OP_SWPB = 1,
	OP_RRA  = 2,
	OP_SXT  = 3,
	OP_PUSH = 4,
	OP_CALL = 5,
	OP_RETI = 6,
	/* Jump opcodes */
	OP_JNE  = 0, /* JNZ */
	OP_JEQ  = 1, /* JEZ */
	OP_JNC  = 2, /* JLO */
	OP_JC   = 3, /* JHS */
	OP_JN   = 4,
	OP_JGE  = 5,
	OP_JL   = 6,
	OP_JMP  = 7,
	/* Two operand opcodes */
	OP_MOV  = 4,
	OP_ADD  = 5,
	OP_ADDC = 6,
	OP_SUBC = 7,
	OP_SUB  = 8,
	OP_CMP  = 9,
	OP_DADD = 10,
	OP_BIT  = 11,
	OP_BIC  = 12,
	OP_BIS  = 13,
	OP_XOR  = 14,
	OP_AND  = 15,
} opcode_t;

typedef struct OpInfo
{
	const char *mnemonic;
	const char *print_fmt;
	bool bmi; /* byte mode is invalid */
} opinfo_t;

static const opinfo_t OP_INFO[IF_NUM][0x10] = {
	[IF_1OP] = {
		[OP_RRC]  = {"RRC",  ""},
		[OP_SWPB] = {"SWPB", "", .bmi=true},
		[OP_RRA]  = {"RRA",  ""},
		[OP_SXT]  = {"SXT",  "", .bmi=true},
		[OP_PUSH] = {"PUSH", ""},
		[OP_CALL] = {"CALL", "", .bmi=true},
		[OP_RETI] = {"RETI", "", .bmi=true},
	},
	[IF_JMP] = {
		[OP_JNE]  = {"JNE",  ""},
		[OP_JEQ]  = {"JEQ",  ""},
		[OP_JNC]  = {"JNC",  ""},
		[OP_JC]   = {"JC",   ""},
		[OP_JN]   = {"JN",   ""},
		[OP_JGE]  = {"JGE",  ""},
		[OP_JL]   = {"JL",   ""},
		[OP_JMP]  = {"JMP",  ""},
	},
	[IF_2OP] = {
		[OP_MOV]  = {"MOV",  ""},
		[OP_ADD]  = {"ADD",  ""},
		[OP_ADDC] = {"ADDC", ""},
		[OP_SUBC] = {"SUBC", ""},
		[OP_SUB]  = {"SUB",  ""},
		[OP_CMP]  = {"CMP",  ""},
		[OP_DADD] = {"DADD", ""},
		[OP_BIT]  = {"BIT",  ""},
		[OP_BIC]  = {"BIC",  ""},
		[OP_BIS]  = {"BIS",  ""},
		[OP_XOR]  = {"XOR",  ""},
		[OP_AND]  = {"AND",  ""},
	},
};

/* Properties of an "Emulated" instruction, which is basically just an alias */
/* -1 means "any value" */
typedef struct EmulatedInstruction
{
	const char *mnemonic;
	insnfmt_t fmt;
	opcode_t opcode;
	register_t src_reg;
	register_t dst_reg;
	addrmode_t src_mode;
	addrmode_t dst_mode;
	bool print_src; /* Should the src arg be printed? */
	bool print_dst; /* Should the dst arg be printed? */
	bool args_match; /* the two operands must be the same */
} emulated_insn_t;

/* table is scanned in order until a match is found */
static const emulated_insn_t EMU_FMTS[] = {
	{"NOP",  IF_2OP, OP_MOV,  REG_ANY, REG_ANY, AM_ANY, AM_ANY, false, false, true},
	{"RET",  IF_2OP, OP_MOV,  REG_SP,  REG_PC,  AM_INC, AM_DIR, false, false, false},
	{"POP",  IF_2OP, OP_MOV,  REG_SP,  REG_ANY, AM_INC, AM_DIR, false, true,  false},
	{"BR",   IF_2OP, OP_MOV,  REG_ANY, REG_PC,  AM_ANY, AM_DIR, true,  false, false},

	{"CLRC", IF_2OP, OP_BIC,  REG_CG,  REG_SR,  AM_IDX, AM_DIR, false, false, false},
	{"SETC", IF_2OP, OP_BIS,  REG_CG,  REG_SR,  AM_IDX, AM_DIR, false, false, false},
	{"CLRZ", IF_2OP, OP_BIC,  REG_CG,  REG_SR,  AM_IND, AM_DIR, false, false, false},
	{"SETZ", IF_2OP, OP_BIS,  REG_CG,  REG_SR,  AM_IND, AM_DIR, false, false, false},
	{"CLRN", IF_2OP, OP_BIC,  REG_SR,  REG_SR,  AM_IND, AM_DIR, false, false, false},
	{"SETN", IF_2OP, OP_BIS,  REG_SR,  REG_SR,  AM_IND, AM_DIR, false, false, false},
	{"DINT", IF_2OP, OP_BIC,  REG_SR,  REG_SR,  AM_INC, AM_DIR, false, false, false},
	{"EINT", IF_2OP, OP_BIS,  REG_SR,  REG_SR,  AM_INC, AM_DIR, false, false, false},

	{"RLA",  IF_2OP, OP_ADD,  REG_ANY, REG_ANY, AM_ANY, AM_ANY, false, true,  true},
	{"RLC",  IF_2OP, OP_ADDC, REG_ANY, REG_ANY, AM_ANY, AM_ANY, false, true,  true},
	/* TODO: fill in the rest... */
};

typedef enum DataSize
{
	DS_WORD = 0,
	DS_BYTE = 1,
} datasize_t;

typedef struct
{
	insnfmt_t fmt;
	datasize_t datasize;
	opcode_t opcode; /* stores jump type for jumps also */
	register_t src_reg; /* IF_2OP only */
	register_t dst_reg;
	addrmode_t src_mode; /* IF_2OP only */
	addrmode_t dst_mode;
	uint16_t src_addr; /* Only used in indexed mode */
	uint16_t dst_addr; /* ditto */
	int pc_off; /* jumps only */
	bool invalid; /* Is this an invalid instruction? */
	uint16_t len; /* byte length of the instruction */
	uint16_t addr; /* start address of instruction */
} insn_t;

/* machine code to insn_t "IR" */
uint16_t parse_insn(insn_t* insn, uint8_t *mem, uint16_t offset);

/* insn_t to a string of assembly */
size_t unparse_insn(char *buf, size_t buflen, insn_t* insn);

#endif /* DISAS_H */
