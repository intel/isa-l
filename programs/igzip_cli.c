/**********************************************************************
  Copyright(c) 2011-2018 Intel Corporation All rights reserved.

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

#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <utime.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdarg.h>
#include "igzip_lib.h"		/* Normally you use isa-l.h instead for external programs */

#if defined (HAVE_THREADS)
# include <pthread.h>
# include "crc.h"
#endif

#if !defined (VERSION)
# if defined (ISAL_VERSION)
#  define VERSION ISAL_VERSION
# else
#  define VERSION "unknown version"
# endif
#endif

#define BAD_OPTION 1
#define BAD_LEVEL 1
#define FILE_EXISTS 0
#define MALLOC_FAILED -1
#define FILE_OPEN_ERROR -2
#define FILE_READ_ERROR -3
#define FILE_WRITE_ERROR -4

#define BUF_SIZE 1024
#define BLOCK_SIZE (1024 * 1024)

#define MAX_FILEPATH_BUF 4096

#define UNIX 3

#define NAME_DEFAULT 0
#define NO_NAME 1
#define YES_NAME 2

#define NO_TEST 0
#define TEST 1

#define LEVEL_DEFAULT 2
#define DEFAULT_SUFFIX_LEN 3
char *default_suffixes[] = { ".gz", ".z" };
int default_suffixes_lens[] = { 3, 2 };

char stdin_file_name[] = "-";
int stdin_file_name_len = 1;

enum compression_modes {
	COMPRESS_MODE,
	DECOMPRESS_MODE
};

enum long_only_opt_val {
	RM
};

enum log_types {
	INFORM,
	WARN,
	ERROR,
	VERBOSE
};

int level_size_buf[10] = {
#ifdef ISAL_DEF_LVL0_DEFAULT
	ISAL_DEF_LVL0_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL1_DEFAULT
	ISAL_DEF_LVL1_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL2_DEFAULT
	ISAL_DEF_LVL2_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL3_DEFAULT
	ISAL_DEF_LVL3_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL4_DEFAULT
	ISAL_DEF_LVL4_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL5_DEFAULT
	ISAL_DEF_LVL5_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL6_DEFAULT
	ISAL_DEF_LVL6_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL7_DEFAULT
	ISAL_DEF_LVL7_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL8_DEFAULT
	ISAL_DEF_LVL8_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL9_DEFAULT
	ISAL_DEF_LVL9_DEFAULT,
#else
	0,
#endif
};

struct cli_options {
	char *infile_name;
	size_t infile_name_len;
	char *outfile_name;
	size_t outfile_name_len;
	char *suffix;
	size_t suffix_len;
	int level;
	int mode;
	int use_stdout;
	int remove;
	int force;
	int quiet_level;
	int verbose_level;
	int name;
	int test;
	int threads;
	uint8_t *in_buf;
	uint8_t *out_buf;
	uint8_t *level_buf;
	size_t in_buf_size;
	size_t out_buf_size;
	size_t level_buf_size;
};

struct cli_options global_options;

void init_options(struct cli_options *options)
{
	options->infile_name = NULL;
	options->infile_name_len = 0;
	options->outfile_name = NULL;
	options->outfile_name_len = 0;
	options->suffix = NULL;
	options->suffix_len = 0;
	options->level = LEVEL_DEFAULT;
	options->mode = COMPRESS_MODE;
	options->use_stdout = false;
	options->remove = false;
	options->force = false;
	options->quiet_level = 0;
	options->verbose_level = 0;
	options->name = NAME_DEFAULT;
	options->test = NO_TEST;
	options->in_buf = NULL;
	options->out_buf = NULL;
	options->level_buf = NULL;
	options->in_buf_size = 0;
	options->out_buf_size = 0;
	options->level_buf_size = 0;
	options->threads = 1;
};

int is_interactive(void)
{
	int ret;
	ret = !global_options.force && !global_options.quiet_level && isatty(fileno(stdin));
	return ret;
}

size_t get_filesize(FILE * fp)
{
	size_t file_size;
	fpos_t pos, pos_curr;

	fgetpos(fp, &pos_curr);	/* Save current position */
#if defined(_WIN32) || defined(_WIN64)
	_fseeki64(fp, 0, SEEK_END);
#else
	fseeko(fp, 0, SEEK_END);
#endif
	fgetpos(fp, &pos);
	file_size = *(size_t *)&pos;
	fsetpos(fp, &pos_curr);	/* Restore position */

	return file_size;
}

