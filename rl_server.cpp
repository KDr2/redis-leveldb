/*-*- c++ -*-
 *
 * rl_server.cpp
 * author : KDr2
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h> /* TCP_NODELAY */
#include <netinet/in.h>  /* inet_ntoa */
#include <arpa/inet.h>   /* inet_ntoa */

#include "rl_util.h"
#include "rl_server.h"
#include "rl_connection.h"

#define EVBACKEND EVFLAG_AUTO

#ifdef __linux
#undef EVBACKEND
#define EVBACKEND EVBACKEND_EPOLL
#endif

#ifdef __APPLE__
#undef EVBACKEND
#define EVBACKEND EVBACKEND_KQUEUE
#endif

RLServer::RLServer(const char *_db_path, const char *_hostaddr, int _port, int dbn):
    db_num(dbn), db_path(_db_path), hostaddr(_hostaddr), port(_port),
    fd(-1), clients_num(0)
{
    options = leveldb_options_create();
    leveldb_options_set_create_if_missing(options, 1);

    read_options = leveldb_readoptions_create();
    write_options = leveldb_writeoptions_create();
    
    char* err = 0;
    
    if(db_num<1){
        db=new leveldb_t*[1];
        db[0] = leveldb_open(options, db_path.c_str(), &err);
        if(err) {
            puts(err);
            exit(1);
        }
    }else{
        db=new leveldb_t*[db_num];
        char buf[16];
        for(int i=0;i<db_num;i++){
            int count = sprintf(buf, "/db-%03d", i);
            //TODO the db path
            db[i] = leveldb_open(options, (db_path+std::string(buf,count)).c_str(), &err);
            if(err) {
                puts(buf);
                puts(err);
                exit(1);
            }
        }
    }

    loop = ev_loop_new(EVBACKEND);
    connection_watcher.data = this;

}

RLServer::~RLServer(){
    if(db_num<1){
        leveldb_close(db[0]);
    }else{
        for(int i=0;i<db_num;i++){
            leveldb_close(db[i]);
        }
    }
    delete[] db;
    if(loop) ev_loop_destroy(loop);
    close(fd);
}


void RLServer::start()
{    
    struct linger ling = {0, 0};
    struct sockaddr_in addr;
    int flags = 1;
  
    if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket()");
        exit(1);
    }
  
    flags = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&flags, sizeof(flags));
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags));
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));

    /* XXX: Sending single byte chunks in a response body? Perhaps there is a
     * need to enable the Nagel algorithm dynamically. For now disabling.
     */
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
  
    /* the memset call clears nonstandard fields in some impementations that
     * otherwise mess things up.
     */
    memset(&addr, 0, sizeof(addr));
  
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if(!hostaddr.empty()){
        addr.sin_addr.s_addr = inet_addr(hostaddr.c_str());
        if(addr.sin_addr.s_addr==INADDR_NONE){
            printf("Bad address(%s) to listen\n",hostaddr.c_str());
            exit(1);
        }
    }else{
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
  
    if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind()");
        if(fd > 0) close(fd);
        exit(1);
    }

    if(listen(fd, MAX_CONNECTIONS) < 0) {
        perror("listen()");
        exit(1);
    }
  
    set_nonblock(fd);
    
    ev_init(&connection_watcher, RLServer::on_connection);
    
    ev_io_set(&connection_watcher, fd, EV_READ);
    ev_io_start(loop, &connection_watcher);
    
    ev_run(loop, 0);
}

void RLServer::on_connection(struct ev_loop *loop, ev_io *watcher, int revents)
{
    RLServer *s = static_cast<RLServer*>(watcher->data);
    if(EV_ERROR & revents) {
        puts("on_connection() got error event, closing server.");
        return;
    }
  
    struct sockaddr_in addr; // connector's address information
    socklen_t addr_len = sizeof(addr); 
    int fd = accept(s->fd, (struct sockaddr*)&addr, &addr_len);

    if(fd < 0) {
        perror("accept()");
        return;
    }

    RLConnection *connection = new RLConnection(s, fd);

    if(connection == NULL) {
        close(fd);
        return;
    } 
    connection->start();
    
}


