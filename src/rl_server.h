/*-*- c++ -*-
 *
 * rl_server.h
 * author : KDr2
 *
 */

#ifndef _RL_SERVER_H_INCLUDED
#define _RL_SERVER_H_INCLUDED

#include <vector>
#include <string>

#include <ev.h>
#include <leveldb/c.h>

class RLServer{
    
public:
    int db_num;
    std::string db_path;
    std::string hostaddr;
    int port;
    int fd;
    int clients_num;
    struct ev_loop* loop;
    ev_io connection_watcher;
    leveldb_options_t* options;
    leveldb_readoptions_t* read_options;
    leveldb_writeoptions_t* write_options;
    leveldb_t **db;

    RLServer(const char *db_path, const char *hostaddr, int port, int dbn=0);
    ~RLServer();
    void start();
    static void on_connection(struct ev_loop *loop, ev_io *watcher, int revents);
};


#endif

