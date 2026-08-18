//
// MemoryAllocatorTests.cpp
//
// Ekstenzivni testovi za MemoryAllocator. Pisani su tako da rade u
// bare-metal okruzenju (bez malloc/new, bez std::vector, bez iostream) -
// koriste iste primitive kao i ostatak projekta: printString / printInteger.
//
// VAZNA NAPOMENA O DIZAJNU TESTOVA:
// MemoryAllocator je singleton i init() se moze pozvati samo jednom
// (drugi poziv samo ispise "Already initialized" i vrati se). To znaci
// da test suite NE MOZE da resetuje alokator izmedju testova. Zbog toga
// svaki test koji nesto alocira MORA da oslobodi bas ono sto je alocirao,
// tako da sledeci test pocinje sa (skoro) istim stanjem heap-a.
// Test "test_heap_returns_to_pristine_state" na kraju svake test grupe
// sluzi kao "kanarinac" koji proverava da je spajanje (coalescing)
// zaista vratilo ceo heap u jedan slobodan blok.
//
#include "../h/MemoryAllocatorTests.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/print.hpp"
#include "../lib/hw.h"

namespace
{
    int g_testsRun = 0;
    int g_testsPassed = 0;
    int g_testsFailed = 0;

    void reportTest(const char *name, bool passed)
    {
        g_testsRun++;
        if (passed)
        {
            g_testsPassed++;
            printString("[PASS] ");
        } else
        {
            g_testsFailed++;
            printString("[FAIL] ");
        }
        printString(name);
        printString("\n");
    }

#define TEST_ASSERT(cond, name) reportTest((name), (cond))

    void printLabeledAddr(const char *label, void *p)
    {
        printString(label);
        printInteger((uint64) p);
        printString("\n");
    }

    void printLabeledNum(const char *label, uint64 v)
    {
        printString(label);
        printInteger(v);
        printString("\n");
    }

    // Stampa povratni kod mem_free-a (0, -1, -2, ili nesto neocekivano),
    // bez oslanjanja na potencijalno problematican cast negativnog int-a
    // u uint64 (printInteger ocekuje nepotpisani broj).
    void printRc(const char *label, int rc)
    {
        printString(label);
        if (rc == 0) printString("0");
        else if (rc == -1) printString("-1");
        else if (rc == -2) printString("-2");
        else printInteger((uint64) rc);
        printString("\n");
    }

    // Racuna korisnu velicinu heap-a na isti nacin kao MemoryAllocator::init(),
    // zaokruzeno naniže na cele MEM_BLOCK_SIZE blokove.
    size_t usableHeapSize()
    {
        size_t heapSize = (char *) HEAP_END_ADDR - (char *) HEAP_START_ADDR;
        heapSize = (heapSize / MEM_BLOCK_SIZE) * MEM_BLOCK_SIZE;
        return heapSize;
    }

    // Broj bajtova koje korisnik moze iskoristiti u JEDNOM najvecem moguce
    // slobodnom bloku (kada je ceo heap slobodan), tj. heap minus header
    // jednog bloka (allocSize se zaokruzuje na MEM_BLOCK_SIZE, pa je ovo
    // najveca velicina koju mem_alloc(size) moze da posluzi u jednom komadu).
    size_t maxSingleAllocPayload()
    {
        size_t heapSize = usableHeapSize();
        // allocSize mora biti <= heapSize i deljivo sa MEM_BLOCK_SIZE.
        // requestedSize = sizeof(BlockHeader) + size <= heapSize
        return heapSize - sizeof(BlockHeader);
    }

    // Jednostavan xorshift32 PRNG (bez zavisnosti od standardne biblioteke),
    // za "stres" testove sa pseudo-slucajnim redosledom alokacija/oslobadjanja.
    uint32 g_rngState = 2463534242u;

    uint32 xorshift32()
    {
        uint32 x = g_rngState;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        g_rngState = x;
        return x;
    }

    size_t randRange(size_t lo, size_t hi) // [lo, hi]
    {
        if (hi <= lo) return lo;
        return lo + (xorshift32() % (hi - lo + 1));
    }

    // Popunjava memorijski blok prepoznatljivim uzorkom da bismo kasnije
    // mogli da proverimo da nije doslo do preklapanja/korupcije susednih
    // blokova.
    void fillPattern(unsigned char *buf, size_t n, unsigned char seed)
    {
        for (size_t i = 0; i < n; i++)
            buf[i] = (unsigned char) (seed + i);
    }

    bool checkPattern(unsigned char *buf, size_t n, unsigned char seed)
    {
        for (size_t i = 0; i < n; i++)
        {
            if (buf[i] != (unsigned char) (seed + i))
                return false;
        }
        return true;
    }
}

// ---------------------------------------------------------------------
// GRUPA 0: Osnovna inicijalizacija i singleton
// ---------------------------------------------------------------------

static void test_singleton_same_instance()
{
    MemoryAllocator &a = MemoryAllocator::getInstance();
    MemoryAllocator &b = MemoryAllocator::getInstance();
    TEST_ASSERT(&a == &b, "getInstance() uvek vraca istu referencu");
}

static void test_double_init_does_not_crash()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    // Prvi init je vec pozvan u runMemoryAllocatorTests(). Drugi poziv
    // ne sme da srusi program niti da pokvari postojece stanje heap-a.
    alloc.init();
    void *p = alloc.mem_alloc(16);
    TEST_ASSERT(p != nullptr, "alokacija radi i posle (no-op) drugog init() poziva");
    if (p) TEST_ASSERT(alloc.mem_free(p) == 0, "oslobadjanje posle drugog init() radi");
}

// ---------------------------------------------------------------------
// GRUPA 1: Osnovna alokacija/oslobadjanje
// ---------------------------------------------------------------------

static void test_basic_alloc_returns_non_null()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    void *p = alloc.mem_alloc(32);
    TEST_ASSERT(p != nullptr, "mem_alloc(32) vraca ne-null pokazivac");
    if (p)
    {
        TEST_ASSERT((char *) p >= (char *) HEAP_START_ADDR &&
                     (char *) p < (char *) HEAP_END_ADDR,
                     "vraceni pokazivac je unutar granica heap-a");
        alloc.mem_free(p);
    }
}

