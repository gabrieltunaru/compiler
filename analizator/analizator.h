//
// Created by tunarug on 28.03.2020.
//

#ifndef ANALIZATOR_ANALIZATOR_H
#define ANALIZATOR_ANALIZATOR_H

#endif //ANALIZATOR_ANALIZATOR_H

#define SAFEALLOC(var, Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");


enum {
    ID,
    BREAK,
    CHAR,
    DOUBLE,
    ELSE,
    FOR,
    IF,
    INT,
    RETURN,
    STRUCT,
    VOID,
    WHILE,
    CT_INT,
    CT_REAL,
    CT_CHAR,
    CT_STRING,
    COMMA,
    SEMICOLON,
    LPAR,
    RPAR,
    LBRACKET,
    RBRACKET,
    LACC,
    RACC,
    ADD,
    SUB,
    MUL,
    DIV,
    DOT,
    AND,
    OR,
    NOT,
    ASSIGN,
    EQUAL,
    NOTEQ,
    LESS,
    LESSEQ,
    GREATER,
    GREATEREQ,
    END
}; // codurile AL

typedef struct _Token {
    int code; // codul (numele)
    union {
        char *text; // folosit pentru ID, CT_STRING (alocat dinamic)
        long int i; // folosit pentru CT_INT, CT_CHAR
        double r; // folosit pentru CT_REAL
    };
    int line; // linia din fisierul de intrare
    struct _Token *next; // inlantuire la urmatorul AL
} Token;
extern Token *crtTk;

Token *lexical(char *);

const char *atomToString(Token *token);

void err(const char *fmt, ...);

void tkerr(const Token *tk, const char *fmt, ...);