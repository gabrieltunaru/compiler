//
// Created by tunarug on 28.03.2020.
//
#include <stdlib.h>
#include <stdio.h>
#include "analizator.h"

Token *tokens;

Token *crtTk = NULL, *consumedTk = NULL;

void crtTkErr(const char *fmt) {
    tkerr(crtTk, fmt);
}

int debugging = 1;

void debug(char *name) {
    if (debugging) {
        printf("@%s %s\n", name, atomToString(crtTk));
    }
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

int typename() {
    return 1;
}


int exprPrimary() {
    debug("exprPrimary");
    Token *startTk = crtTk;
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
            if (!consume(RPAR)) {
                crtTkErr("missing ) in expression");
            }
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

int exprPostfix() {
    debug("exprPostfix");
    Token *startTk = crtTk;
    if (exprPrimary()) {
        if (exprPostfix1()) { return 1; }
    }
    crtTk = startTk;
    return 0;
}

int exprPostfix1() {
    debug("exprPostfix1");
    if (consume(LBRACKET)) {
        if (!expr()) { crtTkErr("invalid expression"); }
        if (!consume(RBRACKET)) { crtTkErr("missing }"); }
        if (!exprPostfix1()) { crtTkErr("invalid expression"); }
    }
    if (consume(DOT)) {
        if (!consume(ID)) { crtTkErr("missing id after ."); }
        if (!exprPostfix1()) { crtTkErr("invalid expression"); }
    }
    return 1;
}

int exprUnary() {
    debug("exprUnary");
    if (consume(SUB) || consume(NOT)) {
        if (!exprUnary()) { crtTkErr("invalid expr"); }
    } else if (!exprPostfix()) { return 0; }
    return 1;
}

int exprCast() {
    debug("exprCast");
    if (!consume(LPAR)) {
        if (exprUnary()) {
            return 1;
        } else {
            return 0;
        }
    }
    if (!typename()) { crtTkErr("invalid type name after ("); }
    if (!consume(RPAR)) { crtTkErr("missing ) in cast"); }
    if (!exprCast()) { crtTkErr("invalid expression"); }
    return 1;
}


/*
 * exprMul: exprMul ( MUL | DIV ) exprCast | exprCast ;
 * exprMul: expreCast exprMul1
 * exprMul1: ( MUL | DIV ) expreCast exprMul1 | epsilon
 */

int exprMul1() {
    debug("exprMul1");
    if (consume(MUL) || consume(DIV)) {
        if (!exprCast()) { crtTkErr("invalid expr after * or /"); }
        exprMul1();
    }
    return 1;
}

int exprMul() {
    debug("exprMul");
    if (!exprCast()) { return 0; }
    exprMul1();
    return 1;
}

/*
 * exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul ;
 * exprAdd: exprMul exprAdd1
 * exprAdd1: ( ADD | SUB ) exprMul expreAdd1 | epsilon
 */

int exprAdd1() {
    debug("exprAdd1");
    if (consume(ADD) || consume(SUB)) {
        if (!exprCast()) { crtTkErr("invalid expr after +-"); }
        exprAdd1();
    }
    return 1;
}

int exprAdd() {
    debug("exprAdd");
    if (!exprMul()) { return 0; }
    if (exprAdd1()) { return 1; }
}

int exprRel1() {
    debug("exprRel1");
    if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
        if (!exprAdd()) { crtTkErr("invalid expr after comparator"); }
        exprRel1();
    }
    return 1;
}

int exprRel() {
    debug("exprRel");
    if (!exprAdd()) { return 0; }
    exprRel1();
    return 1;
}

int exprEq1() {
    debug("exprEq1");
    if (consume(EQUAL) || consume(NOTEQ)) {
        if (!exprRel()) { crtTkErr("invalid expr after =="); }
        exprEq1();
    }
    return 1;
}

int exprEq() {
    debug("exprEq");
    if (!exprRel()) { return 0; }
    exprEq1();
    return 1;
};

int exprAnd1() {
    debug("exprAnd1");
    if (consume(AND)) {
        if (!exprEq()) { crtTkErr("invalid expr after &&"); }
        exprAnd1();
    }
    return 1;
}

int exprAnd() {
    debug("exprAnd");
    if (!exprEq()) { return 0; }
    return exprAnd1();
}

int exprOr1() {
    debug("exprOr1");
    if (consume(OR)) {
        if (!exprAnd()) { crtTkErr("invalid expr after ||"); }
        exprOr1();
    }
    return 1;
}

int exprOr() {
    debug("exprOr");
    if (!exprAnd()) { return 0; }
    return exprOr1();
}

int exprAssign() {
    debug("exprAssign");
    Token *startTk = crtTk;
    if (exprUnary()) {
        if (consume(ASSIGN)) {
            if (exprAssign()) {
                return 1;
            } else {
                crtTkErr("invalid expression after =");
            }
        }
        crtTk = startTk;
    }
    if (exprOr()) {
        return 1;
    }
    return 0;
}

int expr() {
    debug("expr");
    return exprAssign();
}

int stmCompound() {
    debug("stmCompound");
    Token *startTk = crtTk;
    if (!consume(LACC)) { return 0; }
    while (1) {
        if (!declVar() && !stm()) { break; }
    }
    if (!consume(RACC)) { crtTkErr("missing ) in statement"); }
    return 1;
}

int stm() {
    debug("stm");
    if (stmCompound()) { return 1; }
    if (consume(IF)) {
        if (!consume(LPAR)) { crtTkErr("missing ( after if declaration"); }
        if (!expr()) { crtTkErr("invalid boolean condition after ("); }
        if (!consume(RPAR)) { crtTkErr("missing ) in if declaration"); }
        if (!stm()) { crtTkErr("invalid if body"); }
        if (consume(ELSE)) {
            if (!stm()) { crtTkErr("invalid else body"); }
        }
        return 1;
    }
    if (consume(WHILE)) {
        if (!consume(LPAR)) { crtTkErr("missing ( after while declaration"); }
        if (!expr()) { crtTkErr("invalid boolean condition after ("); }
        if (!consume(RPAR)) { crtTkErr("missing ) in while declaration"); }
        if (!stm()) { crtTkErr("invalid while body"); }
        return 1;
    }
    if (consume(FOR)) {
        if (!consume(LPAR)) { crtTkErr("missing ( after for declaration"); }
        expr();
        if (!consume(SEMICOLON)) { crtTkErr("missing ; in for declaration after init"); }
        expr();
        if (!consume(SEMICOLON)) { crtTkErr("missing ; in for declaration after condition"); }
        expr();
        if (!consume(RPAR)) { crtTkErr("missing ) in for declaration"); }
        if (!stm()) { crtTkErr("invalid for body"); }
        return 1;
    }
    if (consume(BREAK)) {
        if (!consume(SEMICOLON)) { crtTkErr("missing ; after BREAK"); }
        return 1;
    }
    if (consume(RETURN)) {
        expr();
        if (!consume(SEMICOLON)) { crtTkErr("missing ; after RETURN"); }
        return 1;
    }
    expr();
    if (consume(SEMICOLON)) {
        return 1;
    }
    return 0;
}

int funcArg() {
    debug("funcArg");
    if (typeBase()) {
        if (!consume(ID)) { crtTkErr("missing id after type declaration"); }
        arrayDecl();
        return 1;
    }
    return 0;
}

int declFunc() {
    debug("declFunc");
    Token *startTk = crtTk;
    if (!consume(VOID)) {
        if (!typeBase()) { return 0; }
        else {
            consume(MUL);
        }
    }
    if (!consume(ID)) { crtTkErr("invalid function name"); }
    if (!consume(LPAR)) {
        crtTk = startTk;
        return 0;
    }
    funcArg();
    while (1) {
        if (!consume(COMMA)) { break; }
        if (!funcArg()) { crtTkErr("missing argument after ,"); }
    }
    if (!consume(RPAR)) { crtTkErr("missing ) in function signature"); }
    if (!stmCompound()) crtTkErr("invalid function body");
    return 1;
}

int typeName() {
    debug("typeName");
    if (!typeBase()) return 0;
    arrayDecl();
}

int arrayDecl() {
    debug("arrayDecl");
    if (!consume(LBRACKET)) return 0;
    expr();
    if (!consume(RBRACKET)) crtTkErr("missing ] in type declaration");
    return 1;
}

int typeBase() {
    debug("typeBase");
    if (consume(INT) || consume(DOUBLE) || consume(CHAR)) {
        return 1;
    }
    if (consume(STRUCT)) {
        if (consume(ID)) {
            return 1;
        } else {
            crtTkErr("missing struct name");
        }
    }
    return 0;
}

int declVar() {
    debug("declVar");
    Token *startTk = crtTk;
    int isDV;
    if (typeBase()) {
        if (consume(ID)) {
            isDV = arrayDecl();
            while (1) {
                if (consume(COMMA)) {
                    isDV = 1;
                    if (consume(ID)) {
                        arrayDecl();
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
    debug("declStruct");
    if (!consume(STRUCT)) { return 0; }
    if (!consume(ID)) { tkerr(crtTk, "missing name after struct declaration"); }
    if (!consume(LACC)) { tkerr(crtTk, "missing { in struct declaration"); }
    while (1) {
        if (!declVar()) { break; }
    }
    if (!consume(RACC)) { tkerr(crtTk, "missing } in struct declaration"); }
    if (!consume(SEMICOLON)) { tkerr(crtTk, "missing ; after struct declaration"); }
    return 1;
}

int unit() {
    debug("unit");
    while (1) {
        if (!declVar() && !declStruct() && !declFunc()) break;
    }
    if (!consume(END)) crtTkErr("unexpected token");
    return 1;
}

void sintactic() {
    tokens = lexical("/home/tunarug/custom/gitprojects/facultate/lftc/analizator/test.c");
    crtTk = tokens;
    unit();
}

int main(int argc, char **argv) {
    sintactic();
    return 0;
}
