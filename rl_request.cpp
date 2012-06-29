/*-*- c++ -*-
 *
 * rl_request.cpp
 * author : KDr2
 *
 */

#include <algorithm>
#include <functional>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>

#include <gmp.h>

#include "rl_util.h"
#include "rl_server.h"
#include "rl_connection.h"
#include "rl_request.h"


std::map<std::string,RLRequest::COMMAND> RLRequest::cmd_map;

void RLRequest::init_cmd_map()
{
    RLRequest::cmd_map["incr"]=&RLRequest::rl_incr;
    RLRequest::cmd_map["incrby"]=&RLRequest::rl_incrby;
    RLRequest::cmd_map["get"]=&RLRequest::rl_get;
    RLRequest::cmd_map["set"]=&RLRequest::rl_set;
    RLRequest::cmd_map["mget"]=&RLRequest::rl_mget;
    RLRequest::cmd_map["mset"]=&RLRequest::rl_mset;
    RLRequest::cmd_map["multi"]=&RLRequest::rl_multi;
    RLRequest::cmd_map["exec"]=&RLRequest::rl_exec;
    RLRequest::cmd_map["discard"]=&RLRequest::rl_discard;
    RLRequest::cmd_map["keys"]=&RLRequest::rl_keys;
}


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

#ifdef DEBUG
    printf("Request Name:%s\n",name.c_str());
    for(std::vector<std::string>::iterator it=args.begin();it!=args.end();it++)
        printf("Request arg:%s\n",it->c_str());
#endif
    
    std::map<std::string,COMMAND>::iterator it=cmd_map.find(name);
    if(it!=cmd_map.end()){
        (this->*(it->second))();
    }else{
        rl_dummy();
    }
    
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
            connection->write_error("ERR EXEC/DISCARD without MULTI");
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

void RLRequest::rl_dummy(){
    connection->write_error("ERR unknown command");
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

    connection->write_integer(str_newv, strlen(str_newv));
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

    connection->write_integer(str_newv, strlen(str_newv));
    free(str_newv);
}

void RLRequest::rl_get(){

    if(args.size()!=1){
        connection->write_error("ERR wrong number of arguments for 'get' command");
        return;
    }
    
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
        connection->write_bulk(out, out_size);      
        free(out);
    }
  
}


void RLRequest::rl_set(){

    if(args.size()!=2){
        connection->write_error("ERR wrong number of arguments for 'set' command");
        return;
    }

    char* err = 0;

    leveldb_put(connection->server->db, connection->server->write_options,
                args[0].c_str(), args[0].size(),
                args[1].c_str(), args[1].size(), &err);
    
    if(err) puts(err);
  
    connection->write_status("OK");
}


void RLRequest::rl_mget(){
    
    if(args.size()<1){
        connection->write_error("ERR wrong number of arguments for 'mget' command");
        return;
    }

    connection->write_mbulk_header(args.size());
    
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
            connection->write_bulk(out, out_size);
            free(out);
        }
    }
}


void RLRequest::rl_mset(){
    
    if(args.size()<2 || args.size()%2!=0){
        connection->write_error("ERR wrong number of arguments for 'mset' command");
        return;
    }

    char* err = 0;
    
    for(uint32_t i=0;i<args.size();i+=2){
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

    connection->write_mbulk_header(tsub.size());    
    std::for_each(tsub.begin(),tsub.end(),std::mem_fun(&RLRequest::_run));
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



void RLRequest::rl_keys(){
    
    if(args.size()!=1){
        connection->write_error("ERR wrong number of arguments for 'keys' command");
        return;
    }

    std::vector<std::string> keys;
    size_t arg_len=args[0].size();
    bool allkeys = (arg_len==1 && args[0][0]=='*');
    if(arg_len>0){
        leveldb_iterator_t *kit = leveldb_create_iterator(connection->server->db,
                                                          connection->server->read_options);
        const char *key;
        size_t key_len;
        leveldb_iter_seek_to_first(kit);
        while(leveldb_iter_valid(kit)) {
            key = leveldb_iter_key(kit, &key_len);
            if(allkeys || stringmatchlen(args[0].c_str(),arg_len,key,key_len,0)){
                keys.push_back(std::string(key,key_len));
            }
            leveldb_iter_next(kit);
        }
        leveldb_iter_destroy(kit);
    }else{
        keys.push_back("");
    }
    
    connection->write_mbulk_header(keys.size());
    //std::for_each(keys.begin(), keys.end(), std::bind1st(std::mem_fun(&RLConnection::write_bulk),connection));
    std::vector<std::string>::iterator it=keys.begin();
    while(it!=keys.end())connection->write_bulk(*it++);
}



