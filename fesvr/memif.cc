/* Author: Andrew S. Waterman
 *         Parallel Computing Laboratory
 *         Electrical Engineering and Computer Sciences
 *         University of California, Berkeley
 *
 * Copyright (c) 2010, The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include "memif.h"
#include "htif.h"

#define align (htif->chunk_align())

void memif_t::read(addr_t addr, size_t len, uint8_t* bytes)
{
  if((addr & (align-1)) || ((len+(addr&(align-1)))&(align-1)))
  {
    addr_t offset = addr & (align-1);
    addr_t new_addr = addr-offset;
    size_t new_len = len+offset;
    if(new_len & (align-1))
      new_len += align - (new_len & (align-1));
    uint8_t* new_bytes = (uint8_t*)malloc(new_len);

    try
    {
      read(new_addr,new_len,new_bytes);
    }
    catch(...)
    {
      free(new_bytes);
      throw;
    }

    memcpy(bytes,new_bytes+offset,len);
    free(new_bytes);
    return;
  }

  // now we're aligned
  htif->read_chunk(addr,len,bytes);
}

void memif_t::write(addr_t addr, size_t len, const uint8_t* bytes)
{
  if(bytes == NULL) // write zeros, the ugly, slow way
  {
    uint8_t* zeros = (uint8_t*)calloc(len,1);
    try
    {
      write(addr,len,zeros);
    }
    catch(...)
    {
      free(zeros);
      throw;
    }
    free(zeros);
    return;
  }

  if((addr&(align-1)) || ((len+(addr&(align-1)))&(align-1)))
  {
    addr_t offset = addr & (align-1);
    size_t end_pad = 0;
    if((len+offset) & (align-1))
      end_pad = align - ((len+offset) & (align-1));
    addr_t new_addr = addr-offset;
    size_t new_len = len + offset + end_pad;
    uint8_t* new_bytes = (uint8_t*)malloc(new_len);

    if(offset)
      htif->read_chunk(new_addr, align, new_bytes);

    if(end_pad)
      htif->read_chunk(new_addr+new_len-align, align, new_bytes+new_len-align);

    memcpy(new_bytes+offset,bytes,len);

    try
    {
      write(new_addr,new_len,new_bytes);
    }
    catch(...)
    {
      free(new_bytes);
      throw;
    }

    free(new_bytes);
    return;
  }

  // now we're basetype-aligned
  htif->write_chunk(addr,len,bytes);
}

#define MEMIF_READ_FUNC \
  if(addr & (sizeof(val)-1)) \
    throw std::runtime_error("misaligned address"); \
  if(align <= sizeof(val)) \
    htif->read_chunk(addr, sizeof(val), (uint8_t*)&val); \
  else \
  { \
    uint8_t chunk[align]; \
    htif->read_chunk(addr & ~align, align, chunk); \
    memcpy(&val, chunk + (addr & align), sizeof(val)); \
  } \
  return val

#define MEMIF_WRITE_FUNC \
  if(addr & (sizeof(val)-1)) \
    throw std::runtime_error("misaligned address"); \
  if(align <= sizeof(val)) \
    htif->write_chunk(addr, sizeof(val), (uint8_t*)&val); \
  else \
  { \
    uint8_t chunk[align]; \
    htif->read_chunk(addr & ~align, align, chunk); \
    memcpy(chunk + (addr & align), &val, sizeof(val)); \
    htif->write_chunk(addr & ~align, align, chunk); \
  } \
  return

uint8_t memif_t::read_uint8(addr_t addr)
{
  uint8_t val;
  MEMIF_READ_FUNC;
}

int8_t memif_t::read_int8(addr_t addr)
{
  int8_t val;
  MEMIF_READ_FUNC;
}

void memif_t::write_uint8(addr_t addr, uint8_t val)
{
  MEMIF_WRITE_FUNC;
}

void memif_t::write_int8(addr_t addr, int8_t val)
{
  MEMIF_WRITE_FUNC;
}

uint16_t memif_t::read_uint16(addr_t addr)
{
  uint16_t val;
  MEMIF_READ_FUNC;
}

int16_t memif_t::read_int16(addr_t addr)
{
  int16_t val;
  MEMIF_READ_FUNC;
}

void memif_t::write_uint16(addr_t addr, uint16_t val)
{
  MEMIF_WRITE_FUNC;
}

void memif_t::write_int16(addr_t addr, int16_t val)
{
  MEMIF_WRITE_FUNC;
}

uint32_t memif_t::read_uint32(addr_t addr)
{
  uint32_t val;
  MEMIF_READ_FUNC;
}

int32_t memif_t::read_int32(addr_t addr)
{
  int32_t val;
  MEMIF_READ_FUNC;
}

void memif_t::write_uint32(addr_t addr, uint32_t val)
{
  MEMIF_WRITE_FUNC;
}

void memif_t::write_int32(addr_t addr, int32_t val)
{
  MEMIF_WRITE_FUNC;
}

uint64_t memif_t::read_uint64(addr_t addr)
{
  uint64_t val;
  MEMIF_READ_FUNC;
}

int64_t memif_t::read_int64(addr_t addr)
{
  int64_t val;
  MEMIF_READ_FUNC;
}

void memif_t::write_uint64(addr_t addr, uint64_t val)
{
  MEMIF_WRITE_FUNC;
}

void memif_t::write_int64(addr_t addr, int64_t val)
{
  MEMIF_WRITE_FUNC;
}
