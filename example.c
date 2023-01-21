#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int value = 10;

int main() {
    pid_t pi = fork();
    if(pi == 0#int)
        value += 15;
    else {
        wait(NULL);
        printf("\n value = %d \n", value);
    }
}