/*-*- c++ -*-
 *
 * rl_request.cpp
 * author : KDr2
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>

#include <algorithm>

#include "rl_util.h"
#include "rl_server.h"
#include "rl_connection.h"
#include "rl_request.h"

extern char *optarg;

RLServer *server=NULL;

int main(int argc, char** argv) {
    int daemon_flag = 0, ch;

    int opt_host=0;
    char hostaddr[64];
    memset(hostaddr,0,64);

    int port=8323;
    int db_num=0;
    
    RLRequest::init_cmd_map();
    
    char data_dir[128];
    memset(data_dir,0,128);
    strncpy(data_dir,"redis.db",8);
  
    while ((ch = getopt(argc, argv, "hdH:P:D:M:")) != -1) {
        switch (ch) {
        case 'h':
            printf("Usage:\n\t./redis-leveldb [options]\n");
            printf("Options:\n\t-d:\t\t daemon\n");
            printf("\t-H host-ip:\t listen host\n");
            printf("\t-P port:\t listen port\n");
            printf("\t-D data-dir:\t data dir\n");
            printf("\t-M number:\t DB count(run in multi-db mode)\n");
            exit(0);
        case 'd':
            daemon_flag = 1;
            break;
        case 'H':
            strcpy(hostaddr,optarg);
            opt_host=1;
            break;
        case 'P':
            port=(int)strtol(optarg, (char **)NULL, 10);
            if(!port){
                printf("Bad port(-P) value\n");
                exit(1);
            }
            break;
        case 'D':
            strcpy(data_dir,optarg);
            break;
        case 'M':
            if(std::find_if(optarg,optarg+strlen(optarg),
                            std::not1(std::ptr_fun(isdigit)))!=optarg+strlen(optarg)){
                printf("Bad DB count(-M) value(must be a num in range [1,%d])\n", MAX_DBCOUNT);
                exit(1);
            }
            db_num=strtol(optarg,NULL,10);
            if(db_num<1 || db_num>MAX_DBCOUNT){
                printf("Bad DB count(-M) value(must be a num in range [1,%d])\n", MAX_DBCOUNT);
                exit(1);
            }
            break;
        case '?':
            //if(optopt=='H' || optopt=='P' || optopt=='D' || optopt=='M')
            exit(1);
            break;
        default:
            break;
        }
    }
  
    if(daemon_flag){
        if(daemon_init() == -1) { 
            printf("can't run as daemon\n"); 
            exit(1);
        }
    }
  
    signal(SIGTERM, sig_term);
    signal(SIGINT,  sig_term);
    signal(SIGPIPE, SIG_IGN);
    
    if(opt_host){
        server = new RLServer(data_dir, hostaddr, port, db_num);
        server->start();
    }else{
        server = new RLServer(data_dir, "", port, db_num);
        server->start();
    }

    return 0;
}

