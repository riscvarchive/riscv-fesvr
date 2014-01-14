// See LICENSE for license details.

#ifndef _HTIF_PTHREAD_H
#define _HTIF_PTHREAD_H

// We don't use pthreads anymore, but the implementation's equivalent.
// We probably shouldn't have exposed it in the name of the class :-)
#include "htif_ucontext.h"
typedef htif_ucontext_t htif_pthread_t;

#endif