static void test_alloc_zero_returns_null()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    void *p = alloc.mem_alloc(0);
    TEST_ASSERT(p == nullptr, "mem_alloc(0) vraca nullptr");
}

static void test_write_read_pattern_inside_block()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    const size_t n = 100;
    unsigned char *buf = (unsigned char *) alloc.mem_alloc(n);
    TEST_ASSERT(buf != nullptr, "alokacija bafera za pattern test uspesna");
    if (buf)
    {
        fillPattern(buf, n, 0x5A);
        bool ok = checkPattern(buf, n, 0x5A);
        TEST_ASSERT(ok, "upisan pattern je nepromenjen posle citanja (nema korupcije)");
        alloc.mem_free(buf);
    }
}

static void test_multiple_allocations_are_distinct_and_non_overlapping()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    const int COUNT = 5;
    void *ptrs[COUNT];
    size_t sizes[COUNT] = {8, 16, 24, 40, 64};

    for (int i = 0; i < COUNT; i++)
        ptrs[i] = alloc.mem_alloc(sizes[i]);

    bool allNonNull = true;
    for (int i = 0; i < COUNT; i++)
        if (ptrs[i] == nullptr) allNonNull = false;
    TEST_ASSERT(allNonNull, "svih 5 alokacija razlicitih velicina je uspesno");

    bool distinct = true;
    for (int i = 0; i < COUNT && distinct; i++)
        for (int j = i + 1; j < COUNT; j++)
            if (ptrs[i] == ptrs[j]) distinct = false;
    TEST_ASSERT(distinct, "svi pokazivaci su medjusobno razliciti");

    // Upisujemo razlicite pattern-e u svaki blok i proveravamo da se
    // ne mesaju (da alokacije ne preklapaju memoriju).
    for (int i = 0; i < COUNT; i++)
        if (ptrs[i]) fillPattern((unsigned char *) ptrs[i], sizes[i], (unsigned char) (0x10 * (i + 1)));

    bool noOverlap = true;
    for (int i = 0; i < COUNT; i++)
        if (ptrs[i] && !checkPattern((unsigned char *) ptrs[i], sizes[i], (unsigned char) (0x10 * (i + 1))))
            noOverlap = false;
    TEST_ASSERT(noOverlap, "podaci u susednim blokovima se ne preklapaju/kvare");

    for (int i = 0; i < COUNT; i++)
        if (ptrs[i]) alloc.mem_free(ptrs[i]);
}

// ---------------------------------------------------------------------
// GRUPA 2: Granicni slucajevi velicine i poravnanja
// ---------------------------------------------------------------------

static void test_allocation_block_alignment()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    void *p = alloc.mem_alloc(1);
    TEST_ASSERT(p != nullptr, "mem_alloc(1) uspesna");
    if (p)
    {
        // Header pocinje sizeof(BlockHeader) ispred korisnickog pokazivaca.
        char *blockStart = (char *) p - sizeof(BlockHeader);
        size_t offsetFromHeapStart = blockStart - (char *) HEAP_START_ADDR;
        TEST_ASSERT(offsetFromHeapStart % MEM_BLOCK_SIZE == 0,
                    "pocetak bloka je poravnat na MEM_BLOCK_SIZE u odnosu na HEAP_START_ADDR");
        alloc.mem_free(p);
    }
}

static void test_allocation_exactly_one_block_boundary()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    // Velicina koja tacno popunjava jedan blok (header + payload == MEM_BLOCK_SIZE).
    if (MEM_BLOCK_SIZE > sizeof(BlockHeader))
    {
        size_t exact = MEM_BLOCK_SIZE - sizeof(BlockHeader);
        void *p1 = alloc.mem_alloc(exact);
        TEST_ASSERT(p1 != nullptr, "alokacija koja tacno popunjava jedan MEM_BLOCK_SIZE blok uspeva");

        // Sledeca alokacija (makar 1 bajt) mora zauzeti NOVI blok, a ne
        // prostor unutar prethodnog.
        void *p2 = alloc.mem_alloc(1);
        TEST_ASSERT(p2 != nullptr, "alokacija posle tacno-popunjenog bloka uspeva");
        if (p1 && p2)
        {
            TEST_ASSERT((char *) p2 - (char *) p1 >= (long) MEM_BLOCK_SIZE,
                        "novi blok pocinje bar MEM_BLOCK_SIZE dalje (nema preklapanja)");
        }
        if (p1) alloc.mem_free(p1);
        if (p2) alloc.mem_free(p2);
    } else
    {
        reportTest("preskocen: MEM_BLOCK_SIZE <= sizeof(BlockHeader)", true);
    }
}

static void test_allocation_just_over_block_boundary_uses_two_blocks()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    if (MEM_BLOCK_SIZE > sizeof(BlockHeader) + 1)
    {
        size_t exactPlusOne = (MEM_BLOCK_SIZE - sizeof(BlockHeader)) + 1;
        void *p = alloc.mem_alloc(exactPlusOne);
        TEST_ASSERT(p != nullptr, "alokacija za 1 bajt vecu od jednog bloka uspeva (zauzima 2 bloka)");
        if (p)
        {
            // Mora moci da se upise po celoj trazenoj velicini bez korupcije.
            fillPattern((unsigned char *) p, exactPlusOne, 0x77);
            TEST_ASSERT(checkPattern((unsigned char *) p, exactPlusOne, 0x77),
                        "moze se pisati/citati po celoj trazenoj velicini (2 bloka)");
            alloc.mem_free(p);
        }
    } else
    {
        reportTest("preskocen: MEM_BLOCK_SIZE previse mali za ovaj test", true);
    }
}

// ---------------------------------------------------------------------
// GRUPA 3: Iscrpljivanje heap-a (out of memory)
// ---------------------------------------------------------------------

static void test_alloc_larger_than_heap_fails()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    size_t tooBig = usableHeapSize() + MEM_BLOCK_SIZE * 10;
    void *p = alloc.mem_alloc(tooBig);
    TEST_ASSERT(p == nullptr, "zahtev veci od celog heap-a vraca nullptr");
}

