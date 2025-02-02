#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include "analizator.h"


int line = 1;
Token *tokens = NULL, *lastToken = NULL;

void err(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, va);
    fputc('\n', stderr);
    va_end(va);
    exit(-1);
}

void tkerr(const Token *tk, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    fprintf(stderr, "error in line %d: ", tk->line);
    vfprintf(stderr, fmt, va);
    fputc('\n', stderr);
    va_end(va);
    exit(-1);
}

Token *addTk(int code) {
    Token *tk;
    SAFEALLOC(tk, Token)
    tk->code = code;
    tk->line = line;
    tk->next = NULL;
    if (lastToken) {
        lastToken->next = tk;
    } else {
        tokens = tk;
    }
    lastToken = tk;
    return tk;
}


char escape(char ch) {
    switch (ch) {
        case 'a':
            return '\a';
        case 'b':
            return '\b';
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case 'v':
            return '\v';
        case '\'':
            return '\'';
        case '?':
            return '\?';
        case '"':
            return '\"';
        case '\\':
            return '\\';
        case '0':
            return '\0';
    }
    return '0';
}

char *createString(const char *pStartCh, const char *pCrtCh) {
    long length = pCrtCh - pStartCh;
    char *str = malloc(length);
    if (str == NULL) {
        err("Not enough memory");
    }
    const char *i;
    int index = 0;
    for (i = pStartCh; i < pCrtCh; i++) {
        char ch = *i;
        if (ch == '\\') {
            i++;
            ch = escape(*i);
        }
        str[index++] = ch;
    }
    str[length] = '\0';
    return str;
}

int makeInt(const char *pStartCh, const char *pCrtCh) {
    char *st = createString(pStartCh, pCrtCh);
    int base = 10;
    if (st[0] == '0') {
        if (st[1] == 'x') {
            base = 16;
        } else {
            base = 8;
        }
    }
    return strtol(st, NULL, base);
}

double makeReal(const char *pStartCh, const char *pCrtCh) {
    char *st = createString(pStartCh, pCrtCh);
    return strtod(st, NULL);
}

char *code = NULL;
const char *pCrtCh;


