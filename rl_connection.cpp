/*-*- c++ -*-
 *
 * rl_connection.cpp
 * author : KDr2
 *
 */

#include <assert.h>
#include <sys/socket.h>

#include "rl_util.h"
#include "rl_server.h"
#include "rl_connection.h"
#include "rl_request.h"


RLConnection::RLConnection(RLServer *s, int fd):
    fd(fd), buffered_data(0), server(s)
{

    ev_init(&read_watcher, RLConnection::on_readable);
    read_watcher.data = this;
    timeout_watcher.data = this;
    
    set_nonblock(fd);
    open=true;
    current_request=NULL;
    //memcpy(sockaddr, &addr, addr_len);
    //ip = inet_ntoa(sockaddr.sin_addr);  
}

RLConnection::~RLConnection()
{
    if(open){
        ev_io_stop(server->loop, &read_watcher);
        close(fd);
    }
}

void RLConnection::start()
{

    ev_io_set(&write_watcher, fd, EV_WRITE);
    ev_io_set(&read_watcher, fd, EV_READ);
    ev_io_start(server->loop, &read_watcher);
}


size_t RLConnection::get_int() {
    char *b = read_buffer+next_idx;
    size_t val = 0;
    while(*b != '\r') {
        val *= 10;
        val += (*b++ - '0');
    }
    if(b<=(read_buffer+buffered_data)){
        if(val>0){
            b += 2;
            next_idx = b-read_buffer;
        }
        return val;
    }
    return -1;
}

void RLConnection::do_request(){
    if(current_request && current_request->completed()){
        current_request->run();
        delete current_request;
        current_request=NULL;
        buffered_data=0;
    }
}

int RLConnection::do_read(){
    if(!current_request)current_request=new RLRequest(this);
    // 1. read the arg count:
    if(current_request->arg_count==0){
        CHECK_BUFFER(4);
        char* b = read_buffer+next_idx;
        if(*b++ != '*') return -1;
        current_request->arg_count=get_int();
        if(current_request->arg_count==0){
            return 0;
        }
    }
    // 2. read the request name
    if(current_request->arg_count>0 && current_request->name.empty()){
        //TODO
    }
    // 3. read a arg

    // 4. do the request
    if(current_request->arg_count>0 && 
       current_request->arg_count == current_request->args.size()){
        do_request();
    }
    // 5. done
    return 1;
}

void RLConnection::on_readable(struct ev_loop *loop, ev_io *watcher, int revents)
{
    RLConnection *connection = static_cast<RLConnection*>(watcher->data);
    size_t offset = connection->buffered_data;
    int left = READ_BUFFER - offset;
    char* recv_buffer = connection->read_buffer + offset;
    ssize_t recved;

    //assert(ev_is_active(&connection->timeout_watcher));
    assert(watcher == &connection->read_watcher);

    // No more buffer space.
    if(left == 0) return;

    if(EV_ERROR & revents) {
        puts("on_readable() got error event, closing connection.");
        return;
    }

    recved = recv(connection->fd, recv_buffer, left, 0);

    if(recved == 0) {
        delete connection;
        return;
    }

    if(recved <= 0) return;

    recv_buffer[recved] = 0;

    connection->buffered_data += recved;

    int ret = connection->do_read();
    switch(ret) {
    case -1:
        puts("bad protocol error");
        // fallthrough
    case 1:
        connection->buffered_data = 0;
        break;
    case 0:
        // more data needed, leave the buffer.
        break;
    default:
        puts("unknown return error");
        break;
    }

    /* rl_connection_reset_timeout(connection); */

    return;

    /* error: */
    /* rl_connection_schedule_close(connection); */
}


