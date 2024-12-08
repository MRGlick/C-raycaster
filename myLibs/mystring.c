#ifndef MYSTRING
#define MYSTRING


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "array.c"

#define false 0
#define true 1
#ifndef bool
#define bool char
#endif

typedef struct String {
    char *data;
    int len;
    bool ref;
} String;

typedef String StringRef;

#define String(str) String_copy_from_literal(str)
#define StringRef(str) (String){.len = strlen(str), .data = str, .ref = true}


String String_copy_from_literal(const char *literal) {
    String str = {
        .len = strlen(literal),
        .ref = false
    };

    str.data = malloc(str.len + 1);
    memcpy(str.data, literal, str.len);

    str.data[str.len] = 0;


    return str;
}

String String_ncopy_from_literal(const char *literal, int len) {
    String str = {
        .len = len,
        .ref = false
    };

    str.data = malloc(str.len + 1);
    memcpy(str.data, literal, str.len);

    str.data[str.len] = '\0';


    return str;
}

String String_new(int len) {
    String new = {
        .len = len,
        .data = malloc(len + 1),
        .ref = false
    };

    new.data[len] = 0;

    return new;
}

String String_concat(StringRef a, StringRef b) {
    String new = String_new(a.len + b.len);

    memcpy(new.data, a.data, a.len);
    memcpy(new.data + a.len, b.data, b.len);

    return new;
}

String String_concatf(String a, String b) {
    String new = String_new(a.len + b.len);

    memcpy(new.data, a.data, a.len);
    memcpy(new.data + a.len, b.data, b.len);

    free(a.data);
    free(b.data);

    return new;
}

void String_append(String *a, StringRef other) {
    String temp = *a;
    *a = String_concat(*a, other);
    free(temp.data);
}

void String_delete(String *str) {
    if (str->ref) {
        printf("ERROR! Deleting a StringRef is not allowed! \n str: '%s'", str->data);
        return;
    }
    free(str->data);
    str->len = -1;
}

String String_from_double(double num, int accuracy) {
    int len = accuracy + get_num_digits((int)num);
    String str = String_new(len + 10); // for the dot and scientific notation (atleast tell me about it??)
    sprintf(str.data, "%.2f", num);
    printf("%s \n", str.data);
    return str;
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


bool String_equal(StringRef a, StringRef b) {
    return strncmp(a.data, b.data, a.len) == 0;
}

StringRef String_slice(StringRef a, int start, int end) {
    if (start > end) {
        printf("Couldn't slice string! Start larger than end! \n");
        return (StringRef){.data = NULL, .len = -1};
    }
    StringRef slice = (StringRef) {
        .data = a.data + start,
        .len = end - start,
        .ref = true
    };

    return slice;
}

String String_cslice(StringRef a, int start, int end) {
    
    if (start > end) {
        printf("Couldn't cslice string! Start larger than end! \n");
        return (StringRef){.data = NULL, .len = -1};
    }
    
    String new = String_new(end - start);

    memcpy(new.data, a.data + start, a.len);

    return new;
}

String String_copy(String original) {
    return String_cslice(original, 0, original.len);
}

bool String_ends_with(StringRef a, StringRef b) {
    if (b.len > a.len) return false;

    return String_equal(String_slice(a, a.len - b.len, a.len), b);
}

bool String_starts_with(StringRef a, StringRef b) {
    if (b.len > a.len) return false;

    return strncmp(a.data, b.data, b.len) == 0;
}

StringRef *String_split(StringRef a, char splitter) {
    int i = 0, word_start = 0;
    StringRef *res = array(StringRef, 5);

    while (i < a.len) {
        if (a.data[i] == splitter) {
            StringRef new = String_slice(a, word_start, i);
            array_append(res, new);
            word_start = i + 1;
        }
        i++;
    }
    StringRef last = String_slice(a, word_start, i);
    array_append(res, last);


    return res;
}



#endif