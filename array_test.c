#define SDL_MAIN_HANDLED
#include "array.c"

int main(int argc, char *argv[]) {
    
    int *arr = array(int, 6);
    array_append(arr, 1);
    array_append(arr, 2);
    array_append(arr, 3);
    array_append(arr, 4);

    array_insert(arr, 7, 0);

    int a = 5, b = 3;

    _swap(&a, &b, sizeof(int));

    printf("a: %d, b: %d \n", a, b);


    for (int i = 0; i < array_length(arr); i++) {
        printf("%d \n", arr[i]);
    }

}