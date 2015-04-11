# memory-alocator
memory simulator that allocates and frees memory from memory.bin file
commands:
    alloc <val> - allocates val bytes if available returns address or -1
    read <addr> - returns value stored in memory at addr
    write <addr> <val> - stores val at address addr val must be in 0..255
    free <addr> - free memory stored at address addr, detects false pointers