uint32_t get_posix_filetime(FILE * fp)
{
	struct stat file_stats;
	fstat(fileno(fp), &file_stats);
	return file_stats.st_mtime;
}

uint32_t set_filetime(char *file_name, uint32_t posix_time)
{
	struct utimbuf new_time;
	new_time.actime = posix_time;
	new_time.modtime = posix_time;
	return utime(file_name, &new_time);
}

void log_print(int log_type, char *format, ...)
{
	va_list args;
	va_start(args, format);

	switch (log_type) {
	case INFORM:
		vfprintf(stdout, format, args);
		break;
	case WARN:
		if (global_options.quiet_level <= 0)
			vfprintf(stderr, format, args);
		break;
	case ERROR:
		if (global_options.quiet_level <= 1)
			vfprintf(stderr, format, args);
		break;
	case VERBOSE:
		if (global_options.verbose_level > 0)
			vfprintf(stderr, format, args);
		break;
	}

	va_end(args);
}

int usage(int exit_code)
{
	int log_type = exit_code ? WARN : INFORM;
	log_print(log_type,
		  "Usage: igzip [options] [infiles]\n\n"
		  "Options:\n"
		  " -h, --help           help, print this message\n"
		  " -#                   use compression level # with 0 <= # <= %d\n"
		  " -o  <file>           output file\n"
		  " -c, --stdout         write to stdout\n"
		  " -d, --decompress     decompress file\n"
		  " -z, --compress       compress file (default)\n"
		  " -f, --force          overwrite output without prompting\n"
		  "     --rm             remove source files after successful (de)compression\n"
		  " -k, --keep           keep source files (default)\n"
		  " -S, --suffix <.suf>  suffix to use while (de)compressing\n"
		  " -V, --version        show version number\n"
		  " -v, --verbose        verbose mode\n"
		  " -N, --name           save/use file name and timestamp in compress/decompress\n"
		  " -n, --no-name        do not save/use file name and timestamp in compress/decompress\n"
		  " -t, --test           test compressed file integrity\n"
		  " -T, --threads <n>    use n threads to compress if enabled\n"
		  " -q, --quiet          suppress warnings\n\n"
		  "with no infile, or when infile is - , read standard input\n\n",
		  ISAL_DEF_MAX_LEVEL);

	exit(exit_code);
}

void print_version(void)
{
	log_print(INFORM, "igzip command line interface %s\n", VERSION);
}

void *malloc_safe(size_t size)
{
	void *ptr = NULL;
	if (size == 0)
		return ptr;

	ptr = malloc(size);
	if (ptr == NULL) {
		log_print(ERROR, "igzip: Failed to allocate memory\n");
		exit(MALLOC_FAILED);
	}

	return ptr;
}

FILE *fopen_safe(char *file_name, char *mode)
{
	FILE *file;
	int answer = 0, tmp;

	/* Assumes write mode always starts with w */
	if (mode[0] == 'w') {
		if (access(file_name, F_OK) == 0) {
			log_print(WARN, "igzip: %s already exists;", file_name);
			if (is_interactive()) {
				log_print(WARN, " do you wish to overwrite (y/n)?");
				answer = getchar();

				tmp = answer;
				while (tmp != '\n' && tmp != EOF)
					tmp = getchar();

				if (answer != 'y' && answer != 'Y') {
					log_print(WARN, "       not overwritten\n");
					return NULL;
				}
			} else if (!global_options.force) {
				log_print(WARN, "       not overwritten\n");
				return NULL;
			}

			if (access(file_name, W_OK) != 0) {
				log_print(ERROR, "igzip: %s: Permission denied\n", file_name);
				return NULL;
			}
		}
	}

	/* Assumes read mode always starts with r */
	if (mode[0] == 'r') {
		if (access(file_name, F_OK) != 0) {
			log_print(ERROR, "igzip: %s does not exist\n", file_name);
			return NULL;
		}

		if (access(file_name, R_OK) != 0) {
			log_print(ERROR, "igzip: %s: Permission denied\n", file_name);
			return NULL;
		}
	}

	file = fopen(file_name, mode);
	if (!file) {
		log_print(ERROR, "igzip: Failed to open %s\n", file_name);
		return NULL;
	}

	return file;
}

