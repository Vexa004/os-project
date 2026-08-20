//
// Created by marko on 20.4.22..
//

#include "../h/tcb.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/workers.hpp"
#include "../h/print.hpp"
#include "../h/riscv.hpp"
//#include "../h/MemoryAllocatorTests.hpp"
#include "../lib/hw.h"

// region Tests
/*
void testMemoryAllocator()
{
    MemoryAllocator &allocator = MemoryAllocator::getInstance();
    allocator.init();

    void *p1 = allocator.mem_alloc(1); // 64 B
    void *p2 = allocator.mem_alloc(64); // 64 B
    void *p3 = allocator.mem_alloc(65); // 128 B
    void *p4 = allocator.mem_alloc(127); // 128 B
    void *p5 = allocator.mem_alloc(128); // 128 B

    printString("1 p1: ");
    printInteger((uint64) p1);
    printString("\n");

    printString("64 p2: ");
    printInteger((uint64) p2);
    printString("\n");

    printString("65 p3: ");
    printInteger((uint64) p3);
    printString("\n");

    printString("127 p4: ");
    printInteger((uint64) p4);
    printString("\n");

    printString("128 p5: ");
    printInteger((uint64) p5);
    printString("\n");
}

void testMemoryFree()
{
    MemoryAllocator &allocator = MemoryAllocator::getInstance();
    allocator.init();

    void *p1 = allocator.mem_alloc(64);
    void *p2 = allocator.mem_alloc(128);

    printString("Freeing p1: ");
    printInteger((uint64) p1);
    printString("\n");
    allocator.mem_free(p1);

    printString("Freeing p2: ");
    printInteger((uint64) p2);
    printString("\n");
    allocator.mem_free(p2);

    printString("Freeing nullptr: ");
    int result = allocator.mem_free(nullptr);
    printInteger(result);
    printString("\n");

    void *invalidPtr = (char *) HEAP_END_ADDR + 1;
    printString("Freeing invalid pointer: ");
    result = allocator.mem_free(invalidPtr);
    printInteger(result);
    printString("\n");
}

void testBasicAlloc()
{
    MemoryAllocator &allocator = MemoryAllocator::getInstance();
    allocator.init();

    void *p1 = allocator.mem_alloc(1);
    void *p2 = allocator.mem_alloc(64);
    void *p3 = allocator.mem_alloc(65);
    void *p4 = allocator.mem_alloc(127);
    void *p5 = allocator.mem_alloc(128);

    printString("=== BASIC ALLOC ===\n");

    printString("p1: ");
    printInteger((uint64) p1);
    printString("\n");

    printString("p2: ");
    printInteger((uint64) p2);
    printString("\n");

    printString("p3: ");
    printInteger((uint64) p3);
    printString("\n");

    printString("p4: ");
    printInteger((uint64) p4);
    printString("\n");

    printString("p5: ");
    printInteger((uint64) p5);
    printString("\n");

    printString("p1 != p2: ");
    printInteger(p1 != p2);
    printString("\n");

    printString("p2 != p3: ");
    printInteger(p2 != p3);
    printString("\n");

    printString("p3 != p4: ");
    printInteger(p3 != p4);
    printString("\n");

    printString("p4 != p5: ");
    printInteger(p4 != p5);
    printString("\n");
}

void testReuse()
{
    MemoryAllocator &allocator = MemoryAllocator::getInstance();

    printString("\n=== REUSE TEST ===\n");

    void *p1 = allocator.mem_alloc(100);
    void *p2 = allocator.mem_alloc(100);
    void *p3 = allocator.mem_alloc(100);

    printString("p1: ");
    printInteger((uint64) p1);
    printString("\n");

    printString("p2: ");
    printInteger((uint64) p2);
    printString("\n");

    printString("p3: ");
    printInteger((uint64) p3);
    printString("\n");

    allocator.mem_free(p2);

    printString("p2 freed\n");

    void *p4 = allocator.mem_alloc(50);

    printString("p4: ");
    printInteger((uint64) p4);
    printString("\n");

    printString("p4 == p2: ");
    printInteger(p4 == p2);
    printString("\n");
}

void testJoin()
{
    MemoryAllocator &allocator = MemoryAllocator::getInstance();

    printString("\n=== JOIN TEST ===\n");

    void *p1 = allocator.mem_alloc(100);
    void *p2 = allocator.mem_alloc(100);
    void *p3 = allocator.mem_alloc(100);

    printString("p1: ");
    printInteger((uint64) p1);
    printString("\n");

    printString("p2: ");
    printInteger((uint64) p2);
    printString("\n");

    printString("p3: ");
    printInteger((uint64) p3);
    printString("\n");

    allocator.mem_free(p2);
    allocator.mem_free(p1);

    printString("p1 and p2 freed\n");

    void *p4 = allocator.mem_alloc(200);

    printString("p4: ");
    printInteger((uint64) p4);
    printString("\n");

    printString("p4 == p1: ");
    printInteger(p4 == p1);
    printString("\n");
}

void testJoinBothSides()
{
    MemoryAllocator &allocator = MemoryAllocator::getInstance();

    printString("\n=== JOIN BOTH SIDES ===\n");

    void *p1 = allocator.mem_alloc(100);
    void *p2 = allocator.mem_alloc(100);
    void *p3 = allocator.mem_alloc(100);

    allocator.mem_free(p1);
    allocator.mem_free(p3);

    printString("p1 and p3 freed\n");

    allocator.mem_free(p2);

    printString("p2 freed\n");

    void *p4 = allocator.mem_alloc(250);

    printString("p4: ");
    printInteger((uint64) p4);
    printString("\n");

    printString("p4 == p1: ");
    printInteger(p4 == p1);
    printString("\n");
}

void testMemoryWrite()
{
    MemoryAllocator &allocator = MemoryAllocator::getInstance();
    allocator.init();

    printString("\n=== MEMORY WRITE ===\n");

    char *p = (char *) allocator.mem_alloc(100);

    if (p == nullptr)
    {
        printString("Allocation failed\n");
        return;
    }

    for (int i = 0; i < 100; i++)
        p[i] = (char) i;

    bool correct = true;

    for (int i = 0; i < 100; i++)
    {
        if (p[i] != (char) i)
        {
            correct = false;
            break;
        }
    }

    printString("Memory write/read: ");
    printInteger(correct);
    printString("\n");

    allocator.mem_free(p);
}

void testAllocatorExtensive()
{
    MemoryAllocator &allocator = MemoryAllocator::getInstance();

    printString("\n=== EXTENSIVE ALLOC/FREE TEST ===\n");

    const int SLOTS = 100;

    void *ptrs[SLOTS];

    for (int i = 0; i < SLOTS; i++)
        ptrs[i] = nullptr;

    int allocations = 0;
    int frees = 0;

    for (int iteration = 0; iteration < 10000; iteration++)
    {
        int slot = iteration % SLOTS;

        // Oslobodi prethodnu alokaciju u ovom slotu
        if (ptrs[slot] != nullptr)
        {
            if (allocator.mem_free(ptrs[slot]) != 0)
            {
                printString("FREE ERROR at iteration ");
                printInteger(iteration);
                printString("\n");
                return;
            }

            ptrs[slot] = nullptr;
            frees++;
        }

        // Razlicite velicine
        size_t size;

        switch (iteration % 8)
        {
            case 0: size = 1;
                break;
            case 1: size = 16;
                break;
            case 2: size = 32;
                break;
            case 3: size = 63;
                break;
            case 4: size = 64;
                break;
            case 5: size = 65;
                break;
            case 6: size = 100;
                break;
            default: size = 200;
                break;
        }

        void *ptr = allocator.mem_alloc(size);

        if (ptr == nullptr)
        {
            printString("ALLOC FAILED at iteration ");
            printInteger(iteration);
            printString("\n");

            // Oslobodi sve sto je ostalo
            for (int i = 0; i < SLOTS; i++)
            {
                if (ptrs[i] != nullptr)
                {
                    allocator.mem_free(ptrs[i]);
                    ptrs[i] = nullptr;
                }
            }

            return;
        }

        ptrs[slot] = ptr;
        allocations++;

        // Upis u alociranu memoriju
        unsigned char *data = (unsigned char *) ptr;

        for (size_t i = 0; i < size; i++)
            data[i] = (unsigned char) (iteration & 0xFF);

        // Provera
        for (size_t i = 0; i < size; i++)
        {
            if (data[i] != (unsigned char) (iteration & 0xFF))
            {
                printString("MEMORY CORRUPTION at iteration ");
                printInteger(iteration);
                printString("\n");

                for (int j = 0; j < SLOTS; j++)
                {
                    if (ptrs[j] != nullptr)
                    {
                        allocator.mem_free(ptrs[j]);
                        ptrs[j] = nullptr;
                    }
                }

                return;
            }
        }

        if (iteration % 1000 == 0)
        {
            printString("Iteration: ");
            printInteger(iteration);
            printString("\n");
        }
    }

    // Oslobodi sve preostale blokove
    for (int i = 0; i < SLOTS; i++)
    {
        if (ptrs[i] != nullptr)
        {
            if (allocator.mem_free(ptrs[i]) != 0)
            {
                printString("FINAL FREE ERROR\n");
                return;
            }

            ptrs[i] = nullptr;
            frees++;
        }
    }

    printString("\nALLOCATIONS: ");
    printInteger(allocations);

    printString("\nFREES: ");
    printInteger(frees);

    printString("\nTEST PASSED\n");
}

void testFreeAndReuse()
{
    MemoryAllocator &allocator = MemoryAllocator::getInstance();

    void *p1 = allocator.mem_alloc(100);
    void *p2 = allocator.mem_alloc(100);
    void *p3 = allocator.mem_alloc(100);

    printString("p1: ");
    printInteger((uint64) p1);
    printString("\n");

    printString("p2: ");
    printInteger((uint64) p2);
    printString("\n");

    printString("p3: ");
    printInteger((uint64) p3);
    printString("\n");

    allocator.mem_free(p1);
    allocator.mem_free(p2);
    allocator.mem_free(p3);

    printString("All freed\n");

    void *big = allocator.mem_alloc(300);

    printString("big: ");
    printInteger((uint64) big);
    printString("\n");

    if (big != nullptr)
        printString("BIG ALLOCATION SUCCESS\n");
    else
        printString("BIG ALLOCATION FAILED\n");

    allocator.mem_free(big);
}

void testAllocatorFragmentation()
{
    MemoryAllocator &allocator = MemoryAllocator::getInstance();

    printString("\n=== FRAGMENTATION TEST ===\n");

    const int SLOTS = 200;

    static void *ptrs[SLOTS];
    static unsigned char patterns[SLOTS];

    for (int i = 0; i < SLOTS; i++)
    {
        ptrs[i] = nullptr;
        patterns[i] = 0;
    }

    int totalAlloc = 0;
    int totalFree = 0;

    for (int cycle = 0; cycle < 5; cycle++)
    {
        printString("\n--- CYCLE ");
        printInteger(cycle + 1);
        printString(" ---\n");

        // 1. Popuni slobodne slotove
        for (int i = 0; i < SLOTS; i++)
        {
            if (ptrs[i] != nullptr)
                continue;

            size_t size =
                    128 + ((i * 137 + cycle * 251) % 3900);

            void *ptr = allocator.mem_alloc(size);

            if (ptr != nullptr)
            {
                ptrs[i] = ptr;
                patterns[i] = (unsigned char) ((i + cycle) & 0xFF);

                totalAlloc++;

                unsigned char *data =
                        (unsigned char *) ptr;

                for (size_t j = 0; j < size; j++)
                    data[j] = patterns[i];
            }
        }

        printString("Allocation phase finished\n");

        // 2. Provera sadržaja
        bool correct = true;

        for (int i = 0; i < SLOTS; i++)
        {
            if (ptrs[i] == nullptr)
                continue;

            unsigned char *data =
                    (unsigned char *) ptrs[i];

            for (int j = 0; j < 16; j++)
            {
                if (data[j] != patterns[i])
                {
                    correct = false;

                    printString("MEMORY CORRUPTION in slot ");
                    printInteger(i);
                    printString("\n");

                    break;
                }
            }

            if (!correct)
                break;
        }

        if (!correct)
            return;

        // 3. Oslobodi nasumično odabrane blokove
        for (int i = 0; i < SLOTS; i++)
        {
            if (((i * 37 + cycle * 17) % 5) < 2)
            {
                if (ptrs[i] != nullptr)
                {
                    if (allocator.mem_free(ptrs[i]) != 0)
                    {
                        printString("FREE ERROR at slot ");
                        printInteger(i);
                        printString("\n");
                        return;
                    }

                    ptrs[i] = nullptr;
                    totalFree++;
                }
            }
        }

        printString("Fragmented free phase finished\n");

        // 4. Ponovo alociraj oslobođene slotove
        for (int i = 0; i < SLOTS; i++)
        {
            if (ptrs[i] != nullptr)
                continue;

            size_t size =
                    64 + ((i * 211 + cycle * 97) % 2500);

            void *ptr =
                    allocator.mem_alloc(size);

            if (ptr != nullptr)
            {
                ptrs[i] = ptr;
                patterns[i] = (unsigned char) (0xA0 + cycle);

                totalAlloc++;

                unsigned char *data =
                        (unsigned char *) ptr;

                for (size_t j = 0; j < size; j++)
                    data[j] = patterns[i];
            }
        }

        printString("Reallocation phase finished\n");

        // 5. Drugo nasumično oslobadjanje
        for (int i = 0; i < SLOTS; i++)
        {
            if (((i * 53 + cycle * 31) % 7) == 0)
            {
                if (ptrs[i] != nullptr)
                {
                    if (allocator.mem_free(ptrs[i]) != 0)
                    {
                        printString("SECOND FREE ERROR at slot ");
                        printInteger(i);
                        printString("\n");
                        return;
                    }

                    ptrs[i] = nullptr;
                    totalFree++;
                }
            }
        }

        printString("Second free phase finished\n");
    }

    // 6. Oslobodi sve što je ostalo
    for (int i = 0; i < SLOTS; i++)
    {
        if (ptrs[i] != nullptr)
        {
            if (allocator.mem_free(ptrs[i]) != 0)
            {
                printString("FINAL FREE ERROR at slot ");
                printInteger(i);
                printString("\n");
                return;
            }

            ptrs[i] = nullptr;
            totalFree++;
        }
    }

    printString("\nTotal allocations: ");
    printInteger(totalAlloc);

    printString("\nTotal frees: ");
    printInteger(totalFree);

    printString("\nFRAGMENTATION TEST PASSED\n");
}

void testFullHeapAndJoin()
{
    MemoryAllocator &allocator = MemoryAllocator::getInstance();

    printString("\n=== FULL HEAP / JOIN TEST ===\n");

    const int MAX_BLOCKS = 500;

    static void *ptrs[MAX_BLOCKS];

    for (int i = 0; i < MAX_BLOCKS; i++)
        ptrs[i] = nullptr;

    int count = 0;

    // 1. Popunjavanje heap-a
    printString("Filling heap...\n");

    while (count < MAX_BLOCKS)
    {
        size_t size;

        switch (count % 5)
        {
            case 0: size = 256;
                break;
            case 1: size = 512;
                break;
            case 2: size = 1024;
                break;
            case 3: size = 1536;
                break;
            default: size = 2048;
                break;
        }

        void *ptr = allocator.mem_alloc(size);

        if (ptr == nullptr)
            break;

        ptrs[count] = ptr;

        // Upis da proverimo da je memorija validna
        unsigned char *data = (unsigned char *) ptr;

        for (size_t i = 0; i < size; i++)
            data[i] = (unsigned char) (count & 0xFF);

        count++;
    }

    printString("Allocated blocks: ");
    printInteger(count);
    printString("\n");

    // 2. Oslobodi SVE
    printString("Freeing all blocks...\n");

    for (int i = 0; i < count; i++)
    {
        if (ptrs[i] != nullptr)
        {
            if (allocator.mem_free(ptrs[i]) != 0)
            {
                printString("FREE ERROR at block ");
                printInteger(i);
                printString("\n");
                return;
            }

            ptrs[i] = nullptr;
        }
    }

    printString("All blocks freed.\n");

    // 3. Sada bi ceo heap trebalo da bude jedan slobodan blok.
    //
    // Tražimo skoro ceo heap.
    size_t heapSize =
            (size_t) ((char *) HEAP_END_ADDR -
                      (char *) HEAP_START_ADDR);

    // Ostavljamo malo prostora da header + poravnanje mogu da stanu.
    size_t requestSize =
            heapSize - 2 * MEM_BLOCK_SIZE;

    printString("Requesting large block: ");
    printInteger(requestSize);
    printString(" bytes\n");

    void *big = allocator.mem_alloc(requestSize);

    if (big == nullptr)
    {
        printString("BIG ALLOCATION FAILED\n");
        return;
    }

    printString("BIG ALLOCATION SUCCESS\n");

    // 4. Provera da veliki blok stvarno može da se koristi
    unsigned char *data = (unsigned char *) big;

    data[0] = 0x11;

    if (requestSize > 1)
        data[requestSize - 1] = 0x77;

    if (data[0] != 0x11)
    {
        printString("FIRST BYTE CHECK FAILED\n");
        allocator.mem_free(big);
        return;
    }

    if (requestSize > 1 &&
        data[requestSize - 1] != 0x77)
    {
        printString("LAST BYTE CHECK FAILED\n");
        allocator.mem_free(big);
        return;
    }

    printString("LARGE BLOCK MEMORY CHECK PASSED\n");

    // 5. Oslobodi veliki blok
    if (allocator.mem_free(big) != 0)
    {
        printString("FINAL FREE FAILED\n");
        return;
    }

    printString("LARGE BLOCK FREED\n");

    printString("=== FULL HEAP / JOIN TEST PASSED ===\n");
}

// endregion*/
/*
int main()
{
    //testMemoryAllocator();
    //testMemoryFree();

    // testBasicAlloc();
    // testReuse();
    // testJoin();
    // testJoinBothSides();

    //testMemoryWrite();

    MemoryAllocator &allocator = MemoryAllocator::getInstance();
    allocator.init();
    // testAllocatorFragmentation();
    testFullHeapAndJoin();

    return 0;
}*/

#include "../h/MemoryAllocator.hpp"

// Ako su testovi u MemoryAllocatorTests.cpp

struct Test
{
    const char *tekst;
    const char *name;
};

int main()
{
    MemoryAllocator::getInstance().init();
    Riscv::w_stvec((uint64) &Riscv::supervisorTrap);
    //TCB *threads[5];

    printIntegerMoj((uint64)HEAP_START_ADDR);
    printStringMoj("\n");
    void *stack = mem_alloc(DEFAULT_STACK_SIZE);
    void *stack2 = mem_alloc(DEFAULT_STACK_SIZE);
    printIntegerMoj((uint64)stack);
    printStringMoj("\n");
    printIntegerMoj((uint64)stack2);
    printStringMoj("\n");
    stack = 0;

    return 0;
}
