#ifndef MYSTRING
#define MYSTRING


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct String {
    char *data;
    int len;
} String;

#define String(str) String_copy_from_literal(str)
#define StringAllocated(str) (String){.len = strlen(str), .data = str}



String String_copy_from_literal(const char *literal) {
    String str = {
        .len = strlen(literal)
    };

    str.data = malloc(str.len + 1);
    memcpy(str.data, literal, str.len);

    str.data[str.len] = 0;


    return str;
}

String String_new(int len) {
    String new = {
        .len = len,
        .data = malloc(len + 1)
    };

    new.data[len] = 0;

    return new;
}

String String_concat(String a, String b) {
    String new = String_new(a.len + b.len);

    memcpy(new.data, a.data, a.len);
    memcpy(new.data + a.len, b.data, b.len);

    return new;
}

String String_delete(String *str) {
    free(str->data);
    str->len = -1;
}

String String_from_int(int num) {

    int len = 0;
    int temp = num;
    do {
        len++;
        temp /= 10;
    } while (temp != 0);

    String string = String_new(len);

    int temp2 = num;
    int i = 0;
    do {
        string.data[len - 1 - i] = '0' + temp2 % 10;
        temp2 /= 10;        
        i++;
    } while (temp2 != 0);

    return string;
}

#endif