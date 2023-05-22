#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>

#define MAP_SIZE (128 * 1024)
#define MAX_VAL (100000000)

bool cookie_flag = false; 
int global_cookie; 
int g_temp = 0; 


typedef struct MallocMetaData {
    int private_cookie;
    bool is_free;
    size_t size;
    MallocMetaData* next_by_size;
    MallocMetaData* prev_by_size;
    MallocMetaData* next;
    MallocMetaData* prev;
} *MMD;

class BlocksLinkedList {
private:
    MMD list;
    MMD list_by_size;

public:
    size_t num_of_map;
    size_t bytes_of_map;
    BlocksLinkedList() : list(nullptr),list_by_size(nullptr),
                         num_of_map(0),bytes_of_map(0) {};
    MMD get_metadata(void *block);
    void* allocateBlock(size_t size);
    void insertBlockToList(MMD new_block);
    void freeBlock(void* block);
    void split(MMD block,size_t size);
    void insertToListSize(MMD block);
    void removeFromListAddress(MMD block);
    void removeFromListSize(MMD block);
    size_t getNumOfTotalBlocks();
    size_t getNumOfTotalBytes();
    size_t getNumOfFreeBlocks();
    size_t getNumOfFreeBytes();
};

void setGlobalCookie() //Challenge 5
{
    if (!cookie_flag)
    {
    std::srand(time(NULL));
    global_cookie = std::rand();
    cookie_flag =true;
    }
}


void checkOverFlow (MMD node) //Challenge 5
{

    if ((node != nullptr) && (node->private_cookie != global_cookie))
        exit(0xdeadbeef);
}

void plusminus()
{
    if (g_temp > 50)
        g_temp--;
    else
        g_temp++;
}

void multi()
{
    if (g_temp > 1000)
        g_temp = g_temp/2;
    else
        g_temp = g_temp*2;

}


////////////////////////////////////
// Blocks List //
//////////////////////////////////

MMD BlocksLinkedList::get_metadata(void *block) {
    return (MMD) ((size_t) block - sizeof(MallocMetaData));
}
void* BlocksLinkedList::allocateBlock(size_t size) {
    size_t allocation_size = sizeof(MallocMetaData) + size;
    MMD iterator = this->list_by_size, wilderness = nullptr;
    plusminus();
    while(iterator)
    {
        checkOverFlow (iterator);
        if (iterator->is_free && size <= iterator->size)
        {
            split(iterator, size);
            iterator->is_free = false;
            removeFromListSize(iterator);
            multi();
            return iterator;
        }
        wilderness = iterator;
        iterator = iterator->next;
        plusminus();
    }
    if(wilderness && wilderness->is_free)
    {
        multi();
        void* prog_break = sbrk(size-wilderness->size);
        if (prog_break == (void*) -1) {
            multi();
            return nullptr;
        }
        removeFromListSize(wilderness);
        wilderness->is_free= false;
        multi();
        wilderness->size=allocation_size - sizeof(MallocMetaData);
        plusminus();
        return wilderness;
    }
    void* prog_break = sbrk(allocation_size);
    multi();
    if (prog_break == (void*) -1) {
        return nullptr;
    }
    MMD new_alloc_block = (MMD) prog_break;
    new_alloc_block->is_free = false;
    new_alloc_block->size = size;
    multi();
    new_alloc_block->private_cookie = global_cookie;
    new_alloc_block->next_by_size = nullptr;
    new_alloc_block->prev_by_size = nullptr;
    multi();
    new_alloc_block->next = nullptr;
    new_alloc_block->prev = nullptr;
    insertBlockToList(new_alloc_block);
    plusminus();
    return prog_break;
}

void BlocksLinkedList::insertBlockToList(MMD new_block) {
    checkOverFlow(new_block);
    plusminus();
    if(this->list==nullptr)
    {
        this->list = new_block;
        multi();
        return;
    }
    MMD last = this->list;
    plusminus();
    MMD prev1 = last;
    while(last) {
        checkOverFlow(last);
        multi();
        prev1 = last;
        last = last->next;
    }
    prev1->next = new_block;
    plusminus();
    new_block->prev = prev1;
    multi();

}
void BlocksLinkedList::insertToListSize(MMD block) {
    checkOverFlow(block);
    multi();
    MMD iterator = this->list_by_size,prev=nullptr;
    plusminus();
    while (iterator) {
            checkOverFlow(iterator);
            plusminus();
        if ( iterator->size > block->size || ( iterator > block && iterator->size == block->size) ) {
            if (!iterator->prev_by_size) {
                multi();
                block->next_by_size= this->list_by_size;
                block->next_by_size->prev_by_size=block;
                plusminus();
                this->list_by_size =block;
            } else {
                 block->next_by_size= iterator;
                 multi();
                block->next_by_size->prev_by_size=block;
                block->prev_by_size=prev;
                plusminus();
                if(prev!=nullptr)
                {
                    multi();
                    block->prev_by_size->next_by_size=block;
                }
            }
            return;
        }
        prev=iterator;
        plusminus();
        iterator = iterator->next_by_size;
    }
    if(!prev)
    {
        multi();
        this->list_by_size=block;
         block->prev_by_size=nullptr;
         plusminus();
        block->next_by_size=nullptr;
        return;
    }
    checkOverFlow(prev);
    multi();
    block->prev_by_size=prev;
    block->next_by_size=iterator;
    prev->next_by_size=block;
    plusminus();
    multi();
}

void BlocksLinkedList::removeFromListAddress(MMD block)
{
    checkOverFlow(block);
    multi();
    if (block->prev)
    {
        checkOverFlow(block->prev);
        if (block->next) {
        checkOverFlow(block->next); 
        block->prev->next = block->next;
        multi();
        block->next->prev = block->prev;
        block->prev = nullptr;
        plusminus();
        block->next = nullptr;
    }
    else {
        block->prev->next = nullptr;
        plusminus();
        block->prev = nullptr;
        multi();
        return;
    }   
    }else 
    {
        this->list = block->next;
        block->next=nullptr;
        plusminus();
        if(this->list)
        {
            this->list->prev=nullptr;
        }
        multi();
        return;
    }
}

void BlocksLinkedList::removeFromListSize(MMD block)
{
    checkOverFlow(block);
    plusminus();
    if (block->prev_by_size)
    {
        checkOverFlow(block->prev_by_size);
        if (block->next_by_size) {
            checkOverFlow(block->next_by_size);
        block->prev_by_size->next_by_size = block->next_by_size;
        multi();
        block->next_by_size->prev_by_size = block->prev_by_size;
        block->prev_by_size = nullptr;
        plusminus();
        block->next_by_size = nullptr;
    }
    else {
        block->prev_by_size->next_by_size = nullptr;
        plusminus();
        block->prev_by_size = nullptr;
        multi();
        return;
    }       
    }else 
    {
        this->list_by_size = block->next_by_size;
        plusminus();
        block->next_by_size=nullptr;
        multi();
        if(this->list_by_size)
        {
            this->list_by_size->prev_by_size=nullptr;
        }
        plusminus();
        return;
    }

}

void BlocksLinkedList::freeBlock(void* ptr) {
    MMD block = get_metadata(ptr);
    multi();
    checkOverFlow(block);
    plusminus();
    if(block->prev && block->next && block->next->is_free && block->prev->is_free)
    {
        checkOverFlow(block->prev);
        multi();
        checkOverFlow(block->next);
        MMD prev_block = block->prev;
        removeFromListSize(block->prev);
        removeFromListSize(block->next);
        multi();
        block->prev->size = (block->next->size + block->prev->size + block->size + 2 * sizeof(MallocMetaData) );
        removeFromListAddress(block->next);
        removeFromListAddress(block);
        plusminus();
        insertToListSize(prev_block);
    }
    else if(block->prev && block->prev->is_free)
    {
        checkOverFlow(block->prev);
        multi();
        MMD prev_block = block->prev;
        removeFromListSize(block);
        removeFromListSize(block->prev);
        block->prev->size = (sizeof(MallocMetaData) + block->prev->size + block->size );
        plusminus();
        removeFromListAddress(block);
        insertToListSize(prev_block);
        multi();
    }
    else if(!(block->next  && block->next->is_free))
    {
        multi();
        insertToListSize(block);
        plusminus();
      
    }
    else
    {
        multi();
        checkOverFlow(block->next);
        removeFromListSize(block);
        removeFromListSize(block->next);
        plusminus();
        block->size = (sizeof(MallocMetaData) + block->size + block->next->size );
        removeFromListAddress(block->next);
        multi();
        insertToListSize(block);
    }
        block->is_free = true;
        plusminus();

}

void BlocksLinkedList::split(MMD block, size_t size)
{
    multi();
    checkOverFlow(block);
    plusminus();
    if(128 + size + sizeof(MallocMetaData) > block->size )
    {
        multi();
        return;
    }
    MMD new_alloc = (MMD) ((char *) block + sizeof(MallocMetaData) + size );
    new_alloc->size = block->size - size - sizeof(MallocMetaData);
    new_alloc->private_cookie = global_cookie;
    multi();
    new_alloc->is_free = true;
    block->size = size;
    plusminus();

    if(block->next && block->next->is_free)
    {
        checkOverFlow(block->next);
        multi();
        new_alloc->size = new_alloc->size + sizeof(MallocMetaData) + block->next->size;
        removeFromListSize(block->next);
        plusminus();
        removeFromListAddress(block->next);
    }
    insertToListSize(new_alloc);
    multi();
    removeFromListSize(block);
    plusminus();
    new_alloc->prev = block;
    multi();
    new_alloc->next = block->next;
    plusminus();
    if(block->next)
    {
        checkOverFlow(block->next);
        multi();
        block->next->prev = new_alloc;
    }
    multi();
    block->next = new_alloc;
    plusminus();
}

size_t BlocksLinkedList::getNumOfTotalBlocks() {
    MMD iterator = this->list;
    multi();
    size_t counter = 0;
    plusminus();
    while (iterator) {
         checkOverFlow(iterator);
         multi();
        if (iterator->size <= MAP_SIZE) {
            counter= counter+1;
            multi();
            iterator = iterator->next;
        }
    }
    counter= counter + this->num_of_map;
    plusminus();
    return counter;
}

size_t BlocksLinkedList::getNumOfTotalBytes() {
    MMD iterator = this->list;
    multi();
    size_t counter = 0;
    plusminus();
    while (iterator) {
        checkOverFlow(iterator);
        if(iterator->size <= MAP_SIZE) {
            multi();
            counter = counter + iterator->size;
            iterator = iterator->next;
        }
    }
    multi();
    counter = counter + this->bytes_of_map;
    plusminus();
    return counter;
}

size_t BlocksLinkedList::getNumOfFreeBlocks() {
    MMD iterator = this->list;
    multi();
    size_t counter = 0;
    plusminus();
    while (iterator) {
        checkOverFlow(iterator);
        multi();
        if(iterator->size<=MAP_SIZE &&iterator->is_free) {
            counter= counter+1;
        }
        iterator = iterator->next;
        multi();
    }
    plusminus();
    return counter;
}

size_t BlocksLinkedList::getNumOfFreeBytes() {
    MMD iterator = this->list;
    multi();
    size_t counter = 0;
    plusminus();
    while (iterator) {
        checkOverFlow(iterator);
        multi();
        if (iterator->size<=MAP_SIZE&&iterator->is_free) {
            counter = counter +  iterator->size;
        }
        multi();
        iterator = iterator->next;
        plusminus();
    }
    return counter;
}

///////////////////////////////////
//  malloc  //
/////////////////////////////////

BlocksLinkedList blocks_list =  BlocksLinkedList(); 
void* smalloc(size_t size) {
    setGlobalCookie();
    plusminus();
    if (size > MAX_VAL || size == 0) {
        multi();
        return nullptr;
    }
    if (size >= MAP_SIZE) {
        void *block = mmap(nullptr, (size + sizeof(MallocMetaData) ), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (block == MAP_FAILED) {
            multi();
            return nullptr;
        }
        MMD my_block=(MMD)block;
        multi();
        my_block->size = (size);
        plusminus();
         my_block->private_cookie = global_cookie;
         multi();
         my_block->is_free = false;
        blocks_list.num_of_map = blocks_list.num_of_map +1;
        multi();
        blocks_list.bytes_of_map= blocks_list.bytes_of_map + (size);
        return (char*)block+sizeof(MallocMetaData);
    }

    void* prog_break = blocks_list.allocateBlock(size);
    plusminus();
    if (prog_break == (void*) -1) {
        multi();
        return nullptr;
    }
    plusminus();
    return (char*) prog_break + sizeof(MallocMetaData); 
}

void* scalloc(size_t num, size_t size) {
    setGlobalCookie();
    plusminus();
    void* ptr = smalloc(size * num);
    if (!ptr) {
        multi();
        return nullptr;
    }
    plusminus();
    memset(ptr, 0, size * num);
    multi();
    return ptr;
}

void sfree(void* p) {
    setGlobalCookie();
    plusminus();
    if (!p) {
        multi();
        return;
    }
    MMD data = blocks_list.get_metadata(p);
    plusminus();
    if(MAP_SIZE > data->size)
    {
        multi();
        blocks_list.freeBlock(p);
    }
    else
    {
        blocks_list.num_of_map--;
        multi();
        blocks_list.bytes_of_map -= data->size;
        plusminus();
        munmap(data, sizeof(MallocMetaData) + data->size);
    }
    multi();
}

void* srealloc(void* oldp, size_t size) {
    setGlobalCookie();
    plusminus();
    if (size > MAX_VAL || size == 0 ) {
        multi();
        return nullptr;
    }
    if (!oldp) {
        plusminus();
        return smalloc(size);
    }
    MMD oldb = blocks_list.get_metadata(oldp);
    multi();
    if (oldb->size >= MAP_SIZE)
    {
        plusminus();
        if (oldb->size == size)
        {
            return oldp;
        }
        sfree(oldp);
        multi();
        return smalloc(size);
    }
    size_t size_old = oldb->size;
    multi();
    if (size <= size_old) { 
        plusminus();
        blocks_list.split(oldb,size);
        multi();
        return oldp;
    }
    unsigned int possible_size = size_old;
    plusminus();
    if(oldb->prev && oldb->prev->is_free)
    {
        plusminus();
        possible_size = possible_size + sizeof(MallocMetaData) + oldb->prev->size ;
    }
    if (possible_size < size)
    {
        multi();
         if(oldb->next) 
        {
            plusminus();
            possible_size = size_old;
            if(oldb->next->is_free)
            {
                multi();
                possible_size = possible_size + sizeof(MallocMetaData) + oldb->next->size ;
            }
            if(possible_size >= size)
            {
                multi();
                blocks_list.removeFromListSize(oldb);
                blocks_list.removeFromListSize(oldb->next);
                plusminus();
                oldb->size = sizeof(MallocMetaData) + oldb->next->size + oldb->size ;
                blocks_list.removeFromListAddress(oldb->next);
                multi();
                blocks_list.split(oldb,size);
                return oldp;
            }
        }
        else
        {
            void* prog_break = sbrk(size - possible_size);
            multi();
            if (prog_break == (void*) -1) {
                plusminus();
                return nullptr;
            }
            if(oldb->prev && oldb->prev->is_free)
            {
                multi();
                MMD prev_block = oldb->prev;
                plusminus();
                oldb->prev->is_free = false;
                oldb->prev->size = size;
                blocks_list.removeFromListAddress(oldb);
                multi();
                memmove( (char *) prev_block + sizeof(MallocMetaData), oldp, oldb->size);
                return  ((char *) prev_block) + sizeof(MallocMetaData);
            }
            else {
                oldb->is_free = false;
                plusminus();
                 oldb->size = size;
                 multi();
                return oldp;
            }
        }
    }
    else
    {
        MMD prev_block = oldb->prev;
        plusminus();
        blocks_list.removeFromListSize(oldb);
        blocks_list.removeFromListSize(oldb->prev);
        multi();
        oldb->prev->is_free = false;
        oldb->prev->size = possible_size;
        plusminus();
        blocks_list.removeFromListAddress(oldb);
        multi();
        blocks_list.split(prev_block, size);
        plusminus();
        memmove( (char *) prev_block + sizeof(MallocMetaData), oldp, oldb->size);
        multi();
        return (char *) prev_block + sizeof(MallocMetaData);
    }

    possible_size = size_old;
    multi();
    if(oldb->prev != nullptr && oldb->prev->is_free)
    {
        plusminus();
        possible_size = possible_size + sizeof(MallocMetaData) + oldb->prev->size;
    }
    if(oldb->next !=nullptr && oldb->next->is_free)
    {
        multi();
        possible_size = possible_size + sizeof(MallocMetaData) + oldb->next->size;
    }
    if(possible_size < size)
    {
        multi();
        if(oldb->next->next) 
        {
            void* newp = smalloc(size);
            memmove(newp, oldp, oldb->size);
            plusminus();
            sfree(oldp);
            return newp;
        }
        else
        {
            multi();
            void* prog_break = sbrk(size - possible_size);
            if (prog_break == (void*) -1) {
                plusminus();
                return nullptr;
            }
            multi();
            blocks_list.removeFromListSize(oldb);
            blocks_list.removeFromListSize(oldb->next);
            plusminus();
            oldb->is_free = false;
            oldb->size = size;
            blocks_list.removeFromListAddress(oldb->next);
            multi();
            if(oldb->prev && oldb->prev->is_free)
            {
                MMD prev_block = oldb->prev;
                plusminus();
                prev_block->is_free = false;
                prev_block->size = oldb->size;
                multi();
                blocks_list.removeFromListSize(oldb);
                plusminus();
                blocks_list.removeFromListSize(oldb->prev);
                blocks_list.removeFromListAddress(oldb);
                multi();
                memmove( (char *) prev_block + sizeof(MallocMetaData), oldp, oldb->size);
                return (char *) prev_block + sizeof(MallocMetaData);
            }
            plusminus();
            return oldp;
        }
    }
    else
    {
        multi();
        MMD prev_block = oldb->prev;
        oldb->prev->is_free=false;
        plusminus();
        blocks_list.removeFromListSize(oldb->prev);
        blocks_list.removeFromListSize(oldb->next);
        plusminus();
        blocks_list.removeFromListSize(oldb);
        oldb->prev->size = possible_size;
        multi();
        blocks_list.removeFromListAddress(oldb->next);
        plusminus();
        blocks_list.removeFromListAddress(oldb);
        blocks_list.split(prev_block, size);
        multi();
        memmove( (char *) prev_block + sizeof(MallocMetaData), oldp, oldb->size);
        return (char *) prev_block + sizeof(MallocMetaData);
    }

    void* newp = smalloc(size);
    if (!newp) {
        multi();
        return nullptr;
    }
    memmove(newp, oldp, oldb->size);
    plusminus();
    sfree(oldp);
    multi();
    return newp;
}


size_t _num_free_blocks() {
    return blocks_list.getNumOfFreeBlocks();
}

size_t _num_free_bytes() {
    return blocks_list.getNumOfFreeBytes();
}

size_t _num_allocated_blocks() {
    return blocks_list.getNumOfTotalBlocks();
}

size_t _num_allocated_bytes() {
    return blocks_list.getNumOfTotalBytes(); 
}

size_t _num_meta_data_bytes() {
    return blocks_list.getNumOfTotalBlocks() * sizeof(MallocMetaData);
}

size_t _size_meta_data() {
    return sizeof(MallocMetaData);
}