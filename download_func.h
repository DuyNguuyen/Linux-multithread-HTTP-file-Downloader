#ifndef DOWNLOAD_FUNC_H
#define DOWNLOAD_FUNC_H

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <mutex>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

using namespace std;

extern string host;
extern int port;

enum state{
    COMPLETED,
    ERROR,
    CANCEL,
    READY,
    DOWNLOADING
};

typedef struct down_stat
{
    int id;
    string down_path = "";
    unsigned int total_size = 0.001;
    unsigned int downloaded = 0;
    enum state status = READY;
    string out_path = "";
    bool pause_signal = false;
} down_stat;

extern vector<down_stat> download_list;
extern mutex download_list_mutex;
extern int running_thread;
extern mutex running_thread_mutex;

int download(int id);
int get_status_code(string response_str);
int get_total_size(string response_str);
int get_index_by_id(int id, vector<down_stat> download_list);
const char *get_stat_string(enum state f);
int compare_state(const down_stat* a, const down_stat* b);

#endif /* DOWNLOAD_FUNC_H */
