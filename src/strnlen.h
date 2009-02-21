/*
 * strnlen.h
 *
 *  ShowEQ Distributed under GPL
 *  http://seq.sourceforge.net/
 */

#ifndef INCLUDED_STRNLEN_PROTOS
#define INCLUDED_STRNLEN_PROTOS

#include "config.h"
#ifndef HAVE_STRNLEN

#include <string.h>

#define strnlen(string, maxlen) strlen((string))
#endif
#endif
