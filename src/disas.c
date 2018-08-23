#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "disas.h"

#define CHECK_CONSTRAINT(x) if ((int)EMU_FMTS[i].x != -1 && EMU_FMTS[i].x != insn->x) continue;

/* Returns the number of bytes the parsed instruction occupied */
// TODO: more robust validity checks
uint16_t parse_insn(insn_t* insn, uint8_t *mem, uint16_t offset)
{
	uint16_t word0 = MEMWORD(mem, offset);
	uint16_t len = sizeof(uint16_t);

	memset(insn, 0, sizeof(insn_t));

	/* http://mspgcc.sourceforge.net/manual/x223.html */
	if ((word0 & IF_1OP_MASK) == IF_1OP_BITS) {
		insn->fmt      = IF_1OP;
		insn->opcode   = BITFIELD(word0, 9,  3);
		insn->datasize = BITFIELD(word0, 6,  1);
		insn->dst_mode = BITFIELD(word0, 5,  2);
		insn->dst_reg  = BITFIELD(word0, 3,  4);
		
		/* Special case: @pc+ is effectively an immediate operand. */
		/* Commonly used for CALL */
		if (insn->dst_reg == 0 && insn->dst_mode == AM_INC) {
			insn->dst_addr = (int16_t)MEMWORD(mem, offset+len);
			len += sizeof(uint16_t);
		}
	} else if ((word0 & IF_JMP_MASK) == IF_JMP_BITS) {
		insn->fmt      = IF_JMP;
		insn->opcode   = BITFIELD(word0, 12, 3);
		insn->pc_off   = BITFIELD(word0, 9,  10);
		if (insn->pc_off >= (1<<9)) {
			insn->pc_off -= (1<<10); /* sign extend */
		}
	} else {
		insn->fmt      = IF_2OP;
		insn->opcode   = BITFIELD(word0, 15, 4);
		insn->src_reg  = BITFIELD(word0, 11, 4);
		insn->dst_mode = BITFIELD(word0, 7,  1);
		insn->datasize = BITFIELD(word0, 6,  1);
		insn->src_mode = BITFIELD(word0, 5,  2);
		insn->dst_reg  = BITFIELD(word0, 3,  4);
		
		/* Special case: @pc+ is effectively an immediate operand */
		if (insn->src_reg == REG_PC && insn->src_mode == AM_INC) {
			insn->src_addr = MEMWORD(mem, offset+len);
			len += sizeof(uint16_t);
		}
	}

	/* r3 is an exception, generates constants */
	if (insn->src_mode == AM_IDX && insn->src_reg != REG_CG) {
		insn->src_addr = MEMWORD(mem, offset+len);
		len += sizeof(uint16_t);
	}

	if (insn->dst_mode == AM_IDX) {
		insn->dst_addr = MEMWORD(mem, offset+len);
		len += sizeof(uint16_t);
	}

	/* Attempted to use byte mode with an unsupported opcode */
	if (insn->datasize && OP_INFO[insn->fmt][insn->opcode].bmi) {
		insn->invalid = true;
	}

	/* Do we know about this opcode? */
	if (OP_INFO[insn->fmt][insn->opcode].mnemonic == NULL) {
		insn->invalid = true;
	}

	insn->addr = offset;
	insn->len = len;
	return len;
}

static void print_addr(char *buf, size_t buflen, uint16_t addr)
{
	// TODO: show strings, symbol offsets etc.
	snprintf(buf+strlen(buf), buflen-strlen(buf), "#0x%04x", addr);
}

static void unparse_arg(char *buf, size_t buflen, register_t reg, addrmode_t mode, uint16_t value, uint16_t pc)
{
	if (reg >= 0x10) {
		strncat(buf, "[invalid register]", buflen);
		return;
	}

	if (reg == REG_SR || reg == REG_CG) {
		snprintf(buf+strlen(buf), buflen-strlen(buf), REG_CONSTANTS[reg][mode], value);
		return;
	}

	switch (mode) {
		case AM_DIR:
			strncat(buf, REGNAME[reg], buflen-strlen(buf));
			break;
		case AM_IDX:
			snprintf(buf+strlen(buf), buflen-strlen(buf), "0x%x(%s)", value, REGNAME[reg]);
			break;
		case AM_IND:
			snprintf(buf+strlen(buf), buflen-strlen(buf), "@%s", REGNAME[reg]);
			break;
		case AM_INC:
			if (reg == REG_PC) {
				print_addr(buf, buflen, value);
			} else {
				snprintf(buf+strlen(buf), buflen-strlen(buf), "@%s+", REGNAME[reg]);
			}
			break;
	}
}

size_t unparse_insn(char *buf, size_t buflen, insn_t* insn)
{
	if (insn->invalid) {
		strncpy(buf, "(bad)", buflen-1);
		return strlen(buf);
	}

	strncpy(buf, "        ", buflen); /* mnemonic padding */

	/* Scan table of "emulated" instructions */
	for (unsigned int i = 0; i < sizeof(EMU_FMTS) / sizeof(EMU_FMTS[0]); i++) {
		CHECK_CONSTRAINT(fmt);
		CHECK_CONSTRAINT(opcode);
		CHECK_CONSTRAINT(src_reg);
		CHECK_CONSTRAINT(dst_reg);
		CHECK_CONSTRAINT(src_mode);
		CHECK_CONSTRAINT(dst_mode);
		if ( EMU_FMTS[i].args_match && (
		     insn->src_reg  != insn->dst_reg  ||
		     insn->src_mode != insn->dst_mode ||
		     insn->src_addr != insn->dst_addr ))
			continue;
		
		/* if we reached here, we found an alias/"emulated" opcode */
		strncpy(buf, EMU_FMTS[i].mnemonic, strlen(EMU_FMTS[i].mnemonic));
		if (insn->datasize) strncpy(strchr(buf, ' '), ".B", 2); // XXX hacky
		
		if (EMU_FMTS[i].print_src) {
			unparse_arg(buf, buflen, insn->src_reg, insn->src_mode, insn->src_addr, insn->addr);
		} else if (EMU_FMTS[i].print_dst) {
			unparse_arg(buf, buflen, insn->dst_reg, insn->dst_mode, insn->dst_addr, insn->addr);
		}
		
		return strlen(buf);
	}

	strncpy(buf, OP_INFO[insn->fmt][insn->opcode].mnemonic, strlen(OP_INFO[insn->fmt][insn->opcode].mnemonic));
	if (insn->datasize) strncpy(strchr(buf, ' '), ".B", 2); // XXX hacky

	switch (insn->fmt) {
		case IF_1OP:
			unparse_arg(buf, buflen, insn->dst_reg, insn->dst_mode, insn->dst_addr, insn->addr);
			break;
		case IF_JMP:
			print_addr(buf, buflen, insn->addr+(insn->pc_off*2)+2);
			break;
		case IF_2OP:
			unparse_arg(buf, buflen, insn->src_reg, insn->src_mode, insn->src_addr, insn->addr);
			strncat(buf, ", ", buflen-strlen(buf));
			unparse_arg(buf, buflen, insn->dst_reg, insn->dst_mode, insn->dst_addr, insn->addr);
			break;
		default:
			snprintf(buf+strlen(buf), buflen-strlen(buf), "disassembler bug(!)");
			break;
	}
	
	return strlen(buf);
}
