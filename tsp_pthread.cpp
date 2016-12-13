/*
 TSP code for CS 4380 / CS 5351
 
 Copyright (c) 2016, Texas State University. All rights reserved.
 
 Redistribution in source or binary form, with or without modification,
 is not permitted. Use in source and binary forms, with or without
 modification, is only permitted for academic use in CS 4380 or CS 5351
 at Texas State University.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 Author: Martin Burtscher
 editors: Jacqueline Moad and Diego Loya
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <sys/time.h>
#include "cs43805351.h"
#include <pthread.h>

//global variables
static int thread_count;
static pthread_mutex_t mutex_p;

static float *px;
static float *py;
static int cities;
static long minchange;


#define dist(a, b) (int)(sqrtf((px[a] - px[b]) * (px[a] - px[b]) + (py[a] - py[b]) * (py[a] - py[b])) + 0.5f)

static void *createNewRoute(void* rank)
{
    long my_minchange =0;
	
	//changing void pointer to ineteger
    long my_rank = (long)rank;
  
        // determine best 2-opt move
	//start of parallelizing loop
        for (int i = my_rank; i < cities - 2; i+=thread_count) {
            for (int j = i + 2; j < cities; j++) {
                long change = dist(i, j) + dist(i + 1, j + 1) - dist(i, i + 1) - dist(j, j + 1);
                change = (change << 32) + (i << 16) + j;
                if (my_minchange > change) {
                    my_minchange = change;
                }
            }
        }
    pthread_mutex_lock(&mutex_p);	//lock the mutex before critical section
    if(minchange>my_minchange)		//critical section if a new route is found 
        minchange=my_minchange;		//change it so its the new route from thread
    pthread_mutex_unlock(&mutex_p);	//unlock mutex
    
    return NULL;
}

static int TwoOpt(int& climbs)
{
    pthread_t* thread_handles;
    thread_handles = (pthread_t*)malloc (thread_count*sizeof(pthread_t));
    pthread_mutex_init(&mutex_p,NULL);
    // link end to beginning
    px[cities] = px[0];
    py[cities] = py[0];
    
    // repeat until no improvement
    int iter = 0;
    do {
        iter++;
        // determine best 2-opt move
        minchange = 0;

		//create worker pthreads
   	for( long thread = 1; thread < thread_count; thread++)
    	{
        	pthread_create (&thread_handles[thread], NULL, createNewRoute, (void*) thread);
    	}
    
		//function call to parallelizing function
    	createNewRoute((void*)0 );
    
		//join pthreads
    	for(long thread = 1; thread < thread_count; thread++)
    	{
        	pthread_join(thread_handles[thread], NULL);
    	}
    
        // apply move if it shortens the tour
        if (minchange < 0) {
            int i = (minchange >> 16) & 0xffff;
            int j = minchange & 0xffff;
            i++;
            while (i < j) {
                float t;
                t = px[i];  px[i] = px[j];  px[j] = t;
                t = py[i];  py[i] = py[j];  py[j] = t;
                i++;
                j--;
            }
        }
    } while (minchange < 0);

    

    climbs = iter;
    
    // compute tour length
    int len = 0;
    for (int i = 0; i < cities; i++) {
        len += dist(i, i + 1);
    }
    

	//destroy the mutex
    pthread_mutex_destroy(&mutex_p);
	//free threads
    free(thread_handles);
    return len;
}




int main(int argc, char *argv[])
{
    
    printf("TSP v1.4 [pthread]\n");
    

    
    
    // read input
  if (argc != 3) {fprintf(stderr, "usage: %s input_file thread_count \n", argv[0]); exit(-1);}
    thread_count = atoi(argv[2]);
    if (thread_count<1){fprintf(stderr, "Number of threads must be at least one\n");exit(-1);}
    
    FILE* f = fopen(argv[1], "rb");  if (f == NULL) {fprintf(stderr, "error: could not open file %s\n", argv[1]); exit(-1);}
    int cnt = fread(&cities, sizeof(int), 1, f);  if (cnt != 1) {fprintf(stderr, "error: failed to read cities\n"); exit(-1);}
    if (cities < 1) {fprintf(stderr, "error: cities must be greater than zero\n"); exit(-1);}
    px= new float [cities + 1];
    py= new float [cities + 1];  // need an extra element later
    cnt = fread(px, sizeof(float), cities, f);  if (cnt != cities) {fprintf(stderr, "error: failed to read px\n"); exit(-1);}
    cnt = fread(py, sizeof(float), cities, f);  if (cnt != cities) {fprintf(stderr, "error: failed to read py\n"); exit(-1);}
    fclose(f);
    printf("configuration: %d cities from %s\n", cities, argv[1]);
        
    
    
    
    
    printf("NUMBER OF THREADS--- %d \n", thread_count);
    // start time
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    // find good tour
    int climbs;
    int len = TwoOpt(climbs);
    
    // end time
    gettimeofday(&end, NULL);
    
    double runtime = end.tv_sec + end.tv_usec / 1000000.0 - start.tv_sec - start.tv_usec / 1000000.0;
    printf("COMPUTE TIME     ---: %.4f s\n", runtime);
    long moves = 1LL * climbs * (cities - 2) * (cities - 1) / 2;
    printf("gigamoves/sec: %.3f\n", moves * 0.000000001 / runtime);
    
    // output result
    printf("tour length = %d\n", len);
    
    // scale and draw final tour
    const int width = 1024;
    unsigned char pic[width][width];
    memset(pic, 0, width * width * sizeof(unsigned char));
    float minx = px[0], maxx = px[0];
    float miny = py[0], maxy = py[0];
    for (int i = 1; i < cities; i++) {
        if (minx > px[i]) minx = px[i];
        if (maxx < px[i]) maxx = px[i];
        if (miny > py[i]) miny = py[i];
        if (maxy < py[i]) maxy = py[i];
    }
    float dist = maxx - minx;
    if (dist < (maxy - miny)) dist = maxy - miny;
    float factor = (width - 1) / dist;
    int x[cities], y[cities];
    for (int i = 0; i < cities; i++) {
        x[i] = (int)(0.5f + (px[i] - minx) * factor);
        y[i] = (int)(0.5f + (py[i] - miny) * factor);
    }
    for (int i = 1; i < cities; i++) {
        line(x[i - 1], y[i - 1], x[i], y[i], 127, (unsigned char*)pic, width);
    }
    line(x[cities - 1], y[cities - 1], x[0], y[0], 128, (unsigned char*)pic, width);
    for (int i = 0; i < cities; i++) {
        line(x[i], y[i], x[i], y[i], 255, (unsigned char*)pic, width);
    }
    writeBMP(width, width, (unsigned char*)pic, "tsp.bmp");
    
    return 0;
}
