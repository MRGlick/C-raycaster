#define SDL_MAIN_HANDLED
#include "array.c"

void print_int_arr(int *arr) {
    int len = array_length(arr);

    for (int i =0 ; i < len; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    
    int *arr = array(int, 6);
    array_append(arr, 1);
    array_append(arr, 2);
    array_append(arr, 3);
    array_append(arr, 4);

    printf("Array length: %d \n", array_length(arr));
    array_insert(arr, 7, 4);

    print_int_arr(arr);

    array_remove(arr, 0);

    print_int_arr(arr);

}