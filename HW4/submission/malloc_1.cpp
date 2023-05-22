#include <unistd.h>
#include <stdio.h>

void* smalloc (size_t size)
{
    if (size>100000000 || size==0)
    {
        return NULL;
    }
    void* current_program_break=sbrk(0);
    if (*(int*)sbrk (size)==-1)
    {
        return NULL;
    }
    return current_program_break;
}
