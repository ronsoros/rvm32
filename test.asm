: abc
push 16
settlen
push 65
outc
pushivt
inc
inc
inc
push @inth
swap
stw
bei
push @abb
jmp
: inth
	eni
	push 67
	outc
	bei
ret
: abb
	push @done
	push 30
	push 30
	eq
	jme
	push 66
	outc
	push @abb
	jmp
# jmp
# invalid opcode below
#dw 0x82
: done
	push 68
	outc
