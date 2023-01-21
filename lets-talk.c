// Assignment 3
// This assignment is for using threads and UDP, two terminals can chat / talk to each other
// Hanra Jeong
// Jooyoung Julia Lee
// Date : due 11th July

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>

// Import the given linked list file
#include "list.h"
#include "list.c"
// 8. Require support for very long lines (up to 4k characters)
#define Max_Buf 4001
#define Key 17
int my_socket;
int user_status = false; // false means offline, true means online

// Related resources : https://w3.cs.jmu.edu/lam2mo/cs470_2018_01/files/04_conditions.pdf

// List 1 : sending message
pthread_cond_t send_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;
bool send_message = false;

// List 2 : receiving message
pthread_cond_t recieve_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t recieve_mutex = PTHREAD_MUTEX_INITIALIZER;
bool recieve_message = false;

struct sockaddr_in rec_add;
// https://en.wikibooks.org/wiki/C_Programming/POSIX_Reference/netdb.h/getaddrinfo
struct addrinfo hints, *server_info;

void *keyborad_input(void *ptr);
void *sender(void *ptr);
void *reciever(void *ptr);
void *printer(void *ptr);
// void* encrypt(char* message);
// void* decrypt(char* message);


int main(int argc, char *argv[]) {
    // lets-talk [my port number] [remote/local machine IP] [remote/local port number]
    // take 3 parameters to start.
    // 1. The IP address of a remote/local client.
    // 2. Port number on which remote/local process is running.
    // 3. Port number your process will listen on for incoming UDP packets.
    if (argc != 4) {
		printf("Usage:\"n");
        printf("\t./lets-talk <local port> <remote host> <remote port>\n");
		printf("Example:\n");
        printf("\t./lets-talk 3000 192.168.0.513 3001\n");
		printf("\t./lets-talk 3000 some-computer-name 3001\n");
        return 0;
	}
    char* local_port = argv[1];
    char* remote_host = argv[2];
    char* remote_port = argv[3];

    // Use the list to communicate between threads.
    // ● Use the list.c and list.h (added already in your repo) to use mutex so that the
    // list should be accessible by only one thread at a time.
    List* list1 = List_create();
    list1->count = 0;
    List* list2 = List_create();
    list2->count = 0;

    printf("Welcome to LETS-Talk! Please type your messages now.\n");

    my_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(my_socket == -1) {
        exit(1);
    }
    // local
    // https://www.tutorialspoint.com/c_standard_library/c_function_memset.htm
    // https://www.cs.cmu.edu/~srini/15-441/S10/lectures/r01-sockets.pdf
    memset(&rec_add, 0, sizeof(rec_add));
    rec_add.sin_family = AF_INET;
    rec_add.sin_addr.s_addr = (INADDR_ANY);
	rec_add.sin_port = htons(atoi(local_port));
    // https://man7.org/linux/man-pages/man2/bind.2.html
    if (bind(my_socket, (const struct sockaddr*)&rec_add, sizeof(rec_add)) < 0) 
    {
        exit(1);
    }
    // https://docs.microsoft.com/en-us/windows/win32/api/ws2def/ns-ws2def-addrinfoa
    memset(&hints, 0, sizeof(hints));
    // The address family is unspecified.
    hints.ai_family = AF_UNSPEC;
    // Supports datagrams, which are connectionless, unreliable buffers of a fixed (typically small) maximum length. Uses the User Datagram Protocol (UDP) for the Internet address family (AF_INET or AF_INET6).
    hints.ai_socktype = SOCK_DGRAM;
    // https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
    // https://docs.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo
    // https://www.freebsd.org/cgi/man.cgi?query=getaddrinfo&apropos=0&sektion=0&manpath=FreeBSD+4.8-RELEASE&format=html
    // int getaddrinfo(const char *nodename, const char *servname, const struct addrinfo *hints, struct addrinfo **res);
    if (getaddrinfo(remote_host, remote_port, &hints, &server_info) != 0) {
        exit(1);
    }

    // All four threads will share access to a list ADT..
    pthread_t thr1, thr2, thr3, thr4;
    // The keyboard input thread, on receipt of the input, adds the input to the list of 
    // messages that need to be sent to the remote lets-talk client.
    pthread_create(&thr1, NULL, keyborad_input, list1);
    // The UDP sender thread will take each message off this list and send it over the 
    // network to the remote client.
    pthread_create(&thr2, NULL, sender, list1);
    // The UDP receiver thread, on receipt of input from the remote lets-talk client, will put 
    // the message onto the list of messages that need to be printed to the local screen.
    pthread_create(&thr3, NULL, reciever, list2);
    // The console output thread will take each message off this list and output it to the 
    // screen.
    pthread_create(&thr4, NULL, printer, list2);

    // 3. The main thread will wait until any of them gives a termination signal, as soon as the
    // main thread gets a termination signal it cancels all threads one by one.
    // Rather than deal with a termination signal,
    // I used the exit() directly for each condition
    
    // https://man7.org/linux/man-pages/man3/pthread_join.3.html
    void* retval;
    pthread_join(thr1, &retval);
    pthread_join(thr2, &retval);
    pthread_join(thr3, &retval);
    pthread_join(thr4, &retval);

    close(my_socket);
    List_free(list1, free);
    List_free(list2, free);
    return 0;
}

