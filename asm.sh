#!/bin/sh
file="$1"
if [ "a$1" == "a" ]; then
	echo "Usage: ./asm.sh [-ohex] <infile> <outfile>"
	exit 1
fi
if [ "a$1" == "a-ohex" ]; then
echo "asm(sh): '-ohex' $2 > $3" >&2
awk -f asm.awk $2 > $3
else
echo "asm(sh): $1 > $2" >&2
awk -f asm.awk $1 > ${2}.hex
./hextobin < ${2}.hex > $2
rm ${2}.hex
fi
