/*
        CIS3207 LAB3

        author: John Glatts
*/
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Thread Data Types
pthread_t worker_pool[MAX_WORK_THREADS];
pthread_t logger_tid;

// Thread synchronization variables
pthread_mutex_t mutex_work, mutex_log;
pthread_cond_t  cond_work, cond_log, cond_buff_full;

// Work and Log buffers
int   work_queue[QUEUE_SIZE];
char* log_queue[QUEUE_SIZE];
char* dict_words[DICT_SIZE];

// WaitList head and rear node
WaitList* head, *rear;

// Log file for logger thread
FILE* log_file;

// Counters for work and log buffers
int work_count_fill = 0;
int log_count_fill = 0;
int work_count_use = 0;
int log_count_use = 0;
int work_count = 0;
int log_count = 0;

// Port for server-client communication
int port = DEFAULT_PORT;

/*
        @brief  Program entry point

        @param  argc       Number of command line arguments (int)
        @param  argv       Array of command line arguments  (char**)
*/
int main(int argc, char** argv) {
        int socket_desc;
        struct sockaddr_in client;
        // Init spell checker dictionary
        init_dict(argc, argv);
        // Init the Network w/ parameters
        init_network(&socket_desc);
        // Init threads synchorinzation
        init_threads_synch();
        // Init worker thread pool
        init_worker_threads();
        // Init logger thread
        init_logger_thread();
        // Main Thread Loop
        while (1) {
                // Accept an incoming connection
                int c = sizeof(struct sockaddr_in);
                int new_socket = accept(socket_desc, (struct sockaddr*) &client, (socklen_t*)&c);
                if (new_socket < 0) {
                        fprintf(stderr, "Error: Accept failed");
                        continue;
                }
                add_new_socket(new_socket);
        }
        return 0;
}

/*
        @brief  Initialize the server's dictionary
                        Can either be user supplied or default dict

        @param  argc       Number of command line arguments (int)
        @param  argv       Array of command line arguments  (char**)
*/
void init_dict(int argc, char** argv) {
        FILE* dict_file = NULL;
        int count = 0;
        if (argc == 2) {
                if (isdigit(argv[1][0]) != 0) {
                        // passed a port number
                        port = atoi(argv[1]);
                        if ((dict_file = fopen(DEFAULT_DICTIONARY, "r")) == NULL) exit(1);
                }
                else {
                        // passed a dict file
                        if ((dict_file = fopen(argv[1], "r")) == NULL) exit(1);
                }
        }
        else if (argc == 3) {
                // open with other dict and port
                port = atoi(argv[2]);
                if ((dict_file = fopen(argv[1], "r")) == NULL) exit(1);
        }
        else {
                // use default dict
                if ((dict_file = fopen(DEFAULT_DICTIONARY, "r")) == NULL) exit(1);
        }
        while (!feof(dict_file)) {
                // copy dict
                char dfile_word[150];
                dict_words[count] = malloc(150 * sizeof(char));
                if (dict_words[count]) {
                        fscanf(dict_file, "%s", dfile_word);
                        strcpy(dict_words[count++], dfile_word);
                }
                else exit(1);
        }
}

/*
        @brief  Initialize the network socket for server-client communication
                        Will terminate the program upon failure to create socket

        @param  *socket_desc       Pointer to server's socket address (int*)
*/
void init_network(int* socket_desc) {
        // Set up network
        struct sockaddr_in server;
        puts("\n***************** Spell Check Server *****************");
        // Create socket (create active socket descriptor)
        *socket_desc = socket(AF_INET, SOCK_STREAM, 0);
        if (*socket_desc == -1) {
                fprintf(stderr, "Error: could not create socket\n");
                exit(1);
        }
        // Prepare sockaddr_instructure
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;    // defaults to 127.0.0.1
        server.sin_port = htons(port);
        // Bind (connect the server's socket address to the socket descriptor)
        int bind_result = bind(*socket_desc, (struct sockaddr*) & server, sizeof(server));
        if (bind_result < 0) {
                fprintf(stderr, "Error: failed to bind\n");
                exit(1);
        }
        // Listen (converts active socket to a LISTENING socket which can accept connections)
        listen(*socket_desc, 3);
}

