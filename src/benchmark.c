/**
 * This code is released under a BSD License.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "simdcomp.h"


void benchmarkSelect() {
	uint32_t buffer[128];
	uint32_t backbuffer[128];
    uint32_t initial = 33;
    uint32_t b;
    clock_t S1, S2, S3;
    int i;
    printf("benchmarking select \n");

    /* this test creates delta encoded buffers with different bits, then
     * performs lower bound searches for each key */
    for (b = 0; b <= 32; b++) {
        uint32_t prev = initial;
    	uint32_t out[128];
        /* initialize the buffer */
        for (i = 0; i < 128; i++) {
            buffer[i] =  ((uint32_t)(1431655765 * i + 0xFFFFFFFF)) ;
            if(b < 32) buffer[i] %= (1<<b);
        }
        for (i = 0; i < 128; i++) {
           buffer[i] = buffer[i] + prev;
           prev = buffer[i];
        }

        for (i = 1; i < 128; i++) {
        	if(buffer[i] < buffer[i-1] )
        		buffer[i] = buffer[i-1];
        }
        assert(simdmaxbitsd1(initial, buffer)<=b);

        for (i = 0; i < 128; i++) {
        	out[i] = 0; /* memset would do too */
        }

        /* delta-encode to 'i' bits */
        simdpackwithoutmaskd1(initial, buffer, (__m128i *)out, b);

        S1 = clock();
        for (i = 0; i < 128 * 10; i++) {
        	uint32_t valretrieved = simdselectd1(initial, (__m128i *)out, b, (uint32_t)i % 128);
            assert(valretrieved == buffer[i%128]);
        }
        S2 = clock();
        for (i = 0; i < 128 * 10; i++) {
        	simdunpackd1(initial,  (__m128i *)out, backbuffer, b);
        	assert(backbuffer[i % 128] == buffer[i % 128]);
        }
        S3 = clock();
        printf("bit width = %d, fast select function time = %lu, naive time = %lu  \n", b, (S2-S1), (S3-S2));
    }
}

int uint32_cmp(const void *a, const void *b)
{
    const uint32_t *ia = (const uint32_t *)a;
    const uint32_t *ib = (const uint32_t *)b;
    if(*ia < *ib)
    	return -1;
    else if (*ia > *ib)
    	return 1;
    return 0;
}

/* adapted from wikipedia */
int binary_search(uint32_t * A, uint32_t key, int imin, int imax)
{
	int imid;
	imax --;
	while(imin + 1 < imax) {
      imid = imin + ((imax - imin) / 2);

      if (A[imid] > key) {
        imax = imid;
      } else if (A[imid] < key) {
        imin = imid;
      } else {
         return imid;
      }
	}
	return imax;
}


/* adapted from wikipedia */
int lower_bound(uint32_t * A, uint32_t key, int imin, int imax)
{
	int imid;
	imax --;
	while(imin + 1 < imax) {
      imid = imin + ((imax - imin) / 2);

      if (A[imid] >= key) {
        imax = imid;
      } else if (A[imid] < key) {
        imin = imid;
      }
	}
	if(A[imin] >= key) return imin;
	return imax;
}

void benchmarkSearch() {
	uint32_t buffer[128];
    uint32_t backbuffer[128];
	uint32_t out[128];
    uint32_t result, initial = 0;
    uint32_t b, i;
    clock_t S1, S2, S3;

    printf("benchmarking search \n");

    /* this test creates delta encoded buffers with different bits, then
     * performs lower bound searches for each key */
    for (b = 0; b <= 32; b++) {
    	uint32_t prev = initial;
        /* initialize the buffer */
        for (i = 0; i < 128; i++) {
            buffer[i] =  ((uint32_t)(1431655765 * i + 0xFFFFFFFF)) ;
            if(b < 32) buffer[i] %= (1<<b);
        }

        qsort(buffer,128, sizeof(uint32_t), uint32_cmp);

        for (i = 0; i < 128; i++) {
           buffer[i] = buffer[i] + prev;
           prev = buffer[i];
        }
        for (i = 1; i < 128; i++) {
        	if(buffer[i] < buffer[i-1] )
        		buffer[i] = buffer[i-1];
        }
        assert(simdmaxbitsd1(initial, buffer)<=b);
        for (i = 0; i < 128; i++) {
        	out[i] = 0; /* memset would do too */
        }

        /* delta-encode to 'i' bits */
        simdpackwithoutmaskd1(initial, buffer, (__m128i *)out, b);
        simdunpackd1(initial,  (__m128i *)out, backbuffer, b);

        for (i = 0; i < 128; i++) {
        	assert(buffer[i] == backbuffer[i]);
        }


        S1 = clock();
        for (i = 0; i < 0; i++) {

        	int pos;
            uint32_t pseudorandomkey  =  ((uint32_t)(3431655765 * i + 0xF0FFF0FF)) ;
            if(b < 32) buffer[i] %= (1<<b);
            if(pseudorandomkey < buffer[0]) pseudorandomkey = buffer[0];
            else if(pseudorandomkey > buffer[127]) pseudorandomkey = buffer[127];
    		printf("fast search i = %d\n", i);

            pos = simdsearchd1(initial, (__m128i *)out, b, 128,
        			pseudorandomkey, &result);
            printf("fast search i = %d done pos = %d \n", i, pos);

            if((result < pseudorandomkey) || (buffer[pos] != result)) {
        		printf("pos = %d, result = %u, pseudorandomkey= %u \n", pos, result, pseudorandomkey);

        		printf("bug A.\n");
        		exit(-1);
        	} else if (pos > 0) {
        		if(buffer[pos-1] >= pseudorandomkey)
        			printf("bug B.\n");
        		    exit(-1);
        	}
        }
		printf("Ok \n");

        S2 = clock();
        for (i = 0; i < 128 * 10; i++) {
        	int pos;
        	uint32_t pseudorandomkey  =  ((uint32_t)(3431655765 * i + 0xF0FFF0FF)) ;
            if(b < 32) buffer[i] %= (1<<b);
            if(pseudorandomkey < buffer[0]) pseudorandomkey = buffer[0];
            printf("slow search i = %d\n", i);
            simdunpackd1(initial,  (__m128i *)out, backbuffer, b);
            pos =  lower_bound(backbuffer, pseudorandomkey, 0, 128);
            printf("slow search i = %d pos = %d \n", i, pos);

            if((result < pseudorandomkey) || (buffer[pos] != result)) {
        		printf("bug C.\n");
        		exit(-1);
        	} else if (pos > 0) {
        		if(buffer[pos-1] >= pseudorandomkey)
        			printf("bug D.\n");
        		    exit(-1);
        	}
        }
        S3 = clock();
        printf("bit width = %d, fast search function time = %lu, naive time = %lu  \n", b, (S2-S1), (S3-S2));
    }
}


int main() {
	benchmarkSearch();
	benchmarkSelect();
}