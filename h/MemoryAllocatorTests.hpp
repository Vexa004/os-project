//
// MemoryAllocatorTests.hpp
// Ekstenzivan test suite za MemoryAllocator (bare-metal, bez std biblioteka,
// bez izuzetaka, bez dinamickih kontejnera - koristi se samo javni interfejs
// klase MemoryAllocator: init(), mem_alloc(), mem_free()).
//
#ifndef PROJECT_BASE_V1_1_MEMORYALLOCATORTESTS_HPP
#define PROJECT_BASE_V1_1_MEMORYALLOCATORTESTS_HPP

// Pokrece kompletan (bezbedan) test suite. Poziva se TACNO JEDNOM iz
// main-a (ili iz test-only ulazne tacke), jer MemoryAllocator::init() ne
// moze da se ponovi (singleton, freeMemHead se ne resetuje).
void runMemoryAllocatorTests();

// ===========================================================================
// DANGER ZONE - vidi opsirne komentare u MemoryAllocatorTests.cpp.
//
// Ove funkcije NAMERNO testiraju scenarije koje implementacija (delimicno
// ili u potpunosti) moze da ne pokrije:
//   - runDoubleFreeDangerTest(): osnovni double-free, kad je blok jos
//     uvek poseban cvor u slobodnoj listi.
//   - runDoubleFreeCoalesceBypassDangerTest(): double-free NAD BLOKOM
//     KOJI JE VEC SPOJEN (coalesced) u susedni - suptilniji scenario.
//   - runInvalidPointerDangerTest(): pokazivac koji ne vodi na pocetak
//     validnog bloka. Rezultat zavisi od sadrzaja memorije.
//
// Sve tri funkcije SAME dokazuju da li je njihov scenario bezbedan: ako
// implementacija odbije operaciju PRE bilo kakve mutacije liste, funkcija
// nastavlja da koristi alokator i na kraju eksplicitno ispisuje da je
// bezbedno nastaviti u istom pokretanju. Ako operacija PRODJE (rc == 0)
// kad ne bi trebalo, funkcija prati posledice jos jednim dijagnostickim
// pozivom i onda staje - u tom slucaju NIKAD vise ne pozivati
// mem_alloc()/mem_free() (ni direktno ni preko runMemoryAllocatorTests())
// u istom pokretanju.
//
// PRAVILO: dok god NISI siguran da odredjeni scenario zavrsava kao
// "bezbedno za nastavak" (svi TEST_ASSERT-ovi PASS), pozivaj SAMO JEDNU
// od ove tri funkcije po pokretanju kernela, kao APSOLUTNO POSLEDNJU
// stvar u main().
// ===========================================================================
void runDoubleFreeDangerTest();
void runInvalidPointerDangerTest();
void runDoubleFreeCoalesceBypassDangerTest();

#endif //PROJECT_BASE_V1_1_MEMORYALLOCATORTESTS_HPP