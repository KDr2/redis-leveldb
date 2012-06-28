/*-*- c++ -*-
 *
 * rl_request.h
 * author : KDr2
 *
 */

#ifndef _RL_REUQEST_H_INCLUDED
#define _RL_REQUEST_H_INCLUDED

#include <vector>
#include <string>

class RLConnection;

class RLRequest{
public:
    RLConnection *connection;
    uint8_t arg_count;
    std::string name;
    std::vector<std::string> args;
    std::vector<RLRequest*> subrequest; /* for MULTI */
    
    /**method**/
    RLRequest(RLConnection *c);
    ~RLRequest();
    void append_arg(std::string arg);
    void run();
    bool completed(){return arg_count==args.size();}
};


#endif

