#include <string.h>
#include <unistd.h>
#define MAX_SIZE 100000000

typedef struct MallocMetaData {
    size_t size;
    bool is_free;
    MallocMetaData* next;
    MallocMetaData* prev;
}MMD;

//********* Data Sector List ***********

class DSList{
private:
    MMD* head;
    MMD* tail;
public:
    size_t free_blocks;
    size_t free_bytes;
    size_t allocated_blocks;
    size_t allocated_bytes;

    DSList();
    void* allocateSpace(size_t size);
    void insertBlockToList(MMD *block);
    void freeBlockFromList(void *ptr);

};


DSList::DSList(): head(nullptr), tail(nullptr) {
    free_blocks = 0;
    free_bytes = 0;
    allocated_blocks = 0;
    allocated_bytes =0;
}


MMD* findFreeBlock(size_t size, MMD* iter)
{
    while(iter)
    {
        if((iter->is_free) && (iter->size>=size))
        {
            return iter;
        }
        iter=iter->next;
    }
    return nullptr;
}


void* DSList::allocateSpace(size_t size) {
    size_t total_size = size + sizeof(MMD);
    MMD* vacant = findFreeBlock(size, head);
    if (vacant)
    {
        vacant->is_free = false;
        free_blocks--;
        free_bytes -= vacant->size;
        return vacant;
    }
    else {
        void *program_break = sbrk(total_size);
        if (program_break == (void *) -1)
            return nullptr;
        MMD *new_block = (MMD *) program_break;
        new_block->size = size;
        new_block->is_free = false;
        new_block->next = nullptr;
        new_block->prev = nullptr;
        insertBlockToList(new_block);
        allocated_blocks++;
        allocated_bytes += size;
        return program_break;
    }
}

void DSList::insertBlockToList(MMD *block){
    if (!head)
    {
        head = block;
        tail = block;
    }
    else
    {
        tail->next =block;
        block->prev =tail;
        tail = block;
    }
}




void DSList::freeBlockFromList(void *ptr){
    MMD* block = (MMD*)((char*)ptr - sizeof(MMD));
    if (!block->is_free)
    {
        block->is_free = true;
        free_blocks++;
        free_bytes += block->size;
    }
}

// ******** Required Functions **********

DSList dsl = DSList();

void* smalloc(size_t size){
    if(size > MAX_SIZE || size == 0)
        return nullptr;

    void* allocated = dsl.allocateSpace(size);
    if(allocated == nullptr)
        return nullptr;
    return (char*)allocated + sizeof(MMD);
}

void* scalloc(size_t num, size_t size){
    void* p = smalloc(size*num);
    if(p == nullptr)
        return nullptr;
    memset(p, 0, size*num);
    return p;
}

void sfree(void* p){
    if(p == nullptr)
        return;
    dsl.freeBlockFromList(p);
}


void* srealloc(void* oldp, size_t size){
    if(size > MAX_SIZE || size == 0)
        return nullptr;

    if(oldp == nullptr)
        return smalloc(size);

    MMD* old_block = (MMD*)((char*)oldp - sizeof(MMD));
    size_t old_size = old_block->size;
    if(size <= old_size)
        return oldp;

    void* newp = smalloc(size);
    if(newp == nullptr)
        return nullptr;
    memcpy(newp, oldp, old_size);
    sfree(oldp);
    return newp;
}

size_t _num_free_blocks(){
    return dsl.free_blocks;
}

size_t _num_free_bytes(){
    return dsl.free_bytes;
}

size_t _num_allocated_blocks(){
    return dsl.allocated_blocks;
}

size_t _num_allocated_bytes(){
    return dsl.allocated_bytes;
}

size_t _num_meta_data_bytes(){
    return dsl.allocated_blocks*sizeof(MMD);
}

size_t _size_meta_data(){
    return sizeof(MMD);
}

