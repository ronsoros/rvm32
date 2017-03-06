#include <stdio.h>
#include <stdint.h>
int main(int argc, char **argv) {
	FILE *f = fopen(argv[1],"wb");
    int32_t code[16384];
    int cp = 0;
    while(!feof(stdin) && scanf("%x",&code[cp])){
	fprintf(stderr, "%08x ", code[cp]);
	fwrite(&code[cp], 4, 1, f);
	cp++;
}
}
