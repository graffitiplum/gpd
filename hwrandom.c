#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <gcrypt.h>
#include <pthread.h>

#include <linux/types.h>
#include <linux/random.h>

#include "hwrandom.h"

hwrandom_t *
hwrandom_init (const char *devname)
#define FUNC_NAME "hwrandom_init"
  {
	hwrandom_t *rnd;

	if ( (rnd = (hwrandom_t *) malloc(sizeof(hwrandom_t))) == NULL )
	  {
		perror(FUNC_NAME ": malloc");
		return(NULL);
	  }

	rnd->fd = -1;
	rnd->devname = devname;

	if ( (rnd->fd = hwrandom_open(rnd, O_RDWR)) == -1 )
	  {
		perror(FUNC_NAME ": hwrandom_open");
		free(rnd);
		return(NULL);
	  }

	if ( (rnd->mutex = (pthread_mutex_t *)
	       malloc(sizeof(pthread_mutex_t))) == NULL )
	  {
		perror(FUNC_NAME ": malloc");
		close(rnd->fd);
		free(rnd);
		return(NULL);
	  }

	if ( pthread_mutex_init(rnd->mutex, NULL) != 0 )
	  {
		perror(FUNC_NAME ": pthread_mutex_init");
		close(rnd->fd);
		free(rnd->mutex);
		free(rnd);
		return(NULL);
	  }

	return(rnd);
  }
#undef FUNC_NAME

int
hwrandom_open (hwrandom_t *rnd, int flags)
#define FUNC_NAME "hwrandom_open"
  {

	if (rnd->fd != -1)
	  close(rnd->fd);

	if ( (rnd->fd = open(rnd->devname, flags)) == -1 )
	  {
		perror(FUNC_NAME ": open");
		rnd->fd = -1;
	  }

	return(rnd->fd);
  }
#undef FUNC_NAME

void
hwrandom_destroy (hwrandom_t *rnd)
#define FUNC_NAME "hwrandom_destroy"
  {
	if (rnd == NULL)
	  return;

	if (rnd->mutex != NULL)
	  {
		pthread_mutex_destroy(rnd->mutex);
		free(rnd->mutex);
	  }

	close(rnd->fd);

	free(rnd);

	return;
  }
#undef FUNC_NAME

char *
hwrandom_hash_buffer(const char *buffer, size_t count)
#define FUNC_NAME "hwrandom_hash_buffer"
  {
        int i;
        char *buf_hash;
	unsigned char *dig_buffer;
	unsigned int dig_len = gcry_md_get_algo_dlen(RND_DIGEST_ALGO_SMALL);

        if ( (buf_hash = malloc((2*dig_len) + 1)) == NULL )
          {
                perror(FUNC_NAME ": malloc");
                return(NULL);
          }
        memset(buf_hash, 0, (2*dig_len) + 1);

	if (gcry_md_test_algo(RND_DIGEST_ALGO_SMALL) != 0)
	  {
		perror(FUNC_NAME ": hash algorithm is not available");
                free(buf_hash);
		return(NULL);
	  }

	if ( (dig_buffer = malloc(dig_len)) == NULL )
	  {
		perror(FUNC_NAME ": malloc");
                free(buf_hash);
		return(NULL);
	  }

	gcry_md_hash_buffer (RND_DIGEST_ALGO_SMALL, dig_buffer, buffer, count);

        for (i=0;i<dig_len;i++)
          {
            snprintf(buf_hash+(2*i), 3, "%02x", dig_buffer[i]);
          }

        free(dig_buffer);

        return(buf_hash);
  }
#undef FUNC_NAME

void
hwrandom_add_entropy (hwrandom_t *rnd, const char *buffer, size_t count)
#define FUNC_NAME "hwrandom_add_entropy"
  {
	char *dig_buffer;
	unsigned int dig_len = gcry_md_get_algo_dlen(RND_DIGEST_ALGO);

	struct
	  {
		int ent_count;
		int buflen;
		unsigned char *data;
	  } entropy;

	if (rnd == NULL)
	  return;

	if ( (rnd->fd == -1) && (hwrandom_open(rnd, O_RDWR) == -1) )
	  return;

	if (gcry_md_test_algo(RND_DIGEST_ALGO) != 0)
	  {
		perror(FUNC_NAME ": hash algorithm is not available");
		return;
	  }

	if ( (dig_buffer = malloc(dig_len)) == NULL )
	  {
		perror(FUNC_NAME ": malloc");
		return;
	  }

	gcry_md_hash_buffer (RND_DIGEST_ALGO, dig_buffer, buffer, count);

	entropy.ent_count = dig_len * 8;
	entropy.buflen = dig_len;
	entropy.data = (unsigned char *) dig_buffer;

	pthread_mutex_lock(rnd->mutex);
	if ( ioctl(rnd->fd, RNDADDENTROPY, &entropy) != 0 )
	  {
		perror(FUNC_NAME ": ioctl");

		pthread_mutex_unlock(rnd->mutex);

		if ( hwrandom_write(rnd, dig_buffer, dig_len) != dig_len )
		  perror(FUNC_NAME ": hwrandom_write");
	  }
	else
	  {
		pthread_mutex_unlock(rnd->mutex);
	  }

	free(dig_buffer);
	return;
  }
#undef FUNC_NAME

ssize_t
hwrandom_read (hwrandom_t *rnd, void *buffer, size_t count)
#define FUNC_NAME "hwrandom_read"
  {
	ssize_t nb = 0;

	if (rnd == NULL)
	  return(-1);

	if ( (rnd->fd == -1) && (hwrandom_open(rnd, O_RDWR) == -1) )
	  return(-1);

	while ( nb < count )
	  {
		size_t nbt;

		pthread_mutex_lock(rnd->mutex);
		if ( (nbt = read(rnd->fd, (void *)(buffer+nb), count-nb)) == -1 )
		  {
			perror(FUNC_NAME ": read");
			close(rnd->fd);
			rnd->fd = -1;
			return(-1);
		  }
		pthread_mutex_unlock(rnd->mutex);

		nb += nbt;
	  }

	return(nb);
  }
#undef FUNC_NAME

ssize_t
hwrandom_write (hwrandom_t *rnd, const void *buffer, const size_t count)
#define FUNC_NAME "hwrandom_write"
  {
	ssize_t nb = 0;

	if (rnd == NULL)
	  return(-1);

	if ( (rnd->fd == -1) && (hwrandom_open(rnd, O_RDWR) == -1) )
	  return(-1);

	while ( nb < count )
	  {
		size_t nbt;

		pthread_mutex_lock(rnd->mutex);
		if ( (nbt = write(rnd->fd, (void *)(buffer+nb), count-nb)) == -1 )
		  {
			perror(FUNC_NAME ": write");
			close(rnd->fd);
			rnd->fd = -1;
			return(-1);
		  }
		pthread_mutex_unlock(rnd->mutex);

		nb += nbt;
	  }

	return(nb);
  }
#undef FUNC_NAME

char *
hwrandom_bytes (hwrandom_t *rnd, size_t count)
#define FUNC_NAME "hwrandom_bytes"
  {
	char *ret;

	if ( rnd == NULL )
	  return(NULL);

	if ( (ret = (char *) malloc(count)) == NULL )
	  return(NULL);

	if ( hwrandom_read(rnd, ret, count) != count )
	  {
		perror(FUNC_NAME ": hwrandom_read" );
		free(ret);
		return(NULL);
	  }

	return(ret);
  }
#undef FUNC_NAME

