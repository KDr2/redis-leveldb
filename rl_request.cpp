/*-*- c++ -*-
 *
 * rl_request.cpp
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

std::map<std::string,RLRequest::COMMAND> RLRequest::cmd_map;

void RLRequest::init_cmd_map()
{
    /* sys commands */
    RLRequest::cmd_map["info"]       = &RLRequest::rl_info;
    RLRequest::cmd_map["keys"]       = &RLRequest::rl_keys;
    RLRequest::cmd_map["select"]     = &RLRequest::rl_select;
    RLRequest::cmd_map["multi"]      = &RLRequest::rl_multi;
    RLRequest::cmd_map["exec"]       = &RLRequest::rl_exec;
    RLRequest::cmd_map["discard"]    = &RLRequest::rl_discard;

    /* kv commands */
    RLRequest::cmd_map["incr"]       = &RLRequest::rl_incr;
    RLRequest::cmd_map["incrby"]     = &RLRequest::rl_incrby;
    RLRequest::cmd_map["get"]        = &RLRequest::rl_get;
    RLRequest::cmd_map["set"]        = &RLRequest::rl_set;
    RLRequest::cmd_map["del"]        = &RLRequest::rl_del;
    RLRequest::cmd_map["mget"]       = &RLRequest::rl_mget;
    RLRequest::cmd_map["mset"]       = &RLRequest::rl_mset;

    /* set commands */
    RLRequest::cmd_map["sadd"]       = &RLRequest::rl_sadd;
    RLRequest::cmd_map["srem"]       = &RLRequest::rl_srem;
    RLRequest::cmd_map["scard"]      = &RLRequest::rl_scard;
    RLRequest::cmd_map["smembers"]   = &RLRequest::rl_smembers;
    RLRequest::cmd_map["sismember"]  = &RLRequest::rl_sismember;

    /* hash commands */
    RLRequest::cmd_map["hget"]       = &RLRequest::rl_hget;
    RLRequest::cmd_map["hset"]       = &RLRequest::rl_hset;
    RLRequest::cmd_map["hsetnx"]     = &RLRequest::rl_hsetnx;
    RLRequest::cmd_map["hdel"]       = &RLRequest::rl_hdel;
    RLRequest::cmd_map["hexists"]    = &RLRequest::rl_hexists;
    RLRequest::cmd_map["hgetall"]    = &RLRequest::rl_hgetall;
    RLRequest::cmd_map["hkeys"]      = &RLRequest::rl_hkeys;
    RLRequest::cmd_map["hvals"]      = &RLRequest::rl_hvals;
    RLRequest::cmd_map["hlen"]       = &RLRequest::rl_hlen;
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

#ifdef DEBUG
        printf("Subrequest Number in this Transaction:%ld\n",
               connection->transaction->subrequest.size());
#endif

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


void RLRequest::rl_select(){
    if(connection->server->db_num<1){
        connection->write_error("ERR redis-leveldb is running in single-db mode");
        return;
    }
    if(args.size()!=1){
        connection->write_error("ERR wrong number of arguments for 'select' command");
        return;
    }
    if(std::find_if(args[0].begin(),args[0].end(),
                    std::not1(std::ptr_fun(isdigit)))!=args[0].end()){
        connection->write_error("ERR argument for 'select' must be a number");
        return;
    }
    int target=strtol(args[0].c_str(),NULL,10);
    if(target<0||target>=connection->server->db_num){
        connection->write_error("ERR invalid DB index");
        return;
    }
    connection->db_index=target;
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
        leveldb_iterator_t *kit = leveldb_create_iterator(connection->server->db[connection->db_index],
                                                          connection->server->read_options);
        const char *key;
        size_t key_len;
        leveldb_iter_seek_to_first(kit);
        while(leveldb_iter_valid(kit)) {
            key = leveldb_iter_key(kit, &key_len);
            if(allkeys || stringmatchlen(args[0].c_str(), arg_len, key, key_len, 0)){
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



void RLRequest::rl_info(){

    if(args.size()>1){
        connection->write_error("ERR wrong number of arguments for 'info' command");
        return;
    }

    std::ostringstream info;
    char *out=NULL;

    info << "redis_version: redis-leveldb " VERSION_STR "\r\n";
    info << "mode: ";
    if(connection->server->db_num<1){
        info << "single\r\n";
    }else{
        info << "multi = " << connection->server->db_num;
        info << "," << connection->db_index << "\r\n";
    }
    char *cwd=getcwd(NULL,0);
    info << "work_dir: " << cwd << "\r\n";
    free(cwd);
    info << "data_path: " << connection->server->db_path << "\r\n";
    info << "clients_num: " << connection->server->clients_num << "\r\n";

    out=leveldb_property_value(connection->server->db[connection->db_index],"leveldb.stats");
    if(out){
        info<< "stats: " << out <<"\r\n";
        free(out);
    }

    /* kyes num */
    if(args.size()>0 && args[0].find('k')!=std::string::npos){
        uint64_t key_num=0;
        leveldb_iterator_t *kit = leveldb_create_iterator(connection->server->db[connection->db_index],
                                                          connection->server->read_options);

        leveldb_iter_seek_to_first(kit);
        while(leveldb_iter_valid(kit)){
            key_num++;
            leveldb_iter_next(kit);
        }
        leveldb_iter_destroy(kit);
        info<< "keys: " << key_num <<"\r\n";
    }

    /** sstables info */
    if(args.size()>0 && args[0].find('t')!=std::string::npos){
        out=leveldb_property_value(connection->server->db[connection->db_index],"leveldb.sstables");
        if(out){
            info<< "sstables:\r\n" << out <<"\r\n";
            free(out);
        }
    }
    connection->write_bulk(info.str());
}
