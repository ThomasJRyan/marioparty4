#include "dolphin.h"
#include "game/memory.h"
#include "game/card.h"
#include "version.h"

static void *cardWork;
u8 UnMountCnt;

static void MountCallBack(s32 chan, s32 result);

/**
 * @brief Initialize the memory card subsystem.
 */
void HuCardInit(void)
{
	CARDInit();
	cardWork = HuMemDirectMalloc(HEAP_HEAP, CARD_WORKAREA_SIZE);
}

/**
 * @brief Probe both memory card slots and mount cards that are ready.
 *
 * @return Bit 0 set when slot A is ready and bit 1 set when slot B is ready.
 */
s32 HuCardCheck(void)
{
	s32 ret = 0;
	s32 memSize, sectorSize;
	s32 result;
	do {
		result = CARDProbeEx(0, &memSize, &sectorSize);
	} while(result == CARD_RESULT_BUSY);
	if(result == CARD_RESULT_READY) {
		ret |= 1;
		OSReport("SlotA Card MemSize %x,Sector Size %x\n", memSize, sectorSize);
		HuCardMount(0);
	}
	do {
		result = CARDProbeEx(1, &memSize, &sectorSize);
	} while(result == CARD_RESULT_BUSY);
	if(result == CARD_RESULT_READY) {
		ret |= 2;
		OSReport("SlotB Card MemSize %x,Sector Size %x\n", memSize, sectorSize);
		HuCardMount(1);
	}
	return ret;
}

/**
 * @brief Probe one memory card slot until the probe is no longer busy.
 *
 * @param [in] slot CARD channel `0` (slot A) or `1` (slot B) to probe.
 *
 * @return The card sector size in bytes, or a negative CARD_RESULT_* error.
 */
s32 HuCardSlotCheck(s16 slot)
{
	s32 memSize, sectorSize;
	s32 result;
	do {
		result = CARDProbeEx(slot, &memSize, &sectorSize);
	} while(result == CARD_RESULT_BUSY);
	if(result < 0) {
		return result;
	} else {
		return sectorSize;
	}
}

/**
 * @brief Mount and validate the memory card in a slot.
 *
 * @param [in] slot CARD channel `0` (slot A) or `1` (slot B) to mount.
 *
 * @return `CARD_RESULT_READY` on success, otherwise a CARD_RESULT_* error;
 *         fatal I/O and encoding errors are normalized to fatal or broken.
 */
s32 HuCardMount(s16 slot)
{
	s32 memSize, sectorSize;
	s32 result;
	do {
		result = CARDProbeEx(slot, &memSize, &sectorSize);
	} while(result == CARD_RESULT_BUSY);
	if(result < 0) {
		return result;
	} else {
		result = CARDMount(slot, cardWork, MountCallBack);
		if(result == CARD_RESULT_FATAL_ERROR || result == CARD_RESULT_IOERROR) {
			return CARD_RESULT_FATAL_ERROR;
		}
		if(result == CARD_RESULT_ENCODING) {
			return CARD_RESULT_BROKEN;
		}
		if(result < 0 && result != CARD_RESULT_BROKEN) {
			return result;
		}
		result = CARDCheck(slot);
		if(result == CARD_RESULT_FATAL_ERROR || result == CARD_RESULT_IOERROR) {
			return CARD_RESULT_FATAL_ERROR;
		}
		if(result == CARD_RESULT_ENCODING) {
			return CARD_RESULT_BROKEN;
		}
		return result;
	}
}

/**
 * @brief Unmount the memory card in a slot.
 *
 * @param [in] slot CARD channel `0` (slot A) or `1` (slot B) to unmount.
 */
void HuCardUnMount(s16 slot)
{
	CARDUnmount(slot);
}

/**
 * @brief Format the memory card in a slot.
 *
 * @param [in] slot CARD channel `0` (slot A) or `1` (slot B) to format.
 *
 * @return The CARDFormat result, with fatal I/O errors normalized to
 *         `CARD_RESULT_FATAL_ERROR`.
 */
s32 HuCardFormat(s16 slot)
{
	s32 result = CARDFormat(slot);
	if(result == CARD_RESULT_FATAL_ERROR || result == CARD_RESULT_IOERROR) {
		return CARD_RESULT_FATAL_ERROR;
	}
	return result;
}

/**
 * @brief Record that a mounted memory card channel was removed.
 *
 * @param [in] chan Card channel whose mount callback ran (`0` or `1`).
 * @param [in] result Mount result supplied by the CARD library; not inspected.
 */
static void MountCallBack(s32 chan, s32 result)
{
	#if VERSION_ENG
	UnMountCnt = (1 << chan);
	#else
	UnMountCnt |= (1 << chan);
	#endif
}

/**
 * @brief Open a named file on a memory card.
 *
 * @param [in] slot CARD channel `0` (slot A) or `1` (slot B) containing the file.
 * @param [in] fileName NUL-terminated card file name passed to CARDOpen().
 * @param [out] fileInfo Receives the opened file handle on success.
 *
 * @return A CARD_RESULT_* status, with fatal I/O and encoding errors
 *         normalized to fatal or broken results.
 */
