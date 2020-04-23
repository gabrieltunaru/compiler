//
// Created by tunarug on 23.04.2020.
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "symbols.h"
#include "analizator.h"

void initSymbols(Symbols *symbols) {
    symbols->begin = NULL;
    symbols->end = NULL;
    symbols->after = NULL;
}

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

void deleteSymbolsAfter(Symbols *symbols, Symbol *symbol) {
    long n = symbols->end - symbols->begin;
    for (long i = 0; i < n; i++) {
        if(symbols->begin[i]==symbol) {
            for (long j=n-1;j>i;i++) {
                free(symbols->begin[j]);
                symbols->begin[j]=NULL;
                n--;
            }
        }
    }
    symbols->end=&symbols->begin[n];
}

