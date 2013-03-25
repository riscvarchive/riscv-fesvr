// See LICENSE for license details.

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
#include <stdexcept>
#include "memif.h"
#include "htif.h"

void memif_t::read(addr_t addr, size_t len, void* bytes)
{
  size_t align = htif->chunk_align();
  if (len && (addr & (align-1)))
  {
    size_t this_len = std::min(len, align - size_t(addr & (align-1)));
    uint8_t chunk[align];

    htif->read_chunk(addr & ~(align-1), align, chunk);
    memcpy(bytes, chunk + (addr & (align-1)), this_len);

    bytes = (char*)bytes + this_len;
    addr += this_len;
    len -= this_len;
  }

  if (len & (align-1))
  {
    size_t this_len = len & (align-1);
    size_t start = len - this_len;
    uint8_t chunk[align];

    htif->read_chunk(addr + start, align, chunk);
    memcpy((char*)bytes + start, chunk, this_len);

    len -= this_len;
  }

  // now we're aligned
  for (size_t pos = 0; pos < len; pos += htif->chunk_max_size())
    htif->read_chunk(addr + pos, std::min(htif->chunk_max_size(), len - pos), (char*)bytes + pos);
}

void memif_t::write(addr_t addr, size_t len, const void* bytes)
{
  size_t align = htif->chunk_align();
  if (len && (addr & (align-1)))
  {
    size_t this_len = std::min(len, align - size_t(addr & (align-1)));
    uint8_t chunk[align];

    htif->read_chunk(addr & ~(align-1), align, chunk);
    memcpy(chunk + (addr & (align-1)), bytes, this_len);
    htif->write_chunk(addr & ~(align-1), align, chunk);

    bytes = (char*)bytes + this_len;
    addr += this_len;
    len -= this_len;
  }

  if (len & (align-1))
  {
    size_t this_len = len & (align-1);
    size_t start = len - this_len;
    uint8_t chunk[align];

    htif->read_chunk(addr + start, align, chunk);
    memcpy(chunk, (char*)bytes + start, this_len);
    htif->write_chunk(addr + start, align, chunk);

    len -= this_len;
  }

  // now we're aligned
  size_t max_chunk = htif->chunk_max_size();
  for (size_t pos = 0; pos < len; pos += max_chunk)
    htif->write_chunk(addr + pos, std::min(max_chunk, len - pos), (char*)bytes + pos);
}

#define MEMIF_READ_FUNC \
  if(addr & (sizeof(val)-1)) \
    throw std::runtime_error("misaligned address"); \
  this->read(addr, sizeof(val), &val); \
  return val

#define MEMIF_WRITE_FUNC \
  if(addr & (sizeof(val)-1)) \
    throw std::runtime_error("misaligned address"); \
  this->write(addr, sizeof(val), &val)

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
