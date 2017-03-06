push 16
settlen
pushivt
push 3
add
push @inth
swap
stw
bei
push @loop
jmp
: discard dw 0
: string
	string "Hello World"
	dw 0
: inth
	eni
	push 'A'
	outc
	bei
	ret
: putstr
	: strloop
	dup
	ldw
	dup
	push 16384
	stw
	swap
	inc
	swap
	push 0
	eq
	push @strend
	jme
	push @strloop
	jmp 
	: strend
	ret
: loop
	push @string
	push @putstr
	call
	push @loop
	jmp
