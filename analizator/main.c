#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#define SAFEALLOC(var, Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");
int line = 0;
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
    EXP,
    CT_REAL,
    ESC,
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

char *createString(const char *pStartCh, const char *pCrtCh) {
    long length = pCrtCh - pStartCh;
    char *str = malloc(length);
    const char *i;
    for (i = pStartCh; i < pCrtCh; i++) {
        str[i - pStartCh] = *i;
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

char *code = NULL;
const char *pCrtCh;

int getNextToken() {
    int state = 0, nCh;
    char ch;
    const char *pStartCh = NULL;
    Token *tk;
    while (1) {
        ch = *pCrtCh;
        switch (state) {
            case (0):
                if (isalpha(ch) || ch == '_') {
                    pStartCh = pCrtCh;
                    pCrtCh++;
                    state = 48;
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
                    pCrtCh++;
                    state = 16;
                }
                break;
            case (2):
                if (ch == '\n' || ch == '\r' || ch == '\0') {
                    pCrtCh++;
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
                tk->i = ch;
                return CT_CHAR;
            case (34):
                if (strchr("abfnrtv'?\"\0", ch)) {
                    state = 31;
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
                    pCrtCh++;
                    state = 37;
                } else {
                    pCrtCh++;
                }
                break;
            case (37):
                tk = addTk(CT_STRING);
                tk->text = createString(pStartCh, pCrtCh);
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
                    state = 49;
                } else {
                    state = 41;
                }
                break;
            case 41:
                tk = addTk(CT_INT);
                tk->i = makeInt(pStartCh, pCrtCh);
                printf("%d\n", tk->i);
                return INT;
            case 42:
                pCrtCh++;
                if (ch == 'x') {
                    state = 44;
                } else if (ch >= '0' && ch <= '7') {
                    state = 43;
                } else if (ch == '8' || ch == 9) {
                    state = 46;
                } else if (ch == '.') {
                    state = 47;
                } else tkerr(addTk(END), "caracter invalid");
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
                if((ch>'0'&&ch<='9')||(ch>'a'&&ch<='f')||(ch>'A'&&ch<='F')) {
                    pCrtCh++;
                    state=45;
                } else {
                    tkerr(addTk(END),"caracter invalid");
                }
                break;
            case 45:
                if((ch>='0'&&ch<'9')||(ch>='a'&&ch<'f')||(ch>='A'&&ch<'F')) {
                    pCrtCh++;
                } else {
                    state=41;
                }
                break;
            case 46:
                if()
                break;
            case (48):
                //todo: id
                pCrtCh++;
                state = 0;
                break;
        }
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
}

int main(int argc, char **argv) {
//    printf("%s",argv[1]);
    read_file(argv[1]);
    printf("%s", code);
    start();
    free(code);
    return 0;
}