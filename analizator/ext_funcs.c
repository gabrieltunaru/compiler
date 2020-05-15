//
// Created by tunarug on 16.05.2020.
//
#include "vm.h"
#include <stdio.h>
#include <time.h>

void put_s(char s[]) {
    printf("#%s\n", popa());
}

void get_s() {
    char s[64];
    scanf("%s", s);
    pusha((void *) s);
}

void put_i() {
    printf("#%ld\n", popi());
}

void get_i() {
    int i;
    scanf("%d", &i);
    pushi(i);
}

void put_d() {
    printf("#%lf\n", popd());
}

void get_d() {
    double d;
    scanf("%lf", &d);
    pushd(d);
}

void put_c() {
    printf("#%c\n", popc());
}

void get_c() {
    char c;
    scanf("%c", &c);
    pushi(c);
}

void seconds() {
    time_t now;
    struct tm *tm;

    now = time(0);
    if ((tm = localtime(&now)) == NULL) {
        printf("Time error\n");
    }
    pushd( tm->tm_sec);
}