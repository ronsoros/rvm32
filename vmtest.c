
#include <stdio.h>

//#define ___VM_DEBUG
#include "vm.c"

int main(int argc, char **argv) {
	if (!(argc-1)) exit(1);
	FILE *fp = fopen(argv[1], "r");
	if (!fp) exit(2);
	int32_t code[4096];
	int32_t clen;
	clen = fread(code, 1, 4096, fp);
	if (!clen) exit(3);
	vm newvm;
	init(&newvm, code, 4096, 32);
	while ( exec(&newvm) ) {}
	return 0;
}
