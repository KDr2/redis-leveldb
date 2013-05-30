/*
 * rl_list.cpp
 *
 *  Created on: 2013-5-19
 *      Author: imessi
 *      Author: KDr2
 */

#define __STDC_FORMAT_MACROS

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <leveldb/write_batch.h>

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
    int64_t flag_index;
    char flag_index_s[32];
    string flag_key = _encode_list_key(lname, LEFT_FLAG);
    size_t args_size = args.size();

    string out;
    leveldb::Status status;
    leveldb::WriteBatch write_batch;

    //get list left
    status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, flag_key, &out);

    if(status.IsNotFound()) {
        string key = _encode_list_key(lname, RIGHT_FLAG);
        write_batch.Put(key, "0");
        flag_index = 0;
    } else if(status.ok()){
        char *temp = (char *)malloc(out.size() + 1);
        memcpy(temp, out.data(), out.size());
        temp[out.size()] = 0;
        flag_index = atoll(temp) - 1;
        free(temp);
    } else {
        connection->write_error("LPUSH ERROR 1");
        return;
    }

    // push list members
    for (uint32_t i = 1; i < args_size; i++){
        sprintf(flag_index_s, "%"PRId64, flag_index);
        string key = _encode_list_key(lname, flag_index_s);
        write_batch.Put(key, args[i]);

        if(i + 1 <  args_size) {
            flag_index--;
        }
    }

    // update left flag
    sprintf(flag_index_s, "%"PRId64, flag_index);
    write_batch.Put(flag_key, flag_index_s);

    status = connection->server->db[connection->db_index]->Write(
        connection->server->write_options, &write_batch);

    if (!status.ok()) {
        connection->write_error("LPUSH ERROR 2");
    } else {
        sprintf(flag_index_s, "%"PRId64, (args_size - 1));
        connection->write_integer(flag_index_s, strlen(flag_index_s));
    }
}

