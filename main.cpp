#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <string.h>
#include <vector>
#include <termios.h>
#include <fcntl.h>
#include "misc_func.h"
#include "download_func.h"

using namespace std;

string host = "localhost";
int port = 8000;
string out_path = "/home/home/Downloads/";

int limit = sysconf(_SC_NPROCESSORS_CONF);
int running_thread = 0;
mutex running_thread_mutex;

vector<down_stat> download_list;
mutex download_list_mutex;

int download_managing();

int main(int argc, char** argv) 
{
    if (argc == 1){
        cout << "Please specific enough parameter!\n";
        return 1;
    }
    
    int out_position = 0;
    int out_path_position = 0;
    int limit_command_position = 0;
    int limit_position;
    int is_replace = 0;
    
    for (int i = 1; i < argc; i++){
        if (!strcmp(argv[i], "--output")){
            if (argc > i + 1){
                out_path = argv[i + 1];
                out_position = i;
                out_path_position = i + 1;
            } else {
                cout << "Please specific path after \"--out\"\n";
                return 1;
            }
        }
        
        if (!strcmp(argv[i], "--limit")){
            if (argc > i + 1){
                int temp = atoi(argv[i + 1]);
                if (temp < limit){
                    limit = temp;
                    limit_command_position = i;
                    limit_position = i + 1;
                }
            } else {
                cout << "Please specific limit number after \"--limit\"\n";
                return 1;
            }
        }
        
        
        if (!strcmp(argv[i], "--replace")){
            is_replace = i;
        }
        
        sleep(0.1);
    }
    
    for (int i = 1, count = 1; i < argc; i++){
        if (i != out_position && i != out_path_position && i != is_replace && i!= limit_command_position && i!=limit_position){
            down_stat down;
            
            down.id = count++;
            
            string temp = argv[i];
            
            for (char c : temp) {
                if (c == ' ') {
                    down.down_path += "%20";
                } else {
                    down.down_path += c;
                }
            }
            
            temp = out_path + temp;
            
            if (is_replace == 0){
                down.out_path = get_new_file_name(temp);
            } else {
                down.out_path = temp;
            }
            
            download_list.push_back(down);
        }
        
        sleep(0.1);
    }    
    
    thread download_managing_thread(download_managing);
    download_managing_thread.detach();
    
    while (true){
        int end_flag = 0;
        int comp_count = 0;
        int fail_count = 0;
        char actual_path[100];
        realpath(out_path.c_str(), actual_path);
        
        download_list_mutex.lock();
        merge_sort(download_list, 0, download_list.size() - 1, compare_state);
        download_list_mutex.unlock();
        
        system("clear");
        cout << "Download file from \"http::/" << host << ":" << port << "/\"\n\n";
        cout << "Saved folder: " << actual_path << "\n";
        printf("ID\tNAME\t\tSTATE\t\tPERC\tCURR\t\tTOTAL\n");
        cout << "--------------\n";
        for (int i = 0; i < download_list.size(); i++){
            string name = download_list[i].down_path;;            
            if (download_list[i].down_path.length() > 12){
                name.resize(9);
                name += "...";
            }
            
            printf("%-8d", download_list[i].id);
            printf("%-16s", name.c_str());
            printf("%-16s", get_stat_string(download_list[i].status));
            printf("%-8.2f", download_list[i].downloaded * 100.0 / download_list[i].total_size);
            printf("%-16.2f", download_list[i].downloaded/1000.0);
            printf("%-16.2f\n", download_list[i].total_size/1000.0);
            
            if (download_list[i].status != DOWNLOADING && download_list[i].status != READY 
                    && download_list[i].status != CANCEL){
                //download_list.erase(download_list.begin() + i);
                end_flag++;
            }
            
            if (download_list[i].status == COMPLETED){
                comp_count++;
            } else {
                fail_count++;
            }
            
            sleep(0.1);
        }
        
        cout << "--------------\n";
        if (end_flag == download_list.size()){
            cout << "All download finished with " << comp_count << " completed and " << fail_count << " failed.\n";
            return 0;
        }
        
        cout << "Enter command to execute: " << flush;

        fd_set watchlist;
        FD_ZERO(&watchlist);
        FD_SET(STDIN_FILENO, &watchlist);

        struct timeval timeout;
        timeout.tv_sec = 1;  // set timeout for 5 seconds
        timeout.tv_usec = 0;
        
        char buffer[256];
        
        // wait for input or timeout
        int result = select(1, &watchlist, NULL, NULL, &timeout);

        if (result == -1) {
            perror("select");
            return 1;
        } else if (result != 0) {
            // input is available, read and process 
            int n = 0;
            char c;
            while (read(STDIN_FILENO, &c, 1) > 0) {
            if (c == '\n') {
                    buffer[n] = '\0'; // add null terminator
                    break;
                } else {
                    buffer[n++] = c;
                }
            }
            if (n == -1) {
                perror("read");
                return 1;
            } else {
                switch (buffer[0]){
                    case 'p':
                    {
                        download_list_mutex.lock();
                        for (int i = 0; i < download_list.size(); i++){
                            if(download_list[i].id == atoi(buffer + 1)){
                                if (download_list[i].downloaded < download_list[i].total_size)
                                    download_list[i].pause_signal = true;
                            }
                            sleep(0.1);
                        }
                        download_list_mutex.unlock();
                        break;
                    }
                    case 'c':
                    {
                        download_list_mutex.lock();
                        for (int i = 0; i < download_list.size(); i++){
                            if(download_list[i].id == atoi(buffer + 1) && download_list[i].status == CANCEL){
                                download_list[i].status = READY;
                                download_list[i].pause_signal = false;
                            }
                            
                            sleep(0.1);
                        }
                        download_list_mutex.unlock();
                        break;
                    }
                    case 'n':
                    {
                        down_stat down;
                        
                        down.id = download_list.size() + 1;
                        
                        string temp = buffer + 1;
                        
                        for (char c : temp) {
                            if (c == ' ') {
                                down.down_path += "%20";
                            } else {
                                down.down_path += c;
                            }
                        }
                        
                        temp = out_path + temp;

                        if (is_replace == 0){
                            down.out_path = get_new_file_name(temp);
                        } else {
                            down.out_path = temp;
                        }
                        
                        download_list_mutex.lock();
                        download_list.push_back(down);
                        download_list_mutex.unlock();
                    }
                }
            }
        }
    }
    
    return 0;
}

int download_managing()
{
    while (true){
        for (int i = 0; i < download_list.size(); i++){
            if (download_list[i].status == READY && running_thread < limit){
                thread download_thread(download, download_list[i].id);
                download_thread.detach();
                sleep(0.1);
            }
        }
        sleep(0.1);
    }
    
    return 0;
}