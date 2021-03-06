#include <stdint.h>
#include <stdio.h>
#ifdef ___VM_DEBUG
	#define ___VM_DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
	#ifdef ___RAISE_DEBUG_INT
	#define ___VM_DEBUG_PRINTF(...) raise_int(4)
	#else
	#define ___VM_DEBUG_PRINTF(...)
	#endif
#endif
enum { false = 0, true = 1 };
typedef struct _vm_type_ {
	int32_t* code;
	int32_t codesize;
	int32_t (*mread)(struct _vm_type_ *cvm, int32_t addr);
	int32_t (*mwrite)(struct _vm_type_ *cvm, int32_t addr, int32_t value);
	int32_t (*push)(struct _vm_type_ *cvm, int32_t val);
	int32_t (*peek)(struct _vm_type_ *cvm);
	int32_t (*pop)(struct _vm_type_ *cvm);
	int32_t (*call)(struct _vm_type_ *cvm, int32_t addr,enum {false, true} jump);
	int32_t pc;
	int32_t sp;
	int32_t enableints;
	int32_t stack;
	int32_t timer;
	int32_t timerinterval;
	int32_t interrupts;
} vm;
#define ___STUB return 0
#define ___STUB_FUNCTION static
#define raise_int(b) ___internal_raise_int(cvm,b,false)
int32_t ___internal_raise_int(vm* cvm, int32_t intn, int _critical) {
	if ( !cvm->enableints ) return -1;
	#ifdef ___VM_DEBUG
		printf("Raised interrupt %d\n", intn);
	#endif
	int32_t ptr = cvm->mread(cvm, cvm->interrupts + intn);
	#ifdef ___VM_DEBUG
		printf("Interrupt ptr: %x // %d\n", ptr, ptr);
	#endif
	if ( ptr == 0 && !_critical ) return ___internal_raise_int(cvm, 2, true);
	if ( ptr == 0 && _critical) { 
		#ifdef ___VM_DEBUG
			printf("Assertion failure (double fault)\n");
			printf("pc=%x sp=%x int=%x\n", cvm->pc, cvm->sp, intn);
			abort();
		#endif
		return -2;
	}
	cvm->call(cvm, ptr, false);
	return 0;
}
#define ___BOUNDSCHECK(addr) if (addr < 0 || addr > cvm->codesize) { raise_int(1); return 0; }
int32_t mem_read(vm* cvm, int32_t addr) {
	___BOUNDSCHECK(addr);
	return cvm->code[addr];
}
int32_t mem_write(vm *cvm, int32_t addr, int32_t value) {
	___BOUNDSCHECK(addr);
	cvm->code[addr] = value;
	return 1;
}
int32_t push(vm *cvm, int32_t val) {
//	___BOUNDSCHECK(cvm->stack + cvm->sp); // not needed
	return cvm->mwrite(cvm, cvm->stack + cvm->sp++, val);
}
int32_t pop(vm *cvm) {
	return cvm->mread(cvm, cvm->stack + --cvm->sp);
}
int32_t peek(vm *cvm) {
	return cvm->mread(cvm, cvm->stack + cvm->sp - 1);
}
int32_t vm_call(vm *cvm, int32_t addr, enum {false, true} jump) {
	if ( jump == false ) {
		cvm->push(cvm, cvm->pc);
	}
	cvm->pc = addr;
}
#define object(n) cvm->mread(cvm, n)
#define pc cvm->pc
#define globals(id) cvm->mread(cvm, cvm->stack - 32 - 64 + id)
#define globalt(id,v) cvm->mwrite(cvm, cvm->stack - 32 - 64 + id,v)
enum { IFETCH, ISTORE, IPUSH, IPOP, IADD, ISUB, ILT, JZ, JNZ, JMP, HALT, IEQ , IMUL, IDIV, IFUNC, IENDF, ICALL = 754, IMWRITE, IMREAD };
int32_t exec(vm *cvm) {
	cvm->timer++;
	int32_t tmpa, tmpb, tmpc;
	if ( cvm->timer == cvm->timerinterval) {
		cvm->timer = 0;
		raise_int(3);
	}
//printf("%d\n", object(pc));
switch (object(pc++))
    { case IFETCH: cvm->push(cvm, globals(object(pc++)));               goto again;
      case ISTORE: globalt(object(pc++), cvm->pop(cvm)); if (object(pc-1)==0) printf("o%d\n",globals(object(pc-1)));              goto again;
      case IMWRITE: tmpa=cvm->pop(cvm); tmpb=cvm->pop(cvm); cvm->mwrite(cvm, tmpb, tmpa); goto again;
      case IMREAD: tmpa=cvm->pop(cvm); cvm->push(cvm, cvm->mread(cvm, tmpa)); goto again;
      case IPUSH : cvm->push(cvm,object(pc++));                        goto again;
      case IPOP  : cvm->pop(cvm);                                 goto again;
      //case IFUNC: globalt(object(pc++),pc); goto again;
      case IADD  : cvm->push(cvm, cvm->pop(cvm) + cvm->pop(cvm));       goto again;
case IMUL  : cvm->push(cvm, cvm->pop(cvm) * cvm->pop(cvm));        goto again;
case IDIV  : cvm->push(cvm, cvm->pop(cvm) / cvm->pop(cvm));        goto again;
case IFUNC: globalt(object(pc++), pc);while (object(pc) != IENDF) pc++; pc++; goto again;
      case ISUB  : cvm->push(cvm,cvm->pop(cvm) - cvm->pop(cvm));        goto again;
      case ILT   : cvm->push(cvm, cvm->pop(cvm) < cvm->pop(cvm));        goto again;
      case IEQ: cvm->push(cvm, cvm->pop(cvm) == cvm->pop(cvm));  goto again;
	case IENDF: pc = cvm->pop(cvm); goto again;
      case JMP   : pc += object(pc);                            goto again;
      case ICALL: cvm->push(cvm,pc); pc = globals(object(pc++)); goto again;
      case JZ    : if (cvm->pop(cvm) == 0) pc += object(pc); else pc++; goto again;
      case JNZ   : if (cvm->pop(cvm) != 0) pc += object(pc); else pc++; goto again;
case HALT: return 0;
	default: raise_int(2);
    }
	again:
	return 1;
}
#undef pc
#undef object
#define ___VM_IS_LOADED 1
#define ___VM_ASSERT(n,int) if (!(n)) { raise_int(int); }
vm* init(vm* cvm, int32_t* code, int32_t codesize, int32_t stacksize) {
	cvm->code = code;
	cvm->codesize = codesize;
	cvm->stack = cvm->codesize - stacksize;
	cvm->sp = 0;
	cvm->pc = 0;
	cvm->timer = 0;
	cvm->timerinterval = 1024;
	cvm->mread = mem_read;
	cvm->mwrite = mem_write;
	cvm->push = push;
	cvm->pop = pop;
	cvm->peek = peek;
	cvm->call = vm_call;
	cvm->interrupts = cvm->codesize - stacksize - 32;
	cvm->enableints = 0;
	return cvm;
}
#ifdef IBREAKTHEVM
	/* ???? */
	#define peek ___vm_escape_peek
	#define pop ___vm_escape_pop
	#define push ___vm_escape_push
#endif
