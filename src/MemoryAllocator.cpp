//
// Created by os on 8/17/26.
//

#include "../h/MemoryAllocator.hpp"
#include "../h/print.hpp"

MemoryAllocator MemoryAllocator::instance;

MemoryAllocator &MemoryAllocator::getInstance()
{
    return instance;
}

void MemoryAllocator::init()
{
    if (freeMemHead != nullptr)
    {
        printString("Already initialized\n");
        return;
    }

    freeMemHead = (BlockHeader *) HEAP_START_ADDR;

    freeMemHead->next = nullptr;
    freeMemHead->size =
            (char *) HEAP_END_ADDR -
            (char *) HEAP_START_ADDR -
            sizeof(BlockHeader);
}

void *MemoryAllocator::mem_alloc(size_t size)
{
    if (size <= 0)
        return nullptr;
    const size_t numOfBlocks = (size + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;

    BlockHeader *blk = freeMemHead, *prev = nullptr;

    for (; blk != nullptr; prev = blk, blk = blk->next)
        if (blk->size >= numOfBlocks * MEM_BLOCK_SIZE)break;

    //printInteger((uint64)freeMemHead->size);

    if (blk == nullptr) return nullptr;

    size_t allocSize = numOfBlocks * MEM_BLOCK_SIZE;
    size_t remainingSize = blk->size - allocSize;
    if (remainingSize >= sizeof(BlockHeader) + MEM_BLOCK_SIZE)
    {
        blk->size = allocSize;
        size_t offset = sizeof(BlockHeader) + allocSize;
        BlockHeader *newBlk = (BlockHeader*)((char*)blk + offset);
        if (prev)prev->next = newBlk;
        else freeMemHead = newBlk;
        newBlk->next = blk->next;
        newBlk->size = remainingSize-sizeof(BlockHeader);
    }else
    {
        if (prev)prev->next = blk->next;
        else freeMemHead = blk->next;
    }


    // printInteger((uint64)freeMemHead);
    // printString("\n");
    // printInteger((uint64)blk);
    blk->next = nullptr;
    return (char *) blk + sizeof(BlockHeader);
}

int MemoryAllocator::mem_free(void *ptr)
{
    return 0;
}