void RLRequest::rl_rpush(){
    if(args.size() < 2){
        connection->write_error("ERR wrong number of arguments for 'rpush' command");
        return;
    }

    string &lname = args[0];
    int64_t flag_index;
    char flag_index_s[32];
    string flag_key = _encode_list_key(lname, RIGHT_FLAG);
    size_t args_size = args.size();

    string out;
    leveldb::Status status;
    leveldb::WriteBatch write_batch;

    //get list left
    status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, flag_key, &out);

    if(status.IsNotFound()) {
        string key = _encode_list_key(lname, LEFT_FLAG);
        write_batch.Put(key, "0");
        flag_index = 0;
    } else if(status.ok()) {
        char *temp = (char *)malloc(out.size() + 1);
        memcpy(temp, out.data(), out.size());
        temp[out.size()] = 0;
        flag_index = atoll(temp) + 1;
        free(temp);
    } else {
        connection->write_error("RPUSH ERROR 1");
        return;
    }

    // push list members
    for (uint32_t i = 1; i < args_size; i++){
        sprintf(flag_index_s, "%"PRId64, flag_index);
        string key = _encode_list_key(lname, flag_index_s);
        write_batch.Put(key, args[i]);
        if(i + 1 <  args_size) {
            flag_index++;
        }
    }

    // update right flag
    sprintf(flag_index_s, "%"PRId64, flag_index);
    write_batch.Put(flag_key, flag_index_s);

    status = connection->server->db[connection->db_index]->Write(
        connection->server->write_options, &write_batch);

    if (!status.ok()) {
        connection->write_error("RPUSH ERROR 2");
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

    string key;
    string out;
    leveldb::Status status;

    //get list left index
    status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, flag_key, &out);

    if(status.IsNotFound()) {
        connection->write_nil();
        return;
    } else if(status.ok()) {
        char *temp = (char*)malloc(out.size() + 1);
        memcpy(temp, out.data(), out.size());
        temp[out.size()] = 0;
        flag_index = atoll(temp);
        free(temp);
    } else {
        connection->write_error("LPOP ERROR 1");
        return;
    }

    sprintf(flag_index_s, "%"PRId64, flag_index);
    key = _encode_list_key(lname, flag_index_s);

    status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, key, &out);

    if(!status.ok()) {
        connection->write_error("LPOP ERROR 2");
        return;
    }

    leveldb::WriteBatch write_batch;
    // delete this member and update left index
    if(status.ok()) {
        string _out;
        write_batch.Delete(key);

        sprintf(flag_index_s, "%"PRId64, flag_index + 1);
        key = _encode_list_key(lname, flag_index_s);

        status = connection->server->db[connection->db_index]->Get(
            connection->server->read_options, key, &_out);

        if (status.ok()) {
            write_batch.Put(flag_key, flag_index_s);
        } else {
            // clear the list
            write_batch.Delete(flag_key);
            flag_key = _encode_list_key(lname, RIGHT_FLAG);
            write_batch.Delete(flag_key);
        }

        status = connection->server->db[connection->db_index]->Write(
            connection->server->write_options, &write_batch);
        if (!status.ok()) {
            connection->write_error("LPOP ERROR 3");
        } else {
            connection->write_bulk(out.data(), out.size());
        }

    } else {
        connection->write_error("LPOP ERROR 4");
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
    string key;

    string out;
    leveldb::Status status;

    //get list right index
    status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, flag_key, &out);

    if(status.IsNotFound()) {
        connection->write_nil();
        return;
    } else if(status.ok()) {
        char *temp = (char*)malloc(out.size() + 1);
        memcpy(temp, out.data(), out.size());
        temp[out.size()] = 0;
        flag_index = atoll(temp);
        free(temp);
    } else {
        connection->write_error("RPOP ERROR 1");
    }

    sprintf(flag_index_s, "%"PRId64, flag_index);
    key = _encode_list_key(lname, flag_index_s);
    status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, key, &out);

    if(!status.ok()) {
        connection->write_error("RPOP ERROR 2");
        return;
    }

    leveldb::WriteBatch write_batch;

    // delete this member and update right index
    if(status.ok()) {
        string _out;
        write_batch.Delete(key);

        sprintf(flag_index_s, "%"PRId64, flag_index - 1);
        key = _encode_list_key(lname, flag_index_s);
        status = connection->server->db[connection->db_index]->Get(
            connection->server->read_options, key, &_out);

        if (status.ok()) {
            write_batch.Put(flag_key, flag_index_s);
        } else {
            write_batch.Delete(flag_key);
            flag_key = _encode_list_key(lname, LEFT_FLAG);
            write_batch.Delete(flag_key);
        }

        status = connection->server->db[connection->db_index]->Write(
            connection->server->write_options, &write_batch);
        if (!status.ok()) {
            connection->write_error("RPOP ERROR 3");
        } else {
            connection->write_bulk(out.data(), out.size());
        }

    } else {
        connection->write_error("RPOP ERROR 4");
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

    string out;
    leveldb::Status status;

    string flag_key = _encode_list_key(lname, RIGHT_FLAG);

    status = connection->server->db[connection->db_index]->Get(
        connection->server->read_options, flag_key, &out);

    if(status.IsNotFound()) {
        connection->write_integer("0", 1);
        return;
    } else if(status.ok()) {
        char *temp = (char*)malloc(out.size() + 1);
        memcpy(temp, out.data(), out.size());
        temp[out.size()] = 0;
        right_index = atoll(temp);
        free(temp);

        flag_key = _encode_list_key(lname, LEFT_FLAG);
        status = connection->server->db[connection->db_index]->Get(
            connection->server->read_options, flag_key, &out);

        if (!status.ok()) {
            connection->write_error("LLEN ERROR 2");
            return;
        } else {
            char *temp = (char*)malloc(out.size() + 1);
            memcpy(temp, out.data(), out.size());
            temp[out.size()] = 0;
            left_index = atoll(temp);
            free(temp);
        }
    } else {
        connection->write_error("LLEN ERROR 1");
        return;
    }

    sprintf(len, "%"PRId64, right_index - left_index + 1);
    connection->write_integer(len, strlen(len));
}
