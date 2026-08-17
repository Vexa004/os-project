//
// Created by os on 8/17/26.
//

#ifndef PROJECT_BASE_V1_1_MEMORYALLOCATOR_HPP
#define PROJECT_BASE_V1_1_MEMORYALLOCATOR_HPP
#include "../lib/hw.h"
#include "../h/print.hpp"

struct BlockHeader
{
    BlockHeader *next;
    size_t size;
};

class MemoryAllocator
{
public:
    static MemoryAllocator &getInstance();

    MemoryAllocator(const MemoryAllocator &) = delete;

    MemoryAllocator &operator=(const MemoryAllocator &) = delete;

    void init();

    void *mem_alloc(size_t size);

    int mem_free(void *ptr);

private:
    static MemoryAllocator instance;

    BlockHeader *freeMemHead;

    MemoryAllocator() : freeMemHead(nullptr)
    {
    }

    static size_t getHeaderSize()
    {
        return ((sizeof(BlockHeader) + MEM_BLOCK_SIZE - 1)
                / MEM_BLOCK_SIZE) * MEM_BLOCK_SIZE;
    }
};


#endif //PROJECT_BASE_V1_1_MEMORYALLOCATOR_HPP
