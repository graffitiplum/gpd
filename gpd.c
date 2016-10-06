#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <pwd.h>
#include <grp.h>
#include <getopt.h>
#include <pthread.h>

#include "options.h"
#include "gpd_thread.h"

static sig_atomic_t got_sigint = 0;

void
sigint_handler ( int signum )
  {
	got_sigint = 1;
  }

void
show_help ( const char *me )
  {
	fprintf(stderr,
		"usage: %s [-hd] [-t <num>]\n"
		"\t-h, --help           :\tthis help message.\n"
		"\t-d, --daemonize      :\trun in the background.\n"
		"\t-t, --threads        :\tnumber of threads to run (default: 1)\n",
		me);
  }

int
main ( int argc, char ** argv )
  {
	char opt;
	int option_index = 0;
	int daemonize = 0;

	gpd_shm_t *gpd_shm = NULL;

	gpd_thread_t **gpd_thread = NULL;
	unsigned int numthreads = 1;

	struct passwd *pwbuf;
	struct group *gbuf;

	static struct option long_options[] = {
		{ "help", 0, NULL, 'h' },
		{ "daemonize", 0, NULL, 'd' },
		{ "threads", 1, NULL, 't' },
	};

	int i;

	if ( (gpd_shm = (gpd_shm_t *) malloc( sizeof(gpd_shm_t) )) == NULL )
	  {
		perror("malloc");
		return(1);
	  }
	gpd_shm->do_quit = 0;

	while ((opt =
		getopt_long(argc, argv, "hdt:", long_options,
			&option_index)) != -1)
	  {
		switch(opt)
		  {

		  case 't':
			numthreads = atoi(optarg);
			if ( (numthreads < 1) || (numthreads > MAX_THREADS) )
			  {
				show_help(argv[0]);
				return(1);
			  }
			break;

		  case 'd':
			daemonize = 1;
			break;

		  case 'h':
		  default:
			show_help(argv[0]);
			return(1);
			break;
		  }
	  }


	/*
	if ( argc <= (optind + 1) )
	  {
		show_help(argv[0]);
		return(1);
	  }
	*/

	pthread_mutex_init(&gpd_shm->mutex, NULL);

	if ( daemonize )
	  {
		if ( (gbuf = getgrnam(BEN_GROUP)) == NULL )
		  {
			perror("getgrnam(" BEN_GROUP ")");
			return(1);
		  }
		if ( (pwbuf = getpwnam(BEN_USER)) == NULL )
		  {
			perror("getpwnam(" BEN_USER ")");
			return(1);
		  }
		if ( setgid(gbuf->gr_gid) == -1 )
		  {
			perror("setgid");
			return(1);
		  }
		if ( setuid(pwbuf->pw_uid) == -1 )
		  {
			perror("setuid");
			return(1);
		  }


		// TODO: not to ignore return values of nice() and daemon()

		nice(NICEVAL);

		daemon(0,0);
	  }

	for (i=0;i<numthreads;i++)
	  {
		gpd_thread_t **bt_tmp;

    	if ( (bt_tmp =
				realloc(gpd_thread, sizeof(gpd_thread_t *) * (i + 1))) == NULL )
		  {
			perror("realloc");
			return(1);
		  }
		else
		  {
			/* realloc succeeded */
			gpd_thread = bt_tmp;
			if ( (gpd_thread[i] =
				gpd_thread_create(gpd_shm)) == NULL )
			  {
				perror("gpd_thread_create");
				return(1);
			  }
		  }
	  }

	signal(SIGINT, sigint_handler);

	while ( !got_sigint && !gpd_shm->do_quit )
	  usleep(161803);

	for ( i=0; i < numthreads; i++ )
	  gpd_thread_destroy(gpd_thread[i]);

	pthread_mutex_destroy(&gpd_shm->mutex);

	return(0);
  }

