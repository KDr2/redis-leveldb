/*
 * rl_list.cpp
 *
 *  Created on: 2013-5-19
 *      Author: imessi
 */

#define __STDC_FORMAT_MACROS

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "rl.h"
#include "rl_util.h"
#include "rl_server.h"
#include "rl_connection.h"
#include "rl_request.h"
#include "rl_compdata.h"

#define RIGHT_FLAG "right"
#define LEFT_FLAG "left"

void RLRequest::rl_lpush(){
    if(args.size()<2){
        connection->write_error("ERR wrong number of arguments for 'lpush' command");
        return;
    }

    string &lname = args[0];
    char *out = 0;
    int64_t flag_index;
    char flag_index_s[32];
    string flag_key = _encode_list_key(lname, LEFT_FLAG);
    char *err = 0;
    size_t out_size = 0;
    size_t args_size = args.size();

    leveldb_writebatch_t *write_batch = leveldb_writebatch_create();

    //get list left
    out = RL_GET(flag_key.data(), flag_key.size(), out_size, err);
    if(err) {
        connection->write_error(err);
        free(err);
        return;
    }

    if(!out) {
        string key = _encode_list_key(lname, RIGHT_FLAG);
        leveldb_writebatch_put(write_batch, key.data(), key.size(), "0", 1);
        flag_index = 0;
    } else {
        char *temp = (char *)malloc(out_size + 1);
        memcpy(temp, out, out_size);
        temp[out_size] = 0;
        flag_index = atoll(temp) - 1;
        free(temp);
        free(out);
    }

    // push list members
    for (uint32_t i = 1; i < args_size; i++){
        sprintf(flag_index_s, "%"PRId64, flag_index);
        string key = _encode_list_key(lname, flag_index_s);
        leveldb_writebatch_put(write_batch, key.data(), key.size(), args[i].data(), args[i].size());

        if(i + 1 <  args_size) {
            flag_index--;
        }
    }

    // update left flag
    sprintf(flag_index_s, "%"PRId64, flag_index);
    leveldb_writebatch_put(write_batch, flag_key.data(), flag_key.size(), flag_index_s, strlen(flag_index_s));

    leveldb_write(connection->server->db[connection->db_index], connection->server->write_options, write_batch, &err);
    if (err) {
        connection->write_error(err);
        free(err);
    } else {
        sprintf(flag_index_s, "%lu", (unsigned long)(args_size - 1));
        connection->write_integer(flag_index_s, strlen(flag_index_s));
    }
}

void RLRequest::rl_rpush(){
    if(args.size() < 2){
        connection->write_error("ERR wrong number of arguments for 'rpush' command");
        return;
    }

    string &lname = args[0];
    char *out = 0;
    int64_t flag_index;
    char flag_index_s[32];
    string flag_key = _encode_list_key(lname, RIGHT_FLAG);
    char *err = 0;
    size_t out_size = 0;
    size_t args_size = args.size();

    leveldb_writebatch_t *write_batch = leveldb_writebatch_create();

    //get list left
    out = RL_GET(flag_key.data(), flag_key.size(), out_size, err);
    if(err) {
        connection->write_error(err);
        free(err);
        return;
    }

    if(!out) {
        string key = _encode_list_key(lname, LEFT_FLAG);
        leveldb_writebatch_put(write_batch, key.data(), key.size(), "0", 1);
        flag_index = 0;
    } else {
        char *temp = (char *)malloc(out_size + 1);
        memcpy(temp, out, out_size);
        temp[out_size] = 0;
        flag_index = atoll(temp) + 1;
        free(temp);
        free(out);
    }

    // push list members
    for (uint32_t i = 1; i < args_size; i++){
        sprintf(flag_index_s, "%"PRId64, flag_index);
        string key = _encode_list_key(lname, flag_index_s);
        leveldb_writebatch_put(write_batch, key.data(), key.size(), args[i].data(), args[i].size());

        if(i + 1 <  args_size) {
            flag_index++;
        }
    }

    // update right flag
    sprintf(flag_index_s, "%"PRId64, flag_index);
    leveldb_writebatch_put(write_batch, flag_key.data(), flag_key.size(), flag_index_s, strlen(flag_index_s));

    leveldb_write(connection->server->db[connection->db_index], connection->server->write_options, write_batch, &err);
    if (err) {
        connection->write_error(err);
        free(err);
    } else {
        sprintf(flag_index_s, "%lu", (unsigned long)(args_size - 1));
        connection->write_integer(flag_index_s, strlen(flag_index_s));
    }
}

