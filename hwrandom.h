#ifndef _HWRANDOM_H

#define _HWRANDOM_H 1

#include <pthread.h>

#include "options.h"

#ifndef RND_DIGEST_ALGO
#define RND_DIGEST_ALGO GCRY_MD_SHA512
#endif

#ifndef RND_DIGEST_ALGO_SMALL
#define RND_DIGEST_ALGO_SMALL GCRY_MD_SHA512
#endif

#ifndef DEVRANDOM
#define DEVRANDOM "/dev/random"
#endif

#ifndef DEVURANDOM
#define DEVURANDOM "/dev/urandom"
#endif

typedef struct
  {
	const char *devname;
	int fd;

	pthread_mutex_t *mutex;
  } hwrandom_t;

hwrandom_t *hwrandom_init (const char *devname);
void hwrandom_destroy(hwrandom_t *rnd);
char *hwrandom_hash_buffer(const char *buffer, size_t count);
void hwrandom_add_entropy (hwrandom_t *rnd, const char *buffer, size_t count);
ssize_t hwrandom_read (hwrandom_t *rnd, void *buf, size_t count);
ssize_t hwrandom_write (hwrandom_t *rnd, const void *buf, const size_t count);
int hwrandom_open(hwrandom_t *rnd, int flags);
char *hwrandom_bytes(hwrandom_t *rnd, size_t count);

#endif /* _HWRANDOM_H */

