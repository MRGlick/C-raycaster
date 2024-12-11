#include <stdio.h>
#include "mystring.c"
#include <stdint.h>

#define commit_sudoku() *(int *)0 = 42


int get_digit(int num, int idx) {

    int i = 0;
    int n = num;

    while (i < idx && n != 0) { // skip unnecessary iterations
        n /= 10;
        i++;
    }

    return n % 10;
}

String scramble_ip_and_port(String ip, String port) {
    String result = String_new(12 + port.len); // 3 digits for each part of ip, port can be 4 or 5 digit

    StringRef *ip_parts = String_split(ip, '.');

    if (array_length(ip_parts) != 4) {
        printf("Invalid IP \n");
        commit_sudoku();
    }

    uint8_t ip_bytes[4] = {0};

    for (int i = 0; i < 4; i++) {
        if (ip_parts[i].len > 3) {
            printf("Invalid IP \n");
            commit_sudoku();
        }
        ip_bytes[i] = String_to_int(ip_parts[i]);
    }

    array_free(ip_parts);

    for (int i = 0; i < result.len; i++) {
        if ((i + 1) % 4 == 0) {
            int pos = (i + 1) / 4 - 1;
            result.data[i] = port.data[pos];
            continue;
        }

        int ip_section = (i + 1) / 4;
        int ip_idx = (i + 1) % 4 - 1; // it works shutup

        result.data[i] = '0' + get_digit(ip_bytes[ip_section], 2 - ip_idx);

    }

    for (int i = 0; i < result.len; i++) { // caeser iteration
        result.data[i] = '0' + (((result.data[i] - '0') + (i)) % 10);
    }

    for (int i = 0; i < result.len / 2; i++) { // reverse iteration
        int temp = result.data[i];
        result.data[i] = result.data[result.len - 1 - i];
        result.data[result.len - 1 - i] = temp;
    }

    return result;
}

// b > 1 !!!
int posmod(int a, int b) {
    return (a + (b * abs(a))) % b;
}

// the string's length cant be determined so the string is being returned
String unscramble_ip_and_port(StringRef scrambled_ip, int *port) {

    if (scrambled_ip.len < 12 || scrambled_ip.len > 17) {
        printf("Invalid code \n");
        return String_null;
    }

    String copy = String_copy(scrambled_ip);

    for (int i = 0; i < copy.len / 2; i++) { // unreverse

        if (copy.data[i] < '0' || copy.data[i] > '9') {
            printf("Invalid code \n");
            return String_null;
        }

        int temp = copy.data[i];
        copy.data[i] = copy.data[copy.len - 1 - i];
        copy.data[copy.len - 1 - i] = temp;
    }

    for (int i = 0; i < copy.len; i++) { // uncaeser
        copy.data[i] = '0' + posmod((copy.data[i]- '0') - i, 10);
    }

    int port_len = scrambled_ip.len / 4;
    int ip_len = scrambled_ip.len - port_len;

    String ip_string = String_new(ip_len + 3);
    int ip_idx = 0;
    int port_result = 0;
    int dot_count = 0;

    for (int i = 0; i < copy.len; i++) {
        if ((i + 1) % 4 == 0) {
            port_result = port_result * 10 + (copy.data[i] - '0');
            
            continue;
        }
        ip_string.data[ip_idx++] = copy.data[i];
        if ((ip_idx + 1) % 4 == 0 && dot_count < 3) {
            ip_string.data[ip_idx++] = '.';
            dot_count++;
        }
        
    }
    if (port != NULL) {
        *port = port_result;
    }

    int *arr = array(int, 5);
    bool trailing = true;
    for (int i = 0; i < ip_string.len - 1; i++) {
        if (trailing) {
            if (ip_string.data[i] == '0' && ip_string.data[i + 1] != '.') {
                array_append(arr, i);
            } else {
                trailing = false;
            }
        }

        if (ip_string.data[i] == '.') trailing = true;

    }

    String final_ip_string = String_new(ip_string.len - array_length(arr));
    int idx = 0;
    for (int i = 0; i < ip_string.len; i++) {
        
        bool in_arr = false;
        for (int j = 0; j < array_length(arr); j++) {
            if (arr[j] == i) {
                in_arr = true;
                break;
            }
        }

        if (in_arr) continue;

        final_ip_string.data[idx++] = ip_string.data[i];

    }

    String_delete(&ip_string);
    array_free(arr);

    return final_ip_string;
}


int main() {

    String result = StringRef("0123675881444690");

    printf("%s \n", result.data);

    int original_port = 0;

    String original_ip = unscramble_ip_and_port(result, &original_port);

    printf("og ip: %s \n", original_ip.data);
    printf("og port: %d \n", original_port);

}