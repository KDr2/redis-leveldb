/*-*- c++ -*-
 *
 * rl_request.cpp
 * author : KDr2
 *
 */

#include <assert.h>
#include <sys/socket.h>

#include "rl_util.h"
#include "rl_server.h"
#include "rl_connection.h"
#include "rl_request.h"


RLRequest::RLRequest(RLConnection *c):
    connection(c), arg_count(0), name("")
{
}

RLRequest::~RLRequest()
{
    //TODO
}

void RLRequest::run()
{
    
}



