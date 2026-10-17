#include "dolphin.h"
#include "game/object.h"
#include "REL/w03Dll.h"
#include "game/board/main.h"
#include "game/board/model.h"

typedef struct SmokeEffectWork {
    s16 smokeModelIds[5];
    char unreferencedSmokeData[2];
    f32 motionDuration;
} SmokeEffectWork;

extern W03BoardState* boardState;
OMOBJ* smokeEffectObjects[2];
extern HUPROCESS *boardObjMan;
extern s16 smokeAnchorModelId;

void SmokeEffectsUpdate(OMOBJ* object);

char* kemuriStrings[] = {
	"kemuri1", "kemuri2", "kemuri3",
	"kemuri4", "kemuri5"
};

void SmokeEffectsCreate(void) {
    f32 floatValue;
    s32 i;
    SmokeEffectWork* value;
    char* text;

    smokeEffectObjects[0] = omAddObjEx(boardObjMan, 0x101, 0U, 0U, -1, SmokeEffectsUpdate);
    value = (SmokeEffectWork*)&smokeEffectObjects[0]->work[0];
    for (i = 0; i < ARRAY_COUNT(value->smokeModelIds); i++) {
        value->smokeModelIds[i] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W03, 0x08), NULL, 0);
        text = kemuriStrings[i];

        if (0.0f == value->motionDuration) {
            value->motionDuration = BoardModelMotionMaxTimeGet(value->smokeModelIds[i]);
        }

        floatValue = value->motionDuration * BoardRandFloat();

        if (floatValue < 1.0f) {
            floatValue = 1.0f;
        }

        BoardModelHookSet(smokeAnchorModelId, text, value->smokeModelIds[i]);
        BoardModelMotionStart(value->smokeModelIds[i], 0, 0x40000001);
        BoardModelMotionTimeSet(value->smokeModelIds[i], floatValue);
        BoardModelLayerSet(value->smokeModelIds[i], 2);
        BoardModelPassSet(value->smokeModelIds[i], 0);
    }
}

void SmokeEffectsUpdate(OMOBJ* object) {
    s32 stateValue;
    s32 i;
    SmokeEffectWork* value;

    value = (SmokeEffectWork*)&object->work[0];

    if (BoardIsKill() != 0) {
        for (i = 0; i < 5; i++) {
            BoardModelKill(value->smokeModelIds[i]);
        }
        smokeEffectObjects[0] = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }

    if (boardState->eventActive != 0) {
        stateValue = 0;
    } else {
        stateValue = 1;
    }

    for (i = 0; i < ARRAY_COUNT(value->smokeModelIds); i++) {
        BoardModelVisibilitySet(value->smokeModelIds[i], stateValue);
    }
}
