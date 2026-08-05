#include "game/dvd.h"
#include "game/data.h"

#include "dolphin/dvd.h"
#include "dolphin/os.h"


static DVDDiskID correctDiskID = {
    { 'M', 'P', 'G', 'C' }, //gameName
    { 'H', 'U' }, //company
    1, //diskNumber 
    1, //gameVersion
    1, //streaming
    0, //streamingBufSize
};


static int CallBackStatus;
static s32 beforeDvdStatus;

/**
 * @brief Mark a synchronous DVD read wrapper as complete.
 *
 * @param [in] result DVD read result; completion alone is recorded.
 * @param [in] fileInfo File associated with the completed read; not inspected.
 */
static void HuDVDReadAsyncCallBack(s32 result, DVDFileInfo* fileInfo)
{
    CallBackStatus = 1;
}

/**
 * @brief Allocate a buffer and start a DVD file read.
 *
 * @param [in, out] file Open DVD file whose entire rounded length is read.
 * @param [in] heap Heap from which the destination buffer is allocated.
 * @param [in] mode Allocation mode: `1` tags the allocation with `num`; other
 *                  accepted values use an untagged allocation.
 * @param [in] num Allocation tag used when `mode` is `1`.
 * @param [in] cb Completion callback passed to DVDReadAsync().
 * @param [in] async If false, poll DVD status until the callback completes.
 *
 * @return Heap-owned, 32-byte-rounded read buffer; allocation failure panics.
 */
static void *HuDvdDataReadWait(DVDFileInfo *file, int heap, int mode, int num, DVDCallback cb, BOOL async)
{
    u32 len;
    void *buf;
    if(mode != 0 && mode != 1 && mode != 2) {
        OSReport("dvd.c: HuDvdDataReadWait Mode Error");
        buf = NULL;
        len = 0;
    }
    len = file->length;
    DirDataSize = len;
    if(mode == 1) {
        buf = HuMemDirectMallocNum(heap, OSRoundUp32B(len), num);
    } else {
        buf = HuMemDirectMalloc(heap, OSRoundUp32B(len));
    }
    if(!buf) {
        OSReport("dvd.c: Memory Allocation Error (Length %x) (mode %d)\n", len, mode);
        OSReport("Rest Memory %x\n", HuMemHeapSizeGet(3)-HuMemUsedMallocSizeGet(3));
        OSPanic("dvd.c", 75, "\n");
        return NULL;
    }
    
    DCInvalidateRange(buf, OSRoundUp32B(len));
    OSReport("Rest Memory %x\n", HuMemHeapSizeGet(3)-HuMemUsedMallocSizeGet(3));
    CallBackStatus = 0;
    DVDReadAsync(file, buf, OSRoundUp32B(len), 0, cb);
    if(!async) {
        while(!CallBackStatus) {
            HuDvdErrorWatch();
        }
        HuDvdErrorWatch();
    }
    
    return buf;
}

/**
 * @brief Read an entire DVD file by path into the DVD heap.
 *
 * @param [in] path Null-terminated absolute DVD path.
 *
 * @return Heap-owned file buffer; a missing file triggers OSPanic().
 */
void *HuDvdDataRead(char *path)
{
    DVDFileInfo file;
    void *data = NULL;
    if(!DVDOpen(path, &file)) {
        OSPanic("dvd.c", 146, "dvd.c: File Open Error");
    } else {
        data = HuDvdDataReadWait(&file, HEAP_DVD, 0, 0, HuDVDReadAsyncCallBack, FALSE);
        DVDClose(&file);
    }
    return data;
}

/**
 * @brief Read a sequence of complete DVD files into separate DVD-heap buffers.
 *
 * @param [in] paths NULL-terminated array of NUL-terminated DVD paths, read
 *                   sequentially in array order.
 *
 * @return Caller-owned main-heap array containing exactly one caller-owned
 *         HEAP_DVD file-buffer pointer per input path; the returned array itself
 *         is not NULL-terminated. Returns `NULL` if a DVD path cannot be opened
 *         after reporting a panic.
 */
void **HuDvdDataReadMulti(char **paths)
{
    DVDFileInfo file;
    int i;
    u32 count;
    void **file_ptrs;
    count = 0;
    while(paths[count]) {
        count++;
    }
    file_ptrs = HuMemDirectMalloc(0, count*sizeof(void *));
    for(i=0; i<count; i++) {
        if(!DVDOpen(paths[i], &file)) {
            OSPanic("dvd.c", 183, "dvd.c: File Open Error");
            return NULL;
        } else {
            file_ptrs[i] = HuDvdDataReadWait(&file, HEAP_DVD, 0, 0, HuDVDReadAsyncCallBack, FALSE);
            DVDClose(&file);
        }
    }
    return file_ptrs;
}

