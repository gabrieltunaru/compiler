//
// Created by tunarug on 28.03.2020.
//
#include <stdlib.h>
#include <stdio.h>
//#include "analizator.h"

#include "gc.h"

Token *tokens2;

Token *crtTk = NULL, *consumedTk = NULL;
int offset, sizeArgs;
Instr *crtLoopEnd;

void crtTkErr(const char *fmt) {
    tkerr(crtTk, fmt);
}

int debugging = 0;

void debug(char *name) {
    if (debugging) {
        printf("@%s %s\n", name, atomToString(crtTk));
    }
}

Symbol *crtStruct, *crtFunc;

void addVar(Token *tkName, Type *t) {
    Symbol *s;
    if (crtStruct) {
        if (findSymbol(&crtStruct->members, tkName->text))
            tkerr(crtTk, "symbol redefinition: %s", tkName->text);
        s = addSymbol(&crtStruct->members, tkName->text, CLS_VAR);
    } else if (crtFunc) {
        s = findSymbol(&symbols, tkName->text);
        if (s && s->depth == crtDepth)
            tkerr(crtTk, "symbol redefinition: %s", tkName->text);
        s = addSymbol(&symbols, tkName->text, CLS_VAR);
        s->mem = MEM_LOCAL;
    } else {
        if (findSymbol(&symbols, tkName->text))
            tkerr(crtTk, "symbol redefinition: %s", tkName->text);
        s = addSymbol(&symbols, tkName->text, CLS_VAR);
        s->mem = MEM_GLOBAL;
    }
    s->type = *t;
    if (crtStruct || crtFunc) {
        s->offset = offset;
    } else {
        s->addr = allocGlobal(typeFullSize(&s->type));
    }
    offset += typeFullSize(&s->type);
}

int consume(int code) {
    if (crtTk->code == code) {
        consumedTk = crtTk;
        crtTk = crtTk->next;
        return 1;
    }
    return 0;
}

int expr();

int exprPostfix();

int exprPostfix1();

int exprPrimary();

int declVar();

int stm();

int typeBase();

int arrayDecl();

int typeName();

