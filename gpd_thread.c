#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <curl/curl.h>

#include "hwrandom.h"
#include "gpd_thread.h"

gpd_thread_t *gpd_thread_create(gpd_shm_t *gpd_shm)
#define FUNC_NAME "gpd_thread_create"
  {
	gpd_thread_t *gpd_thread;
	sigset_t sig_set, old_set;

	if ( (gpd_thread = (gpd_thread_t *)
	                     malloc(sizeof(gpd_thread_t))) == NULL )
	  {
		perror(FUNC_NAME ": malloc");
		return(NULL);
	  }
	gpd_thread->do_quit = 0;
	gpd_thread->gpd_shm = gpd_shm;

	/* block SIGINT in threads (use do_quit from main thread instead) */
	sigemptyset( &sig_set );
	sigaddset( &sig_set, SIGINT );
	pthread_sigmask( SIG_BLOCK, &sig_set, &old_set );

	pthread_attr_init(&gpd_thread->attr);
	pthread_mutex_init(&gpd_thread->mutex, NULL);
	pthread_create(&gpd_thread->thread, &gpd_thread->attr,
	               gpd_thread_run, (void *)gpd_thread);

	/* set SIGINT blocking to its previous state (should be unblocked now) */
	pthread_sigmask( SIG_SETMASK, &old_set, NULL );

	return(gpd_thread);
  }
#undef FUNC_NAME

gpd_thread_t *gpd_thread_destroy(gpd_thread_t *gpd_thread)
#define FUNC_NAME "gpd_thread_destroy"
  {
	if (gpd_thread == NULL)
	  return(NULL);

	/* send signal for thread to quit */
	gpd_thread->do_quit = 1;

	pthread_join(gpd_thread->thread, NULL);
	pthread_mutex_destroy(&gpd_thread->mutex);
	pthread_attr_destroy(&gpd_thread->attr);

	/* destroy any other structures in the thread */

	free(gpd_thread);
	return(NULL);
  }
#undef FUNC_NAME

void *gpd_thread_run(void *btv)
#define FUNC_NAME "gpd_thread"
  {
	gpd_thread_t *gpd_thread = (gpd_thread_t *)btv;
	gpd_shm_t *gpd_shm = gpd_thread->gpd_shm;

	while ( (! gpd_thread->do_quit) && (! gpd_shm->do_quit) )
	  {
		CURL *curl;
		CURLcode res;

		// TODO: encode pool_data to be non-binary.
		// TODO: LOCK /dev/random
		// TODO: remplate 256 with constant variables
		char pool_data[5 + 256 + 1] = "pool=";
		char random_data[256];
		hwrandom_t *hwrandom = hwrandom_init(DEVRANDOM);
		hwrandom_read(hwrandom, random_data, 256);

		// TODO: error check if it is null
		char *hash = hwrandom_hash_buffer(random_data, 256);

		if (hash == NULL) {
			hwrandom_destroy(hwrandom);
			continue;
		}

		strncpy(pool_data + 5, hash, 256);

		/* lock shared mutex */
		pthread_mutex_lock(&gpd_shm->mutex);
 
		curl_global_init(CURL_GLOBAL_ALL);
 
		/* get a curl handle */ 
		curl = curl_easy_init();
		if(curl) {
		    /* First set the URL that is about to receive our POST. This URL can
		       just as well be a https:// URL if that is what should receive the
		       data. */ 
		    curl_easy_setopt(curl, CURLOPT_URL, "https://hacktivity.org/yellowjacket/pool.php");
		    /* Now specify the POST data */ 
		    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pool_data);


		    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
 
		    /* Perform the request, res will get the return code */ 
		    res = curl_easy_perform(curl);

		    /* Check for errors */ 
		    if(res != CURLE_OK) {
		      fprintf(stderr, "curl_easy_perform() failed: %s\n",
		              curl_easy_strerror(res));
		    } else {
			fprintf(stderr, "%s\n", pool_data);
		    }
 
		    /* always cleanup */ 
		    curl_easy_cleanup(curl);
		}
		curl_global_cleanup();

		/* unlock shared mutex */
		pthread_mutex_unlock(&gpd_shm->mutex);

		hwrandom_destroy(hwrandom);

	  }

	pthread_exit(NULL);
  }
#undef FUNC_NAME

