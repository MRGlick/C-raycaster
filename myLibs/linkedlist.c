
#include <stdlib.h>
#include <stdio.h>

typedef struct Node {
    void *val;
    int type;
    struct Node *next;
} Node;

typedef struct LinkedList {
    Node *nodes;
    int length;
} LinkedList;


LinkedList *createLinkedList() {
    LinkedList *list = malloc(sizeof(LinkedList));
    list->length = 0;
    list->nodes = NULL;
    return list;
}

void llAppend(LinkedList *list, void *val, int type) {
    
    
    Node *new = malloc(sizeof(Node));
    new->val = val;
    new->type = type;        
    
    if (list->nodes == NULL) {
        list->nodes = new;
        list->length++;
        return;
    }

    Node *current = list->nodes;

    while (current->next != NULL) current = current->next;

    current->next = new;
    list->length++;
}

Node *llGet(LinkedList *list, int idx) {
    int i = 0;
    Node *current = list->nodes;
    while (current->next != NULL) {
        if (i == idx) return current->next;
        i++;
        current = current->next;
    }

    return NULL;
}

Node **llToArray(LinkedList *list) {

    if (list->nodes == NULL || list->length == 0) return NULL;

    Node **arr = malloc(sizeof(Node *) * list->length);
    Node *current = list->nodes;
    int i = 0;
    while (current != NULL) {
        printf("%p \n", current);
        arr[i] = current->next;
        current = current->next;
        i++;
    }
    return arr;
}

void llAddHead(LinkedList *list, void *val, int type) {
    Node *newHead = malloc(sizeof(Node));
    newHead->val = val;
    newHead->type = type;
    newHead->next = list->nodes->next;
    list->nodes->next = newHead;
    list->length++;
}

void llRemoveHead(LinkedList *list) {
    if (list->nodes->next == NULL) return;

    list->nodes->next = list->nodes->next->next;
    list->length--;
}

Node *llFind(LinkedList *list, void *val) {
    Node *current = list->nodes;
    while (current != NULL) {
        if (current->val == val) {
            return current;
        }
    }

    return NULL;
}

void freeNode(Node *node) {
    free(node->val);
    free(node);
}

void llRemove(LinkedList *list, int idx) {
    int i = 0;
    Node *current = list->nodes->next;
    Node *prev = list->nodes;
    while (current != NULL) {
        if (i == idx) {
            prev->next = current->next;
            free(current);
            list->length--;
            return;
        }
        prev = current;
        current = current->next;
        i++;
    }
}