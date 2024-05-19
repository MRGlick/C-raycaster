#include <stdio.h>
#include <stdlib.h>

typedef struct SortObject {
    void *val;
    double num;
} SortObject;

SortObject *merge(SortObject *arr1, SortObject *arr2, int l1, int l2) {
    SortObject *res = malloc(sizeof(SortObject) * (l1 + l2));
    int arr1Idx = 0;
    int arr2Idx = 0;
    int i = 0;
    while (arr1Idx < l1 && arr2Idx < l2) {
        if (arr1[arr1Idx].num < arr2[arr2Idx].num) {
            res[i] = arr1[arr1Idx];
            arr1Idx++;
        } else {
            res[i] = arr2[arr2Idx];
            arr2Idx++;
        }
        i++;
    }
    while (arr1Idx < l1) {
        res[i] = arr1[arr1Idx];
        arr1Idx++;
        i++;
    }
    while (arr2Idx < l2) {
        res[i] = arr2[arr2Idx];
        arr2Idx++;
        i++;
    }

    return res;
}

SortObject *mergeSort(SortObject arr[], int arrlen) {
    if (arrlen == 1) {
        SortObject *res = malloc(sizeof(SortObject));
        res[0] = arr[0];
        return res;
    }
    int l1 = arrlen / 2;
    int l2 = arrlen % 2 == 0 ? arrlen / 2 : arrlen / 2 + 1;
    SortObject a1[l1];
    SortObject a2[l2];
    for (int i = 0; i < arrlen; i++) {
        if (i < l1) {
            a1[i] = arr[i];
        } else {
            a2[i - l1] = arr[i];
        }
    }

    SortObject *a = mergeSort(a1, l1);
    SortObject *b = mergeSort(a2, l2);
    
    SortObject *sorted = merge(a, b, l1, l2);

    free(a);
    free(b);


    return sorted;

}

void printArr(int *arr, int l) {
    for (int i = 0; i < l; i++) {
        printf("%d, ", arr[i]);
    }
}


int main() {
    int arr[] = {9, 2, 1, 3, 6, 5, 8, 7, 9};
    int l = sizeof(arr) / sizeof(int);
    SortObject objs[l];

    for (int i = 0; i < l; i++) {
        objs[i].num = arr[i];
        objs[i].val = arr + i; // the pointer yknow
    }
    SortObject *sortedObjs = mergeSort(objs, l);

    int sorted[l];
    for (int i = 0; i < l; i++) {
        sorted[i] = sortedObjs[i].num;
    }

    printArr(sorted, l);
}