static void test_single_alloc_of_whole_heap_then_oom_then_free()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    size_t payload = maxSingleAllocPayload();
    void *p = alloc.mem_alloc(payload);
    TEST_ASSERT(p != nullptr, "moguce je alocirati (skoro) ceo heap u jednom komadu");

    if (p)
    {
        void *p2 = alloc.mem_alloc(1);
        TEST_ASSERT(p2 == nullptr, "posle pune alokacije heap-a, sledeca alokacija vraca nullptr");
        if (p2) alloc.mem_free(p2);

        TEST_ASSERT(alloc.mem_free(p) == 0, "oslobadjanje jedinog velikog bloka uspeva");

        // Heap mora ponovo biti u potpunosti iskoristiv.
        void *p3 = alloc.mem_alloc(payload);
        TEST_ASSERT(p3 != nullptr, "posle oslobadjanja, ceo heap je ponovo dostupan");
        if (p3) alloc.mem_free(p3);
    }
}

static void test_exhaust_heap_with_small_blocks_then_free_all()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    const size_t smallSize = 1; // najmanja moguca alokacija

    // Za smallSize=1, allocSize je uvek isti (round-up na MEM_BLOCK_SIZE),
    // pa alokator (first-fit, redom, iz pocetno jednog velikog slobodnog
    // bloka) proizvodi pokazivace na EKVIDISTANTNIM adresama sa korakom
    // 'stride'. To nam omogucava da NE cuvamo svaki pokazivac pojedinacno
    // (za heap od desetina/stotina MB to bi bio niz od miliona pokazivaca,
    // tj. vise megabajta memorije van samog heap-a koji testiramo) - vec
    // ih racunamo iz prve dobijene adrese.
    const size_t requestedSize = sizeof(BlockHeader) + smallSize;
    const size_t stride = ((requestedSize + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE) * MEM_BLOCK_SIZE;
    const size_t expectedBlocks = usableHeapSize() / stride;

    void *first = nullptr;
    long long count = 0;
    // Sigurnosni limit da petlja ne moze da se zaglavi u beskonacnost ako
    // nesto nije u redu - dozvoljavamo malo vise od ocekivanog broja blokova.
    const long long safetyLimit = (long long) expectedBlocks + 16;

    while (count < safetyLimit)
    {
        void *p = alloc.mem_alloc(smallSize);
        if (p == nullptr) break;
        if (first == nullptr) first = p;
        count++;
    }

    TEST_ASSERT(count > 0, "moguce je alocirati bar jedan mali blok");
    TEST_ASSERT(count == (long long) expectedBlocks,
                "broj uspesnih malih alokacija odgovara ocekivanom broju blokova (heap / stride)");

    void *overflow = alloc.mem_alloc(smallSize);
    TEST_ASSERT(overflow == nullptr, "kada je heap zaista pun, dalje alokacije vracaju nullptr");
    if (overflow) alloc.mem_free(overflow);

    // Oslobadjamo koristeci izracunate adrese (first + i*stride), u ISTOM
    // redosledu kao alokaciju.
    bool allFreedOk = true;
    for (long long i = 0; i < count; i++)
    {
        void *p = (void *) ((char *) first + i * (long long) stride);
        if (alloc.mem_free(p) != 0) allFreedOk = false;
    }
    TEST_ASSERT(allFreedOk, "sve alocirane male blokove je moguce osloboditi bez greske");

    // Kanarinac: heap mora da se u potpunosti spoji nazad u jedan komad.
    size_t payload = maxSingleAllocPayload();
    void *whole = alloc.mem_alloc(payload);
    TEST_ASSERT(whole != nullptr,
                "posle oslobadjanja svih malih blokova (uzastopno), ceo heap se spojio nazad u jedan blok");
    if (whole) alloc.mem_free(whole);

    printLabeledNum("ocekivan broj malih blokova (heap/stride): ", (uint64) expectedBlocks);
    printLabeledNum("broj alociranih malih blokova do zasicenja heap-a: ", (uint64) count);
}

// ---------------------------------------------------------------------
// GRUPA 4: Spajanje susednih slobodnih blokova (coalescing)
// ---------------------------------------------------------------------

static void test_coalesce_forward_neighbor()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    void *a = alloc.mem_alloc(16);
    void *b = alloc.mem_alloc(16);
    void *c = alloc.mem_alloc(16);
    TEST_ASSERT(a && b && c, "3 uzastopne alokacije uspevaju (priprema za forward coalescing)");
    if (!a || !b || !c) { if (a) alloc.mem_free(a); if (b) alloc.mem_free(b); if (c) alloc.mem_free(c); return; }

    alloc.mem_free(b); // b postaje slobodan, spaja se sa svojim next-om ako je slobodan
    alloc.mem_free(c); // c postaje slobodan -> b i c bi trebalo da se spoje u jedan blok

    // Da bismo dokazali da su B i C spojeni u JEDAN slobodan blok, pokusavamo
    // da alociramo nesto sto sigurno ne staje u samo jedan od originalnih
    // (malih) blokova, ali staje u B+C zajedno.
    // Racunamo koliko payload-a nudi tacno dva spojena bloka velicine MEM_BLOCK_SIZE
    // (B i C su svaki zauzimali najmanje moguci ceo broj blokova za 16 bajtova).
    size_t oneBlockPayload = MEM_BLOCK_SIZE > sizeof(BlockHeader) ? MEM_BLOCK_SIZE - sizeof(BlockHeader) : 0;
    size_t needsTwoBlocks = oneBlockPayload + 1; // sigurno ne staje u 1 blok

    void *big = alloc.mem_alloc(needsTwoBlocks);
    TEST_ASSERT(big != nullptr, "posle oslobadjanja B pa C, moguce je alocirati blok koji zahteva 2 spojena bloka (forward coalescing)");

    if (big) alloc.mem_free(big);
    alloc.mem_free(a);
}

