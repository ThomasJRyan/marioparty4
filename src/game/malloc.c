#include "game/memory.h"
#include "game/init.h"
#include "dolphin/os.h"

static u32 HeapSizeTbl[HEAP_MAX] = { 0x240000, 0x140000, 0xA80000, 0x580000, 0 };
static void *HeapTbl[HEAP_MAX];

/**
 * @brief Allocates and initializes all game memory heaps.
 *
 * @details Four fixed-size heaps are created first; the final heap consumes
 * the remaining space in the current OS heap.
 */
void HuMemInitAll(void)
{
    s32 i;
    void *ptr;
    u32 free_size;
    for(i=0; i<4; i++) {
        ptr = OSAlloc(HeapSizeTbl[i]);
        if(ptr == NULL) {
            OSReport("HuMem> Failed OSAlloc Size:%d\n", HeapSizeTbl[i]);
            return;
        }
        HeapTbl[i] = HuMemInit(ptr, HeapSizeTbl[i]);
    }
    free_size = OSCheckHeap(currentHeapHandle);
    OSReport("HuMem> left memory space %dKB(%d)\n", free_size/1024, free_size);
    ptr = OSAlloc(free_size);
    if(ptr == NULL) {
        OSReport("HuMem> Failed OSAlloc left space\n");
        return;
    }
    HeapTbl[4] = HuMemInit(ptr, free_size);
    HeapSizeTbl[4] = free_size;
}

/**
 * @brief Initializes a memory heap over a caller-provided region.
 *
 * @param ptr Start of the backing memory region.
 * @param size Region size in bytes.
 * @return The initialized heap pointer.
 */
void *HuMemInit(void *ptr, s32 size)
{
    return HuMemHeapInit(ptr, size);
}

/**
 * @brief Flushes the DVD and general-purpose heaps from the data cache.
 */
void HuMemDCFlushAll()
{
    HuMemDCFlush(2);
    HuMemDCFlush(0);
}

/**
 * @brief Flushes an entire game heap from the data cache without synchronizing.
 *
 * @param heap Heap identifier to flush.
 */
void HuMemDCFlush(HeapID heap)
{
    DCFlushRangeNoSync(HeapTbl[heap], HeapSizeTbl[heap]);
}

/**
 * @brief Allocates 32-byte-aligned memory from a game heap.
 *
 * @param heap Heap to allocate from.
 * @param size Requested byte count.
 * @return Allocated data pointer, or NULL when allocation fails.
 */
void *HuMemDirectMalloc(HeapID heap, s32 size)
{
    register u32 retaddr;
    asm {
        mflr retaddr
    }
    size = (size+31) & 0xFFFFFFE0;
    return HuMemMemoryAlloc(HeapTbl[heap], size, retaddr);
}

/**
 * @brief Allocates aligned memory and associates it with a numeric tag.
 *
 * @param heap Heap to allocate from.
 * @param size Requested byte count.
 * @param num Allocation tag used for grouped freeing.
 * @return Allocated data pointer, or NULL when allocation fails.
 */
void *HuMemDirectMallocNum(HeapID heap, s32 size, u32 num)
{
    register u32 retaddr;
    asm {
        mflr retaddr
    }
    size = (size+31) & 0xFFFFFFE0;
    return HuMemMemoryAllocNum(HeapTbl[heap], size, num, retaddr);
}

/**
 * @brief Frees one allocation from a game heap.
 *
 * @param ptr Allocation to free; NULL is accepted.
 */
void HuMemDirectFree(void *ptr)
{
    register u32 retaddr;
    asm {
        mflr retaddr
    }
    HuMemMemoryFree(ptr, retaddr);
}

/**
 * @brief Frees every allocation carrying a numeric tag in one heap.
 *
 * @param heap Heap containing the tagged allocations.
 * @param num Allocation tag to release.
 */
void HuMemDirectFreeNum(HeapID heap, u32 num)
{
    register u32 retaddr;
    asm {
        mflr retaddr
    }
    HuMemMemoryFreeNum(HeapTbl[heap], num, retaddr);
}

/**
 * @brief Returns the total size of allocated blocks in a game heap.
 *
 * @param heap Heap to inspect.
 * @return Allocated block bytes, including allocator headers and alignment.
 */
s32 HuMemUsedMallocSizeGet(HeapID heap)
{
    return HuMemUsedMemorySizeGet(HeapTbl[heap]);
}

/**
 * @brief Counts allocated blocks in a game heap.
 *
 * @param heap Heap to inspect.
 * @return Number of allocated blocks.
 */
s32 HuMemUsedMallocBlockGet(HeapID heap)
{
    return HuMemUsedMemoryBlockGet(HeapTbl[heap]);
}

/**
 * @brief Returns a game heap's backing-region size.
 *
 * @param heap Heap to inspect.
 * @return Heap size in bytes.
 */
u32 HuMemHeapSizeGet(HeapID heap)
{
    return HeapSizeTbl[heap];
}

/**
 * @brief Returns the allocator root for a game heap.
 *
 * @param heap Heap to inspect.
 * @return Pointer to the heap's first memory-control block.
 */
void *HuMemHeapPtrGet(HeapID heap)
{
    return HeapTbl[heap];
}
