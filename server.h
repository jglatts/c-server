#ifndef SERVER__H
#define SERVER__H

#define DICT_SIZE           100000
#define DEFAULT_PORT        8888
#define DEFAULT_DICTIONARY  "/usr/share/dict/words"
#define MAX_WORK_THREADS    8
#define QUEUE_SIZE          8

#define for_each_in_waitlist(list) \
        for(WaitList* n = list->head; n; n = n->next;)

typedef struct WaitList {
        int fd;
        struct WaitList* next;
} WaitList;

void  init_dict(int argc, char** argv);
void  init_network(int*);
void  init_worker_threads();
void  init_logger_thread();
void  init_threads_synch();
void  add_new_socket(int);
void  add_wait_list_node(int);
void  work_queue_add(int);
void  log_queue_add(char*);
void  check_spelling(int, char*);
void* worker();
void* logger();
int   work_queue_pop();
char* log_queue_pop();

#endif // !SERVER__H
