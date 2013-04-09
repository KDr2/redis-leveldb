/*-*- c++ -*-
 *
 * rl_set.cpp
 * author : KDr2
 *
 */

#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>

#include <gmp.h>

#include "rl.h"
#include "rl_util.h"
#include "rl_server.h"
#include "rl_connection.h"
#include "rl_request.h"


void RLRequest::rl_incr(){

    size_t out_size = 0;
    char* err = 0;
    char* out = leveldb_get(connection->server->db[connection->db_index], connection->server->read_options,
                            args[0].data(), args[0].size(), &out_size, &err);

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

    leveldb_put(connection->server->db[connection->db_index], connection->server->write_options,
                args[0].data(), args[0].size(),
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

    char* out = leveldb_get(connection->server->db[connection->db_index], connection->server->read_options,
                            args[0].data(), args[0].size(), &out_size, &err);

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

    leveldb_put(connection->server->db[connection->db_index], connection->server->write_options,
                args[0].data(), args[0].size(),
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

    char* out = leveldb_get(connection->server->db[connection->db_index], connection->server->read_options,
                            args[0].data(), args[0].size(), &out_size, &err);

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

    leveldb_put(connection->server->db[connection->db_index], connection->server->write_options,
                args[0].data(), args[0].size(),
                args[1].data(), args[1].size(), &err);

    if(err) puts(err);

    connection->write_status("OK");
}

void RLRequest::rl_del(){

    if(args.size()!=1){
        connection->write_error("ERR wrong number of arguments for 'del' command");
        return;
    }

    size_t out_size = 0;
    char* err = 0;
    char* out = leveldb_get(connection->server->db[connection->db_index], connection->server->read_options,
                args[0].data(), args[0].size(), &out_size, &err);
    if(err) {
        puts(err);
        out = 0;
    }

    if(!out) {
        connection->write_integer("0", 1);
    } else {
        leveldb_delete(connection->server->db[connection->db_index], connection->server->write_options,
            args[0].data(), args[0].size(), &err);
        connection->write_integer("1", 1);
        free(out);
    }
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

        char* out = leveldb_get(connection->server->db[connection->db_index], connection->server->read_options,
                                it->data(), it->size(), &out_size, &err);

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
        leveldb_put(connection->server->db[connection->db_index], connection->server->write_options,
                    args[i].data(), args[i].size(),
                    args[i+1].data(), args[i+1].size(), &err);
        if(err) puts(err);
    }
    connection->write_status("OK");
}
