#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <cstring>


typedef struct MallocMetadata
{
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
}MallocMetadata;

size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _size_meta_data();
size_t _num_meta_data_bytes();
void * smalloc(size_t size);
void * scalloc(size_t num, size_t size);
void sfree(void* p);
void * srealloc(void * oldp, size_t size);


bool sizeValid(size_t size)
{
    if(size==0 || size > 100000000)
    {
        return false;
    }
    return true;
}


MallocMetadata* head = nullptr;
MallocMetadata* tail = nullptr;


size_t _num_free_blocks()
{
    if(head == nullptr)
    {
        return 0;
    }
    MallocMetadata* it = head;
    int counter = 0;
    while( it != nullptr)
    {
        if(it->is_free)
        {
            counter++;
        }
        it = it->next;
    }
    return counter;
}

size_t _num_free_bytes()
{
    if(head == nullptr)
    {
        return 0;
    }
    MallocMetadata* it = head;
    int counter = 0;
    while( it != nullptr)
    {
        if(it->is_free)
        {
            counter+=it->size;
        }
        it = it->next;
    }
    return counter;
}


size_t _num_allocated_blocks()
{
    if(head == nullptr)
    {
        return 0;
    }
    MallocMetadata* it = head;
    int counter = 0;
    while( it != nullptr)
    {
        counter++;
        it = it->next;
    }
    return counter;
}

size_t _num_allocated_bytes()
{
    if(head == nullptr)
    {
        return 0;
    }
    MallocMetadata* it = head;
    int counter = 0;
    while( it != nullptr)
    {
        counter+=it->size;
        it = it->next;
    }
    return counter;
}


size_t _size_meta_data()
{
    return sizeof(MallocMetadata);
}


size_t _num_meta_data_bytes()
{
    return _num_allocated_blocks()*_size_meta_data();
}



void * smalloc(size_t size)
{
    if(!sizeValid(size))
    {
        return nullptr;
    }
    MallocMetadata* it = head;
    while(it!= nullptr)
    {
        if(it->size>= size && it->is_free)
        {
            it->is_free= false;
            it++;
            return (void*)(it);
        }
        it = it->next;
    }
    
    void* new_ptr = sbrk((intptr_t)(size+_size_meta_data()));
    
    if(new_ptr == ((void*)-1))
    {
        return nullptr;
    }
    
    MallocMetadata* new_node=(MallocMetadata*)new_ptr;
    new_node->size = size;
    new_node->is_free = false;
    
    if(tail!= nullptr)
    {
        new_node->prev = tail;
        tail->next = new_node;
        tail = new_node;
        new_node->next= nullptr;
    }
    else
    {
        head = new_node;
        tail = new_node;
        new_node->prev= nullptr;
        new_node->next = nullptr;
    }
    return (void*) (new_node + 1);
}


void * scalloc(size_t num, size_t size)
{
    void* new_ptr = smalloc(num*size);
    if(new_ptr == nullptr)
    {
        return nullptr;
    }
    std::memset(new_ptr,0,num*size);
    return ((void*)new_ptr);
}


void sfree(void* p)
{
    if(p== nullptr)
    {
        return ;
    }
    MallocMetadata* ptr_to_release =(MallocMetadata*) p;
    ptr_to_release--;
    ptr_to_release->is_free=true;
}


void * srealloc(void * oldp, size_t size)
{
    if(oldp== nullptr)
    {
        return smalloc(size);
    }
    if(!sizeValid(size))
    {
        return nullptr;
    }
    MallocMetadata* old_ptr_meta =(MallocMetadata*) oldp;
    old_ptr_meta--;
    if ((old_ptr_meta->size) >= size)
    {
        return old_ptr_meta+1;
    }
    else
    {
        void* new_ptr = smalloc(size);
        if (new_ptr == nullptr)
        {
            return nullptr;
        }
        std::memmove(new_ptr,oldp,old_ptr_meta->size);
        sfree(oldp);
        return new_ptr;
    }
}