// Required 4 threads, in each of the processes:
// 1. The keyboard input thread, on receipt of the input, 
// adds the input to the list of messages that need to be sent to the remote lets-talk client.
void *keyborad_input(void *ptr) {
    // printf("keyboard\n");
    fflush(stdout);
    // Allocate the memory for input message
    // char* message = malloc(sizeof(char) * Max_Buf);
    // With the requirement : Require support for very long lines (up to 4k characters)
    // The default value Max_Buf 4k, 
    char message[Max_Buf];
    // This is for checking whether the input value is !exit or not
    // Because the request asked us to quit the connection when "!exit" is given
    // If there is input and it doesn't ask you to exit, 
    while(fgets(message, Max_Buf, stdin)!=NULL) {
        
        char encrypted[Max_Buf];
        for (int i = 0; i < strlen(message); i++) {
            encrypted[i] = (message[i] + Key) % 256;
        }

        int list_append_check = List_append((List*) ptr, encrypted);
        if (list_append_check == -1) {
            // printf("Fail to append the input message to the list. Try again, please.\n");
            exit(1);
        }
        send_message = true;
        pthread_cond_signal(&send_cond);
        // }
    }
}

// 2. The UDP sender thread will take each message off this list and send it over the network
//  to the remote client.
void *sender(void *ptr) {
    // printf("sender\n");
    while(true) {
        // char* message = malloc(sizeof(char) * Max_Buf);
        // The other thread awaits a UDP datagram from another process.
        // Sending can be occured once there is input message
        // Thus, thread should wait for the input message
        pthread_mutex_lock(&send_mutex);
        // Wait untile there is message, meaning that once there is input message, send_message will turn into true
        while(send_message == false) {
            // int pthread_cond_wait(pthread_cond_t *__restrict__, pthread_mutex_t *__restrict__)
            pthread_cond_wait(&send_cond, &send_mutex);
        }
        pthread_mutex_unlock(&send_mutex);

        List_first((List*)ptr);
        char* message = List_remove((List*)ptr);
        // printf("sending: %s", message);
        // https://linux.die.net/man/2/sendto
        // https://pubs.opengroup.org/onlinepubs/7908799/xns/sendto.html
        // ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
        //       const struct sockaddr *dest_addr, socklen_t addrlen);
        size_t sending_result = sendto(my_socket, message, strlen(message), 0, server_info -> ai_addr, (server_info -> ai_addrlen));
        if(sending_result == -1) {
            // printf("Sending message failed.\n");
            user_status = false;
            exit(1);
        }
        else {
            user_status = true;
        }
        List_remove((List*)ptr);

        char decrpted[Max_Buf];
        for (int i = 0; i < strlen(message); i++) {
            decrpted[i] = (message[i] + (256 - Key)) % 256;
        }

        if(strcmp(decrpted, "!exit\n") ==0) {
            exit(0);
        }
        // 4. Checking the ‘online’ status of another terminal
        if(strcmp(decrpted, "!status\n") ==0) {
            if(user_status == false) {
                printf("Offline\n");
            }
            else {
                printf("Online\n");
            }
        }
        send_message = false;
        // free(message);
    }
}

// 3. The UDP receiver thread, on receipt of input from the remote lets-talk client, 
// will put the message onto the list of messages that need to be printed to the local screen.
void *reciever(void *ptr) {
    // printf("reciever\n");
    char message[Max_Buf];
    while(true) {
        // https://linux.die.net/man/2/recvfrom
        // ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
        //         struct sockaddr *src_addr, socklen_t *addrlen);
        size_t recieving_result = recvfrom(my_socket, message, Max_Buf -1 , 0, server_info -> ai_addr, &(server_info -> ai_addrlen));
        if(recieving_result == -1) {
            user_status = false;
            // printf("Recieving message failed.\n");
            exit(1);
        }
        else {
            user_status = true;
        }
        // message[recieving_result] = '\0';
        // Append the list 2, which is for recieving the message
        int list_append_check = List_append((List*) ptr, message);
        if(list_append_check == -1) {
            // printf("Appending for received message failed.\n");
            exit(1);
        }
        recieve_message = true;
        pthread_cond_signal(&recieve_cond);
    }
}

// 4. The console output thread will take each message off this list and output it to the screen.
void *printer(void *ptr) {
    // printf("printer\n");
    while(true) {
        // char* message = malloc(sizeof(char) * Max_Buf);
        // Thread should wait for receiving the message
        pthread_mutex_lock(&recieve_mutex);
        // Wait untile there is message, meaning that once there is recieved message, recieve_message will turn into true
        while(recieve_message == false) {
            // int pthread_cond_wait(pthread_cond_t *__restrict__, pthread_mutex_t *__restrict__)
            pthread_cond_wait(&recieve_cond, &recieve_mutex);
        }
        pthread_mutex_unlock(&recieve_mutex);   

        List_first((List*)ptr);
        char* message = List_remove((List*)ptr);
        // message = decrypt(message);

        char decrpted[Max_Buf];
        for (int i = 0; i < strlen(message); i++) {
            decrpted[i] = (message[i] + (256 - Key)) % 256;
        }
        if(strcmp(decrpted, "!status\n") == 0) {

        }
        else {
            printf("%s", decrpted);
        }
        if(strcmp(decrpted, "!exit\n") == 0) {
            exit(0);
        }
        recieve_message = false;
    }
}