#include "game/process.h"
#include "dolphin/os.h"
#include "game/memory.h"

#ifdef __MWERKS__
#include "game/jmp.h"
#endif

#define FAKE_RETADDR 0xA5A5A5A5

#define EXEC_NORMAL 0
#define EXEC_SLEEP 1
#define EXEC_CHILDWATCH 2
#define EXEC_KILLED 3

static jmp_buf processjmpbuf;
static HUPROCESS *processtop;
static HUPROCESS *processcur;
static u16 processcnt;
u32 procfunc;

/**
 * @brief Initialize the process subsystem.
 */
void HuPrcInit(void)
{
    processcnt = 0;
    processtop = NULL;
}

/**
 * @brief Insert a process into a priority-ordered process list.
 *
 * @param [in, out] root Address of the process-list root.
 * @param [in, out] process Process inserted according to descending priority.
 */
static void LinkProcess(HUPROCESS** root, HUPROCESS* process) {
    HUPROCESS* src_process = *root;

    if (src_process && (src_process->prio >= process->prio)) {
        while (src_process->next && src_process->next->prio >= process->prio) {
            src_process = src_process->next;
        }

        process->next = src_process->next;
        process->prev = src_process;
        src_process->next = process;
        if (process->next) {
            process->next->prev = process;
        }
    } else {
        process->next = (*root);
        process->prev = NULL;
        *root = process;
        if (src_process) {
            src_process->prev = process;
        }
    }
}
/**
 * @brief Remove a process from a process list.
 *
 * @param [in, out] root Address of the process-list root.
 * @param [in, out] process Linked process to detach; neighbor links are updated.
 */
static void UnlinkProcess(HUPROCESS **root, HUPROCESS *process) {
    if (process->next) {
        process->next->prev = process->prev;
    }
    if (process->prev) {
        process->prev->next = process->next;
    } else {
        *root = process->next;
    }
}

/**
 * @brief Allocate and register a cooperatively scheduled process.
 *
 * @param [in] func Entry point installed as the process's initial link register.
 * @param [in] prio Scheduling priority; higher values run earlier.
 * @param [in] stack_size Stack size in bytes; zero selects 2048 bytes.
 * @param [in] extra_size Additional bytes reserved in the private process heap.
 *
 * @return Newly allocated process linked into the scheduler, or `NULL` when
 *         its backing heap cannot be allocated.
 */
HUPROCESS *HuPrcCreate(void (*func)(void), u16 prio, u32 stack_size, s32 extra_size)
{
    HUPROCESS *process;
    s32 alloc_size;
    void *heap;
    if(stack_size == 0) {
        stack_size = 2048;
    }
    alloc_size = HuMemMemoryAllocSizeGet(sizeof(HUPROCESS))
                    +HuMemMemoryAllocSizeGet(stack_size)
                    +HuMemMemoryAllocSizeGet(extra_size);
    if(!(heap = HuMemDirectMalloc(HEAP_HEAP, alloc_size))) {
        OSReport("process> malloc error size %d\n", alloc_size);
        return NULL;
    }
    HuMemHeapInit(heap, alloc_size);
    process = HuMemMemoryAlloc(heap, sizeof(HUPROCESS), FAKE_RETADDR);
    process->heap = heap;
    process->exec = EXEC_NORMAL;
    process->stat = 0;
    process->prio = prio;
    process->sleep_time = 0;
    process->base_sp = ((u32)HuMemMemoryAlloc(heap, stack_size, FAKE_RETADDR))+stack_size-8;
    gcsetjmp(&process->jump);
    process->jump.lr = (u32)func;
    process->jump.sp = process->base_sp;
    process->dtor = NULL;
    process->user_data = NULL;
    LinkProcess(&processtop, process);
    process->child = NULL;
    process->parent = NULL;
    processcnt++;
    return process;
}

/**
 * @brief Attach a process as the newest child of a parent process.
 *
 * @param [in, out] parent Process whose child list receives `child`.
 * @param [in, out] child Process detached from any old parent before linking.
 */
void HuPrcChildLink(HUPROCESS *parent, HUPROCESS *child)
{
    HuPrcChildUnlink(child);
    if(parent->child) {
        parent->child->first_child = child;
    }
    child->next_child = parent->child;
    child->first_child = NULL;
    parent->child = child;
    child->parent = parent;
}

/**
 * @brief Detach a process from its parent and sibling chain.
 *
 * @param [in, out] process Child process to detach; a root process is unchanged.
 */
void HuPrcChildUnlink(HUPROCESS *process)
{
    if(process->parent) {
        if(process->next_child) {
            process->next_child->first_child = process->first_child;
        }
        if(process->first_child) {
            process->first_child->next_child = process->next_child;
        } else {
            process->parent->child = process->next_child;
        }
        process->parent = NULL;
    }
}

