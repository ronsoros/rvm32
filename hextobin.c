#include <stdio.h>
#include <stdint.h>
int main() {
    int32_t code[16384];
    int cp = 0;
    while(!feof(stdin) && scanf("%x",&code[cp])){
fwrite(&code[cp], 4, 1, stdout);
cp++;
}
}
