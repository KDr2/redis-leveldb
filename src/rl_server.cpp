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

#include <leveldb/filter_policy.h>

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

    printf("HOSTADDR:(%s)\n",hostaddr.c_str());
    printf("DB_PATH:(%s)\n",db_path.c_str());
    printf("PORT:(%d)\n",port);

    leveldb::Status status;

    if(db_num<1){
        options = new leveldb::Options[1];
        options[0].create_if_missing = true;
        options[0].filter_policy = leveldb::NewBloomFilterPolicy(10);

        db=new leveldb::DB*[1];
        status = leveldb::DB::Open(options[0], db_path.c_str(), &db[0]);
        if(!status.ok()) {
            puts("leveldb open error");
            exit(1);
        }
    }else{
        options = new leveldb::Options[db_num];

        db=new leveldb::DB*[db_num];
        char buf[16];
        for(int i=0;i<db_num;i++){
            options[i].create_if_missing = true;
            options[i].filter_policy = leveldb::NewBloomFilterPolicy(16);

            int count = sprintf(buf, "/db-%03d", i);
            //TODO the db path
            status = leveldb::DB::Open(options[i], (db_path+std::string(buf,count)).c_str(), &db[i]);
            if(!status.ok()) {
                puts("leveldb open error:");
                puts(buf);
                exit(1);
            }
        }
    }

    loop = ev_loop_new(EVBACKEND);
    connection_watcher.data = this;

}

RLServer::~RLServer(){
    if(db_num<1){
        delete options[0].filter_policy;
        delete db[0];
    }else{
        for(int i=0;i<db_num;i++){
            delete options[i].filter_policy;
            delete db[i];
        }
    }
    delete[] options;
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

    printf("Server running successfully\n");
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
