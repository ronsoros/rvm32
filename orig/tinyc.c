/* file: "tinyc.c" */
/* (C) 2017 Ronsor-OpenStar, available under the RPL 3.0 */
/* Originally Copyright (C) 2001 by Marc Feeley, All Rights Reserved. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 
 * This is a compiler for the Tiny-C language.  Tiny-C is a
 * considerably stripped down version of C and it is meant as a
 * pedagogical tool for learning about compilers.  The integer global
 * variables "a" to "z" are predefined and initialized to zero, and it
 * is not possible to declare new variables.  The compiler reads the
 * program from standard input and prints out the value of the
 * variables that are not zero.  The grammar of Tiny-C in EBNF is:
 *
 *  <program> ::= <statement>
 *  <statement> ::= "if" <paren_expr> <statement> |
 *                  "if" <paren_expr> <statement> "else" <statement> |
 *                  "while" <paren_expr> <statement> |
 *                  "do" <statement> "while" <paren_expr> ";" |
 *                  "{" { <statement> } "}" |
 *                  <expr> ";" |
 *                  ";"
 *  <paren_expr> ::= "(" <expr> ")"
 *  <expr> ::= <test> | <id> "=" <expr>
 *  <test> ::= <sum> | <sum> "<" <sum>
 *  <sum> ::= <term> | <sum> "+" <term> | <sum> "-" <term>
 *  <term> ::= <id> | <int> | <paren_expr>
 *  <id> ::= "a" | "b" | "c" | "d" | ... | "z"
 *  <int> ::= <an_unsigned_decimal_integer>
 *
 * Here are a few invocations of the compiler:
 *
 * % echo "a=b=c=2<3;" | ./a.out
 * a = 1
 * b = 1
 * c = 1
 * % echo "{ i=1; while (i<100) i=i+i; }" | ./a.out
 * i = 128
 * % echo "{ i=125; j=100; while (i-j) if (i<j) j=j-i; else i=i-j; }" | ./a.out
 * i = 25
 * j = 25
 * % echo "{ i=1; do i=i+10; while (i<50); }" | ./a.out
 * i = 51
 * % echo "{ i=1; while ((i=i+10)<50) ; }" | ./a.out
 * i = 51
 * % echo "{ i=7; if (i<5) x=1; if (i<10) y=2; }" | ./a.out
 * i = 7
 * y = 2
 *
 * The compiler does a minimal amount of error checking to help
 * highlight the structure of the compiler.
 */


/*---------------------------------------------------------------------------*/

/* Lexer. */

enum { DO_SYM, ELSE_SYM, IF_SYM, WHILE_SYM, FUNC_SYM, LBRA, RBRA, LPAR, RPAR,
       PLUS, MINUS, LESS, SEMI, EQUAL, INT, ID, IDCALL, EOI, ISEQ, MUL, DIV };

char *words[] = { "do", "else", "if", "while", "function", NULL };

int ch = ' ';
int newch = 0;
int sym;
int int_val;
char id_name[100];
static struct {
 char name[64];
 int id;
} symbols[64];
int stid = 0;
int GetId(char *name) {
 int i; 
   for (i = 0; i < 64; i++ ) {
   //printf("%s=%d\n",symbols[i].name,symbols[i].id);
   if(!strcmp(symbols[i].name, name)) {
     return symbols[i].id; }
 
 }
 for ( i = 0; i < 64; i++ ) {
    if(symbols[i].name[0] == 0) {
      strcpy(symbols[i].name, name); symbols[i].id = stid++;
      return symbols[i].id;
    }
 }
}
int line = 1;
void syntax_error() { fflush(stdout);fprintf(stderr, "syntax error on line %d at '%c'\n", line, ch); exit(1); }
void next_ch() { if ( newch == 0 ) { newch = getchar(); } ch=newch; newch = getchar(); }
void next_sym()
{ again: switch (ch)
    { case '\n': line++; case ' ': case '\t': next_ch(); goto again;
      case EOF: sym = EOI; break;
      case '{': next_ch(); sym = LBRA; break;
      case '}': next_ch(); sym = RBRA; break;
      case '(': next_ch(); sym = LPAR; break;
      case ')': next_ch(); sym = RPAR; break;
      case '+': next_ch(); sym = PLUS; break;
      case '-': next_ch(); sym = MINUS; break;
      case '<': next_ch(); sym = LESS; break;
      case ';': next_ch(); sym = SEMI; break;
      case '*': next_ch(); sym = MUL; break;
      case '/': next_ch(); sym = DIV; break;
      case '=': next_ch();if(ch=='=') { sym = ISEQ; next_ch(); break; } sym = EQUAL; break;
      default:
        if (ch >= '0' && ch <= '9')
          { int_val = 0; /* missing overflow check */
            while (ch >= '0' && ch <= '9')
              { int_val = int_val*10 + (ch - '0'); next_ch(); }
            sym = INT;
          }
        else if (ch >= 'A' && ch <= 'z')
          { int i = 0; /* missing overflow check */
            while ((ch >= 'A' && ch <= 'z') || ch == '_')
              { id_name[i++] = ch; next_ch(); }
            id_name[i] = '\0';
//	printf(id_name);
            sym = 0;
            while (words[sym] != NULL && strcmp(words[sym], id_name) != 0)
              sym++;
            if (words[sym] == NULL){
		
              sym = ID; id_name[0] = 'a'+GetId(id_name); id_name[1]=0;
		if (ch == '(') sym = IDCALL;
		 }
          }
        else
          syntax_error();
    }
}

