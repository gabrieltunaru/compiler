//
// Created by tunarug on 23.04.2020.
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "symbols.h"
#include "ext_funcs.h"
//#include "analizator.h"

int crtDepth;

void initSymbols(Symbols *symbols) {
    symbols->begin = NULL;
    symbols->end = NULL;
    symbols->after = NULL;
}

Symbols symbols;

Symbol *addSymbol(Symbols *symbols, const char *name, int cls) {
    Symbol *s;
    if (symbols->end == symbols->after) { // create more room
        int count = symbols->after - symbols->begin;
        int n = count * 2; // double the room
        if (n == 0)n = 1; // needed for the initial case
        symbols->begin = (Symbol **) realloc(symbols->begin, n * sizeof(Symbol *));
        if (symbols->begin == NULL) err("not enough memory");
        symbols->end = symbols->begin + count;
        symbols->after = symbols->begin + n;
    }
    SAFEALLOC(s, Symbol)
    *symbols->end++ = s;
    s->name = name;
    s->cls = cls;
    s->depth = crtDepth;
    return s;
}

Symbol *findSymbol(Symbols *symbols, const char *name) {
    long n = symbols->end - symbols->begin;
    for (long i = 0; i < n; i++) {
        if (symbols->begin[i]->name != NULL) {
            if (!strcmp(symbols->begin[i]->name, name)) {
                return symbols->begin[i];
            }
        }
    }
    return NULL;
}

Symbol *requireSymbol(Symbols *symbols, const char *name) {
    long n = symbols->end - symbols->begin;
    for (long i = 0; i < n; i++) {
        if (symbols->begin[i]->name != NULL) {
            if (!strcmp(symbols->begin[i]->name, name)) {
                return symbols->begin[i];
            }
        }
    }
    err("symbol not found");
}

void deleteSymbolsAfter(Symbols *symbols, Symbol *symbol) {
    long n = symbols->end - symbols->begin;
    for (long i = 0; i < n; i++) {
        if (symbols->begin[i] == symbol) {
            for (long j = n - 1; j > i; j--) {
                free(symbols->begin[j]);
                symbols->begin[j] = NULL;
                n--;
            }
        }
    }
    symbols->end = &symbols->begin[n];
}

void cast(Type *dst, Type *src) {
    if (src->nElements > -1) {
        if (dst->nElements > -1) {
            if (src->typeBase != dst->typeBase)
                tkerr(crtTk, "an array cannot be converted to an array of another type");
        } else {
            tkerr(crtTk, "an array cannot be converted to a non-array");
        }
    } else {
        if (dst->nElements > -1) {
            tkerr(crtTk, "a non-array cannot be converted to an array");
        }
    }
    switch (src->typeBase) {
        case TB_CHAR:
        case TB_INT:
        case TB_DOUBLE:
            switch (dst->typeBase) {
                case TB_CHAR:
                case TB_INT:
                case TB_DOUBLE:
                    return;
            }
        case TB_STRUCT:
            if (dst->typeBase == TB_STRUCT) {
                if (src->s != dst->s)
                    tkerr(crtTk, "a structure cannot be converted to another one");
                return;
            }
    }
    tkerr(crtTk, "incompatible types");
}

Type getArithType(Type *s1, Type *s2) {
    int t1 = s1->typeBase;
    int t2 = s2->typeBase;
    Type t;
    t.nElements = -1;
    if (t1 == t2) {
        t.typeBase = t1;
        return t;
    }
    if ((t1 == TB_DOUBLE && t2 == TB_INT) || (t2 == TB_DOUBLE && t1 == TB_INT)) {
        t.typeBase = TB_DOUBLE;
        return t;
    }
    if ((t1 == TB_CHAR && t2 == TB_INT) || (t2 == TB_CHAR && t1 == TB_INT)) {
        t.typeBase = TB_INT;
        return t;
    }
    if ((t1 == TB_DOUBLE && t2 == TB_CHAR) || (t2 == TB_DOUBLE && t1 == TB_CHAR)) {
        t.typeBase = TB_DOUBLE;
        return t;
    }
    tkerr(crtTk, "incompatible types");
}

Symbol *addExtFunc(const char *name, Type type, void *addr) {
    Symbol *s = addSymbol(&symbols, name, CLS_EXTFUNC);
    s->type = type;
    s->addr = addr;
    initSymbols(&s->args);
    return s;
}

Symbol *addFuncArg(Symbol *func, const char *name, Type type) {
    Symbol *a = addSymbol(&func->args, name, CLS_VAR);
    a->type = type;
    return a;
}

Type createType(int type, int nElements) {
    Type t;
    t.typeBase = type;
    t.nElements = nElements;
    return t;
}

void addInitFuncs() {
    Symbol *s;
    s = addExtFunc("put_s", createType(TB_VOID, -1), put_s);
    addFuncArg(s, "s", createType(TB_CHAR, 0));
    s = addExtFunc("get_s", createType(TB_VOID, -1), get_s);
    addFuncArg(s, "s", createType(TB_CHAR, 0));
    s = addExtFunc("put_i", createType(TB_VOID, -1), put_i);
    addFuncArg(s, "i", createType(TB_INT, -1));
    s = addExtFunc("get_i", createType(TB_INT, -1), get_i);
    s = addExtFunc("put_d", createType(TB_VOID, -1), put_d);
    addFuncArg(s, "d", createType(TB_DOUBLE, -1));
    s = addExtFunc("get_d", createType(TB_DOUBLE, -1), get_d);
    s = addExtFunc("put_c", createType(TB_VOID, -1), put_c);
    addFuncArg(s, "c", createType(TB_CHAR, -1));
    s = addExtFunc("get_c", createType(TB_CHAR, -1), get_c);
    s = addExtFunc("seconds", createType(TB_DOUBLE, -1), seconds);
}

