#ifndef REL_MODELTEST_H
#define REL_MODELTEST_H

#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"

#include "ext_math.h"

// -------------------------------------------------------------------------- //

#define US_PER_60TH_SEC (1000000.0f / 60.0f)
#define US_TO_60TH_SEC(us) ((us) / US_PER_60TH_SEC)
#define FB_SIZE(width, height) width *height * 2

// -------------------------------------------------------------------------- //

typedef struct {
    s16 sequenceIds[7];
} TeropPatternSequence;

typedef struct {
    Vec pos; // 0000
    Vec rot; // 000C
    Vec scale; // 0018
    s32 reserved24;
    s32 reserved28;
    s32 reserved2C;
    HSFOBJECT *obj; // 0030
} ObjectTransformWork;

extern Vec CRot;
extern Vec Center;
extern float CZoom;
void ModelTestSceneInitialize(OMOBJ *object);
void WorldPositionToScreenPosition(Vec *worldPosition, Vec *screenPosition);
void MultiCameraModelTestUpdate(OMOBJ *object);
void MultiCameraViewUpdate(OMOBJ *object);
void MultiCameraCharacterModelsInitialize(void);

typedef void(ModelTestObjectFunction)(struct omObj_s *);
ModelTestObjectFunction ModelTestSceneUpdate;

#endif

// -------------------------------------------------------------------------- //
