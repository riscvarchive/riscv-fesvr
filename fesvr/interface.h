#ifndef __INTERFACE_H
#define __INTERFACE_H

#include <stdint.h>
#include <stddef.h>

enum
{
  IF_MEM,
  IF_CREG,
};

typedef uint64_t reg_t;
typedef reg_t addr_t;

#endif // __INTERFACE_H
