#include "../h/MemoryAllocator.hpp"
#include "../h/print.hpp"

MemoryAllocator MemoryAllocator::instance;

MemoryAllocator& MemoryAllocator::getInstance()
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

    freeMemHead = (BlockHeader*)HEAP_START_ADDR;
    freeMemHead->next = nullptr;

    size_t heapSize =
        (char*)HEAP_END_ADDR -
        (char*)HEAP_START_ADDR;

    // Ceo slobodan prostor zaokruži naniže na cele blokove.
    heapSize =
        (heapSize / MEM_BLOCK_SIZE) * MEM_BLOCK_SIZE;

    freeMemHead->size = heapSize;
    printInteger((uint64)sizeof(BlockHeader));
}

void* MemoryAllocator::mem_alloc(size_t size)
{
    if (size == 0)
        return nullptr;

    // Ukupna veličina = header + korisnički zahtev.
    const size_t requestedSize =
        sizeof(BlockHeader) + size;

    // Zaokruživanje CELOG regiona na MEM_BLOCK_SIZE.
    const size_t allocSize =
        ((requestedSize + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE)
        * MEM_BLOCK_SIZE;

    // First fit
    BlockHeader* blk = freeMemHead;
    BlockHeader* prev = nullptr;

    for (; blk != nullptr; prev = blk, blk = blk->next)
    {
        if (blk->size >= allocSize)
            break;
    }

    if (blk == nullptr)
        return nullptr;

    const size_t remainingSize =
        blk->size - allocSize;

    // Ako ostaje bar jedan ceo blok,
    // možemo napraviti novi slobodan blok.
    if (remainingSize >= MEM_BLOCK_SIZE)
    {
        BlockHeader* newBlk =
            (BlockHeader*)((char*)blk + allocSize);

        newBlk->size = remainingSize;
        newBlk->next = blk->next;

        if (prev)
            prev->next = newBlk;
        else
            freeMemHead = newBlk;

        // blk je sada zauzet.
        blk->size = allocSize;
        blk->next = nullptr;
    }
    else
    {
        // Uzimamo ceo slobodan blok.
        if (prev)
            prev->next = blk->next;
        else
            freeMemHead = blk->next;

        blk->next = nullptr;
    }

    // Preskačemo header i vraćamo korisniku njegov deo.
    return (char*)blk + sizeof(BlockHeader);
}

int MemoryAllocator::mem_free(void* ptr)
{
    return 0;
}