static void test_coalesce_backward_neighbor()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    void *a = alloc.mem_alloc(16);
    void *b = alloc.mem_alloc(16);
    void *c = alloc.mem_alloc(16);
    TEST_ASSERT(a && b && c, "3 uzastopne alokacije uspevaju (priprema za backward coalescing)");
    if (!a || !b || !c) { if (a) alloc.mem_free(a); if (b) alloc.mem_free(b); if (c) alloc.mem_free(c); return; }

    alloc.mem_free(c); // prvo oslobodi zadnji
    alloc.mem_free(b); // pa prethodni -> b treba da se spoji UNAPRED sa c

    size_t oneBlockPayload = MEM_BLOCK_SIZE > sizeof(BlockHeader) ? MEM_BLOCK_SIZE - sizeof(BlockHeader) : 0;
    size_t needsTwoBlocks = oneBlockPayload + 1;

    void *big = alloc.mem_alloc(needsTwoBlocks);
    TEST_ASSERT(big != nullptr, "posle oslobadjanja C pa B (obrnut redosled), B i C se ipak spajaju (backward coalescing)");

    if (big) alloc.mem_free(big);
    alloc.mem_free(a);
}

static void test_coalesce_both_sides_at_once()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    void *a = alloc.mem_alloc(16);
    void *b = alloc.mem_alloc(16);
    void *c = alloc.mem_alloc(16);
    TEST_ASSERT(a && b && c, "3 uzastopne alokacije uspevaju (priprema za spajanje sa obe strane)");
    if (!a || !b || !c) { if (a) alloc.mem_free(a); if (b) alloc.mem_free(b); if (c) alloc.mem_free(c); return; }

    alloc.mem_free(a); // levi kraj slobodan
    alloc.mem_free(c); // desni kraj slobodan (nije susedan sa A, jer je B jos zauzet)
    alloc.mem_free(b); // srednji blok se oslobadja - mora da se spoji i sa A i sa C

    size_t oneBlockPayload = MEM_BLOCK_SIZE > sizeof(BlockHeader) ? MEM_BLOCK_SIZE - sizeof(BlockHeader) : 0;
    size_t needsThreeBlocks = 2 * oneBlockPayload + 1; // sigurno ne staje u 2 bloka

    void *big = alloc.mem_alloc(needsThreeBlocks);
    TEST_ASSERT(big != nullptr,
                "A, B i C (oslobodjeni redosledom A, C, B) se spajaju u JEDAN blok od sva tri (spajanje sa obe strane)");

    if (big) alloc.mem_free(big);
}

static void test_heap_returns_to_pristine_state()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    size_t payload = maxSingleAllocPayload();
    void *whole = alloc.mem_alloc(payload);
    TEST_ASSERT(whole != nullptr,
                "KANARINAC: posle svih prethodnih testova, ceo heap je ponovo jedan slobodan blok");
    if (whole) alloc.mem_free(whole);
}

// ---------------------------------------------------------------------
// GRUPA 5: Nevalidni pozivi mem_free
// ---------------------------------------------------------------------

static void test_free_nullptr()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    int rc = alloc.mem_free(nullptr);
    TEST_ASSERT(rc != 0, "mem_free(nullptr) vraca gresku (ne 0)");
}

static void test_free_pointer_before_heap()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    void *bogus = (void *) ((char *) HEAP_START_ADDR - 1);
    int rc = alloc.mem_free(bogus);
    TEST_ASSERT(rc != 0, "mem_free(pokazivac ispred HEAP_START_ADDR) vraca gresku");
}

static void test_free_pointer_after_heap()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    void *bogus = (void *) ((char *) HEAP_END_ADDR + 1);
    int rc = alloc.mem_free(bogus);
    TEST_ASSERT(rc != 0, "mem_free(pokazivac iza HEAP_END_ADDR) vraca gresku");
}

static void test_free_pointer_exactly_at_heap_end()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    // Kod koristi "ptr >= HEAP_END_ADDR" kao uslov greske, pa je granica
    // HEAP_END_ADDR sama po sebi van validnog opsega - proveravamo baš to.
    void *bogus = (void *) HEAP_END_ADDR;
    int rc = alloc.mem_free(bogus);
    TEST_ASSERT(rc != 0, "mem_free(HEAP_END_ADDR) (granica, iskljucena) vraca gresku");
}

// NAPOMENA: Namerno NE testiramo mem_free() nad "random" pokazivacem koji
// je unutar heap opsega ali ne odgovara pocetku validnog bloka (npr.
// p + 1). Trenutna implementacija samo proverava da li je pokazivac
// unutar [HEAP_START_ADDR, HEAP_END_ADDR) i da li je blk->size > 0; ako
// "slucajno" naidje na pozitivan broj na toj adresi, upisace taj "blok"
// u slobodnu listu i moze nepovratno pokvariti stanje alokatora za sve
// naredne testove. Ovo je namerno izostavljeno iz automatskog test
// suite-a i predlaze se kao odvojen, izolovan test (idealno u posebnom
// procesu/testu koji se ne oslanja na ispravno stanje alokatora posle
// njega).

// ---------------------------------------------------------------------
// GRUPA 6: Ponovna upotreba oslobodjenog prostora (first-fit)
// ---------------------------------------------------------------------

static void test_free_then_alloc_reuses_space()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    void *p1 = alloc.mem_alloc(32);
    TEST_ASSERT(p1 != nullptr, "prva alokacija uspeva");
    if (!p1) return;
    alloc.mem_free(p1);

    void *p2 = alloc.mem_alloc(32);
    TEST_ASSERT(p2 != nullptr, "druga alokacija (iste velicine) posle free-a uspeva");
    TEST_ASSERT(p2 == p1, "first-fit ponovo koristi upravo oslobodjeni blok (isti pokazivac)");

    if (p2) alloc.mem_free(p2);
}

