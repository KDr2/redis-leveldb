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

    /** the db opers **/
    void rl_dummy();

    void rl_select();

    void rl_get();
    void rl_set();
    void rl_del();

    void rl_mget();
    void rl_mset();

    void rl_incr();
    void rl_incrby();

    void rl_multi();
    void rl_exec();
    void rl_discard();

    void rl_sadd();
    void rl_srem();
    void rl_scard();
    void rl_smembers();

    void rl_keys();

    void rl_info();

};


#endif