/*
        @brief  Initialize all thread mutexs and condition variables
*/
void init_threads_synch() {
        pthread_mutex_init(&mutex_work, NULL);
        pthread_mutex_init(&mutex_log, NULL);
        pthread_cond_init(&cond_work, NULL);
        pthread_cond_init(&cond_log, NULL);
        pthread_cond_init(&cond_buff_full, NULL);
}

/*
        @brief  Initialize all worker threads
                        Number of worker threads = MAX_WORK_THREADS
*/
void init_worker_threads() {
        int i;
        for (i = 0; i < MAX_WORK_THREADS; ++i) {
                if (pthread_create(&(worker_pool[i]), NULL, worker, NULL) != 0)
                        exit(1);
        }
}

/*
        @brief  Initialize logger thread
*/
void init_logger_thread() {
        // Open log file
        if ((log_file = fopen("log.txt", "w")) == NULL) return;
        if (pthread_create(&logger_tid, NULL, logger, NULL) != 0)
                exit(1);
}

/*
        @brief  Add a new socket to the work queue, which is protected by mutex
                        This will be done by the main server thread

        @param  socket       Client file descriptor to add to work queue (int)
*/
void add_new_socket(int socket) {
        // Get exclusive access to work queue
        pthread_mutex_lock(&mutex_work);
        // Make sure there's enough space in buffer
        printf("Work Count: %d\n", work_count);
        if (work_count == QUEUE_SIZE) {
                dprintf(socket, "%s\n", "You've been added to queue\nPlease Wait\n");
                add_wait_list_node(socket);
        }
        while (work_count_fill == QUEUE_SIZE) {
                // Give up CPU if buffer is full
                pthread_cond_wait(&cond_buff_full, &mutex_work);
        }
        if (head) {
                WaitList* temp = head;
                socket = head->fd;
                head = head->next;
                free(temp);
        }
        work_queue_add(socket);
        pthread_cond_signal(&cond_work);
        pthread_mutex_unlock(&mutex_work);
}

/*
        @brief Add a new socket to the waitlist

        @param socket a client file descriptor for the client
*/
void add_wait_list_node(int socket) {
        WaitList* wl = (WaitList*)malloc(sizeof(WaitList));
        wl->fd = socket;
        wl->next = NULL;
        if (!head) {
                head = rear = wl;
        }
        else {
                rear->next = wl;
                rear = wl;
        }
}

/*
        @brief  Atomically add the new socket to work queue
*/
void work_queue_add(int new_socket) {
        work_queue[work_count_fill] = new_socket;
        work_count_fill = (work_count_fill + 1) % QUEUE_SIZE;
        work_count++;
}

/*
        @brief  Atomically pop a socket to work queue
*/
int work_queue_pop() {
        int temp = work_queue[work_count_use];
        work_count_use = (work_count_use + 1) % QUEUE_SIZE;
        return temp;
}

/*
        @brief  Atomically add a new message to log queue
*/
void log_queue_add(char* msg) {
        log_queue[log_count_fill] = malloc(strlen(msg) * sizeof(char));
        if (log_queue[log_count_fill] == NULL) exit(1);
        strcpy(log_queue[log_count_fill], msg);
        log_count_fill = (log_count_fill + 1) % QUEUE_SIZE;
        log_count++;
}

/*
        @brief  Atomically pop a message from log queue
*/
char* log_queue_pop() {
        char* temp = malloc(strlen(log_queue[log_count_use]) * sizeof(char));
        if (temp == NULL) exit(1);
        strcpy(temp, log_queue[log_count_use]);
        log_count_use = (log_count_use + 1) % QUEUE_SIZE;
        log_count--;
        return temp;
}

