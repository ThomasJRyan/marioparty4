#include "game/object.h"
#include "game/dvd.h"
#include "game/memory.h"

#ifndef __MWERKS__
#include <string.h>
#endif

typedef s32 (*DLLProlog)(void);
typedef void (*DLLEpilog)(void);

omDllData *omDLLinfoTbl[OM_DLL_MAX];

static FileListEntry *omDLLFileList;

/**
 * @brief Emit overlay module diagnostics when enabled.
 */
void omDLLDBGOut(void)
{
	OSReport("DLL DBG OUT\n");
}

/**
 * @brief Initialize the overlay module subsystem.
 *
 * @param [in] ovl_list Overlay-indexed REL file table retained by the loader.
 */
void omDLLInit(FileListEntry *ovl_list)
{
	s32 i;
	OSReport("DLL DBG OUT\n");
	for(i=0; i<OM_DLL_MAX; i++) {
		omDLLinfoTbl[i] = NULL;
	}
	omDLLFileList = ovl_list;
}

/**
 * @brief Start an overlay module, linking it if necessary.
 *
 * @param [in] overlay Overlay identifier.
 * @param [in] flag Nonzero forces a newly linked copy; zero reuses an existing
 *                  module after clearing BSS and rerunning its prolog.
 *
 * @return Slot in `omDLLinfoTbl`, or `-1` when all module slots are occupied.
 */
s32 omDLLStart(s16 overlay, s16 flag)
{
	s32 dllno;
	OSReport("DLLStart %d %d\n", overlay, flag);
	dllno = omDLLSearch(overlay);
	if(dllno >= 0 && !flag) {
		omDllData *dll = omDLLinfoTbl[dllno];
		OSReport("objdll>Already Loaded %s(%08x %08x)\n", dll->name, dll->module, dll->bss);
		
		omDLLInfoDump(&dll->module->info);
		omDLLHeaderDump(dll->module);
		memset(dll->bss, 0, dll->module->bssSize);
		HuMemDCFlushAll();
		dll->ret = ((DLLProlog)dll->module->prolog)();
		OSReport("objdll> %s prolog end\n", dll->name);
		return dllno;
	} else {
		for(dllno=0; dllno<OM_DLL_MAX; dllno++) {
			if(omDLLinfoTbl[dllno] == NULL) {
				break;
			}
		}
		if(dllno == OM_DLL_MAX) {
			return -1;
		}
		omDLLLink(&omDLLinfoTbl[dllno], overlay, TRUE);
		return dllno;
	}
}

/**
 * @brief End the loaded module associated with an overlay identifier.
 *
 * @param [in] overlay Overlay identifier.
 * @param [in] flag `1` unlinks the module; other values only run its epilog.
 */
void omDLLNumEnd(s16 overlay, s16 flag)
{
	s16 dllno;
	if(overlay < 0) {
		OSReport("objdll>omDLLNumEnd Invalid dllno %d\n", overlay);
		return;
	}
	OSReport("objdll>omDLLNumEnd %d %d\n", overlay, flag);
	dllno = omDLLSearch(overlay);
	if(dllno < 0) {
		OSReport("objdll>omDLLNumEnd not found DLL No%d\n", overlay);
		return;
	}
	omDLLEnd(dllno, flag);
}

/**
 * @brief End a module by loader-table slot.
 *
 * @param [in] dllno Linked-module index.
 * @param [in] flag `1` runs the epilog and unlinks the REL; other values leave
 *                  it linked after running the epilog.
 */
void omDLLEnd(s16 dllno, s16 flag)
{
	OSReport("objdll>omDLLEnd %d %d\n", dllno, flag);
	if(flag == 1) {
		OSReport("objdll>End DLL:%s\n", omDLLinfoTbl[dllno]->name);
		omDLLUnlink(omDLLinfoTbl[dllno], 1);
		omDLLinfoTbl[dllno] = NULL;
	} else {
		omDllData *dll;
		dll = omDLLinfoTbl[dllno];
		OSReport("objdll>Call Epilog\n");
		((DLLEpilog)dll->module->epilog)();
		OSReport("objdll>End DLL stayed:%s\n", omDLLinfoTbl[dllno]->name);
	}
	OSReport("objdll>End DLL finish\n");
}

/**
 * @brief Load, link, and optionally start an overlay REL module.
 *
 * @param [out] dll_ptr Receives the heap-owned loader record.
 * @param [in] overlay Overlay identifier.
 * @param [in] flag `1` invokes the module prolog after linking.
 *
 * @return Newly allocated loader record; load/link failures are reported by
 *         lower layers rather than represented by a documented sentinel.
 */
