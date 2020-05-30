//
// Created by tunarug on 30.05.2020.
//

#ifndef ANALIZATOR_GC_H
#define ANALIZATOR_GC_H

#endif //ANALIZATOR_GC_H
#include "vm.h"
#include "symbols.h"
#include "stdio.h"

//
// Created by tunarug on 30.05.2020.
//

Instr *getRVal(RetVal *rv);
void addCastInstr(Instr *after,Type *actualType,Type *neededType);
Instr *createCondJmp(RetVal *rv);

int typeBaseSize(Type *type);
int typeFullSize(Type *type);
int typeArgSize(Type *type);