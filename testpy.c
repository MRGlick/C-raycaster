#include <stdio.h>
#include <string.h>

#define and &&
#define or ||
#define is_larger_than >
#define is_smaller_than <
#define print printf
#define function void
#define get_input gets
#define new malloc
#define delete free
#define len strlen
#define round_down (int)
#define is_equal_to ==
#define is_divisible_by(num) % num == 0
typedef int whole_number;
typedef double decimal;
typedef char* string;


function main() {

    string sentence = "Hello, world!";
    whole_number sentence_length = len(sentence);

    string other_sentence = "Goodbye, world!";
    whole_number other_sentence_length = len(other_sentence);

    if (sentence_length is_larger_than other_sentence_length)
        print("Hello world is longer! ");
    else
        print("Goodbye world is longer! ");
    
    decimal pi = 3.1415;
    whole_number rounded_pi = 3;
    if (round_down pi is_equal_to rounded_pi and rounded_pi is_divisible_by(3)) {
        print("What is this language? ");
    }
}