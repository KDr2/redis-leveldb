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

    char *out = 0;
    char *err = 0;
    size_t out_size = 0;

    string key = _encode_hash_key(hname, args[1]);

    out = leveldb_get(connection->server->db[connection->db_index], connection->server->read_options,
          key.data(), key.size(), &out_size, &err);
    if(err) {
        puts(err);
        err = 0;
        out = 0;
        //TODO
    }

    if(!out) {
        connection->write_nil();
    } else {
        connection->write_bulk(out, out_size);
        free(out);
    }
}



void RLRequest::rl_hset(){
    if(args.size()!=3){
        connection->write_error("ERR wrong number of arguments for 'hset' command");
        return;
    }
    string &hname = args[0];
    uint32_t new_mem = 0;

    char *out = 0;
    char *err = 0;
    size_t out_size = 0;

    string sizekey = _encode_compdata_size_key(hname, CompDataType::HASH);
    string key = _encode_hash_key(hname, args[1]);

    out = leveldb_get(connection->server->db[connection->db_index], connection->server->read_options,
          key.data(), key.size(), &out_size, &err);
    if(err) {
        puts(err);
        err = 0;
        out = 0;
        //TODO
    }

    // set even exists
    leveldb_put(connection->server->db[connection->db_index], connection->server->write_options,
        key.data(), key.size(), args[2].data(), args[2].size(), &err);
    if(err){
        puts(err);
        err = 0;
    }else{
        if(!out) ++new_mem;
    }
    if(out){
        free(out);
        out = 0;
    }

    if(new_mem == 0){
        connection->write_integer("0", 1);
        return;
    }
    // update size!
    out = leveldb_get(connection->server->db[connection->db_index], connection->server->read_options,
          sizekey.data(), sizekey.size(), &out_size, &err);

    if(err) {
        puts(err);
        err = 0;
        out = 0;
    }

    mpz_t delta;
    mpz_init(delta);
    mpz_set_ui(delta, new_mem);

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
    mpz_set_str(old_v, str_oldv, 10);
    free(str_oldv);
    mpz_add(old_v, old_v, delta);
    char *str_newv=mpz_get_str(NULL, 10, old_v);
    char *str_delta=mpz_get_str(NULL, 10, delta);
    mpz_clear(delta);
    mpz_clear(old_v);

    leveldb_put(connection->server->db[connection->db_index], connection->server->write_options,
        sizekey.data(), sizekey.size(), str_newv, strlen(str_newv), &err);

    free(str_newv);
    if(err) {
        connection->write_error(err);
        return;
    }

    connection->write_integer(str_delta, strlen(str_delta));
    free(str_delta);
}


void RLRequest::rl_hsetnx(){
    if(args.size()!=3){
        connection->write_error("ERR wrong number of arguments for 'hsetnx' command");
        return;
    }
    string &hname = args[0];
    uint32_t new_mem = 0;

    char *out = 0;
    char *err = 0;
    size_t out_size = 0;

    string sizekey = _encode_compdata_size_key(hname, CompDataType::HASH);
    string key = _encode_hash_key(hname, args[1]);

    out = leveldb_get(connection->server->db[connection->db_index], connection->server->read_options,
          key.data(), key.size(), &out_size, &err);
    if(err) {
        puts(err);
        err = 0;
        out = 0;
        //TODO
    }

    if(!out) {
        // set value
        leveldb_put(connection->server->db[connection->db_index], connection->server->write_options,
            key.data(), key.size(), args[2].data(), args[2].size(), &err);
        if(err){
            puts(err);
            err = 0;
        }else{
            ++new_mem;
        }
    } else {
        free(out);
        out = 0;
    }

    if(new_mem == 0){
        connection->write_integer("0", 1);
        return;
    }
    // update size!
    out = leveldb_get(connection->server->db[connection->db_index], connection->server->read_options,
          sizekey.data(), sizekey.size(), &out_size, &err);

    if(err) {
        puts(err);
        err = 0;
        out = 0;
    }

    mpz_t delta;
    mpz_init(delta);
    mpz_set_ui(delta, new_mem);

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
    mpz_set_str(old_v, str_oldv, 10);
    free(str_oldv);
    mpz_add(old_v, old_v, delta);
    char *str_newv=mpz_get_str(NULL, 10, old_v);
    char *str_delta=mpz_get_str(NULL, 10, delta);
    mpz_clear(delta);
    mpz_clear(old_v);

    leveldb_put(connection->server->db[connection->db_index], connection->server->write_options,
        sizekey.data(), sizekey.size(), str_newv, strlen(str_newv), &err);

    free(str_newv);
    if(err) {
        connection->write_error(err);
        return;
    }

    connection->write_integer(str_delta, strlen(str_delta));
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

    char *out = 0;
    char *err = 0;
    size_t out_size = 0;

    for(uint32_t i=1; i<args.size(); i++){
        string key = _encode_hash_key(hname, args[i]);

        out = leveldb_get(connection->server->db[connection->db_index], connection->server->read_options,
              key.data(), key.size(), &out_size, &err);
        if(err) {
            puts(err);
            err = 0;
            out = 0;
            continue;
        }

        if(!out) {
            // not exist
        } else {
            free(out);
            out = 0;
            // delete value
            leveldb_delete(connection->server->db[connection->db_index], connection->server->write_options,
                key.data(), key.size(), &err);
            if(err){
                puts(err);
                err = 0;
            }else{
                ++del_mem;
            }
        }
    }
    if(del_mem == 0){
        connection->write_integer("0", 1);
        return;
    }
    // update size!
    out = leveldb_get(connection->server->db[connection->db_index], connection->server->read_options,
          sizekey.data(), sizekey.size(), &out_size, &err);

    if(err) {
        puts(err);
        err = 0;
        out = 0;
    }

    mpz_t delta;
    mpz_init(delta);
    mpz_set_ui(delta, del_mem);

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
    mpz_set_str(old_v, str_oldv, 10);
    free(str_oldv);
    mpz_sub(old_v, old_v, delta);
    char *str_newv=mpz_get_str(NULL, 10, old_v);
    char *str_delta=mpz_get_str(NULL, 10, delta);
    mpz_clear(delta);
    mpz_clear(old_v);

    leveldb_put(connection->server->db[connection->db_index], connection->server->write_options,
        sizekey.data(), sizekey.size(), str_newv, strlen(str_newv), &err);

    free(str_newv);
    if(err) {
        connection->write_error(err);
        return;
    }

    connection->write_integer(str_delta, strlen(str_delta));
    free(str_delta);
}

