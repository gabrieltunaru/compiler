//
// Created by tunarug on 28.03.2020.
//
#include <stdlib.h>
#include "analizator.h"

Token *tokens;

Token *crtTk = NULL, *consumedTk = NULL, *startTk = NULL;

void crtTkErr(const char *fmt) {
    tkerr(crtTk, fmt);
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

int exprUnary();

int exprCast();

int exprMul();
int exprAdd();

/*
 * exprPostfix: exprPostfix LBRACKET expr RBRACKET
           | exprPostfix DOT ID
           | exprPrimary ;
 * exprPostFix: exprPrimary exprPostFix1
 * exprPostFix1: LBRACKET expr RBRACKET exprPostfix
            | DOT ID exprPostFix
            | epsilon
 */

int expr() {
    return 1;
}

int typename() {
    return 1;
}

int exprRel1() {
    if(consume(LESS)||consume(LESSEQ)||consume(GREATER)||consume(GREATEREQ)) {
        if (!exprAdd()) {crtTkErr("invalid expr");}
        exprRel1();
    }
    return 1;
}

int exprRel() {
    if (!exprAdd()) { return 0; }
    exprRel1();
    return 1;
}


/*
 * exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul ;
 * exprAdd: exprMul exprAdd1
 * exprAdd1: ( ADD | SUB ) exprMul expreAdd1 | epsilon
 */

int exprAdd1() {
    if (consume(ADD) || consume(SUB)) {
        if (!exprCast()) { crtTkErr("invalid expr"); }
        exprAdd1();
    }
    return 1;
}

int exprAdd() {
    if (!exprMul()) { return 0; }
    if (exprAdd1()) { return 1; }
}

/*
 * exprMul: exprMul ( MUL | DIV ) exprCast | exprCast ;
 * exprMul: expreCast exprMul1
 * exprMul1: ( MUL | DIV ) expreCast exprMul1 | epsilon
 */

int exprMul1() {
    if (consume(MUL) || consume(DIV)) {
        if (!exprCast()) { crtTkErr("invalid expr"); }
        exprMul1();
    }
    return 1;
}

int exprMul() {
    if (!exprCast()) { return 0; }
    if (exprMul1()) { return 1; }
}


int exprCast() {
    if (!consume(LPAR)) {
        if (exprUnary()) {
            return 1;
        } else {
            return 0;
        }
    }
    if (!typename()) { crtTkErr("invalid type name after ("); }
    if (!consume(RPAR)) { crtTkErr("missing )"); }
    if (!exprCast()) { crtTkErr("invalid expression"); }
    return 1;
}

int exprUnary() {
    if (consume(SUB) || consume(NOT)) {
        if (!exprUnary()) { crtTkErr("invalid expr"); }
    } else if (!exprPostfix()) { return 0; }
    return 1;
}

int exprPostfix() {
    if (exprPrimary() || exprPostfix1()) { return 1; }
    return 0;
}

int exprPostfix1() {
    if (consume(LBRACKET)) {
        if (!expr()) { crtTkErr("invalid expression"); }
        if (!consume(RBRACKET)) { crtTkErr("missing }"); }
        if (!exprPostfix()) { crtTkErr("invalid expression"); }
    } else if (consume(DOT)) {
        if (!consume(ID)) { crtTkErr("missing id after ."); }
        if (!exprPostfix()) { crtTkErr("invalid expression"); }
    }
    return 1;
}

int exprPrimary() {
    if (consume(ID)) {
        if (consume(LPAR)) {
            if (expr()) {
                while (1) {
                    if (consume(COMMA)) {
                        if (!expr()) {
                            crtTkErr("invalid expression after ,");
                        }
                    } else {
                        break;
                    };
                }
            }
        }
        if (!consume(RPAR)) {
            crtTkErr("missing )");
        }
        return 1;
    } else if (consume(CT_INT)) { return 1; }
    else if (consume(CT_REAL)) { return 1; }
    else if (consume(CT_CHAR)) { return 1; }
    else if (consume(CT_STRING)) { return 1; }
    else if (consume(LPAR)) {
        if (!expr()) {
            crtTkErr("invalid expr");
        }
        if (!consume(RPAR)) { crtTkErr("missing )"); }
        return 1;
    }
    startTk = crtTk;
    return 0;

}


int typeBase() {
    if (consume(INT) || consume(DOUBLE) || consume(CHAR) || consume(STRUCT)) {
        if (!consume(ID)) { tkerr(crtTk, "invalid type declaration"); }
    }
    return 1;
}

int declVar() {
    if (!typeBase()) { return 0; }
    if (!consume(ID)) { tkerr(crtTk, "invalid type declaration"); }

    return 1;
}

int declStruct() {
    if (!consume(STRUCT)) { return 0; }
    if (!consume(ID)) { tkerr(crtTk, "missing id after struct declaration"); }
    if (!consume(LACC)) { tkerr(crtTk, "missing {"); }
    while (1) {
        if (!declVar()) { break; }
    }
    if (!consume(RACC)) { tkerr(crtTk, "missing }"); }
    if (!consume(SEMICOLON)) { tkerr(crtTk, "missing ;"); }
    return 1;
}

void sintactic() {
    tokens = lexical("/home/tunarug/custom/gitprojects/facultate/lftc/analizator/test.c");
    crtTk = tokens;
}

int main(int argc, char **argv) {
    sintactic();
    return 0;
}
