#ifndef REL_M418DLL_H
#define REL_M418DLL_H

#include "dolphin.h"

#include "ext_math.h"
#include "game/hu3d.h"
#include "game/object.h"

typedef void (*M418DllFunc)(void);
typedef s32 (*M418DllRetFunc)(void);
typedef void (*M418DllInFunc)(s32);
typedef s32 (*M418DllInRetFunc)(s32);

typedef struct M418DllCameraState {
    M418DllFunc updateCallback;
    Vec center;
    Vec targetCenter; // sequence center
    Vec rot;
    Vec targetRotation; // sequence rot
    float zoom;
    float targetZoom; // sequence zoom
    float shakeOffsets[3];
    float interpolationFactor;
} M418DllCameraState; // sizeof 0x4C

typedef struct M418DllSequenceEntry {
    M418DllInFunc inputCallback;
    M418DllInRetFunc resultCallback;
    s32 duration;
} M418DllSequenceEntry; // sizeof 0x24

typedef struct M418DllSequenceState {
    s32 entryIndex;
    M418DllSequenceEntry *entries;
    s32 elapsedFrames;
    s32 entryDuration;
    M418DllFunc activeCallback;
    M418DllRetFunc activeResultCallback;
} M418DllSequenceState; // sizeof 0x18

float interpolateWeightedAverage(float, float, float);
float interpolateLinear(float, float, float, float);
float interpolateSineEaseInOut(float, float, float, float);
void setModelMotion(OMOBJ *, s32, s32, s32, s32);
void setModelMotionIfChanged(OMOBJ *, s32, s32, s32, s32);
void initializeSequence(M418DllSequenceState *, M418DllSequenceEntry *);
s32 advanceSequence(M418DllSequenceState *);
s32 setSequenceIndex(M418DllSequenceState *, s32);
s32 advanceSequenceTimer(M418DllSequenceState *, s32);
void setCameraTargetFromPositions(M418DllCameraState *, Vec, Vec);
void applyCameraTarget(M418DllCameraState *);
void resetCameraTarget(M418DllCameraState *);
void transitionCameraStateEaseOut(M418DllCameraState *, M418DllCameraState *, float, float, float);
void transitionCameraStateSine(M418DllCameraState *, M418DllCameraState *, float, float, float);
void transitionCameraStateCosine(M418DllCameraState *, M418DllCameraState *, float, float, float);

#endif
