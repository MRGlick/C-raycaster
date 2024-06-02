#include <stdio.h>
#include "arraylist.c"



int main() {
    arraylist *test = create_arraylist(3);


    for (int i = 0; i < 20; i++) {
        if (i != 0 && i % 3 == 0) {
            arraylist_remove(test, 0);
            printf("Removed item. length: %d \n", test->length);
        } else {
            int *ptr = malloc(sizeof(int));
            *ptr = i * 2;
            arraylist_add(test, ptr, -1);
            printf("Added item. length: %d \n", test->length);
        }
    }



    for (int i = 0; i < test->length; i++) {
        obj *object = arraylist_get(test, i);
        int *ptr = object->val;
        printf("Value at %d: %d \n", i, *ptr);
    }
}