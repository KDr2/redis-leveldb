/*-*- c++ -*-
 *
 * rl_hash.cpp
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

void RLRequest::rl_hget(){
    if(args.size()!=2){
        connection->write_error("ERR wrong number of arguments for 'hget' command");
        return;
    }
    string &hname = args[0];

    string out;
    leveldb::Status status;

    string key = _encode_hash_key(hname, args[1]);

    status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, key, &out);

    if(status.IsNotFound()) {
        connection->write_nil();
    } else if(status.ok()){
        connection->write_bulk(out.data(), out.size());
    } else {
        connection->write_error("HGET ERROR 1");
    }
}



void RLRequest::rl_hset(){
    if(args.size()!=3){
        connection->write_error("ERR wrong number of arguments for 'hset' command");
        return;
    }
    string &hname = args[0];
    uint32_t new_mem = 0;

    string sizekey = _encode_compdata_size_key(hname, CompDataType::HASH);
    string key = _encode_hash_key(hname, args[1]);

    string out;
    leveldb::Status status;
    leveldb::WriteBatch write_batch;

    status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, key, &out);

    // set even exists
    write_batch.Put(key, args[2]);

    if(status.IsNotFound()){
        ++new_mem;
    }else if(!status.ok()) {
        connection->write_error("HSET ERROR 0");
        return;
    }

    if(new_mem == 0){
        status = connection->server->db[connection->db_index]->Write(
            connection->server->write_options, &write_batch);
        if(!status.ok()){
            connection->write_error("HSET ERROR 1");
        }else{
            connection->write_integer("0", 1);
        }
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
        connection->write_error("HSET ERROR 2");
        return;
    }

    mpz_t delta;
    mpz_init(delta);
    mpz_set_ui(delta, new_mem);

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
    status = connection->server->db[connection->db_index]->Write(
        connection->server->write_options, &write_batch);

    if(!status.ok()) {
        connection->write_error("HSET ERROR 3");
    } else {
        connection->write_integer(str_delta, strlen(str_delta));
    }
    free(str_newv);
    free(str_delta);
}


void RLRequest::rl_hsetnx(){
    if(args.size()!=3){
        connection->write_error("ERR wrong number of arguments for 'hsetnx' command");
        return;
    }
    string &hname = args[0];
    uint32_t new_mem = 0;

    string out;
    leveldb::Status status;
    leveldb::WriteBatch write_batch;

    string sizekey = _encode_compdata_size_key(hname, CompDataType::HASH);
    string key = _encode_hash_key(hname, args[1]);

    status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, key, &out);
    if(!status.ok() && !status.IsNotFound()) {
        connection->write_error("HSETNX ERROR 1");
        return;
    }

    if(status.IsNotFound()) {
        // set value
        write_batch.Put(key, args[2]);
        ++new_mem;
    }

    if(new_mem == 0){
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
    }else {
        connection->write_error("HSETNX ERROR 2");
    }

    mpz_t delta;
    mpz_init(delta);
    mpz_set_ui(delta, new_mem);

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
    status = connection->server->db[connection->db_index]->Write(
        connection->server->write_options, &write_batch);


    if(!status.ok()) {
        connection->write_error("HSETNX ERROR 3");
    } else {
        connection->write_integer(str_delta, strlen(str_delta));
    }
    free(str_newv);
    free(str_delta);
}

void RLRequest::rl_hdel(){
    if(args.size()<2){
        connection->write_error("ERR wrong number of arguments for 'hdel' command");
        return;
    }

    string &hname = args[0];
    string sizekey = _encode_compdata_size_key(hname, CompDataType::HASH);
    uint32_t del_mem = 0;

    string out;
    leveldb::Status status;

    for(uint32_t i=1; i<args.size(); i++){
        string key = _encode_hash_key(hname, args[i]);

        status = connection->server->db[connection->db_index]->Get(
            connection->server->read_options, key, &out);

        if(status.IsNotFound()) {
            // not exist
        } else if(status.ok()) {
            // delete value
            status = connection->server->db[connection->db_index]->Delete(
                connection->server->write_options, key);
            if(status.ok()) {
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
    if(status.ok()){
        str_oldv=(char*)malloc(out.size()+1);
        memcpy(str_oldv, out.data(), out.size());
        str_oldv[out.size()]=0;
    }else{
        connection->write_error("HDEL ERROR 1");
        return;
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
        connection->server->write_options, sizekey, str_newv);

    if(!status.ok()) {
        connection->write_error("HDEL ERROR 2");
    } else {
        connection->write_integer(str_delta, strlen(str_delta));
    }

    free(str_newv);
    free(str_delta);
}

void RLRequest::rl_hexists(){
    if(args.size()<2){
        connection->write_error("ERR wrong number of arguments for 'hexists' command");
        return;
    }

    string &hname = args[0];
    string key = _encode_hash_key(hname, args[1]);

    string out;
    leveldb::Status status;

    status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, key, &out);

    if(status.IsNotFound()) {
        // not a member
        connection->write_integer("0", 1);
    } else if(status.ok()) {
        // is a member
        connection->write_integer("1", 1);
    } else {
        connection->write_error("HEXISTS ERROR 1");
    }
}

void RLRequest::rl_hgetall(){
    if(args.size()!=1){
        connection->write_error("ERR wrong number of arguments for 'hgetall' command");
        return;
    }

    std::vector<std::string> kvs;
    leveldb::Slice key, val;

    string hash_begin = _encode_hash_key(args[0], "");
    leveldb::Iterator *kit = connection->server->db[connection->db_index]->NewIterator(
        connection->server->read_options);

    kit->Seek(hash_begin);

    while(kit->Valid()) {
        key = kit->key();
        val = kit->value();
        if(strncmp(hash_begin.data(), key.data(),  hash_begin.size()) == 0){
            const string &k = _decode_key(key.ToString());
            if(k.size()>0){
                kvs.push_back(k);
                kvs.push_back(val.ToString());
            }
            kit->Next();
        }else{
            break;
        }
    }

    delete kit;

    connection->write_mbulk_header(kvs.size());
    std::vector<std::string>::iterator it=kvs.begin();
    while(it!=kvs.end())connection->write_bulk(*it++);
}

void RLRequest::rl_hkeys(){
    if(args.size()!=1){
        connection->write_error("ERR wrong number of arguments for 'hkeys' command");
        return;
    }

    std::vector<std::string> keys;
    leveldb::Slice key;

    string hash_begin = _encode_hash_key(args[0], "");
    leveldb::Iterator *kit = connection->server->db[connection->db_index]->NewIterator(
        connection->server->read_options);

    kit->Seek(hash_begin);

    while(kit->Valid()) {
        key = kit->key();
        if(strncmp(hash_begin.data(), key.data(),  hash_begin.size()) == 0){
            const string &k = _decode_key(key.ToString());
            if(k.size()>0){
                keys.push_back(k);
            }
            kit->Next();
        }else{
            break;
        }
    }
    delete kit;

    connection->write_mbulk_header(keys.size());
    std::vector<std::string>::iterator it=keys.begin();
    while(it!=keys.end())connection->write_bulk(*it++);
}

void RLRequest::rl_hvals(){
    if(args.size()!=1){
        connection->write_error("ERR wrong number of arguments for 'hvals' command");
        return;
    }

    std::vector<std::string> vals;
    leveldb::Slice key, val;

    string hash_begin = _encode_hash_key(args[0], "");
    leveldb::Iterator *kit = connection->server->db[connection->db_index]->NewIterator(
        connection->server->read_options);
    kit->Seek(hash_begin);

    while(kit->Valid()) {
        key = kit->key();
        val = kit->value();
        if(strncmp(hash_begin.data(), key.data(),  hash_begin.size()) == 0){
            //const string &k = _decode_key(key.ToString());
            if(key.size()>0){
                //vals.push_back(k);
                vals.push_back(val.ToString());
            }
            kit->Next();
        }else{
            break;
        }
    }
    delete kit;

    connection->write_mbulk_header(vals.size());
    std::vector<std::string>::iterator it=vals.begin();
    while(it!=vals.end())connection->write_bulk(*it++);
}

void RLRequest::rl_hlen(){
    if(args.size()!=1){
        connection->write_error("ERR wrong number of arguments for 'hlen' command");
        return;
    }

    string sizekey = _encode_compdata_size_key(args[0], CompDataType::HASH);

    string out;
    leveldb::Status status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, sizekey, &out);

    if(!status.ok()) {
        connection->write_error("HLEN ERROR 1");
    } else {
        connection->write_integer(out.data(), out.size());
    }
}
