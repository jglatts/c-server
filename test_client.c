/*
	Use a multi-threaded client to test
		- threads will send data to the server at same time
		- should be able to handle no problem
*/
#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <pthread.h> 

#define PORT 8888 

// have 10 clients that will open a connection
// with server and start sending words
pthread_t client_workers[10];	
int num_tokens;

void count_tokens(char* buff) {
	char* token = NULL;
	token = strtok(buff, " ");
	num_tokens = 0;
	while (token != NULL) {
		num_tokens++;
		token = strtok(NULL, " ");
	}
}

int main(int argc, char const* argv[]) {
	int sock = 0;
	struct sockaddr_in serv_addr;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}
	if (connect(sock, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) < 0) {
		printf("\nConnection Failed \n");
		return -1;
	}
	while (1) {
		char* buff = NULL;
		char* copy = NULL;
		size_t len = 0;
		printf("\nEnter a Message: ");
		getline(&buff, &len, stdin);
		copy = (char*)malloc(len * sizeof(char));
		strcpy(copy, buff);
		count_tokens(copy);
		send(sock, buff, strlen(buff), 0);
		// use another while loop here
		// and increment a counter until it reaches same value as num_tokens
		// this way we know we recived the correct amount of words back from server
		int i;
		for (i = 0; i < num_tokens; ++i) {
			char buf[1024] = { 0 };
			if (read(sock, buf, 1024) == 0) exit(1);	
			printf("%s\n", buf);
		}
	}
	return 0;
}
