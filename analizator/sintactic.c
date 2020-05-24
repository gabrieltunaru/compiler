//
// Created by tunarug on 28.03.2020.
//
#include <stdlib.h>
#include <stdio.h>
//#include "analizator.h"
#include "symbols.h"

Token *tokens;

Token *crtTk = NULL, *consumedTk = NULL;

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
    Token *startTk = crtTk;
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
                crtDefArg++;
                while (1) {
                    if (consume(COMMA)) {
                        if (!expr(&arg)) {
                            crtTkErr("invalid expression after ,");
                        }
                        if (crtDefArg == s->args.end)tkerr(crtTk, "too many arguments in call");
                        cast(&(*crtDefArg)->type, &arg.type);
                        crtDefArg++;
                    } else {
                        break;
                    };
                }
            }
            if (!consume(RPAR)) {
                crtTkErr("missing ) in expression");
            }
            if (crtDefArg != s->args.end)tkerr(crtTk, "too few arguments in call");
            rv->type = s->type;
            rv->isCtVal = rv->isLVal = 0;
        }
        else {
            if (s->cls == CLS_FUNC || s->cls == CLS_EXTFUNC)
                tkerr(crtTk, "missing call for function %s", tkName->text);
        }
        return 1;
    } else if (consume(CT_INT)) {
        Token *tki = consumedTk;
        rv->type = createType(TB_INT, -1);
        rv->ctVal.i = tki->i;
        rv->isCtVal = 1;
        rv->isLVal = 0;
        return 1;
    } else if (consume(CT_REAL)) {
        Token *tkr = consumedTk;
        rv->type = createType(TB_DOUBLE, -1);
        rv->ctVal.d = tkr->r;
        rv->isCtVal = 1;
        rv->isLVal = 0;;
        return 1;
    } else if (consume(CT_CHAR)) {
        Token *tkc = consumedTk;
        rv->type = createType(TB_CHAR, -1);
        rv->ctVal.i = tkc->i;
        rv->isCtVal = 1;
        rv->isLVal = 0;
        return 1;
    } else if (consume(CT_STRING)) {
        Token *tks = consumedTk;
        rv->type = createType(TB_CHAR, 0);
        rv->ctVal.str = tks->text;
        rv->isCtVal = 1;
        rv->isLVal = 0;
        return 1;
    } else if (consume(LPAR)) {
        if (!expr(rv)) {
            crtTkErr("invalid expr");
        }
        if (!consume(RPAR)) { crtTkErr("missing ) in primary expression"); }
        return 1;
    }
    crtTk = startTk;
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
    if (exprPrimary(rv)) {
        if (exprPostfix1(rv)) { return 1; }
    }
    crtTk = startTk;
    return 0;
}

int exprPostfix1(RetVal *rv) {
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
        } else {  // NOT
            if (rv->type.typeBase == TB_STRUCT)tkerr(crtTk, "'!' cannot be applied to a struct");
            rv->type = createType(TB_INT, -1);
        }
        rv->isCtVal = rv->isLVal = 0;
    } else if (!exprPostfix(rv)) { return 0; }
    return 1;
}

