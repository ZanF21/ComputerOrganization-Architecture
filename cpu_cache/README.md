A simple program to calculate L1 cache block size and associativity

- Simple C code to do the task

-   Enter the directory with block_size.c, associativity.c
-   In the command prompt use following commands:
        
            $ gcc block_size.c -o block_size
            $ ./block_size
       (repeat muliple times and take an average to reduce noise)

    The ouput given will give latency spikes at new block access.
        
            $ gcc associativity.c -o associativity
            $ ./associativity
    
    The ouptup given will be the average latency for a block access.  

This assignment was a team assignment and was done by <a href="https://github.com/rizan21">myself</a> and <a href="">Chathurvedhi</a>.