s32 HuCardOpen(s16 slot, const char *fileName, CARDFileInfo *fileInfo)
{
	s32 result = CARDOpen(slot, fileName, fileInfo);
	if(result == CARD_RESULT_FATAL_ERROR || result == CARD_RESULT_IOERROR) {
		return CARD_RESULT_FATAL_ERROR;
	}
	if(result == CARD_RESULT_ENCODING) {
		return CARD_RESULT_BROKEN;
	}
	return result;
}

/**
 * @brief Read bytes from an open memory card file.
 *
 * @param [in] fileInfo Open file handle returned by HuCardOpen().
 * @param [out] addr Destination buffer for `length` bytes.
 * @param [in] length Number of bytes to transfer.
 * @param [in] offset Byte offset in the file.
 *
 * @return A CARD_RESULT_* status, with fatal I/O errors normalized to
 *         `CARD_RESULT_FATAL_ERROR`.
 */
s32 HuCardRead(CARDFileInfo *fileInfo, void *addr, s32 length, s32 offset)
{
	s32 result = CARDRead(fileInfo, addr, length, offset);
	if(result == CARD_RESULT_FATAL_ERROR || result == CARD_RESULT_IOERROR) {
		return CARD_RESULT_FATAL_ERROR;
	}
	return result;
}

/**
 * @brief Create a sector-aligned file when the card has enough free space.
 *
 * @param [in] slot CARD channel `0` (slot A) or `1` (slot B) receiving the file.
 * @param [in] fileName NUL-terminated name assigned to the new card file.
 * @param [in] size Requested file size in bytes; rounded to a sector boundary.
 * @param [out] fileInfo Receives the created file handle on success.
 *
 * @return The CARDCreate result, `CARD_RESULT_INSSPACE` when bytes or directory
 *         entries are insufficient, or an earlier CARD_RESULT_* error.
 */
s32 HuCardCreate(s16 slot, const char *fileName, s32 size, CARDFileInfo *fileInfo)
{
	s32 sectorSize;
	s32 byteNotUsed, filesNotUsed;
	s32 result;
	result = CARDGetSectorSize(slot, (u32 *)&sectorSize);
	if(result == CARD_RESULT_FATAL_ERROR || result == CARD_RESULT_NOCARD) {
		return result;
	}
	if(size % sectorSize) {
		size = sectorSize*(size+((sectorSize-1)/sectorSize));
	}
	result = CARDFreeBlocks(slot, &byteNotUsed, &filesNotUsed);
	if(result != 0) {
		return result;
	}
	if(filesNotUsed <= 0 || byteNotUsed < size) {
		return CARD_RESULT_INSSPACE;
	}
	result = CARDCreate(slot, fileName, size, fileInfo);
	return result;
}

/**
 * @brief Close an open memory card file.
 *
 * @param [in, out] fileInfo Open file handle to close.
 *
 * @return The CARD_RESULT_* value returned by CARDClose().
 */
s32 HuCardClose(CARDFileInfo *fileInfo)
{
	s32 result = CARDClose(fileInfo);
	return result;
}

/**
 * @brief Write bytes to an open memory card file.
 *
 * @param [in, out] fileInfo Open file handle to write.
 * @param [in] addr Source buffer containing `length` bytes.
 * @param [in] length Number of bytes to transfer.
 * @param [in] offset Byte offset in the file.
 *
 * @return The CARD_RESULT_* value returned by CARDWrite().
 */
s32 HuCardWrite(CARDFileInfo *fileInfo, const void *addr, s32 length, s32 offset)
{
	s32 result = CARDWrite(fileInfo, addr, length, offset);
	return result;
}

/**
 * @brief Delete a named file from a memory card.
 *
 * @param [in] slot CARD channel `0` (slot A) or `1` (slot B) containing the file.
 * @param [in] fileName NUL-terminated name of the card file passed to CARDDelete().
 *
 * @return The CARD_RESULT_* value returned by CARDDelete().
 */
s32 HuCardDelete(s16 slot, const char *fileName)
{
	s32 result = CARDDelete(slot, fileName);
	return result;
}

/**
 * @brief Get the sector size of the card in a slot.
 *
 * @param [in] slot CARD channel `0` (slot A) or `1` (slot B) queried for sector size.
 *
 * @return The sector size in bytes, or a negative CARD_RESULT_* error.
 */
s32 HuCardSectorSizeGet(s16 slot)
{
	u32 sectorSize;
	s32 result = CARDGetSectorSize(slot, &sectorSize);
	if(result < 0) {
		return result;
	} else {
		return sectorSize;
	}
}

/**
 * @brief Query free bytes and directory entries on a memory card.
 *
 * @param [in] slot CARD channel `0` (slot A) or `1` (slot B) queried for capacity.
 * @param [out] byteNotUsed Receives the number of free bytes.
 * @param [out] filesNotUsed Receives the number of available file entries.
 *
 * @return The CARD_RESULT_* value returned by CARDFreeBlocks().
 */
s32 HuCardFreeSpaceGet(s16 slot, u32 *byteNotUsed, u32 *filesNotUsed)
{
	s32 result = CARDFreeBlocks(slot, (s32 *)byteNotUsed, (s32 *)filesNotUsed);
	return result;
}
