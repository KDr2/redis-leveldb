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

#include <leveldb/write_batch.h>

#include "rl.h"
#include "rl_util.h"
#include "rl_server.h"
#include "rl_connection.h"
#include "rl_request.h"
#include "rl_compdata.h"

void RLRequest::rl_sadd(){
    if(args.size()<2){
        connection->write_error("ERR wrong number of arguments for 'sadd' command");
        return;
    }
    string &sname = args[0];
    string sizekey = _encode_compdata_size_key(sname, CompDataType::SET);
    uint32_t new_mem = 0;

    std::string out;
    leveldb::WriteBatch write_batch;
    leveldb::Status status;

    for(uint32_t i=1; i<args.size(); i++){
        string key = _encode_set_key(sname, args[i]);

        status = connection->server->db[connection->db_index]->Get(
            connection->server->read_options, key, &out);

        if(status.IsNotFound()) {
            // set value
            write_batch.Put(key, "\1");
            ++new_mem;
        }
    }
    if(new_mem == 0){
        connection->write_integer("0", 1);
        return;
    }
    // update size!
    status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, sizekey, &out);

    mpz_t delta;
    mpz_init(delta);
    mpz_set_ui(delta, new_mem);

    char *str_oldv = NULL;
    if(status.IsNotFound()){
        str_oldv = strdup("0");
    }else if(status.ok()){
        str_oldv = (char*)malloc(out.size()+1);
        memcpy(str_oldv, out.data(), out.size());
        str_oldv[out.size()] = 0;
    }else{
        connection->write_error("SADD ERROR 1");
        return;
    }

    mpz_t old_v;
    mpz_init(old_v);
    mpz_set_str(old_v, str_oldv, 10);
    free(str_oldv);
    mpz_add(old_v, old_v, delta);
    char *str_newv=mpz_get_str(NULL, 10, old_v);
    char *str_delta=mpz_get_str(NULL, 10, delta);
    mpz_clear(delta);
    mpz_clear(old_v);

    write_batch.Put(sizekey, str_newv);

    status = connection->server->db[connection->db_index]->Write(connection->server->write_options, &write_batch);

    if(!status.ok()) {
        connection->write_error("SADD ERROR 2");
    }else{
        connection->write_integer(str_delta, strlen(str_delta));
    }
    free(str_newv);
    free(str_delta);
}

void RLRequest::rl_srem(){
    if(args.size()<2){
        connection->write_error("ERR wrong number of arguments for 'srem' command");
        return;
    }

    string &sname = args[0];
    string sizekey = _encode_compdata_size_key(sname, CompDataType::SET);
    uint32_t del_mem = 0;

    std::string out;
    leveldb::Status status;

    for(uint32_t i=1; i<args.size(); i++){
        string key = _encode_set_key(sname, args[i]);

        status = connection->server->db[connection->db_index]->Get(
            connection->server->read_options, key, &out);

        if(status.IsNotFound()) {
            // not exist
        } else if(status.ok()){

            // delete value
            status = connection->server->db[connection->db_index]->Delete(
                connection->server->write_options, key);
            if(status.ok()){
                ++del_mem;
            }
        }
    }
    if(del_mem == 0){
        connection->write_integer("0", 1);
        return;
    }
    // update size!
    status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, sizekey, &out);

    char *str_oldv=NULL;
    if(status.IsNotFound()){
        str_oldv = strdup("0");
    }else if(status.ok()){
        str_oldv=(char*)malloc(out.size()+1);
        memcpy(str_oldv, out.data(), out.size());
        str_oldv[out.size()]=0;
    }else{
        //TODO
    }

    mpz_t delta;
    mpz_init(delta);
    mpz_set_ui(delta, del_mem);

    mpz_t old_v;
    mpz_init(old_v);
    mpz_set_str(old_v, str_oldv, 10);
    free(str_oldv);
    mpz_sub(old_v, old_v, delta);
    char *str_newv=mpz_get_str(NULL, 10, old_v);
    char *str_delta=mpz_get_str(NULL, 10, delta);
    mpz_clear(delta);
    mpz_clear(old_v);

    status = connection->server->db[connection->db_index]->Put(
        connection->server->write_options,
        sizekey, str_newv);

    if(!status.ok()) {
        connection->write_error("SREM ERROR 1");
    }else{
        connection->write_integer(str_delta, strlen(str_delta));
    }
    free(str_newv);
    free(str_delta);
}

void RLRequest::rl_scard(){
    if(args.size()!=1){
        connection->write_error("ERR wrong number of arguments for 'scard' command");
        return;
    }

    string sizekey = _encode_compdata_size_key(args[0], CompDataType::SET);

    std::string out;

    leveldb::Status status  = connection->server->db[connection->db_index]->Get(
        connection->server->read_options,
        sizekey, &out);

    if(status.IsNotFound()) {
        connection->write_nil();
    } else if(status.ok()){
        connection->write_integer(out.data(), out.size());
    } else {
        connection->write_error("SCARD ERROR 1");
    }
}

void RLRequest::rl_smembers(){
    if(args.size()!=1){
        connection->write_error("ERR wrong number of arguments for 'smembers' command");
        return;
    }

    std::vector<std::string> keys;
    leveldb::Slice key;

    string set_begin = _encode_set_key(args[0], "");
    leveldb::Iterator *kit = connection->server->db[connection->db_index]->NewIterator(
        connection->server->read_options);

    kit->Seek(set_begin);

    while(kit->Valid()) {
        key = kit->key();
        if(strncmp(set_begin.data(), key.data(),  set_begin.size()) == 0){
            const string &v = _decode_key(key.ToString());
            if(v.size()>0)keys.push_back(v);
            kit->Next();
        }else{
            break;
        }
    }
    delete kit;

    connection->write_mbulk_header(keys.size());
    //std::for_each(keys.begin(), keys.end(), std::bind1st(std::mem_fun(&RLConnection::write_bulk),connection));
    std::vector<std::string>::iterator it=keys.begin();
    while(it!=keys.end())connection->write_bulk(*it++);
}

void RLRequest::rl_sismember(){
    if(args.size()<2){
        connection->write_error("ERR wrong number of arguments for 'sismember' command");
        return;
    }

    string &sname = args[0];
    string key = _encode_set_key(sname, args[1]);

    string out;
    leveldb::Status status;

    status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, key, &out);

    if(status.IsNotFound()) {
        // not a member
        connection->write_integer("0", 1);
    } else if(status.ok()){
        // is a member
        connection->write_integer("1", 1);
    }else {
        connection->write_error("SISMEMBER ERROR 1");
    }
}
