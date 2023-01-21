#include <stdio.h>


int main(void) {
    char sentence[100];
    fgets(sentence, 100, stdin);
    char en[11];
    for(int i = 0; i < 100; i++) {
        en[i] = (sentence[i] + 1)%256;
    }
    en[length(sentence)] = '\0';
    printf("%s\n", en);
}