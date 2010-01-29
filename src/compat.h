/*
 *  compat.h
 *
 *  ShowEQ Distributed under GPL
 *  http://www.sourceforge.net/projects/seq
 *
 */

#ifndef __COMPAT_H__
#define __COMPAT_H__

#define _WINDOWS

#if defined(_WINDOWS) || defined (Q_OS_WIN) || defined(_MSV_VER)


#include <windows.h>
#include <math.h>
#include <time.h>

typedef short in_port_t;
typedef int in_addr_t;


// closest int away from 0
long int lroundf(float x);
long int lrint(double x);
long int lrintf(float x);

typedef unsigned   char  uint8_t;
typedef   signed   char   int8_t;

typedef unsigned   short uint16_t;
typedef   signed   short  int16_t;

typedef unsigned   int   uint32_t;
typedef   signed   int    int32_t;

typedef unsigned __int64 uint64_t;
typedef   signed __int64  int64_t;

#else

#include "config.h"

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

// other (presumably linux)
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#if defined (__GLIBC__) && (__GLIBC__ < 2)
#error "Need glibc 2.1.3 or better"
#endif

#if (defined(__FreeBSD__) || defined(__linux__)) && defined(__GLIBC__) && (__GLIBC__ == 2) && (__GLIBC_MINOR__ < 2)
typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;
#endif

#include <netinet/in.h>
#include <netinet/ip.h>

#if defined(__linux__)
#define __FAVOR_BSD
#endif

#include <netinet/udp.h>

#ifdef __FAVOR_BSD
#undef __FAVOR_BSD
#endif

#include <arpa/inet.h>

#include <netdb.h>
#include <netinet/if_ether.h>


#endif


#endif	// __COMPAT_H__
