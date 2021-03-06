#ifndef _ARPA_INET_H
#define _ARPA_INET_H

#include <sys/cdefs.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>


int inet_aton(const char* cp, struct in_addr* inp) ;
unsigned long int inet_addr(const char* cp) ;
unsigned long int inet_network(const char* cp) ;
char* inet_ntoa(struct in_addr in) ;
char* inet_ntoa_r(struct in_addr in,char* buf) ;
struct in_addr inet_makeaddr(int net, int host) ;
unsigned long int inet_lnaof(struct in_addr in) ;
unsigned long int inet_netof(struct in_addr in) ;

int inet_pton (int AF, const char* CP, void* BUF) ;
const char* inet_ntop (int AF, const void* CP, char* BUF, size_t LEN) ;


#endif
