/*
 * rl_list.cpp
 *
 *  Created on: 2013-5-19
 *      Author: imessi
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


#include "rl.h"
#include "rl_util.h"
#include "rl_server.h"
#include "rl_connection.h"
#include "rl_request.h"
#include "rl_compdata.h"

void RLRequest::rl_lpush(){
    if(args.size()<2){
        connection->write_error("ERR wrong number of arguments for 'lpush' command");
        return;
    }

    string &lname = args[0];
    char *out = 0;
    int64_t flag_index;
    char flag_index_s[32];
    string flag_key = _encode_hash_key(lname, "left");
    char *err = 0;
    size_t out_size = 0;
    size_t args_size = args.size();

    //get list left
    out = RL_GET(flag_key.data(), flag_key.size(), out_size, err);
    if(err) {
        //TODO
    }

    if(!out) {
        string key = _encode_hash_key(lname, "right");
        out = RL_GET(key.data(), key.size(), out_size, err);
        if(err) {
            //TODO
        }

        if(out) {
            long r_index;
            char *temp = (char *)malloc(out_size + 1);
            memcpy(temp, out, out_size);
            temp[out_size] = 0;
            r_index = atoll(temp);
            free(temp);
            free(out);
            flag_index = r_index - 1;
        } else {
            flag_index = 0;
        }

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
        sprintf(flag_index_s, "%lld", flag_index);
        string key = _encode_list_key(lname, flag_index_s);

        RL_SET(key.data(), key.size(), args[i].data(), args[i].size(), err);
        if(err) {
           //TODO:
        }

        if(i + 1 <  args_size) {
            flag_index--;
        }
    }

    // update left flag
    sprintf(flag_index_s, "%lld", flag_index);
    RL_SET(flag_key.data(), flag_key.size(), flag_index_s, strlen(flag_index_s), err);
    if(err) {
        //TODO:
        connection->write_error(err);
        return;
    }

    sprintf(flag_index_s, "%lu", (unsigned long)(args_size - 1));
    connection->write_integer(flag_index_s, strlen(flag_index_s));
}

void RLRequest::rl_rpush(){
    if(args.size()<2){
        connection->write_error("ERR wrong number of arguments for 'rpush' command");
        return;
    }

    string &lname = args[0];
    char *out = 0;
    int64_t flag_index;
    char flag_index_s[32];
    string flag_key = _encode_hash_key(lname, "right");
    char *err = 0;
    size_t out_size = 0;
    size_t args_size = args.size();

    //get list right
    out = RL_GET(flag_key.data(), flag_key.size(), out_size, err);
    if(err) {
        //TODO
    }

    if(!out) {
        string key = _encode_hash_key(lname, "left");
        out = RL_GET(key.data(), key.size(), out_size, err);
        if(err) {
            //TODO
        }

        if(out) {
            long l_index;
            char *temp = (char *)malloc(out_size + 1);
            memcpy(temp, out, out_size);
            temp[out_size] = 0;
            l_index = atoll(temp);
            free(temp);
            free(out);
            flag_index = l_index + 1;
        } else {
            flag_index = 0;
        }

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
        sprintf(flag_index_s, "%lld", flag_index);
        string key = _encode_list_key(lname, flag_index_s);

        RL_SET(key.data(), key.size(), args[i].data(), args[i].size(), err);
        if(err) {
           //TODO:
        }

        if(i + 1 <  args_size) {
            flag_index++;
        }
    }

    // update right flag
    sprintf(flag_index_s, "%lld", flag_index);
    RL_SET(flag_key.data(), flag_key.size(), flag_index_s, strlen(flag_index_s), err);
    if(err) {
        //TODO:
        connection->write_error(err);
        return;
    }

    sprintf(flag_index_s, "%lu", (unsigned long)(args_size - 1));
    connection->write_integer(flag_index_s, strlen(flag_index_s));
}

void RLRequest::rl_lpop(){
    if(args.size() != 1){
            connection->write_error("ERR wrong number of arguments for 'lpop' command");
            return;
    }

    string &lname = args[0];
    int64_t flag_index;
    char flag_index_s[32];
    string flag_key = _encode_hash_key(lname, "left");
    char *out = 0;
    char *err = 0;
    size_t out_size = 0;
    string key;

    //get list left index
    out = RL_GET(flag_key.data(), flag_key.size(), out_size, err);
    if(err) {
        //TODO
    }

    if(!out) {
        flag_index = 0;
    } else {
        char *temp = (char*)malloc(out_size+1);
        memcpy(temp, out, out_size);
        temp[out_size] = 0;
        flag_index = atoll(temp);
        free(temp);
        free(out);
    }

    sprintf(flag_index_s, "%lld", flag_index);
    key = _encode_list_key(lname, flag_index_s);
    out = RL_GET(key.data(), key.size(), out_size, err);
    if(err) {
        //TODO:
        connection->write_error(err);
        return;
    }

    // delete this member and update left index
    if(out) {
        sprintf(flag_index_s, "%lld", flag_index + 1);
        RL_SET(flag_key.data(), flag_key.size(),  flag_index_s, strlen(flag_index_s), err);
        if(err) {
            //TODO
        }
        connection->write_bulk(out, out_size);
        RL_DEL(key.data(), key.size(), err);
        if(err) {
            //TODO:
        }
        free(out);
    } else {
        connection->write_nil();
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
    string flag_key = _encode_hash_key(lname, "right");
    char *out = 0;
    char *err = 0;
    size_t out_size = 0;
    string key;

    //get list right index
    out = RL_GET(flag_key.data(), flag_key.size(), out_size, err);
    if(err) {
        //TODO
    }

    if(!out) {
        flag_index = 0;
    } else {
        char *temp = (char*)malloc(out_size+1);
        memcpy(temp, out, out_size);
        temp[out_size] = 0;
        flag_index = atoll(temp);
        free(temp);
        free(out);
    }

    sprintf(flag_index_s, "%lld", flag_index);
    key = _encode_list_key(lname, flag_index_s);
    out = RL_GET(key.data(), key.size(), out_size, err);
    if(err) {
        //TODO:
        connection->write_error(err);
        return;
    }

    // delete this member and update right index
    if(out) {
        sprintf(flag_index_s, "%lld", flag_index - 1);
        RL_SET(flag_key.data(), flag_key.size(),  flag_index_s, strlen(flag_index_s), err);
        if(err) {
            //TODO
        }
        connection->write_bulk(out, out_size);
        RL_DEL(key.data(), key.size(), err);
        if(err) {
            //TODO:
        }
        free(out);
    } else {
        connection->write_nil();
    }
}

