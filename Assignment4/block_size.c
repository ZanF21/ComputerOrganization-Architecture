#include <stdio.h>
#include <stdint.h>
#include <emmintrin.h>
//array size is defined and used 
#define size 256
//global char array defined
char a[size];

int main()
{   
    //initialization 
    for (int i = 0; i < size; ++i)
        a[i] = 2;
    for (int i = 0; i < size; i++)
        _mm_clflush(&a[i]);
        
	int64_t start, end;
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
    //running size times
    for (int i = 0; i < size; i+=1) 
    {
        //time start
        asm volatile ("CPUID\n\t"
        "RDTSCP\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low)::
        "%rax", "%rbx", "%rcx", "%rdx");

        //simple execution
        a[i]=a[i]+17;

        //time end
        asm volatile("RDTSCP\n\t"
        "mov %%edx,     %0\n\t"
        "mov %%eax,     %1\n\t"
        "CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
        "%rax", "%rbx", "%rcx", "%rdx");

        //print vals
        start = (((uint64_t)cycles_high << 32) | cycles_low );
        end = (((uint64_t)cycles_high1 << 32) | cycles_low1 );
        printf("%d %lu\n", i, (end-start));

    }

}