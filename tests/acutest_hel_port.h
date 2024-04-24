
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#define TEST_INIT \
    int seed = time(NULL); \
    srand(seed); \
    printf("\nrandom seed - %d\n", seed);

#include "acutest.h"
