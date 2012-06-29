/*-*- c++ -*-
 *
 * rl_request.cpp
 * author : KDr2
 *
 */

#include <assert.h>
#include <sys/socket.h>

#include <gmp.h>

#include "rl_util.h"
#include "rl_server.h"
#include "rl_connection.h"
#include "rl_request.h"


RLRequest::RLRequest(RLConnection *c):
    connection(c), arg_count(-1), name("")
{
}

RLRequest::~RLRequest()
{
    std::vector<RLRequest*>::iterator it=subrequest.begin();
    for(;it!=subrequest.end();it++)
        delete (*it);
}


void RLRequest::append_arg(std::string arg)
{
    args.push_back(arg);
}

void RLRequest::_run()
{
        if(name=="incr")
            rl_incr();
        if(name=="incrby")
            rl_incrby();
        if(name=="get")
            rl_get();
        if(name=="set")
            rl_set();
        if(name=="mget")
            rl_mget();
        if(name=="mset")
            rl_mset();
        if(name=="multi")
            rl_multi();
        if(name=="exec")
            rl_exec();
        if(name=="discard")
            rl_discard();
    
    printf("Request Name:%s\n",name.c_str());
    for(std::vector<std::string>::iterator it=args.begin();it!=args.end();it++)
        printf("Request arg:%s\n",it->c_str());
}

void RLRequest::run()
{
    if(name=="multi"){
        _run();
        return;
    }
    if(name=="exec"||name=="discard"){
        if(connection->transaction){
            _run();
        }else{
            connection->write_error("ERR EXEC without MULTI");
        }
        return;
    }
    
    if(connection->transaction){
        connection->transaction->subrequest.push_back(this);
        connection->current_request=NULL;
        connection->write_status("QUEUED");
    }else{
        _run();
    }
}

void RLRequest::rl_incr(){

    size_t out_size = 0;
    char* err = 0;
    char* out = leveldb_get(connection->server->db, connection->server->read_options,
                            args[0].c_str(), args[0].size(), &out_size, &err);

    if(err) {
        puts(err);
        out = 0;
    }

    char *str_oldv=NULL;
    if(!out){
        str_oldv = strdup("0");
    }else{
        str_oldv=(char*)malloc(out_size+1);
        memcpy(str_oldv,out,out_size);
        str_oldv[out_size]=0;
        free(out);
    }

    mpz_t old_v;
    mpz_init(old_v);
    mpz_set_str(old_v,str_oldv,10);
    free(str_oldv);
    mpz_add_ui(old_v,old_v,1);
    char *str_newv=mpz_get_str(NULL,10,old_v);
    mpz_clear(old_v);

    leveldb_put(connection->server->db, connection->server->write_options,
                args[0].c_str(), args[0].size(),
                str_newv, strlen(str_newv), &err);

    if(err) {
        connection->write_error(err);
        free(str_newv);
        return;
    }

    write(connection->fd, ":", 1);
    write(connection->fd, str_newv, strlen(str_newv));
    write(connection->fd, "\r\n", 2);
    free(str_newv);
}


void RLRequest::rl_incrby(){
    
    size_t out_size = 0;
    char* err = 0;

    char* out = leveldb_get(connection->server->db, connection->server->read_options,
                            args[0].c_str(), args[0].size(), &out_size, &err);

    if(err) {
        puts(err);
        out = 0;
    }

    
    mpz_t delta;
    mpz_init(delta);
    mpz_set_str(delta,args[1].c_str(),10);
    
    char *str_oldv=NULL;
    if(!out){
        str_oldv = strdup("0");
    }else{
        str_oldv=(char*)malloc(out_size+1);
        memcpy(str_oldv,out,out_size);
        str_oldv[out_size]=0;
        free(out);
    }

    mpz_t old_v;
    mpz_init(old_v);
    mpz_set_str(old_v,str_oldv,10);
    free(str_oldv);
    mpz_add(old_v,old_v,delta);
    char *str_newv=mpz_get_str(NULL,10,old_v);
    mpz_clear(delta);
    mpz_clear(old_v);
    
    leveldb_put(connection->server->db, connection->server->write_options,
                args[0].c_str(), args[0].size(),
                str_newv, strlen(str_newv), &err);

    if(err) {
        connection->write_error(err);
        free(str_newv);
        return;
    }

    write(connection->fd, ":", 1);
    write(connection->fd, str_newv, strlen(str_newv));
    write(connection->fd, "\r\n", 2);
    free(str_newv);
}