/*
        @brief  	Worker thread function
                        Will block and give up CPU until there is work available
                        Will then connect to client and check for mispelled words
*/
void* worker() {
        while (1) {
                bool client_active = true;
                pthread_mutex_lock(&mutex_work);
                // wait until there's a client fd available
                while (work_count == 0) {
                        pthread_cond_wait(&cond_work, &mutex_work);
                }
                int client_file_desc = work_queue_pop();
                // wake up the main thread for an open slot
                pthread_cond_signal(&cond_buff_full);
                pthread_mutex_unlock(&mutex_work);
                // worker stays in this loop until client disconnects
                while (client_active) {
                        dprintf(client_file_desc, "Enter Message to Check: ");
                        char buf[1024] = { 0 };
                        if (read(client_file_desc, buf, 1024) == 0) client_active = false;
                        if (client_active) {
                                pthread_mutex_lock(&mutex_log);
                                log_queue_add(buf);
                                pthread_cond_signal(&cond_log);
                                pthread_mutex_unlock(&mutex_log);
                                printf("Received: %s", buf);
                                check_spelling(client_file_desc, buf);
                                puts("Sent response to client\n");
                                pthread_mutex_lock(&mutex_log);
                                log_queue_add("\n");
                                fflush(log_file);
                                pthread_cond_signal(&cond_log);
                                pthread_mutex_unlock(&mutex_log);
                                fflush(stdout);
                        }
                }
                pthread_mutex_lock(&mutex_work);
                work_count--;
                pthread_mutex_unlock(&mutex_work);
        }
}

/*
        @brief  	Logger thread function that writes to log file
                        Will block and give up CPU until there is log messages available
*/
void* logger() {
        while (1) {
                // Get exclusive access to log queue
                pthread_mutex_lock(&mutex_log);
                while (log_count == 0) {
                        // give up CPU if log queue is empty
                        pthread_cond_wait(&cond_log, &mutex_log);
                }
                char* msg = log_queue_pop();
                fprintf(log_file, "%s", msg);
                fflush(log_file);
                pthread_mutex_unlock(&mutex_log);
                free(msg);
        }
}

/*
        @brief  	Check spelling of client supplied words
                        Will send the response directly to client

        @param  client_file_desc       Client file descriptor to send response to (int)
        @param  msg                    string to check (char*)
*/
void check_spelling(int client_file_desc, char* msg) {
        char* token = NULL;
        token = strtok(msg, " ");
        while (token != NULL) {
                char* response = NULL;
                bool spelled_wrong = true;
                int len;
                if ((strcmp(token, "\n")) == 0) return;
                for (len = 0; token[len] != '\n' && token[len] != '\0' && token[len] != ' ' && token[len] != '\t'; ++len);
                token[len] = '\0';
                int i;
                for (i = 0; dict_words[i] != NULL; ++i) {
                        if (strcmp(dict_words[i], token) == 0)
                                spelled_wrong = false;
                }
                if (spelled_wrong) {
                        int len = strlen(token) + strlen(" -MISPELLED\n") + 1;
                        response = malloc(len * sizeof(char));
                        strcpy(response, token);
                        strcat(response, " -MISPELLED\n");
                }
                else {
                        int len = strlen(token) + strlen(" -OK\n") + 1;
                        response = malloc(len * sizeof(char));
                        strcpy(response, token);
                        strcat(response, " -OK\n");
                }
                // send response to client
                dprintf(client_file_desc, "%s", response);
                // write response to log
                pthread_mutex_lock(&mutex_log);
                log_queue_add(response);
                pthread_cond_signal(&cond_log);
                pthread_mutex_unlock(&mutex_log);
                token = strtok(NULL, " ");
        }
        dprintf(client_file_desc, "\n");
}