size_t fread_safe(void *buf, size_t word_size, size_t buf_size, FILE * in, char *file_name)
{
	size_t read_size;
	read_size = fread(buf, word_size, buf_size, in);
	if (ferror(in)) {
		log_print(ERROR, "igzip: Error encountered while reading file %s\n",
			  file_name);
		exit(FILE_READ_ERROR);
	}
	return read_size;
}

size_t fwrite_safe(void *buf, size_t word_size, size_t buf_size, FILE * out, char *file_name)
{
	size_t write_size;
	write_size = fwrite(buf, word_size, buf_size, out);
	if (ferror(out)) {
		log_print(ERROR, "igzip: Error encountered while writing to file %s\n",
			  file_name);
		exit(FILE_WRITE_ERROR);
	}
	return write_size;
}

void open_in_file(FILE ** in, char *infile_name)
{
	*in = NULL;
	if (infile_name == NULL)
		*in = stdin;
	else
		*in = fopen_safe(infile_name, "rb");
}

void open_out_file(FILE ** out, char *outfile_name)
{
	*out = NULL;
	if (global_options.use_stdout)
		*out = stdout;
	else if (outfile_name != NULL)
		*out = fopen_safe(outfile_name, "wb");
	else if (!isatty(fileno(stdout)) || global_options.force)
		*out = stdout;
	else {
		log_print(WARN, "igzip: No output location. Use -c to output to terminal\n");
		exit(FILE_OPEN_ERROR);
	}
}

#if defined(HAVE_THREADS)

#define MAX_THREADS 8
#define MAX_JOBQUEUE 16		/* must be a power of 2 */

enum job_status {
	JOB_UNALLOCATED = 0,
	JOB_ALLOCATED,
	JOB_SUCCESS,
	JOB_FAIL
};

struct thread_job {
	uint8_t *next_in;
	uint32_t avail_in;
	uint8_t *next_out;
	uint32_t avail_out;
	uint32_t total_out;
	uint32_t type;
	uint32_t status;
};
struct thread_pool {
	pthread_t threads[MAX_THREADS];
	struct thread_job job[MAX_JOBQUEUE];
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int head;
	int tail;
	int queue;
	int shutdown;
};

// Globals for thread pool
struct thread_pool pool;

static inline int pool_has_space()
{
	return ((pool.head + 1) % MAX_JOBQUEUE) != pool.tail;
}

static inline int pool_has_work()
{
	return (pool.queue != pool.head);
}

int pool_get_work()
{
	assert(pool.queue != pool.head);
	pool.queue = (pool.queue + 1) % MAX_JOBQUEUE;
	return pool.queue;
}

int pool_put_work(struct isal_zstream *stream)
{
	pthread_mutex_lock(&pool.mutex);
	if (!pool_has_space() || pool.shutdown) {
		pthread_mutex_unlock(&pool.mutex);
		return 1;
	}
	int idx = (pool.head + 1) % MAX_JOBQUEUE;
	pool.job[idx].next_in = stream->next_in;
	pool.job[idx].avail_in = stream->avail_in;
	pool.job[idx].next_out = stream->next_out;
	pool.job[idx].avail_out = stream->avail_out;
	pool.job[idx].status = JOB_ALLOCATED;
	pool.job[idx].type = stream->end_of_stream == 0 ? 0 : 1;
	pool.head = idx;
	pthread_cond_signal(&pool.cond);
	pthread_mutex_unlock(&pool.mutex);
	return 0;
}

