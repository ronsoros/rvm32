/****
	Curses-based emulator and standard hardware implementation
	for r117 CPU
****/
#ifndef NCURSES
#include <curses.h>
#else
// curse you GNU!! stop making non standard stuff.
#include <ncurses.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define ___VM_DEBUG
#include "vm.c"
#define _max_code 65536
/*int32_t custom_read(vm* cvm, int32_t addr) {
	if ( addr >= 16384 && addr <= 18384 ) {
		int32_t
	}
}*/ // unneeded
int main(int argc, char **argv) {
	setbuf(stdout, NULL);
	FILE *f = 256;
	int32_t code[_max_code];
	if(!(argc-1)||!(f=fopen(argv[1],"rb"))) {
		puts(f == 256 ? "usage: emulator [binary file]" : "can't open file");
		return 1;
	}
	if ( f == 256 ) { puts("Assertion failure: f != 256: abort()\n"); abort(); }
	int32_t clen = fread(code, 1, _max_code * sizeof(int32_t), f);
	if ( !clen ) abort();
	printf("Binary size: %d\n", clen);
	initscr();
	timeout(0);
	curs_set(0);
	vm emuvm;
	init(&emuvm, code, _max_code, 512);
/*	emuvm->mread = custom_read;
	emuvm->mwrite = custom_write;*/ // also unneeded
	while ( exec(&emuvm) ) {
		int32_t i;
		for ( i = 16384; i <= 18384; i++ ) { // framebuffer
			mvaddch((i - 16384) % 80, (i - 16384) / 25, emuvm.code[i]);
		}
		refresh();
	wgetch(stdscr);
	}
	return 0;
}
