CC=gcc
CFLAGS=-O3
ifeq ($(which tcc),)
else
CC=tcc
endif
all: vmtest hextobin

win32emu:
	$(CC) $(CFLAGS) -Iemuwin32 -mwindows cursesemu.c emuwin32/pdcurses.a -o emulator

linuxemu:
	$(CC) $(CFLAGS) cursesemu.c -lcurses -o emulator

vmtest: vmtest.c vm.c
	$(CC) $(CFLAGS) vmtest.c -o vmtest

hextobin: hextobin.c
	$(CC) $(CFLAGS) hextobin.c -o hextobin

help:
	@echo "The Ronsor Emulator Makefile v1.0"
	@echo "Usage: make [all|vmtest|help]"
	@echo "For more information, do make help-TOPIC"

help-all:
	@echo "Make all components"

help-vmtest:
	@echo "Make the test emulator"

help-help:
	@echo "The Ronsor Emulator Help Component of the Makefile"
	@echo "For more information, do make help"
