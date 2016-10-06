#ifndef _BEN_OPTIONS_H

#define _BEN_OPTIONS_H 1

/* BBS prng key length */
#define BBS_KEY_LEN 2310

/* run as user/group */
#define BEN_USER  "gp"
#define BEN_GROUP "gp"

/* nice value */
#define NICEVAL 13

#define MAX_THREADS 60

#define RND_DIGEST_ALGO GCRY_MD_SHA512
#define DEVRANDOM "/dev/random"

#endif /* _BEN_OPTIONS_H */