static void test_alloc_splits_block_and_remainder_is_usable()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    // Alociramo veliki blok, oslobodimo ga, pa alociramo mnogo manji deo iz
    // njega - ostatak (remainder) mora ostati validan slobodan blok koji se
    // moze iskoristiti za dodatnu alokaciju.
    size_t bigPayload = 10 * MEM_BLOCK_SIZE;
    if (bigPayload > maxSingleAllocPayload()) bigPayload = maxSingleAllocPayload();

    void *big = alloc.mem_alloc(bigPayload);
    TEST_ASSERT(big != nullptr, "velika alokacija za split test uspeva");
    if (!big) return;
    alloc.mem_free(big);

    void *small1 = alloc.mem_alloc(8);
    TEST_ASSERT(small1 != nullptr, "mala alokacija iz upravo oslobodjenog velikog bloka uspeva (split)");

    void *small2 = alloc.mem_alloc(8);
    TEST_ASSERT(small2 != nullptr, "moguce je alocirati jos jedan mali blok iz ostatka (remainder) posle split-a");

    TEST_ASSERT(small1 != small2, "split-ovani ostatak daje razlicit pokazivac od prvog dela");

    if (small1) alloc.mem_free(small1);
    if (small2) alloc.mem_free(small2);
}

// ---------------------------------------------------------------------
// GRUPA 7: Stres test - pseudo-slucajan redosled alokacija/oslobadjanja
// ---------------------------------------------------------------------

static void test_stress_random_alloc_free_pattern()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    const int SLOTS = 64;
    static void *ptrs[SLOTS];
    static size_t sizes[SLOTS];
    for (int i = 0; i < SLOTS; i++) { ptrs[i] = nullptr; sizes[i] = 0; }

    const int ITERATIONS = 500;
    bool anyCorruption = false;
    bool anyUnexpectedFailure = false;

    for (int it = 0; it < ITERATIONS; it++)
    {
        int slot = (int) randRange(0, SLOTS - 1);
        if (ptrs[slot] == nullptr)
        {
            size_t sz = randRange(1, MEM_BLOCK_SIZE * 4);
            void *p = alloc.mem_alloc(sz);
            if (p != nullptr)
            {
                fillPattern((unsigned char *) p, sz, (unsigned char) slot);
                ptrs[slot] = p;
                sizes[slot] = sz;
            }
            // p == nullptr je legitiman ishod ako je heap trenutno pun -
            // to NIJE greska sama po sebi.
        } else
        {
            if (!checkPattern((unsigned char *) ptrs[slot], sizes[slot], (unsigned char) slot))
                anyCorruption = true;
            if (alloc.mem_free(ptrs[slot]) != 0)
                anyUnexpectedFailure = true;
            ptrs[slot] = nullptr;
            sizes[slot] = 0;
        }
    }

    // Provera preostalih pattern-a pre finalnog cleanup-a.
    for (int i = 0; i < SLOTS; i++)
    {
        if (ptrs[i] && !checkPattern((unsigned char *) ptrs[i], sizes[i], (unsigned char) i))
            anyCorruption = true;
    }

    TEST_ASSERT(!anyCorruption, "stres test: nijedan blok nije pokvaren tokom 500 pseudo-slucajnih alokacija/oslobadjanja");
    TEST_ASSERT(!anyUnexpectedFailure, "stres test: mem_free() nikada nije neocekivano vratio gresku za validan pokazivac");

    // Cleanup - oslobodi sve sto je ostalo alocirano.
    for (int i = 0; i < SLOTS; i++)
        if (ptrs[i]) alloc.mem_free(ptrs[i]);
}

// ---------------------------------------------------------------------
// Glavna funkcija test suite-a
// ---------------------------------------------------------------------

void runMemoryAllocatorTests()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();

    printString("\n===== MemoryAllocator test suite - pocetak =====\n");
    printLabeledAddr("HEAP_START_ADDR = ", (void *) HEAP_START_ADDR);
    printLabeledAddr("HEAP_END_ADDR   = ", (void *) HEAP_END_ADDR);
    printLabeledNum("MEM_BLOCK_SIZE  = ", (uint64) MEM_BLOCK_SIZE);
    printLabeledNum("sizeof(BlockHeader) = ", (uint64) sizeof(BlockHeader));

    alloc.init(); // jedini validan poziv init() za ceo proces

    printLabeledNum("Korisna velicina heap-a (zaokruzena) = ", (uint64) usableHeapSize());
    printString("\n-- Grupa 0: singleton / init --\n");
    test_singleton_same_instance();
    test_double_init_does_not_crash();

    printString("\n-- Grupa 1: osnovna alokacija/oslobadjanje --\n");
    test_basic_alloc_returns_non_null();
    test_alloc_zero_returns_null();
    test_write_read_pattern_inside_block();
    test_multiple_allocations_are_distinct_and_non_overlapping();

    printString("\n-- Grupa 2: granicni slucajevi velicine/poravnanja --\n");
    test_allocation_block_alignment();
    test_allocation_exactly_one_block_boundary();
    test_allocation_just_over_block_boundary_uses_two_blocks();

    printString("\n-- Grupa 3: iscrpljivanje heap-a (OOM) --\n");
    test_alloc_larger_than_heap_fails();
    test_single_alloc_of_whole_heap_then_oom_then_free();
    test_exhaust_heap_with_small_blocks_then_free_all();

    printString("\n-- Grupa 4: spajanje susednih slobodnih blokova --\n");
    test_coalesce_forward_neighbor();
    test_coalesce_backward_neighbor();
    test_coalesce_both_sides_at_once();
    test_heap_returns_to_pristine_state();

    printString("\n-- Grupa 5: nevalidni pozivi mem_free --\n");
    test_free_nullptr();
    test_free_pointer_before_heap();
    test_free_pointer_after_heap();
    test_free_pointer_exactly_at_heap_end();

    printString("\n-- Grupa 6: ponovna upotreba prostora / split --\n");
    test_free_then_alloc_reuses_space();
    test_alloc_splits_block_and_remainder_is_usable();

    printString("\n-- Grupa 7: stres test --\n");
    test_stress_random_alloc_free_pattern();

    // Finalni kanarinac - posle bas svega, heap treba da bude ceo ponovo.
    printString("\n-- Finalna provera --\n");
    test_heap_returns_to_pristine_state();

    printString("\n===== Rezultat =====\n");
    printLabeledNum("Ukupno testova:  ", (uint64) g_testsRun);
    printLabeledNum("Uspesno (PASS):  ", (uint64) g_testsPassed);
    printLabeledNum("Neuspesno (FAIL):", (uint64) g_testsFailed);
    printString("===== MemoryAllocator test suite - kraj =====\n\n");
}

