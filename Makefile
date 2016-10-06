CC=gcc

#COPT=-march=prescott -mmmx -msse -msse2 -msse3 \
#	-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
COPT=-march=nocona -mmmx -msse -msse2 -msse3 \
	-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

GCRYPT_CFLAGS=`libgcrypt-config --cflags`
GCRYPT_LIBS=`libgcrypt-config --libs`

CFLAGS=-Wall -O3 -pipe -funroll-loops $(COPT) -D_GNU_SOURCE \
        -fomit-frame-pointer -ftree-vectorize \
	$(GCRYPT_CFLAGS)

LIBS=-lpthread $(GCRYPT_LIBS) -lm -lcurl

OBJ=hwrandom.o gpd_thread.o gpd.o

all: gpd

gpd: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS) $(LIBS)

install: gpd
	install -v -s -m 0755 gpd /usr/bin/

clean:
	rm -f *~ *.o gpd