int exprCast(RetVal *rv) {
    debug("exprCast");
    if (!consume(LPAR)) {
        if (exprUnary(rv)) {
            return 1;
        } else {
            return 0;
        }
    }
    Type t;
    if (!typeName(&t)) { crtTkErr("invalid type name after ("); }
    if (!consume(RPAR)) { crtTkErr("missing ) in cast"); }
    RetVal rve;
    if (!exprCast(&rve)) { crtTkErr("invalid expression"); }
    cast(&t, &rve.type);
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
    RetVal rve;
    if (consume(MUL) || consume(DIV)) {
        if (!exprCast(&rve)) { crtTkErr("invalid expr after * or /"); }
        if (rv->type.nElements > -1 || rve.type.nElements > -1)
            tkerr(crtTk, "an array cannot be multiplied or divided");
        if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be multiplied or divided");
        rv->type = getArithType(&rv->type, &rve.type);
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
    RetVal rve;
    if (consume(ADD) || consume(SUB)) {
        if (!exprCast(&rve)) { crtTkErr("invalid expr after +-"); }
        if (rv->type.nElements > -1 || rve.type.nElements > -1)
            tkerr(crtTk, "an array cannot be added or subtracted");
        if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be added or subtracted");
        rv->type = getArithType(&rv->type, &rve.type);
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
    RetVal rve;
    if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
        if (!exprAdd(&rve)) { crtTkErr("invalid expr after comparator"); }
        if (rv->type.nElements > -1 || rve.type.nElements > -1)
            tkerr(crtTk, "an array cannot be compared");
        if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be compared");
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
    RetVal rve;
    if (consume(EQUAL) || consume(NOTEQ)) {
        if (!exprRel(&rve)) { crtTkErr("invalid expr after =="); }
        if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be compared");
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
    RetVal rve;
    if (consume(AND)) {
        if (!exprEq(&rve)) { crtTkErr("invalid expr after &&"); }
        if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be logically tested");
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
    RetVal rve;
    if (consume(OR)) {
        if (!exprAnd(&rve)) { crtTkErr("invalid expr after ||"); }
        if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be logically tested");
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
    Token *startTk = crtTk;
    if (exprUnary(rv)) {
        if (consume(ASSIGN)) {
            RetVal rve;
            if (exprAssign(&rve)) {
                if (!rv->isLVal)tkerr(crtTk, "cannot assign to a non-lval");
                if (rv->type.nElements > -1 || rve.type.nElements > -1)
                    tkerr(crtTk, "the arrays cannot be assigned");
                cast(&rv->type, &rve.type);
                rv->isCtVal = rv->isLVal = 0;
                return 1;
            } else {
                crtTkErr("invalid expression after =");
            }
        }
        crtTk = startTk;
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
    RetVal rv;
    Token *startTk = crtTk;
    if (stmCompound()) { return 1; }
    if (consume(IF)) {
        if (!consume(LPAR)) { crtTkErr("missing ( after if declaration"); }
        if (!expr(&rv)) { crtTkErr("invalid boolean condition after ("); }
        if (rv.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be logically tested");
        if (!consume(RPAR)) { crtTkErr("missing ) in if declaration"); }
        if (!stm()) { crtTkErr("invalid if body"); }
        if (consume(ELSE)) {
            if (!stm()) { crtTkErr("invalid else body"); }
        }
        return 1;
    }
    if (consume(WHILE)) {
        if (!consume(LPAR)) { crtTkErr("missing ( after while declaration"); }
        if (!expr(&rv)) { crtTkErr("invalid boolean condition after ("); }
        if (rv.type.typeBase == TB_STRUCT)
            tkerr(crtTk, "a structure cannot be logically tested");
        if (!consume(RPAR)) { crtTkErr("missing ) in while declaration"); }
        if (!stm()) { crtTkErr("invalid while body"); }
        return 1;
    }
    if (consume(FOR)) {
        RetVal rv1, rv2, rv3;
        if (!consume(LPAR)) { crtTkErr("missing ( after for declaration"); }
        expr(&rv1);
        if (!consume(SEMICOLON)) { crtTkErr("missing ; in for declaration after init"); }
        if (expr(&rv2)) {
            if (rv2.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be logically tested");
        }
        if (!consume(SEMICOLON)) { crtTkErr("missing ; in for declaration after condition"); }
        expr(&rv3);
        if (!consume(RPAR)) { crtTkErr("missing ) in for declaration"); }
        if (!stm()) { crtTkErr("invalid for body"); }
        return 1;
    }
    if (consume(BREAK)) {
        if (!consume(SEMICOLON)) { crtTkErr("missing ; after BREAK"); }
        return 1;
    }
    if (consume(RETURN)) {
        if (expr(&rv)) {
            if (crtFunc->type.typeBase == TB_VOID)
                tkerr(crtTk, "a void function cannot return a value");
            cast(&crtFunc->type, &rv.type);
        }
        if (!consume(SEMICOLON)) { crtTkErr("missing ; after RETURN"); }
        return 1;
    }
    expr(&rv);
    if (consume(SEMICOLON)) {
        return 1;
    }
    crtTk=startTk;
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
        s = addSymbol(&crtFunc->args, tkName->text, CLS_VAR);
        s->mem = MEM_ARG;
        s->type = t;
        return 1;
    }
    return 0;
}

int declFunc() {
    debug("declFunc");
    Type t;
    Token *tkName;
    Token *startTk = crtTk;
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
    if (!consume(LPAR)) {
        crtTk = startTk;
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
    if (!stmCompound()) crtTkErr("invalid function body");
    deleteSymbolsAfter(&symbols, crtFunc);
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
    if (!consume(LBRACKET)) return 0;
    RetVal rv;
    if (expr(&rv)) {
        if (!rv.isCtVal)tkerr(crtTk, "the array size is not a constant");
        if (rv.type.typeBase != TB_INT)tkerr(crtTk, "the array size is not an integer");
        ret->nElements = rv.ctVal.i;
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
    return 0;
}

int declStruct() {
    Token *tkName;
    Token *start = crtTk;
    debug("declStruct");
    if (!consume(STRUCT)) { return 0; }
    if (!consume(ID)) { tkerr(crtTk, "missing name after struct declaration"); }
    tkName = consumedTk;
    if (!consume(LACC)) {
        crtTk = start;
        return 0;
    }
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
    while (1) {
        if (!declStruct() && !declFunc() && !declVar()) break;
    }
    if (!consume(END)) crtTkErr("unexpected token");
    return 1;
}

void sintactic() {
    crtDepth = 0;
    addInitFuncs();
    tokens = lexical("/home/tunarug/custom/gitprojects/facultate/lftc/analizator/test.c");
    crtTk = tokens;
    unit();
}

int main2(int argc, char **argv) {
    sintactic();
    return 0;
}
