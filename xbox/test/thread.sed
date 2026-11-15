# thread_alloc puts a thread's stack top in P1, the console's cached view
# of RAM past 0x80000000. qemu-sh4 maps no address past 0x7fffffff, so
# under it the top stays where the heap gave it
s/(((Uint32)thread + size) | P1)/((Uint32)thread + size)/