/*---------------------------------------------------------------------------*/

/* Parser. */

enum { VAR, CST, ADD, SUB, LT, SET,
       IF1, IF2, WHILE, DO, EMPTY, SEQ, EXPR, PROG, EQ, GMUL, GDIV, CALL, FUNC };

struct node { int kind; struct node *o1, *o2, *o3; int val; char strv[512]; };
typedef struct node node;

node *new_node(int k)
{ node *x = (node*)malloc(sizeof(node)); x->kind = k; return x; }

node *paren_expr(); /* forward declaration */

node *term()  /* <term> ::= <id> | <int> | <paren_expr> */
{ node *x;
  if (sym == ID) { x=new_node(VAR); x->val=id_name[0]-'a'; next_sym(); }
  else if (sym == INT) { x=new_node(CST); x->val=int_val; next_sym(); }
  else x = paren_expr();
  return x;
}

node *sum()  /* <sum> ::= <term> | <sum> "+" <term> | <sum> "-" <term> */
{ node *t, *x = term();
  while (sym == PLUS || sym == MINUS)
    { t=x; x=new_node(sym==PLUS?ADD:SUB); next_sym(); x->o1=t; x->o2=term(); }
  return x;
}
node *mul() {
 node *t, *x = sum();
  while (sym == MUL || sym == DIV) 
   { t=x;x =new_node(sym==MUL?GMUL:GDIV); next_sym(); x->o1=t; x->o2=sum(); }
   return x;
}
node *test()  /* <test> ::= <sum> | <sum> "<" <sum> */
{ node *t, *x = mul();
  if (sym == LESS)
    { t=x; x=new_node(LT); next_sym(); x->o1=t; x->o2=mul(); }
   if ( sym == ISEQ ) 
    { t=x; x =new_node(EQ); next_sym(); x->o1=t; x->o2=mul(); }
  return x;
}

node *expr()  /* <expr> ::= <test> | <id> "=" <expr> */
{ node *t, *x;
  if (sym != ID) return test();
  x = test();
  if (x->kind == VAR && sym == EQUAL)
    { t=x; x=new_node(SET); next_sym(); x->o1=t; x->o2=expr(); }
  return x;
}

node *paren_expr()  /* <paren_expr> ::= "(" <expr> ")" */
{ node *x;
  if (sym == LPAR) next_sym(); else syntax_error();
  x = expr();
  if (sym == RPAR) next_sym(); else syntax_error();
  return x;
}

node *statement()
{ node *t, *x;
  if (sym == IF_SYM)  /* "if" <paren_expr> <statement> */
    { x = new_node(IF1);
      next_sym();
      x->o1 = paren_expr();
      x->o2 = statement();
      if (sym == ELSE_SYM)  /* ... "else" <statement> */
        { x->kind = IF2;
          next_sym();
          x->o3 = statement();
        }
    }
  else if (sym == WHILE_SYM)  /* "while" <paren_expr> <statement> */
    { x = new_node(WHILE);
      next_sym();
      x->o1 = paren_expr();
      x->o2 = statement();
    }
  else if (sym == FUNC_SYM) {
	//printf("Hi\n");
     x = new_node(FUNC); next_sym();
     if ( sym != ID ) syntax_error();
     x->val = id_name[0]-'a';
	next_sym();
     x->o1 = statement();
     //next_sym();
  }
  else if (sym == DO_SYM)  /* "do" <statement> "while" <paren_expr> ";" */
    { x = new_node(DO);
      next_sym();
      x->o1 = statement();
      if (sym == WHILE_SYM) next_sym(); else syntax_error();
      x->o2 = paren_expr();
      if (sym == SEMI) next_sym(); else syntax_error();
    }
  else if (sym == SEMI)  /* ";" */
    { x = new_node(EMPTY); next_sym(); }
  else if (sym == LBRA)  /* "{" { <statement> } "}" */
    { x = new_node(EMPTY);
      next_sym();
      while (sym != RBRA)
        { t=x; x=new_node(SEQ); x->o1=t; x->o2=statement(); }
      next_sym();
    }
  else if ( sym == IDCALL ) {
	
       next_sym(); next_sym();
        x = new_node(CALL);
        x->val = id_name[0]-'a';
       next_sym();
  }
  else  /* <expr> ";" */
    { x = new_node(EXPR);
      x->o1 = expr();
      if (sym == SEMI) next_sym(); else syntax_error();
    }
  return x;
}

