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

#ifndef __MEMIF_H
#define __MEMIF_H

#include "packet.h"
#include <stdint.h>
#include <string.h>

class htif_t;
class memif_t
{
public:
  memif_t(htif_t* _htif) : htif(_htif) {}
  virtual ~memif_t(){}

  // read and write byte arrays
  virtual void read(addr_t addr, size_t len, void* bytes);
  virtual void write(addr_t addr, size_t len, const void* bytes);

  // read and write 8-bit words
  virtual uint8_t read_uint8(addr_t addr);
  virtual int8_t read_int8(addr_t addr);
  virtual void write_uint8(addr_t addr, uint8_t val);
  virtual void write_int8(addr_t addr, int8_t val);

  // read and write 16-bit words
  virtual uint16_t read_uint16(addr_t addr);
  virtual int16_t read_int16(addr_t addr);
  virtual void write_uint16(addr_t addr, uint16_t val);
  virtual void write_int16(addr_t addr, int16_t val);

  // read and write 32-bit words
  virtual uint32_t read_uint32(addr_t addr);
  virtual int32_t read_int32(addr_t addr);
  virtual void write_uint32(addr_t addr, uint32_t val);
  virtual void write_int32(addr_t addr, int32_t val);

  // read and write 64-bit words
  virtual uint64_t read_uint64(addr_t addr);
  virtual int64_t read_int64(addr_t addr);
  virtual void write_uint64(addr_t addr, uint64_t val);
  virtual void write_int64(addr_t addr, int64_t val);

protected:
  htif_t* htif;
};

#endif // __MEMIF_H