void *thread_worker(void *none)
{
	struct isal_zstream wstream;
	int check;
	int work_idx;
	int level = global_options.level;
	int level_size = level_size_buf[level];
	uint8_t *level_buf = malloc_safe(level_size);
	log_print(VERBOSE, "Start worker\n");

	while (!pool.shutdown) {
		pthread_mutex_lock(&pool.mutex);
		while (!pool_has_work() && !pool.shutdown) {
			pthread_cond_wait(&pool.cond, &pool.mutex);
		}
		if (pool.shutdown) {
			pthread_mutex_unlock(&pool.mutex);
			continue;
		}

		work_idx = pool_get_work();
		pthread_cond_signal(&pool.cond);
		pthread_mutex_unlock(&pool.mutex);

		isal_deflate_stateless_init(&wstream);
		wstream.next_in = pool.job[work_idx].next_in;
		wstream.next_out = pool.job[work_idx].next_out;
		wstream.avail_in = pool.job[work_idx].avail_in;
		wstream.avail_out = pool.job[work_idx].avail_out;
		wstream.end_of_stream = pool.job[work_idx].type;
		wstream.flush = FULL_FLUSH;
		wstream.level = global_options.level;
		wstream.level_buf = level_buf;
		wstream.level_buf_size = level_size;

		check = isal_deflate_stateless(&wstream);
		log_print(VERBOSE, "Worker finished job %d, out=%d\n",
			  work_idx, wstream.total_out);

		pool.job[work_idx].total_out = wstream.total_out;
		pool.job[work_idx].status = JOB_SUCCESS + check;	// complete or fail
		if (check)
			break;
	}
	free(level_buf);
	log_print(VERBOSE, "Worker quit\n");
	pthread_exit(NULL);
}

int pool_create()
{
	int i;
	int nthreads = global_options.threads - 1;
	pool.head = 0;
	pool.tail = 0;
	pool.queue = 0;
	pool.shutdown = 0;
	for (i = 0; i < nthreads; i++)
		pthread_create(&pool.threads[i], NULL, thread_worker, NULL);

	log_print(VERBOSE, "Created %d pool threads\n", nthreads);
	return 0;
}

void pool_quit()
{
	int i;
	pthread_mutex_lock(&pool.mutex);
	pool.shutdown = 1;
	pthread_mutex_unlock(&pool.mutex);
	pthread_cond_broadcast(&pool.cond);
	for (i = 0; i < global_options.threads - 1; i++)
		pthread_join(pool.threads[i], NULL);
	log_print(VERBOSE, "Deleted %d pool threads\n", i);
}

#endif // defined(HAVE_THREADS)