omDllData *omDLLLink(omDllData **dll_ptr, s16 overlay, s16 flag)
{
	omDllData *dll;
	FileListEntry *dllFile = &omDLLFileList[overlay];
	OSReport("objdll>Link DLL:%s\n", dllFile->name);
	dll = HuMemDirectMalloc(HEAP_HEAP, sizeof(omDllData));
	*dll_ptr = dll;
	dll->name = dllFile->name;
	dll->module = HuDvdDataReadDirect(dllFile->name, HEAP_HEAP);
	dll->bss = HuMemDirectMalloc(HEAP_HEAP, dll->module->bssSize);
	if(OSLink(&dll->module->info, dll->bss) != TRUE) {
		OSReport("objdll>++++++++++++++++ DLL Link Failed\n");
	}
	omDLLInfoDump(&dll->module->info);
	omDLLHeaderDump(dll->module);
	OSReport("objdll>LinkOK %08x %08x\n", dll->module, dll->bss);
	if(flag == 1) {
		OSReport("objdll> %s prolog start\n", dllFile->name);
		dll->ret = ((DLLProlog)dll->module->prolog)();
		OSReport("objdll> %s prolog end\n", dllFile->name);
	}
	return dll;
}

/**
 * @brief Optionally run a module epilog, unlink the REL, and free its storage.
 *
 * @param [in] dll_ptr Loader record created by omDLLLink(); invalid afterwards.
 * @param [in] flag `1` invokes the epilog before unlinking.
 */
void omDLLUnlink(omDllData *dll_ptr, s16 flag)
{
	OSReport("odjdll>Unlink DLL:%s\n", dll_ptr->name);
	if(flag == 1) {
		OSReport("objdll>Unlink DLL epilog\n");
		((DLLEpilog)dll_ptr->module->epilog)();
		OSReport("objdll>Unlink DLL epilog finish\n");
	}
	if(OSUnlink(&dll_ptr->module->info) != TRUE) {
		OSReport("objdll>+++++++++++++++++ DLL Unlink Failed\n");
	}
	HuMemDirectFree(dll_ptr->bss);
	HuMemDirectFree(dll_ptr->module);
	HuMemDirectFree(dll_ptr);
}

/**
 * @brief Find the loaded module whose file name matches an overlay.
 *
 * @param [in] overlay Overlay identifier.
 *
 * @return Loader-table slot, or `-1` when that overlay is not loaded.
 */
s32 omDLLSearch(s16 overlay)
{
	s32 i;
	FileListEntry *dllFile = &omDLLFileList[overlay];
	OSReport("Search:%s\n", dllFile->name);
	for(i=0; i<OM_DLL_MAX; i++) {
		omDllData *dll = omDLLinfoTbl[i];
		if(dll != NULL && strcmp(dll->name, dllFile->name) == 0) {
			OSReport("+++++++++++ Find%d: %s\n", i, dll->name);
			return i;
		}
	}
	return -1;
}

/**
 * @brief Print diagnostic information for a linked module.
 *
 * @param [in] module Linked REL module information to report.
 */
void omDLLInfoDump(OSModuleInfo *module)
{
	OSReport("===== DLL Module Info dump ====\n");
	OSReport("                   ID:0x%08x\n", module->id);
	OSReport("             LinkPrev:0x%08x\n", module->link.prev);
	OSReport("             LinkNext:0x%08x\n", module->link.next);
	OSReport("          Section num:%d\n", module->numSections);
	OSReport("Section info tbl ofst:0x%08x\n", module->sectionInfoOffset);
	OSReport("           nameOffset:0x%08x\n", module->nameOffset);
	OSReport("             nameSize:%d\n", module->nameSize);
	OSReport("              version:0x%08x\n", module->version);
	OSReport("===============================\n");
}

/**
 * @brief Print a module header for diagnostics.
 *
 * @param [in] module Linked REL module header to report.
 */
void omDLLHeaderDump(OSModuleHeader *module)
{
	OSReport("==== DLL Module Header dump ====\n");
	OSReport("          bss Size:0x%08x\n", module->bssSize);
	OSReport("        rel Offset:0x%08x\n", module->relOffset);
	OSReport("        imp Offset:0x%08x\n", module->impOffset);
	OSReport("    prolog Section:%d\n", module->prologSection);
	OSReport("    epilog Section:%d\n", module->epilogSection);
	OSReport("unresolved Section:%d\n", module->unresolvedSection);
	OSReport("       prolog func:0x%08x\n", module->prolog);
	OSReport("       epilog func:0x%08x\n", module->epilog);
	OSReport("   unresolved func:0x%08x\n", module->unresolved);
	OSReport("================================\n");
}
