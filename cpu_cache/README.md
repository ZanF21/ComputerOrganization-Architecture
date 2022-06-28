A simple program to calculate L1 cache block size and associativity

- Simple C code to do the task

-   Enter the directory with block_size.c, associativity.c
-   In the command prompt use following commands:
        
        1.  $ gcc block_size.c -o block_size
        2.  $ ./block_size
            (repeat muliple times and take an average to reduce noise)

    The ouput given will give latency spikes at new block access.
        
        3.  $ gcc associativity.c -o associativity
        4.  $ ./associativity
    
    The ouptup given will be the average latency for a block access.  