int compress_file(void)
{
	FILE *in = NULL, *out = NULL;
	unsigned char *inbuf = NULL, *outbuf = NULL, *level_buf = NULL;
	size_t inbuf_size, outbuf_size;
	int level_size = 0;
	struct isal_zstream stream;
	struct isal_gzip_header gz_hdr;
	int ret, success = 0;

	char *infile_name = global_options.infile_name;
	char *outfile_name = global_options.outfile_name;
	char *allocated_name = NULL;
	char *suffix = global_options.suffix;
	size_t infile_name_len = global_options.infile_name_len;
	size_t outfile_name_len = global_options.outfile_name_len;
	size_t suffix_len = global_options.suffix_len;

	int level = global_options.level;

	if (suffix == NULL) {
		suffix = default_suffixes[0];
		suffix_len = default_suffixes_lens[0];
	}

	if (infile_name_len == stdin_file_name_len &&
	    infile_name != NULL &&
	    memcmp(infile_name, stdin_file_name, infile_name_len) == 0) {
		infile_name = NULL;
		infile_name_len = 0;
	}

	if (outfile_name == NULL && infile_name != NULL && !global_options.use_stdout) {
		outfile_name_len = infile_name_len + suffix_len;
		allocated_name = malloc_safe(outfile_name_len + 1);
		outfile_name = allocated_name;
		strncpy(outfile_name, infile_name, infile_name_len + 1);
		strncat(outfile_name, suffix, outfile_name_len + 1);
	}

	open_in_file(&in, infile_name);
	if (in == NULL)
		goto compress_file_cleanup;

	if (infile_name_len != 0 && infile_name_len == outfile_name_len
	    && infile_name != NULL && outfile_name != NULL
	    && strncmp(infile_name, outfile_name, infile_name_len) == 0) {
		log_print(ERROR, "igzip: Error input and output file names must differ\n");
		goto compress_file_cleanup;
	}

	open_out_file(&out, outfile_name);
	if (out == NULL)
		goto compress_file_cleanup;

	inbuf_size = global_options.in_buf_size;
	outbuf_size = global_options.out_buf_size;

	inbuf = global_options.in_buf;
	outbuf = global_options.out_buf;
	level_size = global_options.level_buf_size;
	level_buf = global_options.level_buf;

	isal_gzip_header_init(&gz_hdr);
	if (global_options.name == NAME_DEFAULT || global_options.name == YES_NAME) {
		gz_hdr.time = get_posix_filetime(in);
		gz_hdr.name = infile_name;
	}
	gz_hdr.os = UNIX;
	gz_hdr.name_buf_len = infile_name_len + 1;

	isal_deflate_init(&stream);
	stream.avail_in = 0;
	stream.flush = NO_FLUSH;
	stream.level = level;
	stream.level_buf = level_buf;
	stream.level_buf_size = level_size;
	stream.gzip_flag = IGZIP_GZIP_NO_HDR;
	stream.next_out = outbuf;
	stream.avail_out = outbuf_size;

	isal_write_gzip_header(&stream, &gz_hdr);

	if (global_options.threads > 1) {
#if defined(HAVE_THREADS)
		int q;
		int end_of_stream = 0;
		uint32_t crc = 0;
		uint64_t total_in = 0;

		// Write the header
		fwrite_safe(outbuf, 1, stream.total_out, out, outfile_name);

		do {
			size_t nread;
			size_t inbuf_used = 0;
			size_t outbuf_used = 0;
			uint8_t *iptr = inbuf;
			uint8_t *optr = outbuf;

			for (q = 0; q < MAX_JOBQUEUE - 1; q++) {
				inbuf_used += BLOCK_SIZE;
				outbuf_used += 2 * BLOCK_SIZE;
				if (inbuf_used > inbuf_size || outbuf_used > outbuf_size)
					break;

				nread = fread_safe(iptr, 1, BLOCK_SIZE, in, infile_name);
				crc = crc32_gzip_refl(crc, iptr, nread);
				end_of_stream = feof(in);
				total_in += nread;
				stream.next_in = iptr;
				stream.next_out = optr;
				stream.avail_in = nread;
				stream.avail_out = 2 * BLOCK_SIZE;
				stream.end_of_stream = end_of_stream;
				ret = pool_put_work(&stream);
				if (ret || end_of_stream)
					break;

				iptr += BLOCK_SIZE;
				optr += 2 * BLOCK_SIZE;
			}

			while (pool.tail != pool.head) {	// Unprocessed jobs
				int t = (pool.tail + 1) % MAX_JOBQUEUE;
				if (pool.job[t].status >= JOB_SUCCESS) {	// Finished next
					if (pool.job[t].status > JOB_SUCCESS) {
						success = 0;
						log_print(ERROR,
							  "igzip: Error encountered while compressing file %s\n",
							  infile_name);
						goto compress_file_cleanup;
					}
					fwrite_safe(pool.job[t].next_out, 1,
						    pool.job[t].total_out, out, outfile_name);

					pool.job[t].total_out = 0;
					pool.job[t].status = 0;
					pool.tail = t;
					pthread_cond_broadcast(&pool.cond);
				}
				// Pick up a job while we wait
				pthread_mutex_lock(&pool.mutex);
				if (!pool_has_work()) {
					pthread_mutex_unlock(&pool.mutex);
					continue;
				}

				int work_idx = pool_get_work();
				pthread_cond_signal(&pool.cond);
				pthread_mutex_unlock(&pool.mutex);

				isal_deflate_stateless_init(&stream);
				stream.next_in = pool.job[work_idx].next_in;
				stream.next_out = pool.job[work_idx].next_out;
				stream.avail_in = pool.job[work_idx].avail_in;
				stream.avail_out = pool.job[work_idx].avail_out;
				stream.end_of_stream = pool.job[work_idx].type;
				stream.flush = FULL_FLUSH;
				stream.level = global_options.level;
				stream.level_buf = level_buf;
				stream.level_buf_size = level_size;
				int check = isal_deflate_stateless(&stream);
				log_print(VERBOSE, "Self   finished job %d, out=%d\n",
					  work_idx, stream.total_out);
				pool.job[work_idx].total_out = stream.total_out;
				pool.job[work_idx].status = JOB_SUCCESS + check;	// complete or fail
			}
		} while (!end_of_stream);

		// Write gzip trailer
		fwrite_safe(&crc, sizeof(uint32_t), 1, out, outfile_name);
		fwrite_safe(&total_in, sizeof(uint32_t), 1, out, outfile_name);

#else // No compiled threading support but asked for threads > 1
		assert(1);
#endif
	} else {		// Single thread
		do {
			if (stream.avail_in == 0) {
				stream.next_in = inbuf;
				stream.avail_in =
				    fread_safe(stream.next_in, 1, inbuf_size, in, infile_name);
				stream.end_of_stream = feof(in);
			}

			if (stream.next_out == NULL) {
				stream.next_out = outbuf;
				stream.avail_out = outbuf_size;
			}

			ret = isal_deflate(&stream);

			if (ret != ISAL_DECOMP_OK) {
				log_print(ERROR,
					  "igzip: Error encountered while compressing file %s\n",
					  infile_name);
				goto compress_file_cleanup;
			}

			fwrite_safe(outbuf, 1, stream.next_out - outbuf, out, outfile_name);
			stream.next_out = NULL;

		} while (!feof(in) || stream.avail_out == 0);
	}

	success = 1;

      compress_file_cleanup:
	if (out != NULL && out != stdout)
		fclose(out);

	if (in != NULL && in != stdin) {
		fclose(in);
		if (success && global_options.remove)
			remove(infile_name);
	}

	if (allocated_name != NULL)
		free(allocated_name);

	return (success == 0);
}