node *program()  /* <program> ::= <statement> */
{ node *x = new_node(PROG);
  next_sym(); x->o1 = statement(); if (sym != EOI) syntax_error();
  return x;
}

/*---------------------------------------------------------------------------*/

/* Code generator. */

enum { IFETCH, ISTORE, IPUSH, IPOP, IADD, ISUB, ILT, JZ, JNZ, JMP, HALT, IEQ , IMUL, IDIV, IFUNC, IENDF, ICALL = 754 };

typedef int32_t code;
code object[1000], *here = object;

void g(code c) { *here++ = c; } /* missing overflow check */
code *hole() { return here++; }
void fix(code *src, code *dst) { *src = dst-src; } /* missing overflow check */

void c(node *x)
{ code *p1, *p2;
  switch (x->kind)
    { case VAR  : g(IFETCH); g(x->val); break;
      case CST  : g(IPUSH); g(x->val); break;
      case ADD  : c(x->o1); c(x->o2); g(IADD); break;
case GMUL  : c(x->o1); c(x->o2); g(IMUL); break;
case GDIV  : c(x->o1); c(x->o2); g(IDIV); break;
      case SUB  : c(x->o1); c(x->o2); g(ISUB); break;
      case LT   : c(x->o1); c(x->o2); g(ILT); break;
      case EQ: c(x->o1); c(x->o2); g(IEQ); break;
      case CALL: g(ICALL); g(x->val); break;
      case SET  : c(x->o2); g(ISTORE); g(x->o1->val); break;
      case IF1  : c(x->o1); g(JZ); p1=hole(); c(x->o2); fix(p1,here); break;
      case IF2  : c(x->o1); g(JZ); p1=hole(); c(x->o2); g(JMP); p2=hole();
                  fix(p1,here); c(x->o3); fix(p2,here); break;
      case WHILE: p1=here; c(x->o1); g(JZ); p2=hole(); c(x->o2);
                  g(JMP); fix(hole(),p1); fix(p2,here); break;
      case DO   : p1=here; c(x->o1); c(x->o2); g(JNZ); fix(hole(),p1); break;
      case EMPTY: break;
      case FUNC: g(IFUNC); g(x->val); c(x->o1); g(IENDF); break; 
      case SEQ  : c(x->o1); c(x->o2); break;
      case EXPR : c(x->o1); g(IPOP); break;
      case PROG : c(x->o1); g(HALT); break;
    }
}

/*---------------------------------------------------------------------------*/

/* Virtual machine. */

int globals[64];

void run()
{ int32_t stack[128], *sp = stack;
  int32_t callstack[32], *csp = callstack;
  code pc = 0;
  again: printf("pc=%d *pc=%d\n", pc, object[pc]); switch (object[pc++])
    { case IFETCH: *sp++ = globals[object[pc++]];               goto again;
      case ISTORE: globals[object[pc++]] = sp[-1];              goto again;
      case IPUSH : *sp++ = object[pc++];                        goto again;
      case IPOP  : --sp;                                 goto again;
      //case IFUNC: globals[object[pc++]] = pc; goto again;
      case IADD  : sp[-2] = sp[-2] + sp[-1]; --sp;       goto again;
case IMUL  : sp[-2] = sp[-2] * sp[-1]; --sp;       goto again;
case IDIV  : sp[-2] = sp[-2] / sp[-1]; --sp;       goto again;
case IFUNC: globals[object[pc++]] = pc;while (object[pc] != IENDF) pc++; pc++; goto again;
      case ISUB  : sp[-2] = sp[-2] - sp[-1]; --sp;       goto again;
      case ILT   : sp[-2] = sp[-2] < sp[-1]; --sp;       goto again;
      case IEQ: sp[-2] = (sp[-2] == sp[-1]); --sp; goto again;
	case IENDF: pc = csp[-1]; csp--; goto again;
      case JMP   : pc += object[pc];                            goto again;
      case ICALL: *csp++ = pc+1; pc = globals[object[pc++]]; goto again;
      case JZ    : if (*--sp == 0) pc += object[pc]; else pc++; goto again;
      case JNZ   : if (*--sp != 0) pc += object[pc]; else pc++; goto again;
    }
}

/*---------------------------------------------------------------------------*/

/* Main program. */

int main()
{ int i;

  c(program());

  for (i=0; i<26; i++)
    globals[i] = 0;
  run();
  for (i=0; i<26; i++)
    if (globals[i] != 0)
      printf("%c = %d\n", 'a'+i, globals[i]);

  return 0;
}
