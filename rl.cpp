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

#include "rl_util.h"
#include "rl_server.h"
#include "rl_connection.h"
#include "rl_request.h"

extern char *optarg;


int main(int argc, char** argv) {
  int daemon_flag = 0, ch;

  int opt_host=0;
  char hostaddr[64];
  memset(hostaddr,0,64);

  int port=8323;

  char data_dir[128];
  memset(data_dir,0,128);
  strncpy(data_dir,"redis.db",8);
  
  while ((ch = getopt(argc, argv, "hdH:P:D:")) != -1) {
    switch (ch) {
    case 'h':
      printf("Usage:\n\t./redis-leveldb [options]\n");
      printf("Options:\n\t-d:\t\t daemon\n");
      printf("\t-H host-ip:\t listen host\n");
      printf("\t-P port:\t listen port\n");
      printf("\t-D data-dir:\t data dir\n");
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
    default:
      break;
    }
  }
  
  if(daemon_flag){
    if(daemon_init() == -1) { 
      printf("can't fork self\n"); 
      exit(0);
    }
  }
  
  signal(SIGTERM, sig_term); /* arrange to catch the signal */
  signal(SIGPIPE, SIG_IGN);
  while(1) {
    if(opt_host){
        RLServer s(data_dir, hostaddr, port);
        s.start();
    }else{
        RLServer s(data_dir, "", port);
        s.start();
    }
  } 
  return 0;
}