int decompress_file(void)
{
	FILE *in = NULL, *out = NULL;
	unsigned char *inbuf = NULL, *outbuf = NULL;
	size_t inbuf_size, outbuf_size;
	struct inflate_state state;
	struct isal_gzip_header gz_hdr;
	const int terminal = 0, implicit = 1, stripped = 2;
	int ret = 0, success = 0, outfile_type = terminal;

	char *infile_name = global_options.infile_name;
	char *outfile_name = global_options.outfile_name;
	char *allocated_name = NULL;
	char *suffix = global_options.suffix;
	size_t infile_name_len = global_options.infile_name_len;
	size_t outfile_name_len = global_options.outfile_name_len;
	size_t suffix_len = global_options.suffix_len;
	int suffix_index = 0;
	uint32_t file_time;

	if (infile_name_len == stdin_file_name_len &&
	    infile_name != NULL &&
	    memcmp(infile_name, stdin_file_name, infile_name_len) == 0) {
		infile_name = NULL;
		infile_name_len = 0;
	}

	if (outfile_name == NULL && !global_options.use_stdout) {
		if (infile_name != NULL) {
			outfile_type = stripped;
			while (suffix_index <
			       sizeof(default_suffixes) / sizeof(*default_suffixes)) {
				if (suffix == NULL) {
					suffix = default_suffixes[suffix_index];
					suffix_len = default_suffixes_lens[suffix_index];
					suffix_index++;
				}

				outfile_name_len = infile_name_len - suffix_len;
				if (infile_name_len >= suffix_len
				    && memcmp(infile_name + outfile_name_len, suffix,
					      suffix_len) == 0)
					break;
				suffix = NULL;
				suffix_len = 0;
			}

			if (suffix == NULL && global_options.test == NO_TEST) {
				log_print(ERROR, "igzip: %s: unknown suffix -- ignored\n",
					  infile_name);
				return 1;
			}
		}
		if (global_options.name == YES_NAME) {
			outfile_name_len = 0;
			outfile_type = implicit;
		}
		if (outfile_type != terminal) {
			allocated_name = malloc_safe(outfile_name_len >=
						     MAX_FILEPATH_BUF ? outfile_name_len +
						     1 : MAX_FILEPATH_BUF);
			outfile_name = allocated_name;
		}
	}

	open_in_file(&in, infile_name);
	if (in == NULL)
		goto decompress_file_cleanup;

	file_time = get_posix_filetime(in);

	inbuf_size = global_options.in_buf_size;
	outbuf_size = global_options.out_buf_size;
	inbuf = global_options.in_buf;
	outbuf = global_options.out_buf;

	isal_gzip_header_init(&gz_hdr);
	if (outfile_type == implicit) {
		gz_hdr.name = outfile_name;
		gz_hdr.name_buf_len = MAX_FILEPATH_BUF;
	}

	isal_inflate_init(&state);
	state.crc_flag = ISAL_GZIP_NO_HDR_VER;
	state.next_in = inbuf;
	state.avail_in = fread_safe(state.next_in, 1, inbuf_size, in, infile_name);

	ret = isal_read_gzip_header(&state, &gz_hdr);
	if (ret != ISAL_DECOMP_OK) {
		log_print(ERROR, "igzip: Error invalid gzip header found for file %s\n",
			  infile_name);
		goto decompress_file_cleanup;
	}

	if (outfile_type == implicit)
		file_time = gz_hdr.time;

	if (outfile_name != NULL && infile_name != NULL
	    && (outfile_type == stripped
		|| (outfile_type == implicit && outfile_name[0] == 0))) {
		outfile_name_len = infile_name_len - suffix_len;
		memcpy(outfile_name, infile_name, outfile_name_len);
		outfile_name[outfile_name_len] = 0;
	}

	if (infile_name_len != 0 && infile_name_len == outfile_name_len
	    && infile_name != NULL && outfile_name != NULL
	    && strncmp(infile_name, outfile_name, infile_name_len) == 0) {
		log_print(ERROR, "igzip: Error input and output file names must differ\n");
		goto decompress_file_cleanup;
	}

	if (global_options.test == NO_TEST) {
		open_out_file(&out, outfile_name);
		if (out == NULL)
			goto decompress_file_cleanup;
	}

	do {
		if (state.avail_in == 0) {
			state.next_in = inbuf;
			state.avail_in =
			    fread_safe(state.next_in, 1, inbuf_size, in, infile_name);
		}

		state.next_out = outbuf;
		state.avail_out = outbuf_size;

		ret = isal_inflate(&state);
		if (ret != ISAL_DECOMP_OK) {
			log_print(ERROR,
				  "igzip: Error encountered while decompressing file %s\n",
				  infile_name);
			goto decompress_file_cleanup;
		}

		if (out != NULL)
			fwrite_safe(outbuf, 1, state.next_out - outbuf, out, outfile_name);

	} while (!feof(in) || state.avail_out == 0);

	if (state.block_state != ISAL_BLOCK_FINISH)
		log_print(ERROR, "igzip: Error %s does not contain a complete gzip file\n",
			  infile_name);
	else
		success = 1;

      decompress_file_cleanup:
	if (out != NULL && out != stdout) {
		fclose(out);
		if (success)
			set_filetime(outfile_name, file_time);
	}

	if (in != NULL && in != stdin) {
		fclose(in);
		if (success && global_options.remove)
			remove(infile_name);
	}

	if (allocated_name != NULL)
		free(allocated_name);

	return (success == 0);
}

