#include "download_func.h"

int download(int id) 
{
    running_thread_mutex.lock();
    running_thread++;
    running_thread_mutex.unlock();
    
    //Start intialize download information
    download_list_mutex.lock();
    int down_index = get_index_by_id(id, download_list);
    
    if(down_index < 0){
        return 1;
    }
    
    hostent *server = gethostbyname(host.c_str());
    if (server == NULL) {
        download_list.at(down_index).status = ERROR;
        download_list_mutex.unlock();
        
        running_thread_mutex.lock();
        running_thread--;
        running_thread_mutex.unlock();
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        download_list.at(down_index).status = ERROR;
        download_list_mutex.unlock();
        
        running_thread_mutex.lock();
        running_thread--;
        running_thread_mutex.unlock();
        
        return 1;
    }

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        download_list.at(down_index).status = ERROR;
        download_list_mutex.unlock();
        
        running_thread_mutex.lock();
        running_thread--;
        running_thread_mutex.unlock();

        return 1;
    }

    stringstream request;
    request << "GET " << download_list.at(down_index).down_path << " HTTP/1.0\n"
            << "Host: " << host << "\n"
            << "Connection: close\n"
            << "Range: bytes=" << download_list.at(down_index).downloaded << "-\n"
            << "\n";
    
    string request_str = request.str();
    if (send(sockfd, request_str.c_str(), request_str.length(), 0) < 0) {
        download_list.at(down_index).status = ERROR;
        close(sockfd);
        download_list_mutex.unlock();
        
        running_thread_mutex.lock();
        running_thread--;
        running_thread_mutex.unlock();
        
        return 1;
    }
    
    stringstream response;
    char buffer[BUFFER_SIZE];
    int bytes_read;
    bool headers_read = false;

    while ((bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        response.write(buffer, bytes_read);

        string response_str = response.str();
        
        size_t header_end = response_str.find("\r\n\r\n");
        
        //cout << response_str;
        
        if (header_end != string::npos) {
            headers_read = true;
            
            int status_code = get_status_code(response_str);
            
            if (status_code == 0){
                download_list.at(down_index).status = ERROR;
                close(sockfd);
                download_list_mutex.unlock();
                
                running_thread_mutex.lock();
                running_thread--;
                running_thread_mutex.unlock();
        
                return 1;
            }
            
            if (status_code != 200 && status_code != 206){
                download_list.at(down_index).status = ERROR;
                close(sockfd);
                download_list_mutex.unlock();
                
                running_thread_mutex.lock();
                running_thread--;
                running_thread_mutex.unlock();
                
                return 1;
            }
            
            if (get_total_size(response_str) >= 0){
                download_list.at(down_index).total_size = get_total_size(response_str);
            }          
            break;
        }
        
        sleep(0.001);
    }
    
    if (!headers_read) {
        download_list.at(down_index).status = ERROR;
        download_list_mutex.unlock();
        
        running_thread_mutex.lock();
        running_thread--;
        running_thread_mutex.unlock();
        
        close(sockfd);
        return 1;
    }

    ofstream out_file(download_list.at(down_index).out_path, ios::app);
    
    if (!out_file.is_open()) {
        download_list.at(down_index).status = ERROR;
        download_list_mutex.unlock();
        close(sockfd);
        
        running_thread_mutex.lock();
        running_thread--;
        running_thread_mutex.unlock();
        
        return 1;
    }

    download_list_mutex.unlock();
    
    //Start downloading while writing info
    while ((bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        download_list_mutex.lock();
        down_index = get_index_by_id(id, download_list);
    
        if (download_list.at(down_index).pause_signal){
            download_list.at(down_index).status = CANCEL;
            download_list_mutex.unlock();
            out_file.close();
            close(sockfd);
            
            running_thread_mutex.lock();
            running_thread--;
            running_thread_mutex.unlock();
            
            return 0;
        }
        
        out_file.write(buffer, bytes_read);
        download_list.at(down_index).downloaded += bytes_read;
        download_list.at(down_index).status = DOWNLOADING;
        
        if (bytes_read < 0) {
            download_list.at(down_index).status = ERROR;
            out_file.close();
            download_list_mutex.unlock();
            close(sockfd);
            
            running_thread_mutex.lock();
            running_thread--;
            running_thread_mutex.unlock();
        
            return 1;
        }
        
        download_list_mutex.unlock();
        sleep(0.001);
    }

    download_list.at(down_index).status = COMPLETED;
    out_file.close();
    
    running_thread_mutex.lock();
    running_thread--;
    running_thread_mutex.unlock();
        
    close(sockfd);
    
    return 0;
}

int get_status_code(string response_str)
{
    string status_prefix = "HTTP/1.0 ";
    string::size_type status_start = response_str.find(status_prefix);

    if (status_start == string::npos) {        
        return 0;
    }

    string::size_type status_end = response_str.find(' ', status_start + status_prefix.length());
    if (status_end == string::npos) {
        return 0;
    }

    string status_code_str = response_str.substr(status_start 
            + status_prefix.length(), status_end - status_start - status_prefix.length());
    
    return stoi(status_code_str);
}

int get_total_size(string response_str)
{
    string content_length_prefix = "Content-Length: ";
    string::size_type content_length_start = response_str.find(content_length_prefix);
    long content_length = -1;
    if (content_length_start != string::npos) {
        string::size_type content_length_end = response_str.find("\r\n", 
                content_length_start + content_length_prefix.length());
        if (content_length_end != string::npos) {
            string content_length_str = response_str.substr(content_length_start + content_length_prefix.length()
                    , content_length_end - content_length_start - content_length_prefix.length());
            istringstream iss(content_length_str);
            iss >> content_length;
        }
    }
    
    return content_length;
}

int get_index_by_id(int id, vector<down_stat> download_list){
    for (int i = 0; i < download_list.size(); i++){
        if(id == download_list[i].id){
            return i;
        }
    }
    
    return -1;
}

const char *get_stat_string(enum state f){
    static const char *strings[] = {"Completed", "Error", "Cancel", "Ready", "Downloading"};

    return strings[f];
};

int compare_state(const down_stat* a, const down_stat* b) {
    return a->status - b->status;
};