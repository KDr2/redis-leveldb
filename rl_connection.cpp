/*-*- c++ -*-
 *
 * rl_connection.cpp
 * author : KDr2
 *
 */

#include <algorithm>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>

#include "rl_util.h"
#include "rl_server.h"
#include "rl_connection.h"
#include "rl_request.h"

#define CHECK_BUFFER(N) do{                                                      \
        if(next_idx+(N)>(read_buffer+buffered_data)){                            \
            next_idx=old_ni;                                                     \
            memmove(read_buffer,next_idx,buffered_data-(next_idx-read_buffer));  \
            buffered_data-=(next_idx-read_buffer);next_idx=read_buffer;          \
            read_buffer[buffered_data]=0;                                        \
            return 0;}}while(0)


RLConnection::RLConnection(RLServer *s, int fd):
    fd(fd), server(s), buffered_data(0)
{

    next_idx = read_buffer;
    ev_init(&read_watcher, RLConnection::on_readable);
    read_watcher.data = this;
    timeout_watcher.data = this;
    
    set_nonblock(fd);
    open=true;
    current_request=NULL;
    transaction=NULL;
    server->clients_num++;
    //memcpy(sockaddr, &addr, addr_len);
    //ip = inet_ntoa(sockaddr.sin_addr);  
}

RLConnection::~RLConnection()
{
    if(open){
        ev_io_stop(server->loop, &read_watcher);
        close(fd);
    }
    server->clients_num--;
}

void RLConnection::start()
{

    ev_io_set(&write_watcher, fd, EV_WRITE);
    ev_io_set(&read_watcher, fd, EV_READ);
    ev_io_start(server->loop, &read_watcher);
}


size_t RLConnection::get_int() {
    char *b = next_idx;
    size_t val = 0;
    while(*b != '\r') {
        val *= 10;
        val += (*b++ - '0');
    }
    if(b<=(read_buffer+buffered_data-1)){
        b += 2;
        next_idx = b;
        return val;
    }
    return -1;
}

void RLConnection::do_request(){
    if(current_request && current_request->completed()){
        current_request->run();
        if(current_request){
            delete current_request;
            current_request=NULL;
        }
    }
}

int RLConnection::do_read(){
    char *old_ni=next_idx;
    while(next_idx<(read_buffer+buffered_data)){
        old_ni=next_idx;
        if(!current_request)current_request=new RLRequest(this);
        // 1. read the arg count:
        if(current_request->arg_count<0){
            CHECK_BUFFER(4);
            if(*next_idx++ != '*') return -1;
            current_request->arg_count=get_int();
            current_request->arg_count--;
            old_ni=next_idx;
        }
        // 2. read the request name
        if(current_request->arg_count>=0 && current_request->name.empty()){
            CHECK_BUFFER(4);
            if(*next_idx++ != '$') return -1;
            int len=get_int();
            CHECK_BUFFER(len+2);
            current_request->name=std::string(next_idx,len);
            std::transform(current_request->name.begin(), current_request->name.end(),
                           current_request->name.begin(), ::tolower);
            next_idx+=len+2;
            old_ni=next_idx;
        }
        // 3. read a arg
        if(current_request->arg_count>=0 &&
           current_request->arg_count - current_request->args.size()>0){
            CHECK_BUFFER(4);
            if(*next_idx++ != '$') return -1;
            int len=get_int();
            CHECK_BUFFER(len+2);
            current_request->append_arg(std::string(next_idx,len));
            next_idx+=len+2;
            old_ni=next_idx;
        }
        // 4. do the request
        if(current_request->arg_count>=0 && 
           current_request->arg_count - current_request->args.size()==0){
            do_request();
            if(next_idx>=(read_buffer+buffered_data)){
                buffered_data=0;
                next_idx=read_buffer;
                old_ni=next_idx;
                return 1;
            }
        }
    }
    old_ni=next_idx;
    CHECK_BUFFER(1);
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
        //TODO
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


void RLConnection::write_nil(){
    writen(fd, "$-1\r\n", 5);
}
    
void RLConnection::write_error(const char* msg){
    writen(fd, "-", 1);
    writen(fd, msg, strlen(msg));
    writen(fd, "\r\n", 2);
}

void RLConnection::write_status(const char* msg){
    writen(fd, "+", 1);
    writen(fd, msg, strlen(msg));
    writen(fd, "\r\n", 2);
}

void RLConnection::write_integer(const char *out, size_t out_size){
    writen(fd, ":", 1);
    writen(fd, out, out_size);
    writen(fd, "\r\n", 2);
}

void RLConnection::write_bulk(const char *out, size_t out_size){
    write_buffer[0] = '$';
    int count = sprintf(write_buffer + 1, "%ld", out_size);            
    writen(fd, write_buffer, count + 1);
    writen(fd, "\r\n", 2);
    writen(fd, out, out_size);
    writen(fd, "\r\n", 2);
}

void RLConnection::write_bulk(const std::string &out){
    write_bulk(out.c_str(), out.size());
}

void RLConnection::write_mbulk_header(int n){
    write_buffer[0] = '*';
    int count = sprintf(write_buffer + 1, "%d", n);
    writen(fd, write_buffer, count + 1);
    writen(fd, "\r\n", 2);
}

