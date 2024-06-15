#include <stdio.h>
#include <stdlib.h>

#define new_array(type, capacity) (type *)create_array(sizeof(type), capacity)
#define array_header(arr) (ArrayHeader *)arr - 1

typedef struct ArrayHeader {
    int length;
    int size;
    int item_size;
} ArrayHeader;


void *create_array(int item_size, int capacity) {
    int mem_size = item_size * capacity + sizeof(ArrayHeader);
    ArrayHeader *header = malloc(mem_size);
    header->size = capacity;
    header->length = 0;
    header->item_size = item_size;

    return header + 1;
}

void array_ensure_capacity(void *arr, int item_count, int item_size) {
    
}

int array_len(void *arr) {
    ArrayHeader *ptr = array_header(arr);

    return ptr->length;
}

void array_append(void *arr) {
    ArrayHeader *header = array_header(arr);
}

int main(int argc, char *argv[]) {
    int a = 5;
    int b = 3;
    printf("Hello, world! b: %d \n", (b = a, b));

}

