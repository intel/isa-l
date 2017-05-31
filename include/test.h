/**********************************************************************
  Copyright(c) 2011-2015 Intel Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************/

#ifndef _TEST_H
#define _TEST_H

#ifdef __cplusplus
extern "C" {
#endif

// Use sys/time.h functions for time
#if defined (__unix__) || (__APPLE__) || (__MINGW32__)
# include <sys/time.h>
#endif

#ifdef _MSC_VER
# define inline __inline
# include <time.h>
# include <Windows.h>
#endif

#include <stdio.h>
#include <stdint.h>

struct perf{
	struct timeval tv;
};


#if defined (__unix__) || (__APPLE__) || (__MINGW32__)
static inline int perf_start(struct perf *p)
{
	return gettimeofday(&(p->tv), 0);
}
static inline int perf_stop(struct perf *p)
{
	return gettimeofday(&(p->tv), 0);
}

static inline void perf_print(struct perf stop, struct perf start, long long dsize)
{
	long long secs = stop.tv.tv_sec - start.tv.tv_sec;
	long long usecs = secs * 1000000 + stop.tv.tv_usec - start.tv.tv_usec;

	printf("runtime = %10lld usecs", usecs);
	if (dsize != 0) {
#if 1 // not bug in printf for 32-bit
		printf(", bandwidth %lld MB in %.4f sec = %.2f MB/s\n", dsize/(1024*1024),
			((double) usecs)/1000000, ((double) dsize) / (double)usecs);
#else
		printf(", bandwidth %lld MB ", dsize/(1024*1024));
		printf("in %.4f sec ",(double)usecs/1000000);
		printf("= %.2f MB/s\n", (double)dsize/usecs);
#endif
	}
	else
		printf("\n");
}
#endif

static inline uint64_t get_filesize(FILE *fp)
{
	uint64_t file_size;
	fpos_t pos, pos_curr;

	fgetpos(fp, &pos_curr);  /* Save current position */
#if defined(_WIN32) || defined(_WIN64)
	_fseeki64(fp, 0, SEEK_END);
#else
	fseeko(fp, 0, SEEK_END);
#endif
	fgetpos(fp, &pos);
	file_size = *(uint64_t *)&pos;
	fsetpos(fp, &pos_curr);  /* Restore position */

	return file_size;
}

#ifdef __cplusplus
}
#endif

#endif // _TEST_H
