/*-*- c++ -*-
 *
 * rl_request.h
 * author : KDr2
 *
 */

#ifndef _RL_REUQEST_H_INCLUDED
#define _RL_REQUEST_H_INCLUDED

#include <vector>
#include <map>
#include <string>

class RLConnection;

class RLRequest{
public:

    typedef void (RLRequest::*COMMAND)();

    RLConnection *connection;
    int32_t arg_count;
    std::string name;
    std::vector<std::string> args;
    std::vector<RLRequest*> subrequest; /* for MULTI */

    /**method**/
    RLRequest(RLConnection *c);
    ~RLRequest();
    void append_arg(std::string arg);
    void _run();
    void run();
    bool completed(){return arg_count>=0 && arg_count-args.size()==0;}

    static std::map<std::string,COMMAND> cmd_map;
    static void init_cmd_map();

    /** sys commands **/
    void rl_dummy();
    void rl_select();
    void rl_keys();
    void rl_info();

    void rl_multi();
    void rl_exec();
    void rl_discard();

    /* kv commands */
    void rl_get();
    void rl_set();
    void rl_del();

    void rl_mget();
    void rl_mset();

    void rl_incr();
    void rl_incrby();

    /* set commands */
    void rl_sadd();
    void rl_srem();
    void rl_scard();
    void rl_smembers();
    void rl_sismember();

    /* hash commands */
    void rl_hget();
    void rl_hset();
    void rl_hsetnx();
    void rl_hdel();
    void rl_hexists();
    void rl_hgetall();
    void rl_hkeys();
    void rl_hvals();
    void rl_hlen();

    /* list commands */
    void rl_lpush();
    void rl_lpop();
    void rl_rpush();
    void rl_rpop();
};

#define RL_GET(key_data, key_size, out_size, err) leveldb_get(connection->server->db[connection->db_index], \
												connection->server->read_options,key_data, key_size, &out_size, &err)

#define RL_SET(key_data, key_size, value_data, value_size, err) leveldb_put(connection->server->db[connection->db_index], \
								connection->server->write_options, key_data, key_size, value_data, value_size, &err)

#define RL_DEL(key_data, key_size, err) leveldb_delete(connection->server->db[connection->db_index], \
								connection->server->write_options, key_data, key_size, &err)
#endif