void RLRequest::rl_lpop(){
    if(args.size() != 1){
        connection->write_error("ERR wrong number of arguments for 'lpop' command");
        return;
    }

    string &lname = args[0];
    int64_t flag_index;
    char flag_index_s[32];
    string flag_key = _encode_list_key(lname, LEFT_FLAG);
    char *out = 0;
    char *err = 0;
    size_t out_size = 0;
    string key;

    leveldb_writebatch_t *write_batch = leveldb_writebatch_create();

    //get list left index
    out = RL_GET(flag_key.data(), flag_key.size(), out_size, err);
    if(err) {
        connection->write_error(err);
        free(err);
        return;
    }

    if(!out) {
        connection->write_nil();
        return;
    } else {
        char *temp = (char*)malloc(out_size + 1);
        memcpy(temp, out, out_size);
        temp[out_size] = 0;
        flag_index = atoll(temp);
        free(temp);
        free(out);
    }

    sprintf(flag_index_s, "%"PRId64, flag_index);
    key = _encode_list_key(lname, flag_index_s);
    out = RL_GET(key.data(), key.size(), out_size, err);
    if(err) {
        connection->write_error(err);
        free(err);
        return;
    }

    // delete this member and update left index
    if(out) {
        leveldb_writebatch_delete(write_batch, key.data(), key.size());

        sprintf(flag_index_s, "%"PRId64, flag_index + 1);
        key = _encode_list_key(lname, flag_index_s);
        size_t _out_size;
        char *_out = RL_GET(key.data(), key.size(), _out_size, err);
        if (_out) {
            RL_SET(flag_key.data(), flag_key.size(),  flag_index_s, strlen(flag_index_s), err);
            leveldb_writebatch_put(write_batch, flag_key.data(), flag_key.size(), flag_index_s, strlen(flag_index_s));
            free(_out);
        } else {
            leveldb_writebatch_delete(write_batch, flag_key.data(), flag_key.size());
            flag_key = _encode_list_key(lname, RIGHT_FLAG);
            leveldb_writebatch_delete(write_batch, flag_key.data(), flag_key.size());
        }

        leveldb_write(connection->server->db[connection->db_index], connection->server->write_options, write_batch, &err);
        if (err) {
            connection->write_error(err);
            free(err);
        } else {
            connection->write_bulk(out, out_size);
        }
        free(out);

    } else {
        //TODO: exception
    }
}

void RLRequest::rl_rpop(){
    if(args.size() != 1){
        connection->write_error("ERR wrong number of arguments for 'rpop' command");
        return;
    }

    string &lname = args[0];
    int64_t flag_index;
    char flag_index_s[32];
    string flag_key = _encode_list_key(lname, RIGHT_FLAG);
    char *out = 0;
    char *err = 0;
    size_t out_size = 0;
    string key;

    leveldb_writebatch_t *write_batch = leveldb_writebatch_create();

    //get list right index
    out = RL_GET(flag_key.data(), flag_key.size(), out_size, err);
    if(err) {
        connection->write_error(err);
        free(err);
        return;
    }

    if(!out) {
        connection->write_nil();
        return;
    } else {
        char *temp = (char*)malloc(out_size + 1);
        memcpy(temp, out, out_size);
        temp[out_size] = 0;
        flag_index = atoll(temp);
        free(temp);
        free(out);
    }

    sprintf(flag_index_s, "%"PRId64, flag_index);
    key = _encode_list_key(lname, flag_index_s);
    out = RL_GET(key.data(), key.size(), out_size, err);
    if(err) {
        connection->write_error(err);
        free(err);
        return;
    }

    // delete this member and update right index
    if(out) {
        leveldb_writebatch_delete(write_batch, key.data(), key.size());

        sprintf(flag_index_s, "%"PRId64, flag_index - 1);
        key = _encode_list_key(lname, flag_index_s);
        size_t _out_size;
        char *_out = RL_GET(key.data(), key.size(), _out_size, err);
        if (_out) {
            RL_SET(flag_key.data(), flag_key.size(),  flag_index_s, strlen(flag_index_s), err);
            leveldb_writebatch_put(write_batch, flag_key.data(), flag_key.size(), flag_index_s, strlen(flag_index_s));
            free(_out);
        } else {
            leveldb_writebatch_delete(write_batch, flag_key.data(), flag_key.size());
            flag_key = _encode_list_key(lname, LEFT_FLAG);
            leveldb_writebatch_delete(write_batch, flag_key.data(), flag_key.size());
        }

        leveldb_write(connection->server->db[connection->db_index], connection->server->write_options, write_batch, &err);
        if (err) {
            connection->write_error(err);
            free(err);
        } else {
            connection->write_bulk(out, out_size);
        }
        free(out);

    } else {
        //TODO: exception
    }
}


void RLRequest::rl_llen(){
    if(args.size() != 1){
        connection->write_error("ERR wrong number of arguments for 'llen' command");
        return;
    }

    string &lname = args[0];
    int64_t right_index = 0;
    int64_t left_index = 0;
    char len[32];
    char *out = 0;
    char *err = 0;
    size_t out_size = 0;
    string flag_key = _encode_list_key(lname, RIGHT_FLAG);

    out = RL_GET(flag_key.data(), flag_key.size(), out_size, err);
    if(err) {
        connection->write_error(err);
        free(err);
        return;
    }

    if(!out) {
        connection->write_integer("0", 1);
        return;
    } else {
        char *temp = (char*)malloc(out_size + 1);
        memcpy(temp, out, out_size);
        temp[out_size] = 0;
        right_index = atoll(temp);
        free(temp);
        free(out);

        flag_key = _encode_list_key(lname, LEFT_FLAG);
        out = RL_GET(flag_key.data(), flag_key.size(), out_size, err);
        if (!out) {
            //TODO: exception
        } else {
            char *temp = (char*)malloc(out_size + 1);
            memcpy(temp, out, out_size);
            temp[out_size] = 0;
            left_index = atoll(temp);
            free(temp);
            free(out);
        }
    }

    sprintf(len, "%"PRId64, right_index - left_index + 1);
    connection->write_integer(len, strlen(len));
}
