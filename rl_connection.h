/*-*- c++ -*-
 *
 * rl_connection.h
 * author : KDr2
 *
 */

#ifndef _RL_CONNECTION_H_INCLUDED
#define _RL_CONNECTION_H_INCLUDED

#include <vector>
#include <string>

#include <ev.h>
#include <leveldb/c.h>

#include "rl.h"

class RLServer;
class RLRequest;

#define CHECK_BUFFER(N) do{if(next_idx+(N)>(read_buffer+buffered_data))return 0;}while(0)

class RLConnection{
    
public:
    bool open;
    int fd;                     
    RLServer *server;
    RLRequest *current_request;
    RLRequest *transaction;

    char *next_idx;        /* next val to handle*/
    int buffered_data;   /* data has been read */
    char read_buffer[READ_BUFFER];
    char write_buffer[64];
    
    ev_io write_watcher;      
    ev_io read_watcher;       
    ev_timer timeout_watcher; 
    ev_timer goodbye_watcher; 

    /*** methods ***/
    
    RLConnection(RLServer *s, int fd);
    ~RLConnection();

    static void on_readable(struct ev_loop *loop, ev_io *watcher, int revents);
    
    void start();
    size_t get_int();
    int  do_read();
    void do_request();

    void write_nil();
    void write_error(const char* msg);
    void write_status(const char* msg);
    void write_integer(const char *out, size_t out_size);
    void write_bulk(const char *out, size_t out_size);
    void write_bulk(std::string &str);
    void write_mbulk_header(int n);
};

#endif

