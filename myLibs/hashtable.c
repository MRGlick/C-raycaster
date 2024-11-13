#ifndef HASH_TABLE_C
#define HASH_TABLE_C

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>   
#include "array.c"


typedef struct HashNode {
    const void *key;
    int key_size;
    void *val;
    struct HashNode *next;
} HashNode;

typedef struct HashTable {

    void **keys;
    int key_size;
    int value_size;

    int capacity;

    HashNode *values;

} HashTable;

#define HashTable(keytype, valtype) _HashTable_new(sizeof(keytype), sizeof(valtype))

int hash(char *val, int val_size, int arr_size);

bool _HashTable_is_hashnode_empty(HashNode node) {

    if (node.key == NULL && node.key_size == 0 && node.next == NULL && node.val == NULL) return true;

    return false;
}

HashTable _HashTable_new(int key_size, int value_size) {
    HashTable table = {0};
    table.key_size = key_size;
    table.value_size = value_size;
    table.capacity = 100;
    table.keys = array(void *, table.capacity);
    table.values = calloc(sizeof(HashNode), table.capacity);
    for (int i = 0; i < table.capacity; i++) {
        table.keys[i] = NULL;
    }

    return table;
}

void HashTable_put(HashTable *table, void *key, void *value) {

    bool found = false;
    for (int i = 0; i < array_length(table->keys); i++) {
        if (table->keys[i] == key) {
            found = true;
            break;
        }
    }
    if (!found) {
        array_append(table->keys, key);
    }

    int hash_value = hash(key, table->key_size, table->capacity);
    HashNode *available = &table->values[hash_value];
    while (available->val != NULL) {
        available = available->next;
    }
    available->key = key;
    available->key_size = table->key_size;
    available->val = value;
}

void *HashTable_get(HashTable *table, void *key) {
    int hash_value = hash(key, table->key_size, table->capacity);
    HashNode *current = &table->values[hash_value];
    if (_HashTable_is_hashnode_empty(*current)) return NULL;

    while (strncmp(current->key, key, table->key_size) != 0) {
        current = current->next;
    }
    return current;
}


void HashTable_delete(HashTable table) {
    array_free(table.keys);
    array_free(table.values);
}


int hash(char *val, int val_size, int arr_size) {

    // Thanks Dan

    unsigned long hash = 5381;
    int c;

    for (int i = 0; i < val_size; i++) {
        c = val[i];
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash % arr_size;
}

void test_hash() {

    int comb_len;
    printf("Enter combination length: ");
    scanf("%d", &comb_len);

    char *combination = malloc(comb_len);
    for (int i = 0; i < comb_len; i++) {
        combination[i] = 'a';
    }
    int arr_size = 20;

    printf("Enter test hashmap size: ");

    scanf("%d", &arr_size);

    int *counter_arr = calloc(sizeof(int), arr_size);

    while (combination[comb_len - 1] < 'z') {
        counter_arr[hash(combination, comb_len, arr_size)]++;

        combination[0]++;
        for (int i = 0; i < comb_len - 1; i++) {
            if (combination[i] > 'z') {
                combination[i] = 'a';
                combination[i + 1]++;
            }
        }
    }

    for (int i = 0; i < arr_size; i++) {
        if (counter_arr[i] > 0) {
            printf("I: %d, Collisions found: %d \n", i, counter_arr[i]);
        }
    }

    free(counter_arr);
    free(combination);
}


int main(int argc, char *argv[]) {

    HashTable t = HashTable(const char *, int);

    int num_values = 0;
    printf("Enter number of entries: ");
    scanf("%d", &num_values);

    for (int i = 0; i < num_values; i++) {
        char *key = calloc(1, 1024);
        int value = 0;

        printf("Enter key: ");
        scanf("%s", key);
        printf("Enter value for key: ");
        scanf("%d", &value);
        HashTable_put(&t, key, &value);
    }


    for (int i = 0; i < array_length(t.keys); i++) {
        printf("Checking key: %s \n", t.keys[i]);
        printf("Value: %d \n", HashTable_get(&t, t.keys[i]));
    }
}


















#endif