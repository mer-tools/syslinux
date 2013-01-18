/* ----------------------------------------------------------------------- *
 *
 *   Copyright 2013 Intel Corporation; author: H. Peter Anvin
 *
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation
 *   files (the "Software"), to deal in the Software without
 *   restriction, including without limitation the rights to use,
 *   copy, modify, merge, publish, distribute, sublicense, and/or
 *   sell copies of the Software, and to permit persons to whom
 *   the Software is furnished to do so, subject to the following
 *   conditions:
 *
 *   The above copyright notice and this permission notice shall
 *   be included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------- */

/*
 * slzm.h
 *
 * Syslinux compressed header format
 */
#ifndef _SYSLINUX_SLZM_H
#define _SYSLINUX_SLZM_H

#include <inttypes.h>

#define SLZM_MAGIC1 0xcd4cfdb8
#define SLZM_MAGIC2 0x464c4521
/* Need to be smarter about these... */
#define SLZM_PLATFORM 1	/* PC-BIOS */
#define SLZM_ARCH     3 /* EM_386 */
#define SLZM_MINVER   0x501
#define SLZM_MAXVER   0x5ff

struct slzm_header {
    uint32_t magic[2];
    uint16_t platform;
    uint16_t arch;
    uint16_t minversion;
    uint16_t maxversion;
    uint32_t zsize;
    uint32_t usize;
    uint32_t resv[6];
};

#endif /* _SYSLINUX_SLZM_H */