// #########################################################################
// #                                                                       #
// #                          D A N G E R   Z O N E                       #
// #                                                                       #
// #  Testovi ispod NAMERNO izazivaju scenarije za koje implementacija    #
// #  nema zastitu (double-free, pokazivac koji ne vodi na pocetak        #
// #  validnog bloka) i koji mogu TRAJNO pokvariti stanje alokatora u     #
// #  ovom pokretanju kernela - ukljucujuci ciklus u slobodnoj listi i    #
// #  "gubljenje" delova heap-a iz liste.                                 #
// #                                                                       #
// #  PRAVILA KORISCENJA (OBAVEZNO):                                       #
// #   1) U main() pozvati SAMO JEDNU od ove dve funkcije                 #
// #      (runDoubleFreeDangerTest() ILI runInvalidPointerDangerTest()),  #
// #      nikad obe u istom pokretanju kernela.                          #
// #   2) Pozvati je kao APSOLUTNO POSLEDNJU stvar u main()/test          #
// #      programu. Posle nje se NIKAD vise ne sme pozvati mem_alloc()    #
// #      ili mem_free() u istom pokretanju - lista moze biti ciklicna    #
// #      ili sadrzati proizvoljne (garbage) size/next vrednosti, pa      #
// #      sledeci poziv moze:                                             #
// #        - beskonacno da petlja (potrebno rucno gasenje QEMU-a),       #
// #        - da dereferencira nasumicnu adresu kao BlockHeader*          #
// #          (pad/nedefinisano ponasanje),                               #
// #        - tiho da vrati korisniku memoriju koja je logicki jos u      #
// #          upotrebi (use-after-free / dupla alokacija istog bloka).    #
// #   3) Normalan test suite (runMemoryAllocatorTests()) NE treba        #
// #      pozivati posle ovih funkcija u istom pokretanju - pokrenuti     #
// #      ih u ODVOJENOM boot-u QEMU-a.                                   #
// #                                                                       #
// #  Svrha ovih testova NIJE da "prodju" u smislu zelenog rezultata -    #
// #  vec da DOKUMENTUJU koje pretpostavke implementacija pravi. FAIL     #
// #  ovde znaci da je pretpostavka pogresna, tj. da postoji realan bug.  #
// #                                                                       #
// #########################################################################

// -------------------------------------------------------------------------
// DANGER TEST 1: Double-free (osnovni slucaj - blok je i dalje poseban
// cvor u slobodnoj listi u trenutku drugog free-a)
//
// Alocira 3 uzastopna bloka A, B, C i NAMERNO ne oslobadja A i C (da B
// ne bi mogao da se spoji sa susedima - time se scenario izoluje od
// uticaja coalescing-a). Zatim oslobadja B DVA PUTA.
//
// NAPOMENA (posle fixa): implementacija sada PRE bilo kakve izmene liste
// prolazi kroz slobodnu listu i proverava da li je blk vec u njoj. Kako je
// ta provera cisto citanje (read-only) i dolazi pre svake mutacije, ako
// detektuje da je B vec u listi i vrati gresku (npr. -3), NIJE doslo
// do nikakve izmene stanja - heap ostaje potpuno ispravan. Ovaj test to
// i dokazuje: posle odbijenog double-free-a nastavlja da koristi alokator
// (oslobadja A i C, pa proverava da se ceo heap ponovo spojio u jedan
// blok) - ako sve to prodje, scenario je zaista bezbedan.
//
// VAZNO OGRANICENJE ove zastite: ona trazi blk kao POSEBAN cvor u listi.
// Ako je blok u medjuvremenu spojen (coalesced) UNAZAD u svog prethodnika,
// on vise nije poseban cvor - i tada provera NE detektuje double-free.
// Taj (opasniji, tihi) scenario je pokriven u DANGER TEST 3 ispod.
// -------------------------------------------------------------------------
void runDoubleFreeDangerTest()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    alloc.init(); // no-op ako je vec inicijalizovano u ovom pokretanju

    printString("\n===== DANGER TEST 1: double-free (osnovni slucaj) - pocetak =====\n");

    // A i C ostaju NAMERNO alocirani (ne oslobadjaju se odmah) - iskljucuje
    // coalescing kao faktor i izoluje cist double-free scenario, u kom B
    // ostaje poseban cvor u slobodnoj listi.
    void *A = alloc.mem_alloc(16);
    void *B = alloc.mem_alloc(16);
    void *C = alloc.mem_alloc(16);

    TEST_ASSERT(A && B && C, "priprema: 3 uzastopne alokacije (A, B, C) uspevaju");
    if (!A || !B || !C)
    {
        printString("Priprema nije uspela - danger test se prekida bez daljih poziva.\n");
        return;
    }

    int rc1 = alloc.mem_free(B);
    TEST_ASSERT(rc1 == 0, "prvo oslobadjanje B uspeva (rc == 0)");

    // DRUGO oslobadjanje ISTOG pokazivaca - ovo je double-free. U ovom
    // trenutku je B i dalje poseban cvor u listi (nije spojen ni sa kim),
    // pa fix treba da ga detektuje PRE bilo kakve mutacije.
    int rc2 = alloc.mem_free(B);
    printRc("mem_free(B) drugi put (double-free) vratio je: ", rc2);

    TEST_ASSERT(rc2 != 0,
                "OCEKIVANO: dvostruko oslobadjanje treba da bude odbijeno (rc != 0). "
                "FAIL ovde = implementacija NEMA zastitu od double-free-a za ovaj scenario.");

    // Dokaz da odbijeni double-free NIJE ostavio korupciju: nastavljamo da
    // koristimo alokator u ISTOM pokretanju.
    int rcA = alloc.mem_free(A);
    int rcC = alloc.mem_free(C);
    TEST_ASSERT(rcA == 0 && rcC == 0,
                "posle odbijenog double-free-a, A i C se i dalje mogu normalno osloboditi");

    size_t payload = maxSingleAllocPayload();
    void *whole = alloc.mem_alloc(payload);
    TEST_ASSERT(whole != nullptr,
                "posle odbijenog double-free-a i oslobadjanja A/C, ceo heap se ponovo spojio "
                "u jedan blok - dokaz da je OVAJ scenario double-free-a sada zaista bezbedan");
    if (whole) alloc.mem_free(whole);

    printString("Ako su svi gornji testovi PASS, ovaj konkretan scenario (B kao poseban cvor u\n");
    printString("listi) je bezbedan i mem_alloc/mem_free se mogu bezbedno dalje koristiti u ovom\n");
    printString("pokretanju. Vidi DANGER TEST 3 za scenario koji ipak zaobilazi ovu zastitu.\n");
    printString("===== DANGER TEST 1: double-free (osnovni slucaj) - kraj =====\n\n");
}

