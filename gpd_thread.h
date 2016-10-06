#ifndef _BEN_THREAD_H

#define _BEN_THREAD_H 1

#include <pthread.h>
#include <signal.h>

#include "options.h"

typedef struct
  {
	pthread_mutex_t mutex;

	sig_atomic_t do_quit;					/* 0 = no , 1 = yes */

  } gpd_shm_t;

typedef struct
  {
	pthread_t thread;
	pthread_attr_t attr;
	pthread_mutex_t mutex;

	sig_atomic_t do_quit;			/* 0 = no , 1 = yes */

	gpd_shm_t *gpd_shm;

  } gpd_thread_t;

gpd_thread_t *gpd_thread_create(gpd_shm_t *gpd_shm);
gpd_thread_t *gpd_thread_destroy(gpd_thread_t *gpd_thread);

#define gpd_thread_stop(gpd_thread) gpd_thread_destroy(gpd_thread)

void *gpd_thread_run(void *btv);

#endif /* _BEN_THREAD_H */