/**
 * @brief Create a process and attach it to a parent.
 *
 * @param [in] func Child entry point.
 * @param [in] prio Scheduling priority; higher values run earlier.
 * @param [in] stack_size Stack size in bytes; zero selects 2048 bytes.
 * @param [in] extra_size Additional bytes reserved in the child's private heap.
 * @param [in, out] parent Process that owns the new child.
 *
 * @return Newly allocated and linked child process, or `NULL` if allocation
 *         fails. Callers are expected to provide a valid parent.
 */
HUPROCESS *HuPrcChildCreate(void (*func)(void), u16 prio, u32 stack_size, s32 extra_size, HUPROCESS *parent)
{
    HUPROCESS *child = HuPrcCreate(func, prio, stack_size, extra_size);
    HuPrcChildLink(parent, child);
    return child;
}

/**
 * @brief Suspend the current process until all children terminate.
 */
void HuPrcChildWatch()
{
    HUPROCESS *curr = HuPrcCurrentGet();
    if(curr->child) {
        curr->exec = EXEC_CHILDWATCH;
        if(!gcsetjmp(&curr->jump)) {
            gclongjmp(&processjmpbuf, 1);
        }
    }
}

/**
 * @brief Get the process currently executing in the scheduler.
 *
 * @return Current process pointer, or `NULL` while no process is selected.
 */
HUPROCESS *HuPrcCurrentGet()
{
    return processcur;
}

/**
 * @brief Mark a process for termination if it is still active.
 *
 * @param [in, out] process Process whose sleep is cancelled before marking it killed.
 *
 * @return `0` when newly marked killed, or `-1` if already killed.
 */
static s32 SetKillStatusProcess(HUPROCESS *process)
{
    if(process->exec != EXEC_KILLED) {
        HuPrcWakeup(process);
        process->exec = EXEC_KILLED;
        return 0;
    } else {
        return -1;
    }
}

/**
 * @brief Mark a process and all descendants for cooperative termination.
 *
 * @param [in, out] process Process to kill, or `NULL` for the current process.
 *
 * @return `0` when the process was newly marked killed, or `-1` if it was
 *         already pending termination.
 */
s32 HuPrcKill(HUPROCESS *process)
{
    if(process == NULL) {
        process = HuPrcCurrentGet();
    }
    HuPrcChildKill(process);
    HuPrcChildUnlink(process);
    return SetKillStatusProcess(process);
}

/**
 * @brief Recursively mark all descendants of a process for termination.
 *
 * @param [in, out] process Parent whose child tree is killed and detached.
 */
void HuPrcChildKill(HUPROCESS *process)
{
    HUPROCESS *child = process->child;
    while(child) {
        if(child->child) {
            HuPrcChildKill(child);
        }
        SetKillStatusProcess(child);
        child = child->next_child;
    }
    process->child = NULL;
}

/**
 * @brief Run a process destructor, unlink it, and return to the scheduler.
 *
 * @param [in, out] process Current process being permanently removed.
 */
static void gcTerminateProcess(HUPROCESS *process)
{
    if(process->dtor) {
        process->dtor();
    }
    UnlinkProcess(&processtop, process);
    processcnt--;
    gclongjmp(&processjmpbuf, 2);
}

/**
 * @brief Terminate the current process and all of its children.
 */
void HuPrcEnd()
{
    HUPROCESS *process = HuPrcCurrentGet();
    HuPrcChildKill(process);
    HuPrcChildUnlink(process);
    gcTerminateProcess(process);
}

/**
 * @brief Yield the current process, optionally sleeping for scheduler ticks.
 *
 * @param [in] time Scheduler ticks to sleep; zero yields without entering the
 *                  timed-sleep state.
 */
void HuPrcSleep(s32 time)
{
    HUPROCESS *process = HuPrcCurrentGet();
    if(time != 0 && process->exec != EXEC_KILLED) {
        process->exec = EXEC_SLEEP;
        process->sleep_time = time;
    }
    if(!gcsetjmp(&process->jump)) {
        gclongjmp(&processjmpbuf, 1);
    }
}

/**
 * @brief Yield the current process until the next scheduler pass.
 */
void HuPrcVSleep()
{
    HuPrcSleep(0);
}

/**
 * @brief Clear a process's remaining sleep time.
 *
 * @param [in, out] process Process whose sleep counter is reset to zero.
 */
void HuPrcWakeup(HUPROCESS *process)
{
    process->sleep_time = 0;
}

/**
 * @brief Set or clear the destructor of an explicitly supplied process.
 *
 * @param [in, out] process Process whose `dtor` field is assigned directly;
 *                          unlike HuPrcDestructorSet(), the current process is
 *                          not selected implicitly.
 * @param [in] func Destructor invoked when `process` terminates, or `NULL` to
 *                  clear the process's destructor.
 */
void HuPrcDestructorSet2(HUPROCESS *process, void (*func)(void))
{
    process->dtor = func;
}