// -------------------------------------------------------------------------
// DANGER TEST 2: Pokazivac koji ne vodi na pocetak validnog bloka
//
// (I dalje potpuno relevantan posle fixa iz DANGER TEST 1 - ta izmena
// dodaje samo proveru za double-free, ne dodaje nikakav potpis/magic broj
// koji bi potvrdio da ptr zaista pokazuje na pocetak validnog bloka.)
//
// mem_free() proverava samo da li je ptr unutar [HEAP_START_ADDR,
// HEAP_END_ADDR) i da li je (ptr - sizeof(BlockHeader))->size > 0. Ne
// postoji nikakav "potpis"/magic broj u headeru koji potvrdjuje da ptr
// zaista pokazuje na pocetak stvarno alociranog bloka. Ovaj test prosledjuje
// namerno pomeren pokazivac (validan blok + 3 bajta) i posmatra da li ga
// alokator (pogresno) prihvati kao ispravan.
//
// Rezultat je nuzno zavisan od trenutnog sadrzaja memorije na toj adresi
// (koji bajtovi header-a i susednih polja se tu "slucajno" nadju), pa ga
// treba shvatiti kao demonstraciju odsustva provere, a ne kao determinisitcki
// regresioni test.
// -------------------------------------------------------------------------
void runInvalidPointerDangerTest()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    alloc.init(); // no-op ako je vec inicijalizovano u ovom pokretanju

    printString("\n===== DANGER TEST: nevalidan (garbage) pokazivac - pocetak =====\n");
    printString(">>> UPOZORENJE: posle ovog testa NE pozivati mem_alloc/mem_free ponovo u ovom pokretanju. <<<\n");

    void *p = alloc.mem_alloc(64);
    TEST_ASSERT(p != nullptr, "priprema: alokacija uspeva");
    if (!p)
    {
        printString("Priprema nije uspela - danger test se prekida bez daljih poziva.\n");
        return;
    }

    // Namerno pomeren pokazivac - NE odgovara pocetku nijednog validnog
    // korisnickog bloka. blk = (BlockHeader*)((char*)garbage -
    // sizeof(BlockHeader)) ce pogoditi sredinu stvarnog header-a/podataka
    // i procitace proizvoljne bajtove kao 'size' i 'next'.
    void *garbage = (void *) ((char *) p + 3);

    printLabeledAddr("Prosledjujem mem_free() namerno pomeren pokazivac: ", garbage);

    int rc = alloc.mem_free(garbage);
    printRc("mem_free(garbage) vratio je: ", rc);

    TEST_ASSERT(rc != 0,
                "OCEKIVANO: mem_free na pokazivac koji ne odgovara pocetku validnog bloka "
                "treba da bude odbijen (rc != 0). FAIL ovde = implementacija je prihvatila "
                "proizvoljne bajtove kao 'ispravan' BlockHeader (blk->size je slucajno > 0), "
                "sto moze trajno pokvariti slobodnu listu (realan bug).");

    printString("Napomena: jedina provera je 'blk->size <= 0 => greska'. Bez potpisa/magic\n");
    printString("broja u headeru, rezultat zavisi od toga sta se slucajno nalazi na toj\n");
    printString("adresi u memoriji u trenutku poziva.\n");

    printString("===== DANGER TEST: nevalidan pokazivac - kraj. STOP - dalje ne koristiti alokator. =====\n\n");
}