/**
 * @brief Read an entire DVD file by path into a selected heap.
 *
 * @param [in] path Null-terminated absolute DVD path.
 * @param [in] heap Heap that owns the full-file buffer returned to the caller.
 *
 * @return Heap-owned file buffer; a missing file triggers OSPanic().
 */
void *HuDvdDataReadDirect(char *path, HeapID heap)
{
    DVDFileInfo file;
    void *data = NULL;
    if(!DVDOpen(path, &file)) {
        OSPanic("dvd.c", 202, "dvd.c: File Open Error");
    } else {
        data = HuDvdDataReadWait(&file, heap, 2, 0, HuDVDReadAsyncCallBack, FALSE);
        DVDClose(&file);
    }
    return data;
}

/**
 * @brief Read an entire DVD file by file-system entry into the DVD heap.
 *
 * @param [in] entrynum DVD file-system entry returned by path conversion and
 *                      synchronously opened with DVDFastOpen().
 *
 * @return Heap-owned file buffer; an invalid entry triggers OSPanic().
 */
void *HuDvdDataFastRead(s32 entrynum)
{
    DVDFileInfo file;
    void *data = NULL;
    if(!DVDFastOpen(entrynum, &file)) {
        OSPanic("dvd.c", 243, "dvd.c: File Open Error");
    } else {
        data = HuDvdDataReadWait(&file, HEAP_DVD, 0, 0, HuDVDReadAsyncCallBack, FALSE);
        DVDClose(&file);
    }
    return data;
}

/**
 * @brief Read a DVD entry into a tagged DVD-heap allocation.
 *
 * @param [in] entrynum DVD file-system entry opened synchronously before its
 *                      contents receive allocation tag `num`.
 * @param [in] num Allocation tag used to release related buffers as a group.
 *
 * @return Tagged, heap-owned file buffer; an invalid entry triggers OSPanic().
 */
void *HuDvdDataFastReadNum(s32 entrynum, s32 num)
{
    DVDFileInfo file;
    void *data = NULL;
    if(!DVDFastOpen(entrynum, &file)) {
        (void)num;
        OSPanic("dvd.c", 258, "dvd.c: File Open Error");
    } else {
        data = HuDvdDataReadWait(&file, HEAP_DVD, 1, num, HuDVDReadAsyncCallBack, FALSE);
        DVDClose(&file);
    }
    return data;
}

/**
 * @brief Start an asynchronous read of a DVD entry into the DVD heap.
 *
 * @param [in] entrynum DVD file-system entry opened and queued for asynchronous read.
 * @param [in, out] stat Read record that retains the open DVDFileInfo until
 *                       HuDataDirReadAsyncCallBack() closes it.
 *
 * @return Heap-owned destination buffer immediately after the read is queued;
 *         an invalid entry triggers OSPanic().
 */
void *HuDvdDataFastReadAsync(s32 entrynum, HUDATASTAT *stat)
{
    DVDFileInfo file;
    void *data = NULL;
    if(!DVDFastOpen(entrynum, &stat->dvdFile)) {
        OSPanic("dvd.c", 274, "dvd.c: File Open Error");
    } else {
        data = HuDvdDataReadWait(&stat->dvdFile, HEAP_DVD, 0, 0, HuDataDirReadAsyncCallBack, TRUE);
    }
    return data;
}

/**
 * @brief Free a DVD data buffer when it is non-NULL.
 *
 * @param [in] ptr Buffer returned by a HuDvdData*Read function, or `NULL`.
 */
void HuDvdDataClose(void *ptr)
{
    if(ptr) {
        HuMemDirectFree(ptr);
    }
}

/**
 * @brief Report changes in the DVD drive error state.
 */
void HuDvdErrorWatch()
{
    int status = DVDGetDriveStatus();
    if(status == beforeDvdStatus) {
        return;
    }
    beforeDvdStatus = status;
    switch(status+1) {
        case 0:
            OSReport("DVD ERROR:Fatal error occurred\n***HALT***");
            while(1);
            break;
            
        case 5:
            OSReport("DVD ERROR:No disk\n");
            break;
            
        case 6:
            OSReport("DVD ERROR:Cover open\n");
            break;
            
        case 7:
            OSReport("DVD ERROR:Wrong disk\n");
            break;
            
        case 12:
            OSReport("DVD ERROR:Please retry\n");
            break;
            
        default:
            break;
    }
}