void RLRequest::rl_hexists(){
    if(args.size()<2){
        connection->write_error("ERR wrong number of arguments for 'hexists' command");
        return;
    }
    string &hname = args[0];

    char *out = 0;
    char *err = 0;
    size_t out_size = 0;

    string key = _encode_hash_key(hname, args[1]);

    out = leveldb_get(connection->server->db[connection->db_index], connection->server->read_options,
          key.data(), key.size(), &out_size, &err);
    if(err) {
        puts(err);
        out = 0;
    }

    if(!out) {
        // not a member
        connection->write_integer("0", 1);
    } else {
        free(out);
        // is a member
        connection->write_integer("1", 1);
    }
}

void RLRequest::rl_hgetall(){
    if(args.size()!=1){
        connection->write_error("ERR wrong number of arguments for 'hgetall' command");
        return;
    }

    std::vector<std::string> kvs;
    const char *key, *val;
    size_t key_len, val_len;

    string hash_begin = _encode_hash_key(args[0], "");
    leveldb_iterator_t *kit = leveldb_create_iterator(connection->server->db[connection->db_index],
                              connection->server->read_options);
    leveldb_iter_seek(kit, hash_begin.data(), hash_begin.size());

    while(leveldb_iter_valid(kit)) {
        key = leveldb_iter_key(kit, &key_len);
        val = leveldb_iter_value(kit, &val_len);
        if(strncmp(hash_begin.data(), key,  hash_begin.size()) == 0){
            const string &k = _decode_key(std::string(key, key_len));
            if(k.size()>0){
                kvs.push_back(k);
                kvs.push_back(std::string(val, val_len));
            }
            leveldb_iter_next(kit);
        }else{
            break;
        }
    }
    leveldb_iter_destroy(kit);

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
    const char *key;
    size_t key_len;

    string hash_begin = _encode_hash_key(args[0], "");
    leveldb_iterator_t *kit = leveldb_create_iterator(connection->server->db[connection->db_index],
                              connection->server->read_options);
    leveldb_iter_seek(kit, hash_begin.data(), hash_begin.size());

    while(leveldb_iter_valid(kit)) {
        key = leveldb_iter_key(kit, &key_len);
        if(strncmp(hash_begin.data(), key,  hash_begin.size()) == 0){
            const string &k = _decode_key(std::string(key, key_len));
            if(k.size()>0){
                keys.push_back(k);
            }
            leveldb_iter_next(kit);
        }else{
            break;
        }
    }
    leveldb_iter_destroy(kit);

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
    const char *key, *val;
    size_t key_len, val_len;

    string hash_begin = _encode_hash_key(args[0], "");
    leveldb_iterator_t *kit = leveldb_create_iterator(connection->server->db[connection->db_index],
                              connection->server->read_options);
    leveldb_iter_seek(kit, hash_begin.data(), hash_begin.size());

    while(leveldb_iter_valid(kit)) {
        key = leveldb_iter_key(kit, &key_len);
        val = leveldb_iter_value(kit, &val_len);
        if(strncmp(hash_begin.data(), key,  hash_begin.size()) == 0){
            const string &k = _decode_key(std::string(key, key_len));
            if(k.size()>0){
                //vals.push_back(k);
                vals.push_back(std::string(val, val_len));
            }
            leveldb_iter_next(kit);
        }else{
            break;
        }
    }
    leveldb_iter_destroy(kit);

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
    size_t out_size = 0;
    char* err = 0;
    char* out = leveldb_get(connection->server->db[connection->db_index], connection->server->read_options,
                sizekey.data(), sizekey.size(), &out_size, &err);

    if(err) {
        puts(err);
        out = 0;
    }

    if(!out) {
        connection->write_nil();
    } else {
        connection->write_integer(out, out_size);
        free(out);
    }
}