// -------------------------------------------------------------------------
// DANGER TEST 3: Double-free NAD BLOKOM KOJI JE VEC SPOJEN (coalesced) u
// suseda - suptilniji scenario od DANGER TEST 1.
//
// Provera u mem_free() koja trazi blk kao poseban cvor u listi NE hvata
// slucaj kad je blok u medjuvremenu spojen (coalesced) UNAZAD u svog
// prethodnika - adresa mu se vise ne pojavljuje direktno kao cvor, samo
// je njegov prostor sada deo veceg susednog bloka. Ako je uz to dodata i
// provera "da li adresa pada UNUTAR opsega nekog vec slobodnog bloka"
// (current < blk < current + current->size), i ovaj scenario treba da
// bude uhvacen - PRE bilo kakve mutacije liste, pa i ovaj (suptilniji)
// slucaj postaje bezbedan. Test to i dokazuje: ako je drugi free(B)
// odbijen, nastavljamo da koristimo alokator i proveravamo da se heap
// u potpunosti vraca u pristine stanje.
//
// Ako PAK provera ovo NE hvata (rc == 0), test demonstrira da se moze
// desiti TIHA korupcija: naredna mem_alloc() moze vratiti prostor koji se
// fizicki preklapa sa jos uvek zivim C blokom, bez ijedne greske u
// povratnim vrednostima. U tom slucaju se test ODMAH zaustavlja i vise
// NE sme dalje da koristi alokator u istom pokretanju.
//
// Scenario:
//   A, B, C - 3 uzastopna bloka, svaki tacno velicine jednog MEM_BLOCK_SIZE.
//   free(A) -> A sam u listi.
//   free(B) -> B se spaja UNAZAD u A (legitimno). A->size postaje
//              2*MEM_BLOCK_SIZE. B vise NIJE poseban cvor.
//   upisemo canary u C (C ostaje alociran - "zrtva" koju posmatramo).
//   free(B) DRUGI PUT -> double-free nad vec-upijenim blokom.
// -------------------------------------------------------------------------
void runDoubleFreeCoalesceBypassDangerTest()
{
    MemoryAllocator &alloc = MemoryAllocator::getInstance();
    alloc.init(); // no-op ako je vec inicijalizovano u ovom pokretanju

    printString("\n===== DANGER TEST 3: double-free nad vec-spojenim blokom - pocetak =====\n");

    if (MEM_BLOCK_SIZE <= sizeof(BlockHeader))
    {
        printString("Preskoceno: MEM_BLOCK_SIZE je premali za ovaj scenario.\n");
        printString("===== DANGER TEST 3 - kraj (preskoceno) =====\n\n");
        return;
    }

    const size_t onePayload = MEM_BLOCK_SIZE - sizeof(BlockHeader); // tacno 1 blok

    void *A = alloc.mem_alloc(onePayload);
    void *B = alloc.mem_alloc(onePayload);
    void *C = alloc.mem_alloc(onePayload);

    TEST_ASSERT(A && B && C, "priprema: A, B, C - svaki tacno jedan MEM_BLOCK_SIZE - uspevaju");
    if (!A || !B || !C)
    {
        printString("Priprema nije uspela - danger test se prekida bez daljih poziva.\n");
        return;
    }

    // Upisemo canary u C ODMAH nakon alokacije, dok je heap jos netaknut.
    const unsigned char canaryByte = 0xC3;
    fillPattern((unsigned char *) C, onePayload, canaryByte);

    int rcFreeA = alloc.mem_free(A);
    TEST_ASSERT(rcFreeA == 0, "free(A) uspeva (A sam u listi)");

    int rcFreeB1 = alloc.mem_free(B);
    TEST_ASSERT(rcFreeB1 == 0, "free(B) uspeva i (ocekivano) spaja B UNAZAD u A");

    // Provera da je canary u C i dalje netaknut posle legitimnog spajanja A+B.
    TEST_ASSERT(checkPattern((unsigned char *) C, onePayload, canaryByte),
                "posle legitimnog spajanja A+B, C-ov canary je i dalje netaknut (ocekivano)");

    // DRUGI free(B) - double-free nad vec-upijenim blokom.
    int rcFreeB2 = alloc.mem_free(B);
    printRc("mem_free(B) drugi put, POSLE sto je B vec spojen u A, vratio je: ", rcFreeB2);

    TEST_ASSERT(rcFreeB2 != 0,
                "OCEKIVANO: i ovaj (suptilniji) double-free treba da bude odbijen (rc != 0). "
                "FAIL ovde = implementacija NEMA zastitu ni za ovaj scenario (ozbiljniji, tihi bug).");

    if (rcFreeB2 != 0)
    {
        // ---- Odbijeno: dokazujemo da nema korupcije, nastavljamo dalje. ----
        printString("Drugi free(B) je odbijen PRE bilo kakve izmene liste - dokazujemo da je\n");
        printString("stanje ostalo ispravno, nastavljajuci normalno koriscenje alokatora.\n");

        TEST_ASSERT(checkPattern((unsigned char *) C, onePayload, canaryByte),
                    "posle odbijenog double-free-a, C-ov canary je i dalje netaknut");

        int rcFreeC = alloc.mem_free(C);
        TEST_ASSERT(rcFreeC == 0, "posle odbijenog double-free-a, C se i dalje moze normalno osloboditi");

        size_t payload = maxSingleAllocPayload();
        void *whole = alloc.mem_alloc(payload);
        TEST_ASSERT(whole != nullptr,
                    "posle odbijenog double-free-a i oslobadjanja C, ceo heap se ponovo spojio "
                    "u jedan blok - dokaz da je i OVAJ (suptilniji) scenario double-free-a bezbedan");
        if (whole) alloc.mem_free(whole);

        printString("Ako su svi gornji testovi PASS, ovaj scenario (double-free nad vec-spojenim\n");
        printString("blokom) je takodje bezbedan i mem_alloc/mem_free se mogu dalje bezbedno\n");
        printString("koristiti u ovom pokretanju.\n");
        printString("===== DANGER TEST 3: double-free nad vec-spojenim blokom - kraj =====\n\n");
        return;
    }

    // ---- Nije odbijeno (rc == 0): pratimo tihu korupciju dalje, PA STAJEMO. ----
    printString(">>> rc == 0 znaci da provera NIJE uhvatila double-free. Od ovog trenutka se\n");
    printString(">>> alokator NE SME dalje koristiti u ovom pokretanju - pratimo posledice\n");
    printString(">>> samo jos jednim pokusajem alokacije radi dijagnostike. <<<\n");

    size_t claimedPayload = 3 * MEM_BLOCK_SIZE - sizeof(BlockHeader); // ociekivano lazno prijavljena velicina
    void *big = alloc.mem_alloc(claimedPayload);

    if (big != nullptr)
    {
        printLabeledAddr("mem_alloc() je vratio blok velicine 3*MEM_BLOCK_SIZE na adresi: ", big);
        fillPattern((unsigned char *) big, claimedPayload, 0x99);

        bool canaryIntact = checkPattern((unsigned char *) C, onePayload, canaryByte);
        TEST_ASSERT(canaryIntact,
                    "posle upisa u 'lazno' velik blok, C-ov canary je i dalje netaknut. FAIL ovde = "
                    "DOKAZANA tiha korupcija: alokator je izdao memoriju koja se fizicki preklapa "
                    "sa jos uvek zivim C blokom (dva razlicita 'validna' pokazivaca dele istu memoriju).");
    } else
    {
        printString("mem_alloc(claimedPayload) je vratio nullptr - moguce je da se korupcija manifestuje\n");
        printString("na drugi nacin (npr. 'fantomski' cvor unutar A-ovog opsega umesto direktnog\n");
        printString("preklapanja sa C-om). Stanje liste je ipak nepouzdano od ovog trenutka.\n");
    }

    printString("Napomena: bez obzira na ishod gornje provere, stanje slobodne liste posle ovog\n");
    printString("testa vise NIJE pouzdano. Ovo pokretanje kernela vise NE SME da poziva\n");
    printString("mem_alloc/mem_free.\n");

    printString("===== DANGER TEST 3: double-free nad vec-spojenim blokom - kraj. STOP. =====\n\n");
}