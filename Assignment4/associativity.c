#include <stdio.h>
#include <stdint.h>
#include <emmintrin.h>
#include <time.h>

// 192Kib L1d Cache -> Ryzen 5 5600H 
#define CSIZE 1024*192

int main(){
    // associativity for : 17 , so 17 + 1 = 18
    int ways=18;
    int offset;

    uint64_t start, end,latency[ways];
    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;

    // Char to access every byte
    char *arr = (char *)malloc(ways*CSIZE*sizeof(char));
    char tmp;
    
    //setting latency to 0
    for(int i = 0; i < ways; i++)
        latency[i] = 0;

    // Run tests 100 times to remove random spikes
    for(int i = 0; i < 100; i++){
        for(int i = 0; i< ways; i++)
            tmp = arr[i*CSIZE];   
        
        for(int i = 0; i< ways; i++){
            offset = (ways-i)*CSIZE;

            //Obtaining start time
            _mm_mfence();
            asm volatile ("CPUID\n\t"
            "RDTSCP\n\t"
            "mov %%edx, %0\n\t"
            "mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low)::
            "%rax", "%rbx", "%rcx", "%rdx");
            _mm_mfence();
            
            //data access
            tmp = arr[offset];

            //obtaining end time
            _mm_mfence();
            asm volatile("RDTSCP\n\t"
            "mov %%edx,     %0\n\t"
            "mov %%eax,     %1\n\t"
            "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
            "%rax", "%rbx", "%rcx", "%rdx");
            _mm_mfence();

            start = ( ((uint64_t)cycles_high << 32) | cycles_low );
            end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
            
            //getting latency summation
            latency[i] += (end - start);
        }
    }
    //printing avg latency
    for (int i = 0; i<ways; i++)
        printf("%d\t%lu \n", i, latency[i]/100);
  
  free(arr);
  return 0;
}

//runby : $ gcc ass.c -o ass
//        $ ./ass
//
//  0       204
//  1       196
//  2       203
//  3       204
//  4       211
//  5       214
//  6       228
//  7       235
//  8       237
//  9       238
//  10      233
//  11      211
//  12      200
//  13      193
//  14      193
//  15      189
//  16      188
//  17      182