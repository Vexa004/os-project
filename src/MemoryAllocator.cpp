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
        printStringMoj("Already initialized\n");
        return;
    }
    printStringMoj("Initializing memory allocator\n");

    freeMemHead = (BlockHeader *) HEAP_START_ADDR;
    freeMemHead->next = nullptr;

    size_t heapSize =
            (char *) HEAP_END_ADDR -
            (char *) HEAP_START_ADDR;

    // Ceo slobodan prostor zaokruži naniže na cele blokove.
    heapSize =
            (heapSize / MEM_BLOCK_SIZE) * MEM_BLOCK_SIZE;

    freeMemHead->size = heapSize;
    //printInteger((uint64) sizeof(BlockHeader));
}

void *MemoryAllocator::mem_alloc(size_t size)
{
    if (size == 0 || size > (size_t) ((char *) HEAP_END_ADDR - (char *) HEAP_START_ADDR))
        return nullptr;


    // Ukupna veličina = header + korisnički zahtev.
    const size_t requestedSize =
            sizeof(BlockHeader) + size;

    // Zaokruživanje CELOG regiona na MEM_BLOCK_SIZE.
    const size_t allocSize =
            ((requestedSize + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE)
            * MEM_BLOCK_SIZE;

    // First fit
    BlockHeader *blk = freeMemHead;
    BlockHeader *prev = nullptr;

    for (; blk != nullptr; prev = blk, blk = blk->next)
    {
        if (blk->size >= allocSize)
            break;
    }

    if (blk == nullptr || blk->size < allocSize)
        return nullptr;

    const size_t remainingSize =
            blk->size - allocSize;

    // Ako ostaje bar jedan ceo blok,
    // možemo napraviti novi slobodan blok.
    if (remainingSize >= MEM_BLOCK_SIZE)
    {
        BlockHeader *newBlk =
                (BlockHeader *) ((char *) blk + allocSize);

        newBlk->size = remainingSize;
        newBlk->next = blk->next;

        if (prev)
            prev->next = newBlk;
        else
            freeMemHead = newBlk;

        // blk je sada zauzet.
        blk->size = allocSize;
        blk->next = nullptr;
    } else
    {
        // Uzimamo ceo slobodan blok.
        if (prev)
            prev->next = blk->next;
        else
            freeMemHead = blk->next;

        blk->next = nullptr;
    }

    // Preskačemo header i vraćamo korisniku njegov deo.

    return (char *) blk + sizeof(BlockHeader);
}

int MemoryAllocator::tryToJoin(BlockHeader *current)
{
    if (!current) return 0;

    if (current->next && (char *) current + current->size == (char *) current->next)
    {
        current->size += current->next->size;
        current->next = current->next->next;
        return 1;
    } else
        return 0;
}


int MemoryAllocator::mem_free(void *ptr)
{
    if (ptr == nullptr || ptr < (void *) HEAP_START_ADDR || ptr >= (void *) HEAP_END_ADDR)
    {
        //printString("ERR ptr\n");
        return -1;
    }


    BlockHeader *blk = (BlockHeader *) ((char *) ptr - sizeof(BlockHeader));

    if (blk->size <= 0)
    {
        //printString("ERR blk\n");
        return -2;
    }
    size_t offset = (char *) blk - (char *) HEAP_START_ADDR;
    if (offset % MEM_BLOCK_SIZE != 0)
        return -2; // nije poravnat na granicu bloka

    BlockHeader *current = freeMemHead;
    while (current)
    {
        if (current == blk || ((char *) blk > (char *) current && (char *) blk < (char *) current + current->size))
        {
            //printString("ERR double free\n");
            return -3;
        }
        current = current->next;
    }

    /*printString("BLK\n");
    printInteger((uint64) blk);
    printString("\nBLK\n");*/

    current = nullptr;

    if (!freeMemHead || (char *) blk < (char *) freeMemHead)
    {
        current = nullptr;
    } else
    {
        for (current = freeMemHead; current->next && (char *) blk > (char *) (current->next); current = current->next);
    }

    if (current)blk->next = current->next;
    else blk->next = freeMemHead;
    if (current) current->next = blk;
    else freeMemHead = blk;

    tryToJoin(blk);
    tryToJoin(current);

    return 0;
}