/**
 * @brief Set the destructor for the current process.
 *
 * @param [in] func Callback invoked when the current process terminates, or
 *                  `NULL` to clear it.
 */
void HuPrcDestructorSet(void (*func)(void))
{
    HUPROCESS *process = HuPrcCurrentGet();
    process->dtor = func;
}

/**
 * @brief Run one scheduler pass for all game processes.
 *
 * @param [in] tick Elapsed ticks subtracted from timed sleeps during this pass.
 */
void HuPrcCall(s32 tick)
{
    HUPROCESS *process;
    s32 ret;
    processcur = processtop;
    ret = gcsetjmp(&processjmpbuf);
    while(1) {
        switch(ret) {
            case 2:
                HuMemDirectFree(processcur->heap);
            case 1:
                if(((u8 *)(processcur->heap))[4] != 165) {
                    printf("stack overlap error.(process pointer %x)\n", processcur);
                    while(1);
                } else {
                    processcur = processcur->next;
                }
                break;
        }
        process = processcur;
        if(!process) {
            return;
        }
        procfunc = process->jump.lr;
        if((process->stat & (PROCESS_STAT_PAUSE|PROCESS_STAT_UPAUSE)) && process->exec != EXEC_KILLED) {
            ret = 1;
            continue;
        }
        switch(process->exec) {
            case EXEC_SLEEP:
                if(process->sleep_time > 0) {
                    process->sleep_time -= tick;
                    if(process->sleep_time <= 0) {
                        process->sleep_time = 0;
                        process->exec = EXEC_NORMAL;
                    }
                }
                ret = 1;
                break;
                
            case EXEC_CHILDWATCH:
                if(process->child) {
                    ret = 1;
                } else {
                    process->exec = EXEC_NORMAL;
                    ret = 0;
                }
                break;
                
            case EXEC_KILLED:
                process->jump.lr = (u32)HuPrcEnd;
            case EXEC_NORMAL:
                gclongjmp(&process->jump, 1);
                break;
        }
    }
}

/**
 * @brief Allocate memory from the current process's private heap.
 *
 * @param [in] size Size in bytes.
 *
 * @return Process-owned allocation, or `NULL` when its heap lacks space.
 */
void *HuPrcMemAlloc(s32 size)
{
    HUPROCESS *process = HuPrcCurrentGet();
    return HuMemMemoryAlloc(process->heap, size, FAKE_RETADDR);
}

/**
 * @brief Free an allocation from a process heap.
 *
 * @param [in] ptr Pointer previously returned by a process-heap allocator.
 */
void HuPrcMemFree(void *ptr)
{
    HuMemMemoryFree(ptr, FAKE_RETADDR);
}

/**
 * @brief Enable status bits on a process.
 *
 * @param [in, out] process Process whose status word is modified.
 * @param [in] value Bit mask to OR into the status word.
 */
void HuPrcSetStat(HUPROCESS *process, u16 value)
{
    process->stat |= value;
}

/**
 * @brief Clear status bits on a process.
 *
 * @param [in, out] process Process whose status word is modified.
 * @param [in] value Bit mask to clear from the status word.
 */
void HuPrcResetStat(HUPROCESS *process, u16 value)
{
    process->stat &= ~value;
}

/**
 * @brief Apply or clear the normal-pause bit across scheduled processes.
 *
 * @param [in] flag Nonzero pauses processes lacking PROCESS_STAT_PAUSE_EN;
 *                  zero clears PROCESS_STAT_PAUSE from all processes.
 */
void HuPrcAllPause(s32 flag)
{
    HUPROCESS *process = processtop;
    if(flag) {
        while(process != NULL) {
            if(!(process->stat & PROCESS_STAT_PAUSE_EN)) {
                HuPrcSetStat(process, PROCESS_STAT_PAUSE);
            }
            
            process = process->next;
        }
    } else {
        while(process != NULL) {
            if(process->stat & PROCESS_STAT_PAUSE) {
                HuPrcResetStat(process, PROCESS_STAT_PAUSE);
            }
            
            process = process->next;
        }
    }
}

/**
 * @brief Apply or clear the alternate-pause bit across scheduled processes.
 *
 * @param [in] flag Nonzero pauses processes lacking PROCESS_STAT_UPAUSE_EN;
 *                  zero clears PROCESS_STAT_UPAUSE from all processes.
 */
void HuPrcAllUPause(s32 flag)
{
    HUPROCESS *process = processtop;
    if(flag) {
        while(process != NULL) {
            if(!(process->stat & PROCESS_STAT_UPAUSE_EN)) {
                HuPrcSetStat(process, PROCESS_STAT_UPAUSE);
            }
            
            process = process->next;
        }
    } else {
        while(process != NULL) {
            if(process->stat & PROCESS_STAT_UPAUSE) {
                HuPrcResetStat(process, PROCESS_STAT_UPAUSE);
            }
            
            process = process->next;
        }
    }
}
