/*
	Simple HTTP-TCP server for Windows platforms
	Reads the HTTP headers sent from the client and will write it to a log file

	Author: John Glatts
	Compiler: Microsoft (R) C/C++ Optimizing Compiler Version 19.23.28107 for x86

	****** Visual Studio Must Be Installed to Compile and Run ******

	How to Complile and Run:
		1. Save this program in the directory where your Visual Studio Repos are saved. 
		1. Open a terminal in windows and navigate to where your visual studio projects are saved.
		3. Type "cl windows_http_server.cpp" into the terminal prompt to compile the source.
		4. Then type "windows_http_server.cpp" to run the program and follow the prompts. 
*/
#include <stdio.h>
#include <winsock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#include <mstcpip.h>

#define MAX_BUF_LEN 1024

// Winsock Library
#pragma comment(lib, "ws2_32.lib") 

static int  get_port();
static void init_server(SOCKET*, int);
static void server_handle_client(SOCKET, FILE*);
static void send_http_response(SOCKET, char*);

int main(int argc, char* argv[]) {
	WSADATA wsa;
	SOCKET server_socket;
	FILE* log;

	// WSAStartup() must be called first for any windows-socket program
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("Failed. Error Code : %d\n", WSAGetLastError());
		return 1;
	}

	fopen_s(&log, "lab_6_log.txt", "w");
	init_server(&server_socket, get_port());
	server_handle_client(server_socket, log);
	WSACleanup();

	return 0;
}

static int get_port() {
	int port = 8888;
	printf("Enter Port Number: ");
	scanf_s("%d", &port);
	return port;
}

static void init_server(SOCKET* server_socket, int port) {
	struct sockaddr_in server;
	char ip[500];

	// TCP socket
	if ((*server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		printf("Could not create socket : %d\n", WSAGetLastError());
		exit(1);
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;	// default source address
	server.sin_port = htons(port);			// port supplied by user

	if (inet_ntop(AF_INET, (PVOID) & ((PSOCKADDR_IN)&server)->sin_addr, ip, 100)) {
		printf("Server Socket Created\nServer IP: %s (localhost)\nPort: %d\n", ip, port);
	}

	if (bind(*server_socket, (struct sockaddr*) & server, sizeof(server)) == SOCKET_ERROR) {
		printf("Bind failed with error code : %d\n", WSAGetLastError());
	}

	listen(*server_socket, 3);
}

static void server_handle_client(SOCKET server_socket, FILE* log) {
	SOCKET client_socket;
	struct sockaddr_in client;
	char ip[500];
	char recv_buf[MAX_BUF_LEN];
	int c = sizeof(struct sockaddr_in);

	puts("\nWaiting for incoming connections...");
	client_socket = accept(server_socket, (struct sockaddr*) & client, &c);
	if (client_socket == INVALID_SOCKET) {
		printf("accept() failed with error : %d", WSAGetLastError());
	}

	puts("Connection accepted");
	if (inet_ntop(AF_INET, (PVOID) & ((PSOCKADDR_IN)&client)->sin_addr, ip, 100)) {
		printf("Client IP: %s\n", ip);
	}

	// get packet from client
	int bytes_read = recv(client_socket, recv_buf, MAX_BUF_LEN, 0);
	if (bytes_read > MAX_BUF_LEN) {
		puts("Error - Buffer to small for HTTP packet sent from client, please increase size");
		exit(1);
	}

	recv_buf[bytes_read] = '\0'; // NULL terminate!!
	printf("Read: %d bytes\nGot: \n%s\n", bytes_read, recv_buf);
	fprintf(log, recv_buf);
	fclose(log);
	send_http_response(client_socket, ip);
	closesocket(server_socket);
}

static void send_http_response(SOCKET client, char* client_ip) {
	SYSTEMTIME t;
	GetLocalTime(&t);
	char buf[300];
	// HTTP header to send to client
	char reply[1024] =
		"HTTP/1.1 200 OK\n"
		"Server: JDG Server\n"
		"Content-Type: text/html\n"
		"Content-Length: 3000\n"
		"Accept-Ranges: bytes\n"
		"Connection: keep-alive\n"
		"\n";

	// HTTP data to send to client
	snprintf(buf, 100, "<title>JDG HTTP Server</title>\n");
	strcat_s(reply, buf);
	snprintf(buf, 150, "<img src=\"https://i.ebayimg.com/images/g/3JsAAOSwL7ValxM6/s-l300.jpg\">\n");
	strcat_s(reply, buf);
	snprintf(buf, 100, "<h2 style=\"color:red;\">Your IP Address: %s</h2>\n", client_ip);
	strcat_s(reply, buf);
	snprintf(buf, 300, "<h2 style=\"color:red\">It Is Now: %d-%d-%d %d:%d:%d</h2>\n",
		t.wMonth, t.wDay, t.wYear, t.wHour, t.wMinute, t.wSecond);
	strcat_s(reply, buf);
	send(client, reply, strlen(reply), 0);
	// wait until client disconnects
	while (recv(client, buf, 300, 0) > 0) { }
}
