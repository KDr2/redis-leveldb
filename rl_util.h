/*-*- c++ -*-
 *
 * rl_util.h
 * author : KDr2
 *
 */

#ifndef _RL_UTIL_H_INCLUDED
#define _RL_UTIL_H_INCLUDED

void set_nonblock(int fd);

int daemon_init(void);

void sig_term(int signo);

int stringmatchlen(const char *pattern, int patternLen,
                   const char *string, int stringLen, int nocase);
int stringmatch(const char *pattern, const char *string, int nocase);

#endif
