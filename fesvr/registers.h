#ifndef __REGISTER
#define __REGISTER

#define R_COREID            10
#define R_RESET             29
#define R_ROUTE_TABLE       32
#define R_CHIPID            33
#define R_COHERENCE_ENABLE  34
#define R_ADDR_TABLE        35
#define R_OFF_CHIP_NODE     36

#define ROUTE_TABLE_REQ(node,payload)   (((uint64_t)node << 56) | payload)

#endif
