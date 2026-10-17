#include "game/object.h"
#include "math.h"

void CheckNisInitialization(OMOBJ *object);

void ObjectSetup(void)
{
    HUPROCESS *process = omInitObjMan(0xA, 0x2000);
    OMOBJ *object = omAddObjEx(process, 0, 0, 0, -1, CheckNisInitialization);
    u32 *initializationStorage = object->work;
}

void CheckNisInitialization(OMOBJ *object)
{
    s8 *initializationFlag = (s8 *)object->work;
    if (*initializationFlag != 0) {
        (void)initializationFlag;
        return;
    }
}

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif
