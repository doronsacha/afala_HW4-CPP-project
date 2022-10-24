#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <cstring>


#define CURRENT_HEAP sbrk((intptr_t)0)

typedef struct MallocMetadata
{
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
}MallocMetadata;

///***********************************************************************************************************************************///
///****************************************    segel helpers functions headers     **************************************************///
///*********************************************************************************************************************************///

size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _size_meta_data();
size_t _num_meta_data_bytes();

///***********************************************************************************************************************************///
///****************************************    helpers functions headers    *********************************************************///
///*********************************************************************************************************************************///

bool the_block_is_free_and_enough(MallocMetadata* block,size_t size);
void* allocate_new_ptr(size_t size);
void initialize_ya_block_metadata(MallocMetadata* block, size_t size,bool is_free);
void initialize_la_block_metadata(MallocMetadata* ptr_to_la, size_t size,bool is_free);
void* advance_k_bit(void* p,size_t k);
MallocMetadata* get_the_ya_metadata_of_ptr(void* ptr);
MallocMetadata* get_the_la_metadata_of_ptr(void* ptr,size_t size);
void initialize_ptr(void* ptr, size_t size,bool is_free);
void initialize_block(MallocMetadata* block,size_t size,bool is_free );
bool can_break(MallocMetadata* block, size_t size);
size_t calculate_size_of_new_block(MallocMetadata* initial_block,size_t size);
MallocMetadata* get_the_begin_of_new_cut_block(MallocMetadata* initial_block,size_t size);
MallocMetadata* create_and_get_new_block(MallocMetadata* initial_block, size_t size);
void cut_initial_block(MallocMetadata* initial_block, size_t size);
void insert_block_to_list(MallocMetadata* new_block);
MallocMetadata* break_the_block(MallocMetadata *initial_block, size_t size);
void* find_block_with_at_least(size_t size);
MallocMetadata* get_la_metadata_of_the_previous_block(MallocMetadata* block);
MallocMetadata* get_ya_metadata_of_next_block(MallocMetadata* block);
bool previous_block_free(void* ptr_to_release);
bool next_block_free(void* ptr_to_release);
void* go_back_k_bit(void* p,size_t k);
MallocMetadata* get_the_ya_meta_from_la(MallocMetadata* la_metadata);
MallocMetadata* get_the_previous_block(MallocMetadata* block);
void remove_the_blocks_from_list(MallocMetadata* first_block,MallocMetadata* second_block);
MallocMetadata* join_prev_block(void* ptr_to_release);
MallocMetadata *get_the_next_block(MallocMetadata *block);
MallocMetadata* join_next_block(void* ptr_to_release);
bool sizeValid(size_t size);
MallocMetadata* get_the_topmost();
bool the_last_chunk_is_free();
size_t get_size_of_last_chunk();
void* join_topmost_and_new_block(size_t size);
MallocMetadata *join_if_needed(MallocMetadata *ptr_to_release);

//list functions
MallocMetadata* find_prev_block_by_size(size_t size);
bool the_list_is_empty();
void insert_block_in_middle(MallocMetadata* prev_block, MallocMetadata* block_to_insert);
void insert_block_to_tail(MallocMetadata* block_to_insert);
MallocMetadata* find_prev_block_by_ptr(MallocMetadata* ptr_block);
void remove_the_head();
void remove_the_tail();
void remove_midl_block_from_list(MallocMetadata* prev_block);
void remove_block_from_list(MallocMetadata* block);
void insert_block_to_list_empty(MallocMetadata* new_block);
void insert_block_to_head(MallocMetadata* new_block);
void insert_block_to_non_empty_list(MallocMetadata* new_block, size_t size);
void insert_block_to_list(MallocMetadata* new_block, size_t size);


///***********************************************************************************************************************************///
///*******************************   malloc, calloc, free, realoc,... headers    ****************************************************///
///*********************************************************************************************************************************///
void * smalloc(size_t size);
void * scalloc(size_t num, size_t size);
void sfree(void* p);
void * srealloc(void * oldp, size_t size);


#include <sys/mman.h>

///***********************************************************************************************************************************///
///****************************************    Global   *************************************************************************///
///*********************************************************************************************************************************///

MallocMetadata *head = nullptr;
MallocMetadata *tail = nullptr;

size_t mmap_blocks = 0;
size_t mmap_bytes = 0;
void* heap = nullptr;


void* getAddress(void* start, size_t num_bytes, bool is_inc){
    char* tmp = (char*)start;
    if(is_inc){
        tmp += num_bytes;
    } else{
        tmp -= num_bytes;
    }
    return (void*)tmp;
}


void init_heap()
{
    if(heap== nullptr)
    {
        void* iw = sbrk(0) ;  
        long ptr = (long)iw;
        long reste = ptr % 8;
        if(reste != 0)
        {
            void* old_iw = sbrk((intptr_t)(8 - reste));
            heap = getAddress(old_iw, (size_t)(8 - reste), true);
        }
        else
        {
            heap = iw;
        }
    }
}

///***********************************************************************************************************************************///
///****************************************    segel helpers functions     **********************************************************///
///*********************************************************************************************************************************///

size_t align(size_t a)
{
    a=((a+7)/8)*8;
    return a;
}

size_t _num_free_blocks()
{
    if (head == nullptr)
    {
        return 0;
    }
    MallocMetadata *block = head;
    int counter = 0;
    while (block != nullptr)
    {
        if (block->is_free)
        {
            counter++;
        }
        block = block->next;
    }
    return counter;
}

size_t _num_free_bytes()
{
    if (head == nullptr)
    {
        return 0;
    }
    MallocMetadata *block = head;
    int counter = 0;
    while (block != nullptr)
    {
        if (block->is_free)
        {
            counter += block->size;
        }
        block = block->next;
    }
    return counter;
}

size_t _num_allocated_blocks()
{
    if (head == nullptr)
    {
        return mmap_blocks;
    }
    MallocMetadata *block = head;
    int counter = 0;
    while (block != nullptr)
    {
        counter++;
        block = block->next;
    }
    return counter+mmap_blocks;
}

size_t _num_allocated_bytes()
{
    if (head == nullptr)
    {
        return mmap_bytes;
    }
    MallocMetadata *block = head;
    int counter = 0;
    while (block != nullptr)
    {
        counter += block->size;
        block = block->next;
    }
    return counter+mmap_bytes;
}

size_t _size_meta_data()
{
    return 2*sizeof(MallocMetadata);
}

size_t _num_meta_data_bytes()
{
    return _num_allocated_blocks() * _size_meta_data();
}

///***********************************************************************************************************************************///
///****************************************    mmap functions     ****************************************************************///
///*********************************************************************************************************************************///


void* mmap_malloc(size_t size)
{
    size = align(size);
    void * ptr_to_mem = mmap(NULL, size + _size_meta_data(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (ptr_to_mem == (void*)(-1))
    {
        return nullptr;
    }
    MallocMetadata* mmap_meta_data = (MallocMetadata*)ptr_to_mem;
    mmap_meta_data->size = size; //only size matters here, it isn't connected to other mmap memory areas nor can it ever be free

    mmap_bytes += size;
    mmap_blocks++;
    //size_t a = _num_allocated_bytes();
    return mmap_meta_data + 1;
}

void mmap_free(void* p)
{
    MallocMetadata* meta = (MallocMetadata*) p;
    meta--;
    mmap_blocks--;
    mmap_bytes -= meta->size;
    munmap((void*)meta, meta->size + _size_meta_data());
}

size_t min(size_t a, size_t b)
{
    if (a<b){
        return a;
    }
    return b;
}

void* mmap_realloc(void* p, size_t size)
{
    size = align(size);
    MallocMetadata* old_meta = (MallocMetadata*) p;
    old_meta--;
    if (size == old_meta->size)
    {
        return p;
    }
    MallocMetadata* new_allocation = (MallocMetadata*)mmap_malloc(size);
    if(new_allocation == nullptr)
    {
        return nullptr;
    }
    size_t minimum = min(size,old_meta->size);
    memmove(new_allocation,p,minimum);
    mmap_free(p);
    return new_allocation;
}

///***********************************************************************************************************************************///
///****************************************    helpers functions     ****************************************************************///
///*********************************************************************************************************************************///


//get a block
//return bool
bool the_block_is_free_and_enough(MallocMetadata *block, size_t size)
{
    if (block->size >= size && block->is_free)
    {
        return true;
    }
    return false;
}

//move the sbrk and return a ptr
void* allocate_new_ptr(size_t size)
{
    MallocMetadata *new_ptr = (MallocMetadata *) sbrk((intptr_t) (size + (_size_meta_data())));
    new_ptr++;
    return (void*)(new_ptr);
}


//get a metadata
void initialize_ya_block_metadata(MallocMetadata *block, size_t size, bool is_free)
{
    block->size = size;
    block->is_free = is_free;
    block->next = nullptr;
    block->prev = nullptr;
}

//get a metadata
void initialize_la_block_metadata(MallocMetadata *ptr_to_la, size_t size, bool is_free)
{
    ptr_to_la->size = size;
    ptr_to_la->is_free = is_free;
    ptr_to_la->next = nullptr;
    ptr_to_la->prev = nullptr;
}

//get a pointer
//doesn't change the initial pointer
//return a pointer to the memory space after k bits
void *advance_k_bit(void *p, size_t k)
{
    char *temp = (char *) p;
    temp += k;
    return (void *) temp;
}

//get a pointer
//return a metadata
MallocMetadata* get_the_ya_metadata_of_ptr(void *ptr)
{
    MallocMetadata *temp = (MallocMetadata *) ptr;
    temp--;
    return temp;
}

//get a pointer
//return a metadata
MallocMetadata *get_the_la_metadata_of_ptr(void *ptr, size_t size)
{
    void *temp = (MallocMetadata *) ptr;
    temp = advance_k_bit(temp, size);
    return (MallocMetadata *) temp;
}

//get a pointer
void initialize_ptr(void *ptr, size_t size, bool is_free)
{
    MallocMetadata *new_block = get_the_ya_metadata_of_ptr(ptr);
    initialize_ya_block_metadata(new_block, size, is_free);
    MallocMetadata *la_metadata_of_new_block = get_the_la_metadata_of_ptr(ptr, size);
    initialize_la_block_metadata(la_metadata_of_new_block, size, is_free);
}

//get a metadata
bool sufficient(MallocMetadata* biger_block, size_t size)
{
    if (biger_block->size >= size)
    {
        return true;
    }
    return false;
}

//get a metadata
void initialize_block(MallocMetadata *block, size_t size, bool is_free)
{
    initialize_ya_block_metadata(block, size, is_free);
    initialize_la_block_metadata(get_the_la_metadata_of_ptr(block+1,size), size, is_free);
}

//get a metadata
bool can_break(MallocMetadata *block, size_t size)
{
    size_t size_of_block = block->size;
    int res = (int)(size_of_block+_size_meta_data()-size-2*_size_meta_data());
    if (res >= 128)
    {
        return true;
    }
    return false;
}

//get a metadata
size_t calculate_size_of_new_block(MallocMetadata *initial_block, size_t size)
{
    size_t size_of_initial_block = initial_block->size-_size_meta_data();
    return size_of_initial_block - size;
}

//get a metadtata
//return a metadata
MallocMetadata* get_the_begin_of_new_cut_block(MallocMetadata *initial_block, size_t size)
{
    initial_block++;//pass the ya metadata
    initial_block = (MallocMetadata *) advance_k_bit((void *) initial_block, size);
    initial_block++;
    return initial_block;
}

//get a metadata
//return a metadata
MallocMetadata *create_and_get_new_block(MallocMetadata *initial_block, size_t size)
{
    size_t new_size = calculate_size_of_new_block(initial_block, size);
    MallocMetadata *new_block = get_the_begin_of_new_cut_block(initial_block, size);
    initialize_block(new_block, new_size, true);
    return new_block;
}

//get a metadata
void cut_initial_block(MallocMetadata *initial_block, size_t size)
{
    initialize_block(initial_block, size, false);
}

//TODO: change the name of the function
//get a metadata
void insert_block_to_list(MallocMetadata *new_block)
{
    insert_block_to_list(new_block, new_block->size);
}

//get a metadata
//return a pointer
MallocMetadata* break_the_block(MallocMetadata *initial_block, size_t size)
{
    remove_block_from_list(initial_block);

    MallocMetadata *new_block = initial_block+1;
    MallocMetadata* to_enter_initial  = initial_block;
    new_block  =(MallocMetadata *) advance_k_bit(new_block,size);
    new_block++;
    MallocMetadata* to_enter = new_block;
    new_block->size = (initial_block->size - _size_meta_data() - size);
    new_block->is_free = true;
    new_block->prev = nullptr;
    new_block->next = nullptr;
    new_block = new_block + 1 ;
    new_block = (MallocMetadata *)advance_k_bit(new_block,(initial_block->size - _size_meta_data() - size));
    new_block->size = (initial_block->size - _size_meta_data() - size);
    new_block->is_free = true;
    new_block->prev = nullptr;
    new_block->next = nullptr;


    initial_block->size =size;
    initial_block->is_free = false;
    initial_block->prev = nullptr;
    initial_block->next = nullptr;
    initial_block = initial_block + 1 ;
    initial_block = (MallocMetadata *)advance_k_bit(initial_block,size);
    initial_block->size = size;
    initial_block->is_free = false;
    initial_block->prev = nullptr;
    initial_block->next = nullptr;

    insert_block_to_list(to_enter);
    insert_block_to_list(to_enter_initial);

    initial_block++;
    //MallocMetadata* a = get_the_ya_metadata_of_ptr(to_enter+1);
    //MallocMetadata* b = get_the_next_block(get_the_ya_metadata_of_ptr(to_enter+1));
    join_if_needed(to_enter+1);
    return to_enter_initial+1;
}

//get a size
//return a pointer
void* enlarge_top_most(size_t size)
{
    size_t size_of_last = get_size_of_last_chunk();
    MallocMetadata* last_top_most = get_the_topmost();
    MallocMetadata* new_block =(MallocMetadata*) sbrk((intptr_t)(size-size_of_last));
    if(new_block==(void*)-1)
    {
        return nullptr;
    }
    else
    {
        MallocMetadata* aD= get_the_ya_meta_from_la(last_top_most);
        remove_block_from_list(aD);
        aD->size = size;
        aD->is_free = false;
        MallocMetadata* temp = (MallocMetadata*)advance_k_bit(aD,size);
        temp++;
        temp->size = size;
        temp->is_free = false;
        insert_block_to_list(aD);
        return aD+1;
    }
}

//return a pointer
void *find_block_with_at_least(size_t size)
{
    MallocMetadata *block = head;
    while (block != nullptr)
    {
        if (the_block_is_free_and_enough(block, size))
        {
            if (can_break(block, size))
            {
                break_the_block(block, size);
            }
            else
            {
                block->is_free=false;
                block++;
                //MallocMetadata* temp= get_the_la_metadata_of_ptr(block,size);
                block->is_free=false;
                block--;
            }
            block++;
            return (void *) (block);
        }
        block = block->next;
    }
    if(the_last_chunk_is_free())
    {
        return enlarge_top_most(size);
    }
    return nullptr;
}

//return a metadata
void *find_block_at_least(size_t size)
{
    MallocMetadata *block = head;
    while (block != nullptr)
    {
        if (the_block_is_free_and_enough(block, size))
        {
            return block;
        }
        block = block->next;
    }
    return nullptr;
}

//get a metadata
//return a metadata
MallocMetadata *get_la_metadata_of_the_previous_block(MallocMetadata *block)
{
    block--;
    return block;
}

//get a metadata
//return a metadata
MallocMetadata *get_ya_metadata_of_next_block(MallocMetadata *block)
{
    size_t block_size = block->size;
    block++;
    block = (MallocMetadata *) advance_k_bit((void *) block, block_size);
    block++;
    return block;
}

//check that the previous is not the beginig of the heap
//ptr to release is just after the head meta data
bool previous_block_free(void *ptr_to_release)
{
    void *block = (void *) get_the_ya_metadata_of_ptr(ptr_to_release);
    if (block <= heap)
    {
        return false;
    }
    MallocMetadata *previous_block_la_metadata = get_la_metadata_of_the_previous_block((MallocMetadata *) block);
    if (previous_block_la_metadata->is_free)
    {
        return true;
    }
    return false;
}

//get a pointer
bool next_block_free(void *ptr_to_release)
{
    MallocMetadata *block = get_the_ya_metadata_of_ptr(ptr_to_release);
    MallocMetadata *next_block = get_ya_metadata_of_next_block(block);
    if (((void *) next_block) >= CURRENT_HEAP)
    {
        return false;
    }
    if (next_block->is_free)
    {
        return true;
    }
    return false;
}

void *go_back_k_bit(void *p, size_t k)
{
    char *temp = (char *) p;
    temp -= k;
    return (void *) temp;
}

//get a metadata
//return a metadata
MallocMetadata *get_the_ya_meta_from_la(MallocMetadata *la_metadata)
{
    size_t size = la_metadata->size;
    MallocMetadata *ptr = (MallocMetadata *) go_back_k_bit((void *) la_metadata, size);
    return get_the_ya_metadata_of_ptr(ptr);
}

//get a metadata
//return a metadata
MallocMetadata *get_the_previous_block(MallocMetadata *block)
{
    MallocMetadata *la_previous_block = get_la_metadata_of_the_previous_block(block);
    return get_the_ya_meta_from_la(la_previous_block);
}

//get two metadata
void remove_the_blocks_from_list(MallocMetadata *first_block, MallocMetadata *second_block)
{
    remove_block_from_list(first_block);
    remove_block_from_list(second_block);
}


/**
 *
 * @param pointer
 * @return metadata
 */
MallocMetadata *join_prev_block(void *ptr_to_release)
{
    MallocMetadata *initial_block = get_the_ya_metadata_of_ptr(ptr_to_release);
    MallocMetadata *previous_block = get_the_previous_block(initial_block);
    size_t new_size = initial_block->size + previous_block->size+_size_meta_data();
    remove_the_blocks_from_list(initial_block, previous_block);
    initialize_block(previous_block, new_size, true);
    insert_block_to_list(previous_block);
    return previous_block;
}

/**
 *
 * @param metadata
 * @return metadata
 */
MallocMetadata *get_the_next_block(MallocMetadata *block)
{
    size_t size_of_block = block->size;
    block++;
    MallocMetadata *next_block = (MallocMetadata *) advance_k_bit((void *) block, size_of_block);
    next_block++;
    return next_block;
}

//get a pointer
//return a metadata
MallocMetadata *join_next_block(void *ptr_to_release)
{
    MallocMetadata *initial_block = get_the_ya_metadata_of_ptr(ptr_to_release);
    MallocMetadata *next_block = get_the_next_block(initial_block);
    next_block++;
    void *ptr = (void *) (next_block);
    return join_prev_block(ptr);
}

bool sizeValid(size_t size)
{
    if (size == 0 || size > 100000000)
    {
        return false;
    }
    return true;
}

//return a metadata
MallocMetadata *get_the_topmost()
{
    MallocMetadata *curr_heap = (MallocMetadata *) CURRENT_HEAP;
    curr_heap--;
    return curr_heap;
}

bool the_last_chunk_is_free()
{
    MallocMetadata *curr_heap = get_the_topmost();
    if (curr_heap->is_free)
    {
        return true;
    }
    return false;
}

size_t get_size_of_last_chunk()
{
    MallocMetadata *curr_heap = get_the_topmost();
    return curr_heap->size;
}


//get a pointer
//return a pointer
MallocMetadata *join_if_needed(MallocMetadata *ptr_to_release)
{
    MallocMetadata *block;
    MallocMetadata* temp = ptr_to_release;
    if (next_block_free(temp))
    {
        block = join_next_block(temp);
        block++;
        temp = block;
    }
    if (previous_block_free(temp))
    {
        block = join_prev_block(temp);
        block++;
        temp = block;
    }
    return temp;
}

///****************************************    list functions     ****************************************************************///

// if there is no prev(smallest size), returning nullptr
//doron should check case of empty list
//doron should also check if value returned is the tail, because insertion is different
MallocMetadata *find_prev_block_by_size(size_t size)
{
    MallocMetadata *it = head;
    MallocMetadata *it_next = head->next;

    if (it->size > size)
    {
        //case of our node being first
        return nullptr;
    }
    while (it_next != nullptr)
    {
        if (size >= it->size && size <= it_next->size)
        {
            return it;
        }
        it_next = it_next->next;
        it = it->next;
    }
    return it;
}

bool the_list_is_empty()
{
    if (tail == nullptr)
    {
        return true;
    }
    return false;
}

void insert_block_in_middle(MallocMetadata *prev_block, MallocMetadata *block_to_insert)
{
    MallocMetadata *next_node = prev_block->next;
    prev_block->next = block_to_insert;
    block_to_insert->next = next_node;
    next_node->prev = block_to_insert;
    block_to_insert->prev = prev_block;
}

void insert_block_to_tail(MallocMetadata *block_to_insert)
{
    block_to_insert->prev = tail;
    tail->next = block_to_insert;
    tail = block_to_insert;
}

MallocMetadata *find_prev_block_by_ptr(MallocMetadata *ptr_block)
{
    MallocMetadata *it = head;
    if (it == ptr_block)
    {
        return nullptr;
    }
    while (it != nullptr)
    {
        if (it->next != nullptr && it->next == ptr_block)
        {
            return it;
        }
        it = it->next;
    }
    //we don't need to return at this point
    return nullptr;
}

void remove_the_head()
{
    if (head == nullptr)
    {
        return;
    }
    if (head->next == nullptr)
    {
        head = nullptr;
        tail = nullptr;
    } else {
        head->next->prev = nullptr;
        head = head->next;
    }
}

void remove_the_tail()
{
    if (tail == nullptr)
    {
        return;
    }
    if (tail->prev == nullptr)
    {
        head = nullptr;
        tail = nullptr;
    } else
    {
        tail->prev->next = nullptr;
        tail = tail->prev;
    }
}

void remove_midl_block_from_list(MallocMetadata *prev_block)
{
    MallocMetadata *next_node = prev_block->next->next;
    MallocMetadata *node_to_del = prev_block->next;
    prev_block->next = next_node;
    next_node->prev = prev_block;
    node_to_del->next = nullptr;
    node_to_del->prev = nullptr;
}

void remove_block_from_list(MallocMetadata *block)
{
    MallocMetadata *prev_block = find_prev_block_by_ptr(block);
    if (prev_block == nullptr)
    {
        remove_the_head();
    }
    else if (block == tail)
    {
        remove_the_tail();
    }
    else
    {
        remove_midl_block_from_list(prev_block);
    }
}

void insert_block_to_list_empty(MallocMetadata *new_block)
{
    head = new_block;
    tail = new_block;
}

void insert_block_to_head(MallocMetadata *new_block)
{
    head->prev = new_block;
    new_block->next = head;
    head = new_block;
}

void insert_block_to_non_empty_list(MallocMetadata *new_block, size_t size)
{
    MallocMetadata *prev_block = find_prev_block_by_size(size);
    if (prev_block == nullptr)
    {
        insert_block_to_head(new_block);
    }
    else if (prev_block == tail)
    {
        insert_block_to_tail(new_block);
    }
    else
    {
        insert_block_in_middle(prev_block, new_block);
    }
}

void insert_block_to_list(MallocMetadata *new_block, size_t size)
{
    if (the_list_is_empty())
    {
        insert_block_to_list_empty(new_block);
    } else
    {
        insert_block_to_non_empty_list(new_block, size);
    }
}

///***********************************************************************************************************************************///
///****************************************    mallocs,realoc,calloc,...     ********************************************************///
///*********************************************************************************************************************************///



void* init_block_in_realoc(MallocMetadata* block,void* ptr,bool is_free)
{
    block->is_free = is_free;
    MallocMetadata *temp = get_the_la_metadata_of_ptr(ptr, (block->size)-32);
    temp->is_free = is_free;
    return ptr;
}


void *smalloc(size_t size)
{
    init_heap();
    if (!sizeValid(size))
    {
        return NULL;
    }
    if (size >= (128*1024)){
        return mmap_malloc(size);
    }
    size = align(size);
    if (the_last_chunk_is_free())
    {
        if (can_break(get_the_topmost(),size))
        {
            // MallocMetadata* la_meta = get_the_topmost();
            //MallocMetadata* ya_meta = get_the_ya_meta_from_la(get_the_topmost());
            return break_the_block(get_the_ya_meta_from_la(get_the_topmost()), size);
        }
        if(!sufficient(get_the_topmost(),size))
        {
            enlarge_top_most(size);
            return get_the_ya_meta_from_la(get_the_topmost())+1;
        }
        else
        {
            get_the_ya_meta_from_la(get_the_topmost())->is_free = false;
            get_the_topmost()->is_free = false;
            return get_the_ya_meta_from_la(get_the_topmost())+1;
        }
    }
    void *node_enough = find_block_at_least(size);
    if (node_enough != NULL)
    {
        if(can_break((MallocMetadata*)node_enough,size))
        {
            return break_the_block((MallocMetadata*)node_enough,size);
        }
        init_block_in_realoc((MallocMetadata*)node_enough,(MallocMetadata*)(node_enough)+1, false);
        return ((MallocMetadata*)(node_enough)+1);
    }
    MallocMetadata *new_ptr = (MallocMetadata *) allocate_new_ptr(size);
    if (new_ptr == ((void *) -1))
    {
        return nullptr;
    }
    initialize_ptr(new_ptr, size, false);
    MallocMetadata* new_block = get_the_ya_metadata_of_ptr(new_ptr);
    insert_block_to_list(new_block, size);
    return (void *) (new_ptr);
}

void *scalloc(size_t num, size_t size)
{
    init_heap();
    void *new_ptr = smalloc(num * size);
    if (new_ptr == NULL)
    {
        return NULL;
    }
    std::memset(new_ptr, 0, num * size);
    return ((void *) new_ptr);
}

void sfree(void *p)
{
    init_heap();
    if (p == nullptr)
    {
        return;
    }
    MallocMetadata * p_mm = (MallocMetadata *) p;
    p_mm--;
    if (p_mm->size >= (128*1024))
    {
        mmap_free(p);
        return;
    }
    MallocMetadata *ptr_to_release = (MallocMetadata *) p;
    ptr_to_release=join_if_needed(ptr_to_release);
    ptr_to_release--;
    ptr_to_release->is_free=true;
    size_t size = ptr_to_release->size;
    ptr_to_release++;
    MallocMetadata* temp= get_the_la_metadata_of_ptr(ptr_to_release,size);
    temp->is_free = true;
    temp->size =size;
}

//get a pointer
size_t get_size_of_ptr(void* ptr)
{
    MallocMetadata* temp = (MallocMetadata*) ptr;
    temp --;
    return temp->size;
}

//get a metadata
bool can_reuse_current_block(MallocMetadata* old_metadata,size_t size)
{
    if (old_metadata->size >= size)
    {
        return true;
    }
    return false;
}

//get a metadata
bool is_wilderness(MallocMetadata* biger_block)
{
    size_t size_of_block = biger_block->size;
    biger_block++;
    if(get_the_la_metadata_of_ptr(biger_block,size_of_block) == get_the_topmost())
    {
        return true;
    }
    return false;
}



//get a pointer
bool the_next_is_wilderness(void* oldp)
{
    MallocMetadata* curr_block = get_the_ya_metadata_of_ptr(oldp);
    MallocMetadata* next_block =get_the_next_block(curr_block);
    if(is_wilderness(next_block))
    {
        return true;
    }
    return false;
}

bool will_be_sufficient_two_blocks(MallocMetadata* block1, MallocMetadata* block2,size_t size)
{
    if(block1->size + block2->size + _size_meta_data() >= size)
    {
        return true;
    }
    return false;
}

bool will_be_sufficient_tree_blocks(MallocMetadata* block1, MallocMetadata* block2,MallocMetadata* block3,size_t size)
{
    if(block1->size + block2->size + block3->size+ _size_meta_data() + _size_meta_data() >= size)
    {
        return true;
    }
    return false;
}

size_t distance_from_three_blocks_being_sufficient(MallocMetadata* block1, MallocMetadata* block2,MallocMetadata* block3,size_t size)
{
    return (size - (block1->size + block2->size + block3->size+ _size_meta_data() + _size_meta_data()));
}

size_t distance_from_two_blocks_being_sufficient(MallocMetadata* block1, MallocMetadata* block2,size_t size)
{
    return (size - (block1->size + block2->size + _size_meta_data() + _size_meta_data()));
}




void compile_and_init_the_two_next_blocks(MallocMetadata* next_blk)
{
    MallocMetadata *next_next_blk = get_the_next_block(next_blk);
    MallocMetadata *temp = join_prev_block(next_next_blk + 1);
    temp->is_free = true;
    MallocMetadata *temp_t = get_the_la_metadata_of_ptr(temp + 1, temp->size);
    temp_t->is_free = false;
}

bool the_next_of_the_next_free(MallocMetadata* new_block)
{
    MallocMetadata *next_blk = get_the_next_block(new_block);
    if (next_block_free(next_blk + 1))
    {
        return true;
    }
    return false;
}




void *srealloc(void *oldp, size_t size)
{
    init_heap();
    if (oldp == nullptr)
    {
        return smalloc(size);
    }
    if (!sizeValid(size))
    {
        return nullptr;
    }
    if (size >= (128*1024)){
        return mmap_realloc(oldp,size);
    }
    size = align(size);
    MallocMetadata *old_meta = ((MallocMetadata *) oldp) - 1;
    const size_t old_size = old_meta->size;
    //a
    if (can_reuse_current_block(old_meta, size))
    {
        if (can_break(old_meta, size))
        {
            return break_the_block(old_meta,size);
        }
        else
        {
            old_meta->is_free = false;
            get_the_la_metadata_of_ptr(oldp,old_meta->size)->is_free = false;
            return oldp;
        }
    }
    //b
    if (previous_block_free(oldp) && ((will_be_sufficient_two_blocks(get_the_previous_block(old_meta), old_meta, size) ) || (is_wilderness(old_meta))))
    {
        MallocMetadata *bigger_block = join_prev_block(oldp);
        if (can_break(bigger_block, size))
        {
            void *new_p = break_the_block(bigger_block, size);
            std::memmove(new_p, oldp, old_size);
            return bigger_block;
        }
        if(sufficient(bigger_block,size))
        {
            bigger_block->is_free = false;
            get_the_la_metadata_of_ptr(bigger_block+1,bigger_block->size)->is_free = false;
            bigger_block++;
            std::memmove(bigger_block, oldp, old_size);
            return bigger_block;
        }
        //b2
        if (is_wilderness(old_meta))
        {
            MallocMetadata *new_block = (MallocMetadata *) (enlarge_top_most(size));
            std::memmove(new_block, oldp, old_size);
            return new_block;
        }
    }
    //c
    if (is_wilderness(old_meta))
    {
        old_meta = (MallocMetadata *)(enlarge_top_most(size));
        return old_meta;
    }
    //d
    if (next_block_free(oldp) && will_be_sufficient_two_blocks(get_the_next_block(old_meta), old_meta, size))
    {
        MallocMetadata *bigger_block = join_next_block(oldp);
        if (can_break(bigger_block, size))
        {
            void *new_p = break_the_block(bigger_block, size);
            std::memmove(new_p, oldp, old_size);
            return bigger_block;
        }
        if(sufficient(bigger_block,size))
        {
            bigger_block->is_free = false;
            get_the_la_metadata_of_ptr(bigger_block+1,bigger_block->size)->is_free = false;
            bigger_block++;
            std::memmove(bigger_block, oldp, old_size);
            return bigger_block;
        }
    }
    //e
    if (next_block_free(oldp) && previous_block_free(oldp) && will_be_sufficient_tree_blocks(get_the_next_block(old_meta), get_the_previous_block(old_meta), old_meta,size))
    {
        void *new_p = join_if_needed((MallocMetadata *) oldp);
        MallocMetadata* bigger_block = ((MallocMetadata*) new_p)-1;
        init_block_in_realoc(bigger_block,bigger_block+1, false);
        bigger_block++;
        memmove(bigger_block, oldp, old_size);
        return bigger_block;
    }
    //f
    if (the_next_is_wilderness(oldp) && next_block_free(oldp))
    {
        //case i
        if (previous_block_free(oldp))
        {
            join_if_needed((MallocMetadata*)oldp);
            enlarge_top_most(size);
            init_block_in_realoc(get_the_ya_meta_from_la(get_the_topmost()),get_the_ya_meta_from_la(get_the_topmost())+1,false);
            memmove(get_the_ya_meta_from_la(get_the_topmost())+1, oldp, old_size);
            return get_the_ya_meta_from_la(get_the_topmost())+1;
        }
            //case ii
        else
        {
            join_next_block(oldp);
            init_block_in_realoc(get_the_ya_meta_from_la(get_the_topmost()),get_the_ya_meta_from_la(get_the_topmost())+1,false);
            enlarge_top_most(size);
            memmove(get_the_ya_meta_from_la(get_the_topmost())+1, oldp, old_size);
            return get_the_ya_meta_from_la(get_the_topmost())+1;
        }
    }
    //g
    MallocMetadata *block = (MallocMetadata *) find_block_at_least(size);
    if (block != nullptr)
    {
        block++;
        std::memmove(block, oldp, old_size);
        sfree(oldp);
        if (can_break(block-1, size))
        {
            void *new_p = break_the_block(block - 1, size);
            return new_p;
        }
        init_block_in_realoc(block-1,block,false);
        return block;
    }
    //h
    void *new_ptr = smalloc(size);
    if (new_ptr == nullptr)
    {
        return nullptr;
    }
    std::memmove(new_ptr, oldp, old_size);
    sfree(oldp);
    return new_ptr;
}

