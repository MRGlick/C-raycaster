#include <stdio.h>
#include <stdlib.h>
#include <string.h> //memmove
#include <stdbool.h>

typedef struct obj {
    void *val;
    int type;

} obj;


typedef struct arraylist {
    obj **objects;
    size_t size, length;
} arraylist;

arraylist *create_arraylist(size_t size) {
    arraylist *list = malloc(sizeof(arraylist));
    if (list == NULL) {
        return NULL;
    }
    list->size = size;
    list->length = 0;
    list->objects = malloc(sizeof(obj *) * size);
    return list;
}


void extend_list(arraylist *list) {
    obj **newvals = malloc(sizeof(obj *) * list->size * 2);
    if (newvals == NULL) {
        printf("Couldn't allocate memory. Returning \n");
        return;
    }
    for (int i = 0; i < list->size; i++) {
        newvals[i] = list->objects[i];
    }
    free(list->objects);
    list->objects = newvals;
    list->size *= 2;
}

void arraylist_add(arraylist *list, void *val, int type) {
    obj *object = malloc(sizeof(obj));
    object->val = val;
    object->type = type;

    if (list->length + 1 >= list->size) {
        extend_list(list);
    }
    list->objects[list->length] = object;
    list->length++;
}

// 0 1 2 3 N

void arraylist_insert(arraylist *list, void *val, int type, int idx) {
    if (idx >= list->length || idx < 0) return;

    if (idx == list->length - 1) {
        arraylist_add(list, val, type);
        return;
    }

    if (list->length + 1 >= list->size) extend_list(list);

    list->length++;

    memmove(list->objects + idx + 1, list->objects + idx, sizeof(obj *) * (list->length - 1 - idx));

    obj *object = malloc(sizeof(obj));
    object->type = type;
    object->val = val;

    list->objects[idx] = object;

}

int arraylist_find(arraylist *list, void *val) {
    for (int i = 0; i < list->length; i++) if (list->objects[i]->val == val) return i;
    return -1;
}

void free_object(obj *object) {
    if (object == NULL) return;
    free(object->val);
    free(object);
}
// doesnt free automatically
int arraylist_remove(arraylist *list, int idx) { 
    if (idx < 0 || idx >= list->length) {
        printf("Index out of bounds while trying to remove. index: %d \n", idx);
        return 0;
    }
    memmove(list->objects + idx, list->objects + idx + 1, sizeof(obj *) * list->length - idx);
    list->length--;
    return 1;
}

void arraylist_clear(arraylist *list) {
    for (int i = list->length - 1; i >= 0; i--) {
        arraylist_remove(list, i);
    }
}

void arraylist_free(arraylist *list) {
    arraylist_clear(list);
    free(list);
}

obj *arraylist_get(arraylist *list, int idx) {
    if (idx < 0 || idx >= list->length) return NULL;
    return list->objects[idx];
}

bool arraylist_contains(arraylist *list, void *val) {

    for (int i = 0; i < list->length; i++) {
        if (arraylist_get(list, i)->val == val) return true;
    }

    return false;
}