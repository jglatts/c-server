/*
  	Simple HTTP-TCP server for Windows platforms

	Author: John Glatts
	Compiler: Microsoft (R) C/C++ Optimizing Compiler Version 19.23.28107 for x86

	How to Complile and Run:
		1. Open a terminal in windows and navigate to where this cpp file is saved.
		2. The Microsoft C/C++ compiler must be installed to compile and run.
		3. Type "cl server.cpp" into the terminal prompt to compile the source.
		4. Then type "server" to run the program.
*/
#include <stdio.h>
#include <winsock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#include <mstcpip.h>

// Winsock Library
#pragma comment(lib,"ws2_32.lib") 

void init_server(SOCKET*);
void server_handle_client(SOCKET);
void send_http_response(SOCKET, char*);

int main(int argc, char* argv[]) {
	WSADATA wsa;
	SOCKET server_socket;

	// WSAStartup() must be called first for any windows-socket program
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("Failed. Error Code : %d\n", WSAGetLastError());
		return 1;
	}

	init_server(&server_socket);
	server_handle_client(server_socket);
	WSACleanup();

	return 0;
}

void init_server(SOCKET* server_socket) {
	struct sockaddr_in server;
	char ip[500]; 

	if ((*server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		printf("Could not create socket : %d\n", WSAGetLastError());
		exit(1);
	}

	server.sin_family = AF_INET;			// TCP socket
	server.sin_addr.s_addr = INADDR_ANY;		// default source address
	server.sin_port = htons(8888);			// port #8888

	if (inet_ntop(AF_INET, (PVOID)&((PSOCKADDR_IN)&server)->sin_addr, ip, 100)) {
		printf("Server Socket Created\nServer IP: %s (localhost)\n", ip);
	}

	if (bind(*server_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
		printf("Bind failed with error code : %d\n", WSAGetLastError());
	}

	listen(*server_socket, 3);
}

void server_handle_client(SOCKET server_socket) {
	SOCKET client_socket;
	struct sockaddr_in client;
	char ip[500];
	int c = sizeof(struct sockaddr_in);

	puts("\nWaiting for incoming connections...");
	client_socket = accept(server_socket, (struct sockaddr*)&client, &c);
	if (client_socket == INVALID_SOCKET) {
		printf("accept() failed with error : %d", WSAGetLastError());
	}

	puts("Connection accepted");
	if (inet_ntop(AF_INET, (PVOID)&((PSOCKADDR_IN)&client)->sin_addr, ip, 100)) {
		printf("Client IP: %s\n", ip);
	}

	send_http_response(client_socket, ip);
	closesocket(server_socket);
}

void send_http_response(SOCKET client, char* client_ip) {
	SYSTEMTIME t;     
	GetLocalTime(&t); 
	char buf[300];
	// add the HTTP header content
	char reply[3000] =
		"HTTP/1.1 200 OK\n"
		"Server: JDG Server\n"
		"Content-Type: text/html\n"
		"Content-Length: 3000\n"
		"Accept-Ranges: bytes\n"
		"Connection: keep-alive\n"
		"\n";	
	
	// add the HTTP data to be rendered by the browser
	snprintf(buf, 100, "<title>JDG HTTP Server</title>\n");
	strcat_s(reply, buf);
	snprintf(buf, 100, "Your IP Address: %s\n", client_ip);
	strcat_s(reply, buf);
	snprintf(buf, 300, "</br></br>\nIt Is Now: %d-%d-%d %d:%d:%d\n", 
			t.wMonth, t.wDay, t.wYear, t.wHour, t.wMinute, t.wSecond);
	strcat_s(reply, buf);
	printf("Sending:\n%s\n", reply);
	send(client, reply, strlen(reply), 0);
	// keep connection open 
	while (1) {}
}
