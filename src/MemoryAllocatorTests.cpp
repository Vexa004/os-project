//
// MemoryAllocatorTests.cpp
//
// Run in a dedicated allocator-test kernel configuration.
// The allocator should be otherwise unused while these tests execute.
//

#include "../h/MemoryAllocator.hpp"
#include "../h/print.hpp"

namespace MemoryAllocatorTests
{

#ifndef MA_STRESS_CYCLES
#define MA_STRESS_CYCLES 2000
#endif

#ifndef MA_STRESS_OPERATIONS
#define MA_STRESS_OPERATIONS 10000
#endif

//
// Set to 1 ONLY if your project specification explicitly requires
// returned payload pointers themselves to be MEM_BLOCK_SIZE-aligned.
//
// With the allocator implementation supplied in the question, this
// will normally fail when sizeof(BlockHeader) is not a multiple of
// MEM_BLOCK_SIZE.
//
#ifndef MA_REQUIRE_MEM_BLOCK_ALIGNED_PAYLOAD
#define MA_REQUIRE_MEM_BLOCK_ALIGNED_PAYLOAD 0
#endif


static MemoryAllocator &allocator()
{
    return MemoryAllocator::getInstance();
}


static uint64 addr(const void *p)
{
    return (uint64)p;
}


static size_t heapBytes()
{
    size_t n =
        (char *)HEAP_END_ADDR -
        (char *)HEAP_START_ADDR;

    return (n / MEM_BLOCK_SIZE) * MEM_BLOCK_SIZE;
}


static size_t roundedRegion(size_t payload)
{
    // Only use this helper with controlled, non-overflowing test values.
    return ((sizeof(BlockHeader) + payload + MEM_BLOCK_SIZE - 1)
            / MEM_BLOCK_SIZE)
           * MEM_BLOCK_SIZE;
}


static size_t unitRegion()
{
    // Smallest region capable of allocating one byte.
    return roundedRegion(1);
}


static size_t payloadForRegion(size_t region)
{
    return region - sizeof(BlockHeader);
}


static BlockHeader *headerOf(void *p)
{
    return (BlockHeader *)
        ((char *)p - sizeof(BlockHeader));
}

/*
static const BlockHeader *headerOf(const void *p)
{
    return (const BlockHeader *)
        ((const char *)p - sizeof(BlockHeader));
}


static void *payloadFromBlock(void *block)
{
    return (char *)block + sizeof(BlockHeader);
}*/


static bool failAt(int line, const char *expr)
{
    printString("    FAIL line ");
    printInteger((uint64)line);
    printString(": ");
    printString(expr);
    printString("\n");
    return false;
}


#define MA_ASSERT(x)                         \
    do                                       \
    {                                        \
        if (!(x))                            \
            return failAt(__LINE__, #x);     \
    } while (0)


static bool allocationShapeOK(void *p, size_t requested)
{
    if (!p)
        return false;

    BlockHeader *h = headerOf(p);
    size_t expected = roundedRegion(requested);
    size_t heap = heapBytes();

    uint64 hs = addr(HEAP_START_ADDR);
    uint64 he = hs + heap;
    uint64 bs = addr(h);

    if (bs < hs || bs >= he)
        return false;

    if (h->size != expected)
        return false;

    if (h->size == 0 || h->size > heap)
        return false;

    if (bs > he - h->size)
        return false;

    if (p != (void *)((char *)h + sizeof(BlockHeader)))
        return false;

    // Allocated blocks are detached from the free list.
    if (h->next != nullptr)
        return false;

    return true;
}


static bool blocksDisjoint(void *a, void *b)
{
    const BlockHeader *ha = headerOf(a);
    const BlockHeader *hb = headerOf(b);

    uint64 as = addr(ha);
    uint64 ae = as + ha->size;

    uint64 bs = addr(hb);
    uint64 be = bs + hb->size;

    return ae <= bs || be <= as;
}


static unsigned char patternByte(unsigned char tag, size_t offset)
{
    return (unsigned char)
        (tag ^
         (unsigned char)offset ^
         (unsigned char)(offset >> 8));
}


static void fillPattern(void *p, size_t n, unsigned char tag)
{
    unsigned char *b = (unsigned char *)p;

    for (size_t i = 0; i < n; i++)
        b[i] = patternByte(tag, i);
}


static bool verifyPattern(
    const void *p,
    size_t n,
    unsigned char tag)
{
    const unsigned char *b =
        (const unsigned char *)p;

    for (size_t i = 0; i < n; i++)
    {
        if (b[i] != patternByte(tag, i))
            return false;
    }

    return true;
}


//
// A successful call proves that the free list currently represents
// one fully coalesced heap-sized free block.
//
// It also restores the allocator to that state before returning.
//
static bool probePristineHeap()
{
    const size_t heap = heapBytes();

    if (heap <= sizeof(BlockHeader))
        return false;

    const size_t maxPayload =
        heap - sizeof(BlockHeader);

    void *p = allocator().mem_alloc(maxPayload);

    if (!p)
        return false;

    BlockHeader *h = headerOf(p);

    bool ok =
        ((void *)h == (void *)HEAP_START_ADDR) &&
        (h->size == heap);

    //
    // A max-size allocation should leave nothing usable.
    //
    void *extra = allocator().mem_alloc(1);

    if (extra != nullptr)
    {
        // Do not attempt recovery from a potentially overlapping
        // allocation. Stop the test run.
        return false;
    }

    if (allocator().mem_free(p) != 0)
        return false;

    return ok;
}


static bool test_environment_geometry()
{
    MA_ASSERT(MEM_BLOCK_SIZE > 1);

    //
    // The split implementation writes a BlockHeader whenever at least
    // MEM_BLOCK_SIZE bytes remain, so a block needs to be large enough
    // for a header.
    //
    MA_ASSERT(MEM_BLOCK_SIZE >= sizeof(BlockHeader));

    MA_ASSERT(heapBytes() > sizeof(BlockHeader));

    //
    // HEAP_START must be suitable for storing BlockHeader.
    //
    MA_ASSERT(
        addr(HEAP_START_ADDR) %
        alignof(BlockHeader) == 0);

    //
    // Because every later header is an integral number of
    // MEM_BLOCK_SIZE bytes after the first, this should hold too.
    //
    MA_ASSERT(
        MEM_BLOCK_SIZE %
        alignof(BlockHeader) == 0);

    //
    // If your project promises MEM_BLOCK_SIZE-aligned block starts,
    // test that environmental assumption explicitly.
    //
    MA_ASSERT(
        addr(HEAP_START_ADDR) %
        MEM_BLOCK_SIZE == 0);

    return true;
}


static bool test_zero_and_safe_invalid()
{
    MA_ASSERT(allocator().mem_alloc(0) == nullptr);

    MA_ASSERT(allocator().mem_free(nullptr) != 0);

    MA_ASSERT(
        allocator().mem_free(
            (void *)HEAP_END_ADDR) != 0);

    //
    // Safely outside the accepted interval, so current mem_free()
    // returns before looking for a header.
    //
    void *below =
        (void *)((char *)HEAP_START_ADDR - 1);

    MA_ASSERT(allocator().mem_free(below) != 0);

    return true;
}


static bool test_rounding_and_header_layout()
{
    static size_t sizes[4];
    static void *ptrs[4];

    sizes[0] = 1;
    sizes[1] = MEM_BLOCK_SIZE - 1;
    sizes[2] = MEM_BLOCK_SIZE;
    sizes[3] = MEM_BLOCK_SIZE + 1;

    size_t required = 0;

    for (int i = 0; i < 4; i++)
        required += roundedRegion(sizes[i]);

    MA_ASSERT(
        required + MEM_BLOCK_SIZE <= heapBytes());

    char *expectedBlock =
        (char *)HEAP_START_ADDR;

    for (int i = 0; i < 4; i++)
    {
        ptrs[i] =
            allocator().mem_alloc(sizes[i]);

        MA_ASSERT(ptrs[i] != nullptr);

        BlockHeader *h =
            headerOf(ptrs[i]);

        MA_ASSERT(
            (void *)h ==
            (void *)expectedBlock);

        MA_ASSERT(
            ptrs[i] ==
            (void *)(expectedBlock +
                     sizeof(BlockHeader)));

        MA_ASSERT(
            h->size ==
            roundedRegion(sizes[i]));

        MA_ASSERT(h->next == nullptr);

        //
        // The entire requested payload must fit before the
        // next block boundary.
        //
        MA_ASSERT(
            sizeof(BlockHeader) + sizes[i]
            <= h->size);

        fillPattern(
            ptrs[i],
            sizes[i],
            (unsigned char)(0x31 + i));

        expectedBlock += h->size;
    }

    for (int i = 0; i < 4; i++)
    {
        MA_ASSERT(
            verifyPattern(
                ptrs[i],
                sizes[i],
                (unsigned char)(0x31 + i)));
    }

    for (int i = 3; i >= 0; i--)
        MA_ASSERT(
            allocator().mem_free(ptrs[i]) == 0);

    return true;
}


static bool test_alignment()
{
    void *p1 = allocator().mem_alloc(1);
    void *p2 = allocator().mem_alloc(1);

    MA_ASSERT(p1 != nullptr);
    MA_ASSERT(p2 != nullptr);

    BlockHeader *h1 = headerOf(p1);
    BlockHeader *h2 = headerOf(p2);

    MA_ASSERT(
        addr(h1) % alignof(BlockHeader) == 0);

    MA_ASSERT(
        addr(h2) % alignof(BlockHeader) == 0);

    //
    // Natural pointer alignment is a reasonable minimum allocator
    // requirement in this kernel.
    //
    MA_ASSERT(
        addr(p1) % alignof(void *) == 0);

    MA_ASSERT(
        addr(p2) % alignof(void *) == 0);

    //
    // Current allocator invariant:
    // headers are separated by a block-size multiple.
    //
    MA_ASSERT(
        (addr(h2) - addr(h1)) %
        MEM_BLOCK_SIZE == 0);

#if MA_REQUIRE_MEM_BLOCK_ALIGNED_PAYLOAD

    //
    // Enable only if required by your assignment.
    //
    MA_ASSERT(addr(p1) % MEM_BLOCK_SIZE == 0);
    MA_ASSERT(addr(p2) % MEM_BLOCK_SIZE == 0);

#endif

    MA_ASSERT(allocator().mem_free(p1) == 0);
    MA_ASSERT(allocator().mem_free(p2) == 0);

    return true;
}


static bool test_split_block()
{
    const size_t u = unitRegion();

    MA_ASSERT(heapBytes() >= 6 * u);

    void *a =
        allocator().mem_alloc(
            payloadForRegion(u));

    void *b =
        allocator().mem_alloc(
            payloadForRegion(3 * u));

    void *c =
        allocator().mem_alloc(
            payloadForRegion(u));

    MA_ASSERT(a && b && c);

    char *bBlock =
        (char *)headerOf(b);

    MA_ASSERT(allocator().mem_free(b) == 0);

    //
    // Split 3u hole into u + 2u.
    //
    void *x =
        allocator().mem_alloc(
            payloadForRegion(u));

    MA_ASSERT(x == b);

    MA_ASSERT(
        headerOf(x)->size == u);

    void *y =
        allocator().mem_alloc(
            payloadForRegion(2 * u));

    MA_ASSERT(y != nullptr);

    MA_ASSERT(
        (char *)headerOf(y) ==
        bBlock + u);

    MA_ASSERT(
        headerOf(y)->size == 2 * u);

    MA_ASSERT(blocksDisjoint(x, y));

    MA_ASSERT(allocator().mem_free(x) == 0);
    MA_ASSERT(allocator().mem_free(y) == 0);
    MA_ASSERT(allocator().mem_free(a) == 0);
    MA_ASSERT(allocator().mem_free(c) == 0);

    return true;
}


static bool test_consume_whole_hole()
{
    const size_t u = unitRegion();

    MA_ASSERT(heapBytes() >= 6 * u);

    void *a =
        allocator().mem_alloc(
            payloadForRegion(u));

    void *b =
        allocator().mem_alloc(
            payloadForRegion(2 * u));

    void *c =
        allocator().mem_alloc(
            payloadForRegion(u));

    MA_ASSERT(a && b && c);

    char *cBlock =
        (char *)headerOf(c);

    MA_ASSERT(allocator().mem_free(b) == 0);

    //
    // Exactly consume the complete 2u hole.
    //
    void *x =
        allocator().mem_alloc(
            payloadForRegion(2 * u));

    MA_ASSERT(x == b);

    //
    // The exact-fit block must have disappeared from the free list.
    // The next allocation must come from after c.
    //
    void *y =
        allocator().mem_alloc(
            payloadForRegion(u));

    MA_ASSERT(y != nullptr);

    MA_ASSERT(
        (char *)headerOf(y) ==
        cBlock + u);

    MA_ASSERT(blocksDisjoint(x, y));

    MA_ASSERT(allocator().mem_free(y) == 0);
    MA_ASSERT(allocator().mem_free(x) == 0);
    MA_ASSERT(allocator().mem_free(a) == 0);
    MA_ASSERT(allocator().mem_free(c) == 0);

    return true;
}


static bool test_free_list_address_order()
{
    const size_t u = unitRegion();

    MA_ASSERT(heapBytes() >= 7 * u);

    void *a = allocator().mem_alloc(payloadForRegion(u));
    void *b = allocator().mem_alloc(payloadForRegion(u));
    void *c = allocator().mem_alloc(payloadForRegion(u));
    void *d = allocator().mem_alloc(payloadForRegion(u));
    void *e = allocator().mem_alloc(payloadForRegion(u));

    MA_ASSERT(a && b && c && d && e);

    //
    // Free lower-address B first, then higher-address D.
    //
    // A simple LIFO free list would now be D -> B.
    // A sorted list must be B -> D.
    //
    MA_ASSERT(allocator().mem_free(b) == 0);
    MA_ASSERT(allocator().mem_free(d) == 0);

    void *x =
        allocator().mem_alloc(payloadForRegion(u));

    void *y =
        allocator().mem_alloc(payloadForRegion(u));

    MA_ASSERT(x == b);
    MA_ASSERT(y == d);

    MA_ASSERT(allocator().mem_free(x) == 0);
    MA_ASSERT(allocator().mem_free(y) == 0);
    MA_ASSERT(allocator().mem_free(a) == 0);
    MA_ASSERT(allocator().mem_free(c) == 0);
    MA_ASSERT(allocator().mem_free(e) == 0);

    return true;
}


static bool test_first_fit_skips_small_hole()
{
    const size_t u = unitRegion();

    MA_ASSERT(heapBytes() >= 8 * u);

    void *a = allocator().mem_alloc(payloadForRegion(u));
    void *b = allocator().mem_alloc(payloadForRegion(u));
    void *c = allocator().mem_alloc(payloadForRegion(u));

    void *d =
        allocator().mem_alloc(
            payloadForRegion(2 * u));

    void *e = allocator().mem_alloc(payloadForRegion(u));

    MA_ASSERT(a && b && c && d && e);

    MA_ASSERT(allocator().mem_free(b) == 0);
    MA_ASSERT(allocator().mem_free(d) == 0);

    //
    // B is first in address order but is too small.
    //
    void *large =
        allocator().mem_alloc(
            payloadForRegion(2 * u));

    MA_ASSERT(large == d);

    //
    // B should still be free.
    //
    void *small =
        allocator().mem_alloc(
            payloadForRegion(u));

    MA_ASSERT(small == b);

    MA_ASSERT(allocator().mem_free(large) == 0);
    MA_ASSERT(allocator().mem_free(small) == 0);
    MA_ASSERT(allocator().mem_free(a) == 0);
    MA_ASSERT(allocator().mem_free(c) == 0);
    MA_ASSERT(allocator().mem_free(e) == 0);

    return true;
}


static bool test_forward_coalesce()
{
    const size_t u = unitRegion();

    void *a = allocator().mem_alloc(payloadForRegion(u));
    void *b = allocator().mem_alloc(payloadForRegion(u));
    void *c = allocator().mem_alloc(payloadForRegion(u));
    void *d = allocator().mem_alloc(payloadForRegion(u));

    MA_ASSERT(a && b && c && d);

    //
    // C exists in free list first.
    // Then B is inserted immediately before it.
    //
    MA_ASSERT(allocator().mem_free(c) == 0);
    MA_ASSERT(allocator().mem_free(b) == 0);

    void *joined =
        allocator().mem_alloc(
            payloadForRegion(2 * u));

    MA_ASSERT(joined == b);

    MA_ASSERT(allocator().mem_free(joined) == 0);
    MA_ASSERT(allocator().mem_free(a) == 0);
    MA_ASSERT(allocator().mem_free(d) == 0);

    return true;
}


static bool test_backward_coalesce()
{
    const size_t u = unitRegion();

    void *a = allocator().mem_alloc(payloadForRegion(u));
    void *b = allocator().mem_alloc(payloadForRegion(u));
    void *c = allocator().mem_alloc(payloadForRegion(u));
    void *d = allocator().mem_alloc(payloadForRegion(u));

    MA_ASSERT(a && b && c && d);

    //
    // B is already free.
    // C is inserted after it, requiring merging through "current".
    //
    MA_ASSERT(allocator().mem_free(b) == 0);
    MA_ASSERT(allocator().mem_free(c) == 0);

    void *joined =
        allocator().mem_alloc(
            payloadForRegion(2 * u));

    MA_ASSERT(joined == b);

    MA_ASSERT(allocator().mem_free(joined) == 0);
    MA_ASSERT(allocator().mem_free(a) == 0);
    MA_ASSERT(allocator().mem_free(d) == 0);

    return true;
}


static bool test_bridge_coalesce()
{
    const size_t u = unitRegion();

    void *a = allocator().mem_alloc(payloadForRegion(u));
    void *b = allocator().mem_alloc(payloadForRegion(u));
    void *c = allocator().mem_alloc(payloadForRegion(u));
    void *d = allocator().mem_alloc(payloadForRegion(u));
    void *e = allocator().mem_alloc(payloadForRegion(u));

    MA_ASSERT(a && b && c && d && e);

    //
    // B and D are separate free islands.
    //
    MA_ASSERT(allocator().mem_free(b) == 0);
    MA_ASSERT(allocator().mem_free(d) == 0);

    //
    // C bridges both islands.
    //
    MA_ASSERT(allocator().mem_free(c) == 0);

    void *joined =
        allocator().mem_alloc(
            payloadForRegion(3 * u));

    MA_ASSERT(joined == b);

    MA_ASSERT(allocator().mem_free(joined) == 0);
    MA_ASSERT(allocator().mem_free(a) == 0);
    MA_ASSERT(allocator().mem_free(e) == 0);

    return true;
}


static void *fragmentBlocks[12];
static void *fragmentReuse[6];


static bool test_fragmentation_and_reuse()
{
    const size_t u = unitRegion();
    const size_t count = 12;

    MA_ASSERT(heapBytes() >= count * u);

    for (size_t i = 0; i < count; i++)
    {
        fragmentBlocks[i] =
            allocator().mem_alloc(
                payloadForRegion(u));

        MA_ASSERT(fragmentBlocks[i] != nullptr);
    }

    //
    // Pin the entire remaining heap in one allocated block.
    // This prevents the large-request test below from simply succeeding
    // from the large tail block.
    //
    size_t used = count * u;
    size_t remaining = heapBytes() - used;

    void *tail = nullptr;

    if (remaining > sizeof(BlockHeader))
    {
        tail =
            allocator().mem_alloc(
                remaining - sizeof(BlockHeader));

        MA_ASSERT(tail != nullptr);
    }

    //
    // Produce isolated one-region holes:
    //
    // FREE, LIVE, FREE, LIVE, ...
    //
    for (size_t i = 0; i < count; i += 2)
        MA_ASSERT(
            allocator().mem_free(
                fragmentBlocks[i]) == 0);

    //
    // Total free memory is >= 6u, but no contiguous free extent is 2u.
    //
    void *tooLarge =
        allocator().mem_alloc(
            payloadForRegion(2 * u));

    MA_ASSERT(tooLarge == nullptr);

    //
    // Reuse all holes. Sorted first-fit should return them in ascending
    // address order.
    //
    for (size_t i = 0; i < 6; i++)
    {
        fragmentReuse[i] =
            allocator().mem_alloc(
                payloadForRegion(u));

        MA_ASSERT(
            fragmentReuse[i] ==
            fragmentBlocks[2 * i]);
    }

    for (size_t i = 0; i < 6; i++)
        MA_ASSERT(
            allocator().mem_free(
                fragmentReuse[i]) == 0);

    for (size_t i = 1; i < count; i += 2)
        MA_ASSERT(
            allocator().mem_free(
                fragmentBlocks[i]) == 0);

    if (tail)
        MA_ASSERT(allocator().mem_free(tail) == 0);

    return true;
}


static bool test_large_near_heap_limit()
{
    const size_t heap = heapBytes();
    const size_t hs = sizeof(BlockHeader);
    const size_t u = unitRegion();

    MA_ASSERT(heap > hs + u);

    const size_t maxPayload = heap - hs;

    //
    // One byte below maximum still rounds to the complete heap region.
    //
    void *p =
        allocator().mem_alloc(maxPayload - 1);

    MA_ASSERT(p != nullptr);
    MA_ASSERT(headerOf(p)->size == heap);
    MA_ASSERT(allocator().mem_alloc(1) == nullptr);
    MA_ASSERT(allocator().mem_free(p) == 0);

    //
    // Exact maximum.
    //
    p = allocator().mem_alloc(maxPayload);

    MA_ASSERT(p != nullptr);
    MA_ASSERT(headerOf(p) == (BlockHeader *)HEAP_START_ADDR);
    MA_ASSERT(headerOf(p)->size == heap);

    unsigned char *b = (unsigned char *)p;

    b[0] = 0xA5;
    b[maxPayload - 1] = 0x5A;

    MA_ASSERT(b[0] == 0xA5);
    MA_ASSERT(b[maxPayload - 1] == 0x5A);

    MA_ASSERT(allocator().mem_free(p) == 0);

    //
    // Just over maximum.
    //
    MA_ASSERT(
        allocator().mem_alloc(
            maxPayload + 1) == nullptr);

    //
    // Leave precisely one allocation region behind.
    //
    MA_ASSERT(heap > u + hs);

    size_t largeRegion = heap - u;

    p =
        allocator().mem_alloc(
            payloadForRegion(largeRegion));

    MA_ASSERT(p != nullptr);
    MA_ASSERT(headerOf(p)->size == largeRegion);

    void *last =
        allocator().mem_alloc(
            payloadForRegion(u));

    MA_ASSERT(last != nullptr);

    MA_ASSERT(
        (char *)headerOf(last) ==
        (char *)HEAP_START_ADDR + largeRegion);

    MA_ASSERT(allocator().mem_alloc(1) == nullptr);

    MA_ASSERT(allocator().mem_free(last) == 0);
    MA_ASSERT(allocator().mem_free(p) == 0);

    return true;
}


static bool test_exhaust_and_rebuild_heap()
{
    const size_t heap = heapBytes();
    const size_t hs = sizeof(BlockHeader);

    //
    // Choose a block size giving roughly 64-128 allocations rather
    // than potentially millions of one-block allocations.
    //
    size_t minLinkRegion =
        roundedRegion(sizeof(void *));

    size_t chunk =
        (heap / 64 / MEM_BLOCK_SIZE)
        * MEM_BLOCK_SIZE;

    if (chunk < minLinkRegion)
        chunk = minLinkRegion;

    MA_ASSERT(chunk <= heap);
    MA_ASSERT(chunk > hs);

    void *list = nullptr;
    size_t count = 0;

    for (;;)
    {
        void *p =
            allocator().mem_alloc(
                payloadForRegion(chunk));

        if (!p)
            break;

        //
        // Intrusive list stored in allocated payload.
        // No giant pointer array on the kernel stack.
        //
        *((void **)p) = list;
        list = p;

        count++;

        //
        // The chunk formula should keep this modest.
        //
        MA_ASSERT(count <= 256);
    }

    MA_ASSERT(count > 0);

    size_t used = count * chunk;
    MA_ASSERT(used <= heap);

    size_t remainder = heap - used;

    void *tail = nullptr;

    //
    // Consume a smaller final remainder if it can hold any payload.
    //
    if (remainder > hs)
    {
        tail =
            allocator().mem_alloc(
                remainder - hs);

        MA_ASSERT(tail != nullptr);
    }

    //
    // No positive request should now fit anywhere.
    //
    MA_ASSERT(allocator().mem_alloc(1) == nullptr);

    if (tail)
        MA_ASSERT(allocator().mem_free(tail) == 0);

    //
    // Allocations were made in increasing-address order, so the
    // intrusive list naturally frees them in descending address order.
    //
    while (list)
    {
        void *next = *((void **)list);

        MA_ASSERT(
            allocator().mem_free(list) == 0);

        list = next;
    }

    //
    // The complete heap should have coalesced into one block.
    //
    const size_t maxPayload = heap - hs;

    void *giant =
        allocator().mem_alloc(maxPayload);

    MA_ASSERT(giant != nullptr);

    MA_ASSERT(
        headerOf(giant) ==
        (BlockHeader *)HEAP_START_ADDR);

    MA_ASSERT(headerOf(giant)->size == heap);

    MA_ASSERT(allocator().mem_alloc(1) == nullptr);

    MA_ASSERT(allocator().mem_free(giant) == 0);

    return true;
}


static bool test_repeated_cycles()
{
    static void *p[4];

    const size_t sizes[4] =
    {
        1,
        MEM_BLOCK_SIZE - 1,
        MEM_BLOCK_SIZE,
        MEM_BLOCK_SIZE + 1
    };

    for (size_t cycle = 0;
         cycle < MA_STRESS_CYCLES;
         cycle++)
    {
        for (int i = 0; i < 4; i++)
        {
            p[i] =
                allocator().mem_alloc(sizes[i]);

            MA_ASSERT(p[i] != nullptr);
            MA_ASSERT(
                allocationShapeOK(
                    p[i], sizes[i]));

            unsigned char *b =
                (unsigned char *)p[i];

            b[0] =
                (unsigned char)(0x40 + i);

            b[sizes[i] - 1] =
                (unsigned char)(0x80 + i);
        }

        //
        // Different freeing permutations exercise insertion and
        // coalescing from different directions.
        //
        switch (cycle & 3)
        {
            case 0:
                MA_ASSERT(allocator().mem_free(p[1]) == 0);
                MA_ASSERT(allocator().mem_free(p[3]) == 0);
                MA_ASSERT(allocator().mem_free(p[0]) == 0);
                MA_ASSERT(allocator().mem_free(p[2]) == 0);
                break;

            case 1:
                MA_ASSERT(allocator().mem_free(p[3]) == 0);
                MA_ASSERT(allocator().mem_free(p[2]) == 0);
                MA_ASSERT(allocator().mem_free(p[1]) == 0);
                MA_ASSERT(allocator().mem_free(p[0]) == 0);
                break;

            case 2:
                MA_ASSERT(allocator().mem_free(p[0]) == 0);
                MA_ASSERT(allocator().mem_free(p[2]) == 0);
                MA_ASSERT(allocator().mem_free(p[1]) == 0);
                MA_ASSERT(allocator().mem_free(p[3]) == 0);
                break;

            default:
                MA_ASSERT(allocator().mem_free(p[2]) == 0);
                MA_ASSERT(allocator().mem_free(p[0]) == 0);
                MA_ASSERT(allocator().mem_free(p[3]) == 0);
                MA_ASSERT(allocator().mem_free(p[1]) == 0);
                break;
        }

        //
        // Periodically require complete coalescing, not merely that
        // another small request works.
        //
        if ((cycle & 127) == 127)
            MA_ASSERT(probePristineHeap());
    }

    return true;
}


struct Slot
{
    void *p;
    size_t requested;
    unsigned char tag;
};


static Slot mixedSlots[48];


static bool test_many_sizes_and_overlap()
{
    const size_t maxSlots = 48;
    const size_t heap = heapBytes();

    for (size_t i = 0; i < maxSlots; i++)
    {
        mixedSlots[i].p = nullptr;
        mixedSlots[i].requested = 0;
        mixedSlots[i].tag = 0;
    }

    size_t usedBudget = 0;
    size_t n = 0;

    //
    // Use no more than roughly half the heap so this test isn't
    // accidentally an exhaustion test.
    //
    for (size_t i = 0; i < maxSlots; i++)
    {
        size_t span =
            7 * MEM_BLOCK_SIZE + 29;

        size_t request =
            1 +
            ((i * 73 + i * i * 19)
             % span);

        size_t region =
            roundedRegion(request);

        if (usedBudget + region > heap / 2)
            break;

        void *p =
            allocator().mem_alloc(request);

        MA_ASSERT(p != nullptr);
        MA_ASSERT(allocationShapeOK(p, request));

        //
        // Check against every already-live region before writing data.
        //
        for (size_t j = 0; j < i; j++)
        {
            if (mixedSlots[j].p)
                MA_ASSERT(
                    blocksDisjoint(
                        p,
                        mixedSlots[j].p));
        }

        mixedSlots[i].p = p;
        mixedSlots[i].requested = request;
        mixedSlots[i].tag =
            (unsigned char)(1 + i);

        fillPattern(
            p,
            request,
            mixedSlots[i].tag);

        usedBudget += region;
        n++;
    }

    MA_ASSERT(n >= 12);

    for (size_t i = 0; i < n; i++)
    {
        MA_ASSERT(
            verifyPattern(
                mixedSlots[i].p,
                mixedSlots[i].requested,
                mixedSlots[i].tag));
    }

    //
    // Free every third block.
    //
    for (size_t i = 0; i < n; i += 3)
    {
        MA_ASSERT(
            allocator().mem_free(
                mixedSlots[i].p) == 0);

        mixedSlots[i].p = nullptr;
    }

    //
    // Freeing neighbors must not damage live payloads.
    //
    for (size_t i = 0; i < n; i++)
    {
        if (mixedSlots[i].p)
        {
            MA_ASSERT(
                verifyPattern(
                    mixedSlots[i].p,
                    mixedSlots[i].requested,
                    mixedSlots[i].tag));
        }
    }

    //
    // Refill the holes with a different deterministic size sequence.
    //
    for (size_t i = 0; i < n; i += 3)
    {
        size_t span =
            3 * MEM_BLOCK_SIZE + 17;

        size_t request =
            1 +
            (((i + 11) * 41) % span);

        void *p =
            allocator().mem_alloc(request);

        MA_ASSERT(p != nullptr);
        MA_ASSERT(allocationShapeOK(p, request));

        for (size_t j = 0; j < n; j++)
        {
            if (mixedSlots[j].p)
                MA_ASSERT(
                    blocksDisjoint(
                        p,
                        mixedSlots[j].p));
        }

        mixedSlots[i].p = p;
        mixedSlots[i].requested = request;
        mixedSlots[i].tag =
            (unsigned char)(0x80 + i);

        fillPattern(
            p,
            request,
            mixedSlots[i].tag);
    }

    //
    // Verify all live user data after reuse/splitting.
    //
    for (size_t i = 0; i < n; i++)
    {
        MA_ASSERT(
            verifyPattern(
                mixedSlots[i].p,
                mixedSlots[i].requested,
                mixedSlots[i].tag));
    }

    //
    // Deliberately awkward free order.
    //
    for (size_t i = 1; i < n; i += 2)
    {
        MA_ASSERT(
            allocator().mem_free(
                mixedSlots[i].p) == 0);

        mixedSlots[i].p = nullptr;
    }

    for (size_t k = n; k > 0; k--)
    {
        size_t i = k - 1;

        if ((i & 1) == 0)
        {
            MA_ASSERT(
                allocator().mem_free(
                    mixedSlots[i].p) == 0);

            mixedSlots[i].p = nullptr;
        }
    }

    return true;
}


static uint64 nextDeterministic(uint64 &state)
{
    //
    // xorshift -- deterministic, reproducible, and no libc rand().
    //
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;

    return state;
}


static Slot stressSlots[24];


static bool test_deterministic_stress()
{
    const size_t maxSlots = 24;

    for (size_t i = 0; i < maxSlots; i++)
    {
        stressSlots[i].p = nullptr;
        stressSlots[i].requested = 0;
        stressSlots[i].tag = 0;
    }

    const size_t maxRequest =
        2 * MEM_BLOCK_SIZE + 17;

    const size_t worstRegion =
        roundedRegion(maxRequest);

    size_t activeSlots =
        heapBytes() / (2 * worstRegion);

    if (activeSlots > maxSlots)
        activeSlots = maxSlots;

    MA_ASSERT(activeSlots >= 4);

    uint64 state =
        0x6a09e667f3bcc909ULL;

    size_t successfulAllocations = 0;

    for (size_t op = 0;
         op < MA_STRESS_OPERATIONS;
         op++)
    {
        size_t index =
            nextDeterministic(state)
            % activeSlots;

        Slot &s = stressSlots[index];

        if (s.p)
        {
            //
            // Verify before freeing so corruption is attributed as
            // close as possible to its source.
            //
            MA_ASSERT(
                verifyPattern(
                    s.p,
                    s.requested,
                    s.tag));

            MA_ASSERT(
                allocator().mem_free(s.p) == 0);

            s.p = nullptr;
            s.requested = 0;
        }
        else
        {
            size_t request =
                1 +
                (nextDeterministic(state)
                 % maxRequest);

            void *p =
                allocator().mem_alloc(request);

            if (p)
            {
                MA_ASSERT(
                    allocationShapeOK(
                        p, request));

                for (size_t j = 0;
                     j < activeSlots;
                     j++)
                {
                    if (stressSlots[j].p)
                    {
                        MA_ASSERT(
                            blocksDisjoint(
                                p,
                                stressSlots[j].p));
                    }
                }

                s.p = p;
                s.requested = request;
                s.tag =
                    (unsigned char)
                    (1 + ((op * 31 + index * 7) % 251));

                fillPattern(
                    p,
                    request,
                    s.tag);

                successfulAllocations++;
            }
            else
            {
                //
                // An allocation failure can be legitimate due to
                // fragmentation. Deterministically free one live block
                // and continue.
                //
                bool freed = false;

                for (size_t j = 0;
                     j < activeSlots;
                     j++)
                {
                    if (stressSlots[j].p)
                    {
                        MA_ASSERT(
                            verifyPattern(
                                stressSlots[j].p,
                                stressSlots[j].requested,
                                stressSlots[j].tag));

                        MA_ASSERT(
                            allocator().mem_free(
                                stressSlots[j].p) == 0);

                        stressSlots[j].p = nullptr;
                        stressSlots[j].requested = 0;

                        freed = true;
                        break;
                    }
                }

                //
                // If nothing at all is allocated, a small request should
                // never fail.
                //
                MA_ASSERT(freed);
            }
        }

        //
        // Periodic global payload verification.
        //
        if ((op % 97) == 0)
        {
            for (size_t j = 0;
                 j < activeSlots;
                 j++)
            {
                if (stressSlots[j].p)
                {
                    MA_ASSERT(
                        verifyPattern(
                            stressSlots[j].p,
                            stressSlots[j].requested,
                            stressSlots[j].tag));
                }
            }
        }
    }

    MA_ASSERT(successfulAllocations > 100);

    for (size_t i = activeSlots;
         i > 0;
         i--)
    {
        Slot &s = stressSlots[i - 1];

        if (s.p)
        {
            MA_ASSERT(
                verifyPattern(
                    s.p,
                    s.requested,
                    s.tag));

            MA_ASSERT(
                allocator().mem_free(s.p) == 0);

            s.p = nullptr;
        }
    }

    return true;
}


//
// This is safe even against your current implementation because the
// returned bogus tiny allocation is immediately freed before reporting
// the failure.
//
static bool test_size_overflow()
{
    const size_t impossible =
        (size_t)-1;

    void *p =
        allocator().mem_alloc(impossible);

    if (p != nullptr)
    {
        //
        // Current implementation is expected to take this branch.
        //
        allocator().mem_free(p);

        return failAt(
            __LINE__,
            "mem_alloc(SIZE_MAX) must return nullptr");
    }

    return true;
}


typedef bool (*TestFunction)();


static bool runOne(
    const char *name,
    TestFunction test)
{
    printString("[TEST] ");
    printString(name);
    printString("\n");

    if (!test())
    {
        printString("  RESULT: FAIL\n");
        return false;
    }

    //
    // Every safe test promises to restore the entire heap.
    // Check that promise after every test rather than allowing subtle
    // leaks to contaminate later tests.
    //
    if (!probePristineHeap())
    {
        printString(
            "    FAIL: heap did not return to one "
            "fully-coalesced free block\n");

        printString("  RESULT: FAIL\n");
        return false;
    }

    printString("  RESULT: PASS\n");
    return true;
}


void runCoreTests()
{
    printString(
        "\n=== MemoryAllocator core tests ===\n");

    //
    // IMPORTANT:
    // Run this once, before other kernel code starts using the heap.
    //
    allocator().init();

    if (!runOne(
            "environment geometry",
            test_environment_geometry))
        return;

    if (!runOne(
            "zero and safe invalid inputs",
            test_zero_and_safe_invalid))
        return;

    if (!runOne(
            "rounding and header layout",
            test_rounding_and_header_layout))
        return;

    if (!runOne(
            "alignment",
            test_alignment))
        return;

    if (!runOne(
            "split free block",
            test_split_block))
        return;

    if (!runOne(
            "consume whole free hole",
            test_consume_whole_hole))
        return;

    if (!runOne(
            "free-list address ordering",
            test_free_list_address_order))
        return;

    if (!runOne(
            "first-fit skips small hole",
            test_first_fit_skips_small_hole))
        return;

    if (!runOne(
            "forward coalescing",
            test_forward_coalesce))
        return;

    if (!runOne(
            "backward coalescing",
            test_backward_coalesce))
        return;

    if (!runOne(
            "two-sided bridge coalescing",
            test_bridge_coalesce))
        return;

    if (!runOne(
            "fragmentation and reuse",
            test_fragmentation_and_reuse))
        return;

    if (!runOne(
            "large near heap limit",
            test_large_near_heap_limit))
        return;

    if (!runOne(
            "heap exhaustion and rebuild",
            test_exhaust_and_rebuild_heap))
        return;

    if (!runOne(
            "repeated alloc/free cycles",
            test_repeated_cycles))
        return;

    if (!runOne(
            "many sizes / corruption / overlap",
            test_many_sizes_and_overlap))
        return;

    if (!runOne(
            "deterministic adversarial stress",
            test_deterministic_stress))
        return;

    //
    // Deliberately put the known arithmetic robustness check last,
    // so it does not prevent the structural tests from running first.
    //
    if (!runOne(
            "allocation size overflow",
            test_size_overflow))
        return;

    printString(
        "=== ALL CORE TESTS PASSED ===\n");
}

} // namespace MemoryAllocatorTests