int exprPrimary(RetVal *rv) {
    debug("exprPrimary");
    Instr *i;
    Token *startTk = crtTk;
    Instr *startLastInstr = lastInstruction;
    if (consume(ID)) {
        Token *tkName = consumedTk;
        Symbol *s = findSymbol(&symbols, tkName->text);
        if (!s)tkerr(crtTk, "undefined symbol %s", tkName->text);
        rv->type = s->type;
        rv->isCtVal = 0;
        rv->isLVal = 1;
        if (consume(LPAR)) {
            Symbol **crtDefArg = s->args.begin;
            if (s->cls != CLS_FUNC && s->cls != CLS_EXTFUNC)
                tkerr(crtTk, "call of the non-function %s", tkName->text);
            RetVal arg;
            if (expr(&arg)) {
                if (crtDefArg == s->args.end)tkerr(crtTk, "too many arguments in call");
                cast(&(*crtDefArg)->type, &arg.type);
                if ((*crtDefArg)->type.nElements < 0) {  //only arrays are passed by addr
                    i = getRVal(&arg);
                } else {
                    i = lastInstruction;
                }
                addCastInstr(i, &arg.type, &(*crtDefArg)->type);
                crtDefArg++;
                while (1) {
                    if (consume(COMMA)) {
                        if (!expr(&arg)) {
                            crtTkErr("invalid expression after ,");
                        }
                        if (crtDefArg == s->args.end)tkerr(crtTk, "too many arguments in call");
                        cast(&(*crtDefArg)->type, &arg.type);
                        if ((*crtDefArg)->type.nElements < 0) {
                            i = getRVal(&arg);
                        } else {
                            i = lastInstruction;
                        }
                        addCastInstr(i, &arg.type, &(*crtDefArg)->type);
                        crtDefArg++;
                    } else {
                        break;
                    };
                }
            }
            if (!consume(RPAR)) {
                crtTkErr("missing ) in function call");
            }
            if (crtDefArg != s->args.end)tkerr(crtTk, "too few arguments in call");
            rv->type = s->type;
            rv->isCtVal = rv->isLVal = 0;
            i = addInstr(s->cls == CLS_FUNC ? O_CALL : O_CALLEXT);
            i->args[0].addr = s->addr;
        } else {
            if (s->cls == CLS_FUNC || s->cls == CLS_EXTFUNC)
                tkerr(crtTk, "missing call for function %s", tkName->text);
            if (s->depth) {
                addInstrI(O_PUSHFPADDR, s->offset);
            } else {
                addInstrA(O_PUSHCT_A, s->addr);
            }
        }
        return 1;
    } else if (consume(CT_INT)) {
        Token *tki = consumedTk;
        rv->type = createType(TB_INT, -1);
        rv->ctVal.i = tki->i;
        rv->isCtVal = 1;
        rv->isLVal = 0;
        addInstrI(O_PUSHCT_I, tki->i);
        return 1;
    } else if (consume(CT_REAL)) {
        Token *tkr = consumedTk;
        rv->type = createType(TB_DOUBLE, -1);
        rv->ctVal.d = tkr->r;
        rv->isCtVal = 1;
        rv->isLVal = 0;
        i = addInstr(O_PUSHCT_D);
        i->args[0].d = tkr->r;
        return 1;
    } else if (consume(CT_CHAR)) {
        Token *tkc = consumedTk;
        rv->type = createType(TB_CHAR, -1);
        rv->ctVal.i = tkc->i;
        rv->isCtVal = 1;
        rv->isLVal = 0;
        addInstrI(O_PUSHCT_C, tkc->i);
        return 1;
    } else if (consume(CT_STRING)) {
        Token *tks = consumedTk;
        rv->type = createType(TB_CHAR, 0);
        rv->ctVal.str = tks->text;
        rv->isCtVal = 1;
        rv->isLVal = 0;
        addInstrA(O_PUSHCT_A, tks->text);
        return 1;
    } else if (consume(LPAR)) {
        if (!expr(rv)) {
            crtTkErr("invalid expr");
        }
        if (!consume(RPAR)) { crtTkErr("missing ) in primary expression"); }
        return 1;
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;

}


/*
 * exprPostfix: exprPostfix LBRACKET expr RBRACKET
           | exprPostfix DOT ID
           | exprPrimary ;
 * exprPostFix: exprPrimary exprPostFix1
 * exprPostFix1: LBRACKET expr RBRACKET exprPostfix
            | DOT ID exprPostFix
            | epsilon
 */

int exprPostfix(RetVal *rv) {
    debug("exprPostfix");
    Token *startTk = crtTk;
    Instr *startLastInstr = lastInstruction;
    if (exprPrimary(rv)) {
        if (exprPostfix1(rv)) { return 1; }
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

int exprPostfix1(RetVal *rv) { //TODO: maybe cg code shoud be before second call
    debug("exprPostfix1");
    if (consume(LBRACKET)) {
        RetVal rve;
        if (!expr(&rve)) { crtTkErr("invalid expression"); }
        if (rv->type.nElements < 0)tkerr(crtTk, "only an array can be indexed");
        Type typeInt = createType(TB_INT, -1);
        cast(&typeInt, &rve.type);
        rv->type = rv->type;
        rv->type.nElements = -1;
        rv->isLVal = 1;
        rv->isCtVal = 0;
        if (!consume(RBRACKET)) { crtTkErr("missing }"); }
        if (!exprPostfix1(rv)) { crtTkErr("invalid expression"); }
        addCastInstr(lastInstruction, &rve.type, &typeInt);
        getRVal(&rve);
        if (typeBaseSize(&rv->type) != 1) {
            addInstrI(O_PUSHCT_I, typeBaseSize(&rv->type));
            addInstr(O_MUL_I);
        }
        addInstr(O_OFFSET);
    }
    if (consume(DOT)) {
        if (!consume(ID)) { crtTkErr("missing id after ."); }
        Token *tkName = consumedTk;
        Symbol *sStruct = rv->type.s;
        Symbol *sMember = findSymbol(&sStruct->members, tkName->text);
        if (!sMember)
            tkerr(crtTk, "struct %s does not have a member %s", sStruct->name, tkName->text);
        rv->type = sMember->type;
        rv->isLVal = 1;
        rv->isCtVal = 0;
        if (!exprPostfix1(rv)) { crtTkErr("invalid expression"); }
        if (sMember->offset) {
            addInstrI(O_PUSHCT_I, sMember->offset);
            addInstr(O_OFFSET);
        }
    }
    return 1;
}

int exprUnary(RetVal *rv) {
    debug("exprUnary");
    if (consume(SUB) || consume(NOT)) {
        if (!exprUnary(rv)) { crtTkErr("invalid expr"); }
        Token *tkop = consumedTk;
        if (tkop->code == SUB) {
            if (rv->type.nElements >= 0)tkerr(crtTk, "unary '-' cannot be applied to an array");
            if (rv->type.typeBase == TB_STRUCT)
                tkerr(crtTk, "unary '-' cannot be applied to a struct");
            getRVal(rv);
            switch (rv->type.typeBase) {
                case TB_CHAR:
                    addInstr(O_NEG_C);
                    break;
                case TB_INT:
                    addInstr(O_NEG_I);
                    break;
                case TB_DOUBLE:
                    addInstr(O_NEG_D);
                    break;
            }
        } else {  // NOT
            if (rv->type.typeBase == TB_STRUCT)tkerr(crtTk, "'!' cannot be applied to a struct");
            if (rv->type.nElements < 0) {
                getRVal(rv);
                switch (rv->type.typeBase) {
                    case TB_CHAR:
                        addInstr(O_NOT_C);
                        break;
                    case TB_INT:
                        addInstr(O_NOT_I);
                        break;
                    case TB_DOUBLE:
                        addInstr(O_NOT_D);
                        break;
                }
            } else {
                addInstr(O_NOT_A);
            }
            rv->type = createType(TB_INT, -1);
        }
        rv->isCtVal = rv->isLVal = 0;
    } else if (!exprPostfix(rv)) { return 0; }
    return 1;
}

int exprCast(RetVal *rv) {
    debug("exprCast");
    Instr *oldLastInstr = lastInstruction;
    Token *startTk=crtTk;
    if (!consume(LPAR)) {
        if (exprUnary(rv)) {
            return 1;
        } else {
            crtTk=startTk;
            deleteInstructionsAfter(oldLastInstr);
            return 0;
        }
    }
    Type t;
    if (!typeName(&t)) { crtTkErr("invalid type name after ("); }
    if (!consume(RPAR)) { crtTkErr("missing ) in cast"); }
    RetVal rve;
    if (!exprCast(&rve)) { crtTkErr("invalid expression"); }
    cast(&t, &rve.type);
    if (rv->type.nElements < 0 && rv->type.typeBase != TB_STRUCT) {
        switch (rve.type.typeBase) {
            case TB_CHAR:
                switch (t.typeBase) {
                    case TB_INT:
                        addInstr(O_CAST_C_I);
                        break;
                    case TB_DOUBLE:
                        addInstr(O_CAST_C_D);
                        break;
                }
                break;
            case TB_DOUBLE:
                switch (t.typeBase) {
                    case TB_CHAR:
                        addInstr(O_CAST_D_C);
                        break;
                    case TB_INT:
                        addInstr(O_CAST_D_I);
                        break;
                }
                break;
            case TB_INT:
                switch (t.typeBase) {
                    case TB_CHAR:
                        addInstr(O_CAST_I_C);
                        break;
                    case TB_DOUBLE:
                        addInstr(O_CAST_I_D);
                        break;
                }
                break;
        }
    }
    rv->type = t;
    rv->isCtVal = rv->isLVal = 0;
    return 1;
}


/*
 * exprMul: exprMul ( MUL | DIV ) exprCast | exprCast ;
 * exprMul: expreCast exprMul1
 * exprMul1: ( MUL | DIV ) expreCast exprMul1 | epsilon
 */

int exprMul1(RetVal *rv) {
    debug("exprMul1");
    Instr *i1, *i2;
    Type t1, t2;
    Token *tkop;
    RetVal rve;
    if (consume(MUL) || consume(DIV)) {
        tkop = consumedTk;
        i1 = getRVal(rv);
        t1 = rv->type;
        if (!exprCast(&rve)) { crtTkErr("invalid expr after * or /"); }
        if (rv->type.nElements > -1 || rve.type.nElements > -1)
            tkerr(crtTk, "an array cannot be multiplied or divided");
        if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be multiplied or divided");
        rv->type = getArithType(&rv->type, &rve.type);
        i2 = getRVal(&rve);
        t2 = rve.type;
        addCastInstr(i1, &t1, &rv->type);
        addCastInstr(i2, &t2, &rv->type);
        if (tkop->code == MUL) {
            switch (rv->type.typeBase) {
                case TB_INT:
                    addInstr(O_MUL_I);
                    break;
                case TB_DOUBLE:
                    addInstr(O_MUL_D);
                    break;
                case TB_CHAR:
                    addInstr(O_MUL_C);
                    break;
            }
        } else {
            switch (rv->type.typeBase) {
                case TB_INT:
                    addInstr(O_DIV_I);
                    break;
                case TB_DOUBLE:
                    addInstr(O_DIV_D);
                    break;
                case TB_CHAR:
                    addInstr(O_DIV_C);
                    break;
            }
        }
        rv->isCtVal = rv->isLVal = 0;
        exprMul1(rv);
    }
    return 1;
}

int exprMul(RetVal *rv) {
    debug("exprMul");
    if (!exprCast(rv)) { return 0; }
    exprMul1(rv);
    return 1;
}

/*
 * exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul ;
 * exprAdd: exprMul exprAdd1
 * exprAdd1: ( ADD | SUB ) exprMul expreAdd1 | epsilon
 */

int exprAdd1(RetVal *rv) {
    debug("exprAdd1");
    Instr *i1, *i2;
    Type t1, t2;
    Token *tkop;
    RetVal rve;
    if (consume(ADD) || consume(SUB)) {
        i1 = getRVal(rv);
        t1 = rv->type;
        tkop = consumedTk;
        if (!exprMul(&rve)) { crtTkErr("invalid expr after +-"); }
        if (rv->type.nElements > -1 || rve.type.nElements > -1)
            tkerr(crtTk, "an array cannot be added or subtracted");
        if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be added or subtracted");
        rv->type = getArithType(&rv->type, &rve.type);
        i2 = getRVal(&rve);
        t2 = rve.type;
        addCastInstr(i1, &t1, &rv->type);
        addCastInstr(i2, &t2, &rv->type);
        if (tkop->code == ADD) {
            switch (rv->type.typeBase) {
                case TB_INT:
                    addInstr(O_ADD_I);
                    break;
                case TB_DOUBLE:
                    addInstr(O_ADD_D);
                    break;
                case TB_CHAR:
                    addInstr(O_ADD_C);
                    break;
            }
        } else {
            switch (rv->type.typeBase) {
                case TB_INT:
                    addInstr(O_SUB_I);
                    break;
                case TB_DOUBLE:
                    addInstr(O_SUB_D);
                    break;
                case TB_CHAR:
                    addInstr(O_SUB_C);
                    break;
            }
        }
        rv->isCtVal = rv->isLVal = 0;
        exprAdd1(rv);
    }
    return 1;
}

int exprAdd(RetVal *rv) {
    debug("exprAdd");
    if (!exprMul(rv)) { return 0; }
    if (exprAdd1(rv)) { return 1; }
}

int exprRel1(RetVal *rv) {
    debug("exprRel1");
    Instr *i1, *i2;
    Type t, t1, t2;
    Token *tkop;
    RetVal rve;
    if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
        tkop = consumedTk;
        i1 = getRVal(rv);
        t1 = rv->type;
        if (!exprAdd(&rve)) { crtTkErr("invalid expr after comparator"); }
        if (rv->type.nElements > -1 || rve.type.nElements > -1)
            tkerr(crtTk, "an array cannot be compared");
        if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be compared");
        i2 = getRVal(&rve);
        t2 = rve.type;
        t = getArithType(&t1, &t2);
        addCastInstr(i1, &t1, &t);
        addCastInstr(i2, &t2, &t);
        switch (tkop->code) {
            case LESS:
                switch (t.typeBase) {
                    case TB_INT:
                        addInstr(O_LESS_I);
                        break;
                    case TB_DOUBLE:
                        addInstr(O_LESS_D);
                        break;
                    case TB_CHAR:
                        addInstr(O_LESS_C);
                        break;
                }
                break;
            case LESSEQ:
                switch (t.typeBase) {
                    case TB_INT:
                        addInstr(O_LESSEQ_I);
                        break;
                    case TB_DOUBLE:
                        addInstr(O_LESSEQ_D);
                        break;
                    case TB_CHAR:
                        addInstr(O_LESSEQ_C);
                        break;
                }
                break;
            case GREATER:
                switch (t.typeBase) {
                    case TB_INT:
                        addInstr(O_GREATER_I);
                        break;
                    case TB_DOUBLE:
                        addInstr(O_GREATER_D);
                        break;
                    case TB_CHAR:
                        addInstr(O_GREATER_C);
                        break;
                }
                break;
            case GREATEREQ:
                switch (t.typeBase) {
                    case TB_INT:
                        addInstr(O_GREATEREQ_I);
                        break;
                    case TB_DOUBLE:
                        addInstr(O_GREATEREQ_D);
                        break;
                    case TB_CHAR:
                        addInstr(O_GREATEREQ_C);
                        break;
                }
                break;
        }
        rv->type = createType(TB_INT, -1);
        rv->isCtVal = rv->isLVal = 0;
        exprRel1(rv);
    }
    return 1;
}

int exprRel(RetVal *rv) {
    debug("exprRel");
    if (!exprAdd(rv)) { return 0; }
    exprRel1(rv);
    return 1;
}

int exprEq1(RetVal *rv) {
    debug("exprEq1");
    Instr *i1, *i2;
    Type t, t1, t2;
    Token *tkop;
    RetVal rve;
    if (consume(EQUAL) || consume(NOTEQ)) {
        tkop = consumedTk;
        i1 = rv->type.nElements < 0 ? getRVal(rv) : lastInstruction;
        t1 = rv->type;
        if (!exprRel(&rve)) { crtTkErr("invalid expr after =="); }
        if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be compared");
        if (rv->type.nElements >= 0) {      // vectors
            addInstr(tkop->code == EQUAL ? O_EQ_A : O_NOTEQ_A);
        } else {  // non-vectors
            i2 = getRVal(&rve);
            t2 = rve.type;
            t = getArithType(&t1, &t2);
            addCastInstr(i1, &t1, &t);
            addCastInstr(i2, &t2, &t);
            if (tkop->code == EQUAL) {
                switch (t.typeBase) {
                    case TB_INT:
                        addInstr(O_EQ_I);
                        break;
                    case TB_DOUBLE:
                        addInstr(O_EQ_D);
                        break;
                    case TB_CHAR:
                        addInstr(O_EQ_C);
                        break;
                }
            } else {
                switch (t.typeBase) {
                    case TB_INT:
                        addInstr(O_NOTEQ_I);
                        break;
                    case TB_DOUBLE:
                        addInstr(O_NOTEQ_D);
                        break;
                    case TB_CHAR:
                        addInstr(O_NOTEQ_C);
                        break;
                }
            }
        }
        rv->type = createType(TB_INT, -1);
        rv->isCtVal = rv->isLVal = 0;
        exprEq1(rv);
    }
    return 1;
}

int exprEq(RetVal *rv) {
    debug("exprEq");
    if (!exprRel(rv)) { return 0; }
    exprEq1(rv);
    return 1;
};

int exprAnd1(RetVal *rv) {
    debug("exprAnd1");
    Instr *i1, *i2;
    Type t, t1, t2;
    RetVal rve;
    if (consume(AND)) {
        i1 = rv->type.nElements < 0 ? getRVal(rv) : lastInstruction;
        t1 = rv->type;
        if (!exprEq(&rve)) { crtTkErr("invalid expr after &&"); }
        if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be logically tested");
        if (rv->type.nElements >= 0) {      // vectors
            addInstr(O_AND_A);
        } else {  // non-vectors
            i2 = getRVal(&rve);
            t2 = rve.type;
            t = getArithType(&t1, &t2);
            addCastInstr(i1, &t1, &t);
            addCastInstr(i2, &t2, &t);
            switch (t.typeBase) {
                case TB_INT:
                    addInstr(O_AND_I);
                    break;
                case TB_DOUBLE:
                    addInstr(O_AND_D);
                    break;
                case TB_CHAR:
                    addInstr(O_AND_C);
                    break;
            }
        }
        rv->type = createType(TB_INT, -1);
        rv->isCtVal = rv->isLVal = 0;
        exprAnd1(rv);
    }
    return 1;
}

int exprAnd(RetVal *rv) {
    debug("exprAnd");
    if (!exprEq(rv)) { return 0; }
    return exprAnd1(rv);
}

int exprOr1(RetVal *rv) {
    debug("exprOr1");
    Instr *i1, *i2;
    Type t, t1, t2;
    RetVal rve;
    if (consume(OR)) {
        i1 = rv->type.nElements < 0 ? getRVal(rv) : lastInstruction;
        t1 = rv->type;
        if (!exprAnd(&rve)) { crtTkErr("invalid expr after ||"); }
        if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be logically tested");
        if (rv->type.nElements >= 0) {      // vectors
            addInstr(O_OR_A);
        } else {  // non-vectors
            i2 = getRVal(&rve);
            t2 = rve.type;
            t = getArithType(&t1, &t2);
            addCastInstr(i1, &t1, &t);
            addCastInstr(i2, &t2, &t);
            switch (t.typeBase) {
                case TB_INT:
                    addInstr(O_OR_I);
                    break;
                case TB_DOUBLE:
                    addInstr(O_OR_D);
                    break;
                case TB_CHAR:
                    addInstr(O_OR_C);
                    break;
            }
        }
        rv->type = createType(TB_INT, -1);
        rv->isCtVal = rv->isLVal = 0;
        exprOr1(rv);
    }
    return 1;
}

int exprOr(RetVal *rv) {
    debug("exprOr");
    if (!exprAnd(rv)) { return 0; }
    return exprOr1(rv);
}

int exprAssign(RetVal *rv) {
    debug("exprAssign");
    Instr *i;
    Token *startTk = crtTk;
    Instr *startLastInstr = lastInstruction;
    if (exprUnary(rv)) {
        if (consume(ASSIGN)) {
            RetVal rve;
            if (exprAssign(&rve)) {
                if (!rv->isLVal)tkerr(crtTk, "cannot assign to a non-lval");
                if (rv->type.nElements > -1 || rve.type.nElements > -1)
                    tkerr(crtTk, "the arrays cannot be assigned");
                cast(&rv->type, &rve.type);
                i = getRVal(&rve);
                addCastInstr(i, &rve.type, &rv->type);
                //TODO: duplicate the value on top before the dst addr?
                addInstrII(O_INSERT,
                           sizeof(void *) + typeArgSize(&rv->type),
                           typeArgSize(&rv->type));
                addInstrI(O_STORE, typeArgSize(&rv->type));
                rv->isCtVal = rv->isLVal = 0;
                return 1;
            } else {
                crtTkErr("invalid expression after =");
            }
        }
        crtTk = startTk;
        deleteInstructionsAfter(startLastInstr);
    }
    if (exprOr(rv)) {
        return 1;
    }
    return 0;
}

int expr(RetVal *rv) {
    debug("expr");
    return exprAssign(rv);
}

int stmCompound() {
    debug("stmCompound");
    Symbol *start = symbols.end[-1];
    Token *startTk = crtTk;
    Instr *startLastInstr = lastInstruction;
    if (!consume(LACC)) { return 0; }
    crtDepth++;
    while (1) {
        if (!declVar() && !stm()) { break; }
    }
    if (!consume(RACC)) { crtTkErr("invalid statement or missing }"); }
    crtDepth--;
    deleteSymbolsAfter(&symbols, start);
    return 1;
}

int stm() {
    debug("stm");
    Instr *i, *i1, *i2, *i3, *i4, *is, *ib3, *ibs;
    RetVal rv;
    Token *startTk = crtTk;
    Instr *startLastInstr = lastInstruction;
    if (stmCompound()) { return 1; }
    if (consume(IF)) {
        if (!consume(LPAR)) { crtTkErr("missing ( after if declaration"); }
        if (!expr(&rv)) { crtTkErr("invalid boolean condition after ("); }
        if (rv.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be logically tested");
        if (!consume(RPAR)) { crtTkErr("missing ) in if declaration"); }
        i1 = createCondJmp(&rv);
        if (!stm()) { crtTkErr("invalid if body"); }
        if (consume(ELSE)) {
            i2 = addInstr(O_JMP);
            if (!stm()) { crtTkErr("invalid else body"); }
            i1->args[0].addr = i2->next;
            i1 = i2;
        }
        i1->args[0].addr = addInstr(O_NOP);
        return 1;
    }
    if (consume(WHILE)) {
        Instr *oldLoopEnd = crtLoopEnd;
        crtLoopEnd = createInstr(O_NOP);
        i1 = lastInstruction;
        if (!consume(LPAR)) { crtTkErr("missing ( after while declaration"); }
        if (!expr(&rv)) { crtTkErr("invalid boolean condition after ("); }
        if (rv.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be logically tested");
        if (!consume(RPAR)) { crtTkErr("missing ) in while declaration"); }
        i2 = createCondJmp(&rv);
        if (!stm()) { crtTkErr("invalid while body"); }
        addInstrA(O_JMP, i1->next);
        appendInstr(crtLoopEnd);
        i2->args[0].addr = crtLoopEnd;
        crtLoopEnd = oldLoopEnd;
        return 1;
    }
    if (consume(FOR)) {
        RetVal rv1, rv2, rv3;
        Instr *oldLoopEnd = crtLoopEnd;
        crtLoopEnd = createInstr(O_NOP);
        if (!consume(LPAR)) { crtTkErr("missing ( after for declaration"); }
        if (expr(&rv1)) {
            if (typeArgSize(&rv1.type))
                addInstrI(O_DROP, typeArgSize(&rv1.type));
        }

        if (!consume(SEMICOLON)) { crtTkErr("missing ; in for declaration after init"); }
        i2 = lastInstruction; /* i2 is before rv2 */
        if (expr(&rv2)) {
            if (rv2.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be logically tested");
            i4 = createCondJmp(&rv2);
        } else {
            i4 = NULL;
        }
        if (!consume(SEMICOLON)) { crtTkErr("missing ; in for declaration after condition"); }
        ib3 = lastInstruction;
        if (expr(&rv3)) {
            if (typeArgSize(&rv3.type))
                addInstrI(O_DROP, typeArgSize(&rv3.type));
        }
        if (!consume(RPAR)) { crtTkErr("missing ) in for declaration"); }
        ibs = lastInstruction;
        if (!stm()) { crtTkErr("invalid for body"); }
        if (ib3 != ibs) {
            i3 = ib3->next;
            is = ibs->next;
            ib3->next = is;
            is->last = ib3;
            lastInstruction->next = i3;
            i3->last = lastInstruction;
            ibs->next = NULL;
            lastInstruction = ibs;
        }
        addInstrA(O_JMP, i2->next);
        appendInstr(crtLoopEnd);
        if (i4)i4->args[0].addr = crtLoopEnd;
        crtLoopEnd = oldLoopEnd;
        return 1;
    }
    if (consume(BREAK)) {
        if (!consume(SEMICOLON)) { crtTkErr("missing ; after BREAK"); }
        if (!crtLoopEnd)tkerr(crtTk, "break without for or while");
        addInstrA(O_JMP, crtLoopEnd);
        return 1;
    }
    if (consume(RETURN)) {
        if (expr(&rv)) {
            if (crtFunc->type.typeBase == TB_VOID)
                tkerr(crtTk, "a void function cannot return a value");
            cast(&crtFunc->type, &rv.type);
            i = getRVal(&rv);
            addCastInstr(i, &rv.type, &crtFunc->type);
        }
        if (!consume(SEMICOLON)) { crtTkErr("missing ; after RETURN"); }
        if (crtFunc->type.typeBase == TB_VOID) {
            addInstrII(O_RET, sizeArgs, 0);
        } else {
            addInstrII(O_RET, sizeArgs, typeArgSize(&crtFunc->type));
        }
        return 1;
    }
    if (expr(&rv)) { //TODO: check if it really is rv and not rv1
        if (typeArgSize(&rv.type))addInstrI(O_DROP, typeArgSize(&rv.type));
    }
    if (consume(SEMICOLON)) {
        return 1;
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

int funcArg() {
    debug("funcArg");
    Type t;
    Token *tkName;
    if (typeBase(&t)) {
        if (!consume(ID)) { crtTkErr("missing id after type declaration"); }
        tkName = consumedTk;
        if (!arrayDecl()) {
            t.nElements = -1;
        };
        Symbol *s = addSymbol(&symbols, tkName->text, CLS_VAR);
        s->mem = MEM_ARG;
        s->type = t;
        s->offset = offset;
        s = addSymbol(&crtFunc->args, tkName->text, CLS_VAR);
        s->mem = MEM_ARG;
        s->type = t;
        s->offset = offset;
        offset += typeArgSize(&s->type);
        return 1;
    }
    return 0;
}

int declFunc() {
    debug("declFunc");
    Symbol **ps;
    Type t;
    Token *tkName;
    Token *startTk = crtTk;
    Instr *startLastInstr = lastInstruction;
    if (!consume(VOID)) {
        if (!typeBase(&t)) { return 0; }
        else {
            if (consume(MUL)) {
                t.nElements = 0;
            } else {
                t.nElements = -1;
            }
        }
    } else {
        t.typeBase = TB_VOID;
    }
    if (!consume(ID)) { crtTkErr("invalid function name"); }
    tkName = consumedTk;
    sizeArgs = offset = 0;
    if (!consume(LPAR)) {
        crtTk = startTk;
        deleteInstructionsAfter(startLastInstr);
        return 0;
    }
    if (findSymbol(&symbols, tkName->text))
        tkerr(crtTk, "symbol redefinition: %s", tkName->text);
    crtFunc = addSymbol(&symbols, tkName->text, CLS_FUNC);
    initSymbols(&crtFunc->args);
    crtFunc->type = t;
    crtDepth++;
    funcArg();
    while (1) {
        if (!consume(COMMA)) { break; }
        if (!funcArg()) { crtTkErr("missing argument after ,"); }
    }
    if (!consume(RPAR)) { crtTkErr("missing ) in function signature"); }
    crtDepth--;
    crtFunc->addr = addInstr(O_ENTER);
    sizeArgs = offset;
    //update args offsets for correct FP indexing
    for (ps = symbols.begin; ps != symbols.end; ps++) {
        if ((*ps)->mem == MEM_ARG) {
            //2*sizeof(void*) == sizeof(retAddr)+sizeof(FP)
            (*ps)->offset -= sizeArgs + 2 * sizeof(void *);
        }
    }
    offset = 0;
    if (!stmCompound()) crtTkErr("invalid function body");
    deleteSymbolsAfter(&symbols, crtFunc);
    ((Instr *) crtFunc->addr)->args[0].i = offset;  // setup the ENTER argument
    if (crtFunc->type.typeBase == TB_VOID) {
        addInstrII(O_RET, sizeArgs, 0);
    }
    crtFunc = NULL;
    return 1;
}

int typeName(Type *ret) {
    debug("typeName");
    if (!typeBase(&ret)) return 0;
    if (!arrayDecl(&ret)) {
        ret->nElements = -1;
    }
}

int arrayDecl(Type *ret) {
    debug("arrayDecl");
    Instr *instrBeforeExpr;
    if (!consume(LBRACKET)) return 0;
    RetVal rv;
    instrBeforeExpr = lastInstruction;
    if (expr(&rv)) {
        if (!rv.isCtVal)tkerr(crtTk, "the array size is not a constant");
        if (rv.type.typeBase != TB_INT)tkerr(crtTk, "the array size is not an integer");
        ret->nElements = rv.ctVal.i;
        deleteInstructionsAfter(instrBeforeExpr);
    }
    ret->nElements = 0;
    if (!consume(RBRACKET)) crtTkErr("missing ] in type declaration");
    return 1;
}

int typeBase(Type *ret) {
    debug("typeBase");
    if (consume(INT)) {
        ret->typeBase = TB_INT;
        return 1;
    }
    if (consume(DOUBLE)) {
        ret->typeBase = TB_DOUBLE;
        return 1;
    }
    if (consume(CHAR)) {
        ret->typeBase = TB_CHAR;
        return 1;
    }
    if (consume(STRUCT)) {
        ret->typeBase = TB_STRUCT;
        if (consume(ID)) {
            Token *tkName = consumedTk;
            Symbol *s = findSymbol(&symbols, tkName->text);
            if (s == NULL)tkerr(crtTk, "undefined symbol: %s", tkName->text);
            else if (s->cls != CLS_STRUCT)tkerr(crtTk, "%s is not a struct", tkName->text);
            ret->typeBase = TB_STRUCT;
            ret->s = s;
            return 1;
        } else {
            crtTkErr("missing struct name");
        }
    }
    return 0;
}

int declVar() {
    debug("declVar");
    Type t;
    Token *tkName;
    Token *startTk = crtTk;
    Instr *startLastInstr = lastInstruction;
    int isDV;
    if (typeBase(&t)) {
        if (consume(ID)) {
            tkName = consumedTk;
            isDV = arrayDecl(&t);
            if (!isDV) {
                t.nElements = -1;
            }
            addVar(tkName, &t);
            while (1) {
                if (consume(COMMA)) {
                    isDV = 1;
                    if (consume(ID)) {
                        tkName = consumedTk;
                        if (!arrayDecl(&t)) {
                            t.nElements = -1;
                        };
                        addVar(tkName, &t);
                    } else {
                        crtTkErr("missing variable name after ,");
                    }
                } else break;
            }
            if (consume(SEMICOLON)) {
                return 1;
            } else {
                if (isDV) crtTkErr("missing ; after variable declaration");
            }
        }
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

int declStruct() {
    Token *tkName;
    Token *start = crtTk;
    Instr *startLastInstr = lastInstruction;
    debug("declStruct");
    if (!consume(STRUCT)) { return 0; }
    if (!consume(ID)) { tkerr(crtTk, "missing name after struct declaration"); }
    tkName = consumedTk;
    if (!consume(LACC)) {
        crtTk = start;
        deleteInstructionsAfter(startLastInstr);
        return 0;
    }
    offset = 0;
    if (findSymbol(&symbols, tkName->text))
        tkerr(tkName, "symbol redefinition: %s", tkName->text);
    crtStruct = addSymbol(&symbols, tkName->text, CLS_STRUCT);
    initSymbols(&crtStruct->members);
    while (1) {
        if (!declVar()) { break; }
    }
    if (!consume(RACC)) { tkerr(crtTk, "missing } in struct declaration"); }
    if (!consume(SEMICOLON)) { tkerr(crtTk, "missing ; after struct declaration"); }
    crtStruct = NULL;
    return 1;
}

int unit() {
    debug("unit");
    Instr *labelMain = addInstr(O_CALL);
    addInstr(O_HALT);
    while (1) {
        if (!declStruct() && !declFunc() && !declVar()) break;
    }
    labelMain->args[0].addr = requireSymbol(&symbols, "main")->addr;
    if (!consume(END)) crtTkErr("unexpected token");

    return 1;
}

void sintactic() {
    crtDepth = 0;
    addInitFuncs();
    tokens2 = lexical("../test.c");
    crtTk = tokens2;
    unit();
}

int main2(int argc, char **argv) {
    sintactic();
    return 0;
}
