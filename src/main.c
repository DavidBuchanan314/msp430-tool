#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "disas.h"
#include "queue.h"

#define MEM_SIZE 0x10000
#define MAX_DEPTH 0x100 /* Disassembly happens recursively */

uint8_t mem[MEM_SIZE];
insn_t* insns[MEM_SIZE]; /* Every other entry will be "wasted" space */

int main(int argc, char* argv[]) {
	queue_t q;
	uint16_t addr;
	
	if (argc != 2) {
		printf("USAGE: %s memory_dump.bin\n", argv[0]);
		return -1;
	}
	
	FILE *fp = fopen(argv[1], "r");
	if (fp == NULL) {
		perror("fopen");
		return -1;
	}
	
	if(!fread(mem, sizeof(mem), 1, fp)) {
		fclose(fp);
		printf("ERROR: Input file should be a 64k memory image\n");
		return -1;
	}
	
	fclose(fp);
	
	if (!queue_init(&q, MAX_DEPTH)) {
		perror("queue_init");
		return -1;
	}
	
	enqueue(&q, MEMWORD(mem, 0xfffe)); /* reset vector */
	
	/* Disassemble all code branches in a BFS */
	while (!queue_empty(&q)) {
		dequeue(&q, &addr);
		if (insns[addr] != NULL) continue; /* already visisted */
		insn_t* insn = insns[addr] = calloc(1, sizeof(insn_t));
		parse_insn(insn, mem, addr);
		if (insn->invalid) continue;
		if (insn->fmt == IF_JMP) {
			enqueue(&q, addr+(insn->pc_off*2)+2);
			if (insn->opcode == OP_JMP) continue;
		}
		/* BR #0xXXXX instruction */
		if ( insn->fmt      == IF_2OP &&
		     insn->opcode   == OP_MOV &&
		     insn->src_reg  == REG_PC &&
		     insn->dst_reg  == REG_PC &&
		     insn->src_mode == AM_INC &&
		     insn->dst_mode == AM_DIR ) {
			enqueue(&q, insn->src_addr);
			continue;
		}
		enqueue(&q, addr+insn->len);
	}
	
	queue_free(&q);
	
	for (unsigned int i=0; i < MEM_SIZE; i++) {
		if (insns[i] == NULL) continue;
		printf("0x%04x:   ", i);
		char dis[100];
		unparse_insn(dis, sizeof(dis), insns[i]);
		puts(dis);
		free(insns[i]);
	}
	
	return 0;
}