char atomName[10];
int getNextToken() {
    int state = 0, length;
    char ch, escaped;
    const char *pStartCh = NULL;
    Token *tk;
    while (1) {
        ch = *pCrtCh;
        switch (state) {
            case (0):
                if (isalpha(ch) || ch == '_') {
                    pStartCh = pCrtCh;
                    pCrtCh++;
                    state = 53;
                } else if (ch == ' ' || ch == '\r' || ch == '\t') {
                    pCrtCh++;
                } else if (ch == '\n') {
                    line++;
                    pCrtCh++;
                } else if (ch == '/') {
                    state = 1;
                    pCrtCh++;
                } else if (ch == ',') {
                    pCrtCh++;
                    state = 5;
                } else if (ch == ';') {
                    pCrtCh++;
                    state = 6;
                } else if (ch == '(') {
                    pCrtCh++;
                    state = 7;
                } else if (ch == ')') {
                    pCrtCh++;
                    state = 8;
                } else if (ch == '[') {
                    pCrtCh++;
                    state = 9;
                } else if (ch == ']') {
                    pCrtCh++;
                    state = 10;
                } else if (ch == '{') {
                    pCrtCh++;
                    state = 11;
                } else if (ch == '}') {
                    pCrtCh++;
                    state = 12;
                } else if (ch == '+') {
                    pCrtCh++;
                    state = 13;
                } else if (ch == '-') {
                    pCrtCh++;
                    state = 14;
                } else if (ch == '*') {
                    pCrtCh++;
                    state = 15;
                } else if (ch == '.') {
                    pCrtCh++;
                    state = 17;
                } else if (ch == '&') {
                    pCrtCh++;
                    state = 18;
                } else if (ch == '|') {
                    pCrtCh++;
                    state = 20;
                } else if (ch == '!') {
                    pCrtCh++;
                    state = 22;
                } else if (ch == '=') {
                    pCrtCh++;
                    state = 24;
                } else if (ch == '<') {
                    pCrtCh++;
                    state = 26;
                } else if (ch == '>') {
                    pCrtCh++;
                    state = 28;
                } else if (ch == '\'') {
                    pCrtCh++;
                    state = 30;
                } else if (ch == '"') {
                    pCrtCh++;
                    pStartCh = pCrtCh;
                    state = 35;
                } else if (ch == '0') {
                    pStartCh = pCrtCh;
                    pCrtCh++;
                    state = 42;
                } else if (ch >= '1' && ch <= '9') {
                    pStartCh = pCrtCh;
                    pCrtCh++;
                    state = 40;
                } else if (ch == 0) {
                    addTk(END);
                    return END;
                } else tkerr(addTk(END), "caracter invalid");
                break;
                //comment
            case (1):
                if (ch == '/') {
                    state = 2;
                    pCrtCh++;
                } else if (ch == '*') {
                    state = 3;
                    pCrtCh++;
                } else {
//                    pCrtCh++;
                    state = 16;
                }
                break;
            case (2):
                if (ch == '\n' || ch == '\r' || ch == '\0') {
                    pCrtCh++;
                    line++;
                    state = 0;
                } else {
                    pCrtCh++;
                }
                break;
            case (3):
                if (ch == '*') {
                    pCrtCh++;
                    state = 4;
                } else {
                    if (ch == '\n') {
                        line++;
                    }
                    pCrtCh++;
                }
                break;
            case (4):
                if (ch == '*') {
                    pCrtCh++;
                } else if (ch == '/') {
                    state = 0;
                    pCrtCh++;
                } else {
                    state = 3;
                    pCrtCh++;
                }
                break;
            case (5):
                tk = addTk(COMMA);
                return COMMA;
            case (6):
                tk = addTk(SEMICOLON);
                return SEMICOLON;
            case (7):
                tk = addTk(LPAR);
                return LPAR;
            case (8):
                tk = addTk(RPAR);
                return RPAR;
            case (9):
                tk = addTk(LBRACKET);
                return LBRACKET;
            case (10):
                tk = addTk(RBRACKET);
                return RBRACKET;
            case (11):
                tk = addTk(LACC);
                return LACC;
            case (12):
                tk = addTk(RACC);
                return RACC;
            case (13):
                tk = addTk(ADD);
                return ADD;
            case (14):
                tk = addTk(SUB);
                return SUB;
            case (15):
                tk = addTk(MUL);
                return MUL;
            case (16):
                tk = addTk(DIV);
                return DIV;
            case (17):
                tk = addTk(DOT);
                return DOT;
            case (18):
                if (ch == '&') {
                    pCrtCh++;
                    state = 19;
                } else {
                    tkerr(addTk(END), "caracter invalid");
                }
                break;
            case (19):
                addTk(AND);
                return AND;
            case (20):
                if (ch == '|') {
                    pCrtCh++;
                    state = 21;
                } else {
                    tkerr(addTk(END), "caracter invalid");
                }
                break;
            case (21):
                addTk(OR);
                return OR;
            case (22):
                if (ch == '=') {
                    pCrtCh++;
                    state = 23;
                    break;
                } else {
                    addTk(NOT);
                    return NOT;
                }
            case (23):
                addTk(NOTEQ);
                return NOTEQ;
            case (24):
                if (ch == '=') {
                    pCrtCh++;
                    state = 25;
                    break;
                } else {
                    addTk(ASSIGN);
                    return ASSIGN;
                }
            case (25):
                addTk(EQUAL);
                return EQUAL;
            case (26):
                if (ch == '=') {
                    pCrtCh++;
                    state = 27;
                    break;
                }
                addTk(LESS);
                return LESS;
            case (27):
                addTk(LESSEQ);
                return LESSEQ;
            case (28):
                if (ch == '=') {
                    pCrtCh++;
                    state = 29;
                    break;
                }
                addTk(GREATER);
                return GREATER;
            case (29):
                addTk(GREATEREQ);
                return GREATEREQ;
            case (30):
                if (ch == '\\') {
                    pCrtCh++;
                    state = 34;
                } else {
                    pStartCh = pCrtCh;
                    pCrtCh++;
                    state = 31;
                }
                break;
            case (31):
                if (ch == '\'') {
                    pCrtCh++;
                    state = 32;
                }
                break;
            case (32):
                tk = addTk(CT_CHAR);

                tk->i = *pStartCh;
                return CT_CHAR;
            case (34):
                if (strchr("abfnrtv'?\"\\0", ch)) {
                    state = 31;
                    escaped = escape(ch);
                    pStartCh = &escaped;
                    pCrtCh++;
                } else {
                    tkerr(addTk(END), "caracter invalid");
                }
                break;
            case (35):
                if (ch == '\\') {
                    pCrtCh++;
                    state = 38;
                } else if (ch == '"') {
                    pCrtCh++;
                    state = 37;
                } else {
                    pCrtCh++;
                    state = 36;
                }
                break;
            case (36):
                if (ch == '\\') {
                    pCrtCh++;
                    state = 38;
                } else if (ch == '"') {
                    state = 37;
                } else {
                    pCrtCh++;
                }
                break;
            case (37):
                tk = addTk(CT_STRING);
                tk->text = createString(pStartCh, pCrtCh);
                pCrtCh++;
                return CT_STRING;
            case 38:
                if (strchr("abfnrtv'?\"\0", ch)) {
                    state = 36;
                    pCrtCh++;
                } else {
                    tkerr(addTk(END), "caracter invalid");
                }
                break;
            case 40:
                if (ch >= '0' && ch <= '9') {
                    pCrtCh++;
                } else if (ch == '.') {
                    pCrtCh++;
                    state = 47;
                } else if (ch == 'e' || ch == 'E') {
                    pCrtCh++;
                    state = 49;
                } else {
                    state = 41;
                }
                break;
            case 41:
                tk = addTk(CT_INT);
                tk->i = makeInt(pStartCh, pCrtCh);
                return INT;
            case 42:
                if (ch == 'x') {
                    pCrtCh++;
                    state = 44;
                } else if (ch >= '0' && ch <= '7') {
                    pCrtCh++;
                    state = 43;
                } else if (ch == '8' || ch == 9) {
                    pCrtCh++;
                    state = 46;
                } else if (ch == '.') {
                    pCrtCh++;
                    state = 47;
                } else state=41;
                break;
            case 43:
                if (ch >= '0' && ch <= '7') {
                    pCrtCh++;
                } else if (ch == 'e' || ch == 'E') {
                    pCrtCh++;
                    state = 49;
                } else if (ch == '.') {
                    pCrtCh++;
                    state = 47;
                } else {
                    state = 41;
                }
                break;
            case 44:
                if ((ch > '0' && ch <= '9') || (ch > 'a' && ch <= 'f') || (ch > 'A' && ch <= 'F')) {
                    pCrtCh++;
                    state = 45;
                } else {
                    tkerr(addTk(END), "caracter invalid");
                }
                break;
            case 45:
                if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
                    pCrtCh++;
                } else {
                    state = 41;
                }
                break;
            case 46:
                if (ch >= '0' && ch <= '9') {
                    pCrtCh++;
                } else if (ch == '.') {
                    pCrtCh;
                    state = 47;
                } else {
                    tkerr(addTk(END), "caracter invalid");
                }
                break;
            case 47:
                if (ch >= '0' && ch <= '9') {
                    pCrtCh++;
                    state = 48;
                } else {
                    tkerr(addTk(END), "caracter invalid");
                }
                break;
            case 48:
                if (ch >= '0' && ch <= '9') {
                    pCrtCh++;
                } else if (ch == 'e' || ch == 'E') {
                    pCrtCh++;
                    state = 49;
                } else {
                    state = 52;
                }
                break;
            case 49:
                if (ch == '+' || ch == '-') {
                    pCrtCh++;
                }
                state = 50;
                break;
            case 50:
                if (ch >= '0' && ch <= '9') {
                    pCrtCh++;
                    state = 51;
                } else {
                    tkerr(addTk(END), "caracter invalid");
                }
                break;
            case 51:
                if (ch >= '0' && ch <= '9') {
                    pCrtCh++;
                } else {
                    state = 52;
                }
                break;
            case 52:
                tk = addTk(CT_REAL);
                tk->r = makeReal(pStartCh, pCrtCh);
//                printf("%s %f\n", createString(pStartCh, pCrtCh), tk->r);
                return CT_REAL;
            case 53:
                if (isalnum(ch) || ch == '_') {
                    pCrtCh++;
                } else {
                    state = 54;
                }
                break;
            case (54):
                length = pCrtCh - pStartCh;
                if (length == 5 && !memcmp(pStartCh, "break", 5))tk = addTk(BREAK);
                else if (length == 4 && !memcmp(pStartCh, "char", 4))tk = addTk(CHAR);
                else if (length == 6 && !memcmp(pStartCh, "double", 6))tk = addTk(DOUBLE);
                else if (length == 4 && !memcmp(pStartCh, "else", 4))
                    tk = addTk(ELSE);
                else if (length == 3 && !memcmp(pStartCh, "for", 3))
                    tk = addTk(FOR);
                else if (length == 2 && !memcmp(pStartCh, "if", 2))
                    tk = addTk(IF);
                else if (length == 3 && !memcmp(pStartCh, "int", 3))tk = addTk(INT);
                else if (length == 6 && !memcmp(pStartCh, "return", 6))tk = addTk(RETURN);
                else if (length == 6 && !memcmp(pStartCh, "struct", 6))tk = addTk(STRUCT);
                else if (length == 4 && !memcmp(pStartCh, "void", 4))tk = addTk(VOID);
                else if (length == 5 && !memcmp(pStartCh, "while", 5))tk = addTk(WHILE);
                else {
                    tk = addTk(ID);
                    tk->text = createString(pStartCh, pCrtCh);
                }
                return tk->code;
                break;
            default:
                addTk(END);
                return END;
        }
    }
}
const char *atomToString(Token *token) {
    switch (token->code) {
        case ID:
            sprintf(atomName, "%s: %s ", "ID", token->text);
            return atomName;
        case BREAK:
            sprintf(atomName, "%s", "BREAK");
            return atomName;
        case CHAR:
            sprintf(atomName, "%s", "CHAR");
            return atomName;
        case DOUBLE:
            sprintf(atomName, "%s", "DOUBLE");
            return atomName;
        case ELSE:
            sprintf(atomName, "%s", "ELSE");
            return atomName;
        case FOR:
            sprintf(atomName, "%s", "FOR");
            return atomName;
        case IF:
            sprintf(atomName, "%s", "IF");
            return atomName;
        case INT:
            sprintf(atomName, "%s", "INT");
            return atomName;
        case RETURN:
            sprintf(atomName, "%s", "RETURN");
            return atomName;
        case STRUCT:
            sprintf(atomName, "%s", "STRUCT");
            return atomName;
        case VOID:
            sprintf(atomName, "%s", "VOID");
            return atomName;
        case WHILE:
            sprintf(atomName, "%s", "WHILE");
            return atomName;
        case CT_INT:
            sprintf(atomName, "%s:%d", "CT_INT", token->i);
            return atomName;
        case CT_REAL:
            sprintf(atomName, "%s:%f", "CT_REAL", token->r);
            return atomName;
        case CT_CHAR:
            sprintf(atomName, "%s:%c", "CT_CHAR", token->i);
            return atomName;
        case CT_STRING:
            sprintf(atomName, "%s:%s", "CT_STRING", token->text);
            return atomName;
        case COMMA:
            sprintf(atomName, "%s", "COMMA");
            return atomName;
        case SEMICOLON:
            sprintf(atomName, "%s", "SEMICOLON");
            return atomName;
        case LPAR:
            sprintf(atomName, "%s", "LPAR");
            return atomName;
        case RPAR:
            sprintf(atomName, "%s", "RPAR");
            return atomName;
        case LBRACKET:
            sprintf(atomName, "%s", "LBRACKET");
            return atomName;
        case RBRACKET:
            sprintf(atomName, "%s", "RBRACKET");
            return atomName;
        case LACC:
            sprintf(atomName, "%s", "LACC");
            return atomName;
        case RACC:
            sprintf(atomName, "%s", "RACC");
            return atomName;
        case ADD:
            sprintf(atomName, "%s", "ADD");
            return atomName;
        case SUB:
            sprintf(atomName, "%s", "SUB");
            return atomName;
        case MUL:
            sprintf(atomName, "%s", "MUL");
            return atomName;
        case DIV:
            sprintf(atomName, "%s", "DIV");
            return atomName;
        case DOT:
            sprintf(atomName, "%s", "DOT");
            return atomName;
        case AND:
            sprintf(atomName, "%s", "AND");
            return atomName;
        case OR:
            sprintf(atomName, "%s", "OR");
            return atomName;
        case NOT:
            sprintf(atomName, "%s", "NOT");
            return atomName;
        case ASSIGN:
            sprintf(atomName, "%s", "ASSIGN");
            return atomName;
        case EQUAL:
            sprintf(atomName, "%s", "EQUAL");
            return atomName;
        case NOTEQ:
            sprintf(atomName, "%s", "NOTEQ");
            return atomName;
        case LESS:
            sprintf(atomName, "%s", "LESS");
            return atomName;
        case LESSEQ:
            sprintf(atomName, "%s", "LESSEQ");
            return atomName;
        case GREATER:
            sprintf(atomName, "%s", "GREATER");
            return atomName;
        case GREATEREQ:
            sprintf(atomName, "%s", "GREATEREQ");
            return atomName;
        case END:
            sprintf(atomName, "%s", "END");
            return atomName;
    }
}
void showAtoms() {
    Token *token = tokens;
    while (token) {
        printf("%d ", token->line);
        printf("%s", atomToString(token));
        printf("\n");
        token = token->next;
    }
}

void read_file(char *file_name) {
    FILE *f = fopen(file_name, "r");
    if (f == NULL) {
        perror("Open file");
        exit(-1);
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *text = malloc(fsize + 1);
    fread(text, 1, fsize, f);
    text[fsize] = '\0';
    code = text;

}

void start() {
    pCrtCh = &code[0];
    while (getNextToken() != END);
//    showAtoms();
}

Token *lexical(char *file) {
    read_file(file);
    start();
    free(code);
    return tokens;
}