int main(int argc, char *argv[])
{
	int c;
	char optstring[] = "hcdz0123456789o:S:kfqVvNntT:";
	int long_only_flag;
	int ret = 0;
	int bad_option = 0;
	int bad_level = 0;
	int bad_c = 0;

	struct option long_options[] = {
		{"help", no_argument, NULL, 'h'},
		{"stdout", no_argument, NULL, 'c'},
		{"to-stdout", no_argument, NULL, 'c'},
		{"compress", no_argument, NULL, 'z'},
		{"decompress", no_argument, NULL, 'd'},
		{"uncompress", no_argument, NULL, 'd'},
		{"keep", no_argument, NULL, 'k'},
		{"rm", no_argument, &long_only_flag, RM},
		{"suffix", no_argument, NULL, 'S'},
		{"fast", no_argument, NULL, '1'},
		{"best", no_argument, NULL, '0' + ISAL_DEF_MAX_LEVEL},
		{"force", no_argument, NULL, 'f'},
		{"quiet", no_argument, NULL, 'q'},
		{"version", no_argument, NULL, 'V'},
		{"verbose", no_argument, NULL, 'v'},
		{"no-name", no_argument, NULL, 'n'},
		{"name", no_argument, NULL, 'N'},
		{"test", no_argument, NULL, 't'},
		{"threads", required_argument, NULL, 'T'},
		/* Possible future extensions
		   {"recursive, no_argument, NULL, 'r'},
		   {"list", no_argument, NULL, 'l'},
		   {"benchmark", optional_argument, NULL, 'b'},
		   {"benchmark_end", required_argument, NULL, 'e'},
		 */
		{0, 0, 0, 0}
	};

	init_options(&global_options);

	opterr = 0;
	while ((c = getopt_long(argc, argv, optstring, long_options, NULL)) != -1) {
		if (c >= '0' && c <= '9') {
			if (c > '0' + ISAL_DEF_MAX_LEVEL)
				bad_level = 1;
			else
				global_options.level = c - '0';

			continue;
		}

		switch (c) {
		case 0:
			switch (long_only_flag) {
			case RM:
				global_options.remove = true;
				break;
			default:
				bad_option = 1;
				bad_c = c;
				break;
			}
			break;
		case 'o':
			global_options.outfile_name = optarg;
			global_options.outfile_name_len = strlen(global_options.outfile_name);
			break;
		case 'c':
			global_options.use_stdout = true;
			break;
		case 'z':
			global_options.mode = COMPRESS_MODE;
			break;
		case 'd':
			global_options.mode = DECOMPRESS_MODE;
			break;
		case 'S':
			global_options.suffix = optarg;
			global_options.suffix_len = strlen(global_options.suffix);
			break;
		case 'k':
			global_options.remove = false;
			break;
		case 'f':
			global_options.force = true;
			break;
		case 'q':
			global_options.quiet_level++;
			break;
		case 'v':
			global_options.verbose_level++;
			break;
		case 'V':
			print_version();
			return 0;
		case 'N':
			global_options.name = YES_NAME;
			break;
		case 'n':
			global_options.name = NO_NAME;
			break;
		case 't':
			global_options.test = TEST;
			global_options.mode = DECOMPRESS_MODE;
			break;
		case 'T':
#if defined(HAVE_THREADS)
			c = atoi(optarg);
			c = c > MAX_THREADS ? MAX_THREADS : c;
			c = c < 1 ? 1 : c;
			global_options.threads = c;
#endif
			break;
		case 'h':
			usage(0);
		default:
			bad_option = 1;
			bad_c = optopt;
			break;
		}
	}

	if (bad_option) {
		log_print(ERROR, "igzip: invalid option ");
		if (bad_c)
			log_print(ERROR, "-%c\n", bad_c);

		else
			log_print(ERROR, ("\n"));

		usage(BAD_OPTION);
	}

	if (bad_level) {
		log_print(ERROR, "igzip: invalid compression level\n");
		usage(BAD_LEVEL);
	}

	if (global_options.outfile_name && optind < argc - 1) {
		log_print(ERROR,
			  "igzip: An output file may be specified with only one input file\n");
		return 0;
	}

	global_options.in_buf_size = BLOCK_SIZE;
	global_options.out_buf_size = BLOCK_SIZE;

#if defined(HAVE_THREADS)
	if (global_options.threads > 1) {
		global_options.in_buf_size += (BLOCK_SIZE * MAX_JOBQUEUE);
		global_options.out_buf_size += (BLOCK_SIZE * MAX_JOBQUEUE * 2);
		pool_create();
	}
#endif
	global_options.in_buf = malloc_safe(global_options.in_buf_size);
	global_options.out_buf = malloc_safe(global_options.out_buf_size);
	global_options.level_buf_size = level_size_buf[global_options.level];
	global_options.level_buf = malloc_safe(global_options.level_buf_size);

	if (global_options.mode == COMPRESS_MODE) {
		if (optind >= argc)
			ret |= compress_file();
		while (optind < argc) {
			global_options.infile_name = argv[optind];
			global_options.infile_name_len = strlen(global_options.infile_name);
			ret |= compress_file();
			optind++;
		}

	} else if (global_options.mode == DECOMPRESS_MODE) {
		if (optind >= argc)
			ret |= decompress_file();
		while (optind < argc) {
			global_options.infile_name = argv[optind];
			global_options.infile_name_len = strlen(global_options.infile_name);
			ret |= decompress_file();
			optind++;
		}
	}
#if defined(HAVE_THREADS)
	if (global_options.threads > 1)
		pool_quit();
#endif

	free(global_options.in_buf);
	free(global_options.out_buf);
	free(global_options.level_buf);
	return ret;
}