void RLRequest::rl_get(){
    
    
    size_t out_size = 0;
    char* err = 0;

    char* out = leveldb_get(connection->server->db, connection->server->read_options,
                            args[0].c_str(), args[0].size(), &out_size, &err);

    if(err) {
        puts(err);
        out = 0;
    }

    if(!out) {
        connection->write_nil();
    } else {
        char buf[256];
        buf[0] = '$';
        int count = sprintf(buf + 1, "%ld", out_size);
      
        write(connection->fd, buf, count + 1);
        write(connection->fd, "\r\n", 2);
        write(connection->fd, out, out_size);
        write(connection->fd, "\r\n", 2);
      
        free(out);
    }
  
}


void RLRequest::rl_set(){
    
    char* err = 0;

    leveldb_put(connection->server->db, connection->server->write_options,
                args[0].c_str(), args[0].size(),
                args[1].c_str(), args[1].size(), &err);
    
    if(err) puts(err);
  
    connection->write_status("OK");
}


void RLRequest::rl_mget(){

    char buf[256];
    buf[0] = '*';
    int count = sprintf(buf + 1, "%ld", args.size());
    write(connection->fd, buf, count + 1);
    write(connection->fd, "\r\n", 2);
    
    std::vector<std::string>::iterator it=args.begin();
    for(;it!=args.end();it++){
        size_t out_size = 0;
        char* err = 0;
        
        char* out = leveldb_get(connection->server->db, connection->server->read_options,
                                it->c_str(), it->size(), &out_size, &err);

        if(err) {
            puts(err);
            out = 0;
        }

        if(!out) {
            connection->write_nil();
        } else {
            char buf[256];
            buf[0] = '$';
            int count = sprintf(buf + 1, "%ld", out_size);
            
            write(connection->fd, buf, count + 1);
            write(connection->fd, "\r\n", 2);
            write(connection->fd, out, out_size);
            write(connection->fd, "\r\n", 2);
            
            free(out);
        }
    }
}


void RLRequest::rl_mset(){
    
    char* err = 0;
    
    for(int i=0;i<args.size();i+=2){
        leveldb_put(connection->server->db, connection->server->write_options,
                    args[i].c_str(), args[i].size(),
                    args[i+1].c_str(), args[i+1].size(), &err);        
        if(err) puts(err);
    }
    connection->write_status("OK");
}


void RLRequest::rl_multi(){
    if(connection->transaction){
        connection->write_error("ERR MULTI calls can not be nested");
        return;
    }
    connection->transaction=this;
    connection->current_request=NULL;
    connection->write_status("OK");
}


void RLRequest::rl_exec(){
    if(!connection->transaction){
        connection->write_error("ERR EXEC without MULTI");
        return;
    }

    std::vector<RLRequest*> tsub=connection->transaction->subrequest;
    std::vector<RLRequest*>::iterator it=tsub.begin();
    
    char buf[256];
    buf[0] = '*';
    int count = sprintf(buf + 1, "%ld", tsub.size());
    write(connection->fd, buf, count + 1);
    write(connection->fd, "\r\n", 2);
    
    for(;it!=tsub.end();it++)
        (**it)._run();    
    delete connection->transaction;
    connection->transaction=NULL;
}

void RLRequest::rl_discard(){
    if(!connection->transaction){
        connection->write_error("ERR DISCARD without MULTI");
        return;
    }

    delete connection->transaction;
    connection->transaction=NULL;
    connection->write_status("OK");
}






