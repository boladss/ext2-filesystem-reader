#include <stdio.h>
#include "parseDir.c"

int main(){
    FILE * fs = fopen("testfs", "r");

    parseSuperBlock(fs, 1024);

    printf("uwu\n");

    return 0;
}