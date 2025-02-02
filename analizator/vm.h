//
// Created by tunarug on 15.05.2020.
//

#ifndef ANALIZATOR_VM_H
#define ANALIZATOR_VM_H

#endif //ANALIZATOR_VM_H

#define STACK_SIZE (32*1024)
extern char stack[STACK_SIZE];
extern char *SP; // Stack Pointer
extern char *stackAfter; // first byte after stack; used for stack limit tests

enum {
    O_ADD_C, O_ADD_D, O_ADD_I,
    O_AND_A, O_AND_C, O_AND_D, O_AND_I,
    O_CALL,
    O_CALLEXT,
    O_CAST_C_D, O_CAST_C_I, O_CAST_D_C, O_CAST_D_I, O_CAST_I_C, O_CAST_I_D,
    O_DIV_C, O_DIV_D, O_DIV_I,
    O_DROP,
    O_ENTER,
    O_EQ_A, O_EQ_C, O_EQ_D, O_EQ_I,
    O_GREATER_C, O_GREATER_D, O_GREATER_I,
    O_GREATEREQ_C, O_GREATEREQ_D, O_GREATEREQ_I,
    O_HALT,
    O_INSERT,
    O_JF_A, O_JF_C, O_JF_D, O_JF_I,
    O_JMP,
    O_JT_A, O_JT_C, O_JT_D, O_JT_I,
    O_LESS_C, O_LESS_D, O_LESS_I,
    O_LESSEQ_C, O_LESSEQ_D, O_LESSEQ_I,
    O_LOAD,
    O_MUL_C, O_MUL_D, O_MUL_I,
    O_NEG_C, O_NEG_D, O_NEG_I,
    O_NOP,
    O_NOT_A, O_NOT_C, O_NOT_D, O_NOT_I,
    O_NOTEQ_A, O_NOTEQ_C, O_NOTEQ_D, O_NOTEQ_I,
    O_OFFSET,
    O_OR_A, O_OR_C, O_OR_D, O_OR_I,
    O_PUSHFPADDR,
    O_PUSHCT_A, O_PUSHCT_C, O_PUSHCT_D, O_PUSHCT_I,
    O_RET,
    O_STORE,
    O_SUB_C, O_SUB_D, O_SUB_I
};

typedef struct _Instr {
    int opcode; // O_*
    union {
        int i; // int, char
        double d;
        void *addr;
    } args[2];
    struct _Instr *last, *next; // links to last, next instructions
} Instr;

extern Instr *instructions, *lastInstruction; // double linked list

#define GLOBAL_SIZE (32*1024)
extern char globals[GLOBAL_SIZE];
extern int nGlobals;

void pushd(double d);

double popd();

void pusha(void *a);

void *popa();

void pushi(int i);

int popi();

void pushc(char c);

char popc();

Instr *createInstr(int opcode);

Instr *addInstr(int opcode);

Instr *addInstrAfter(Instr *after, int opcode);

Instr *addInstrA(int opcode, void *addr);

Instr *addInstrI(int opcode, int val);

Instr *addInstrII(int opcode, int val1, int val2);

Instr *appendInstr(Instr *i);

void *allocGlobal(int size);

void deleteInstructionsAfter(Instr *start);