#include "game/disp.h"
#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"

#include "game/init.h"

#include "game/esprite.h"

#include "game/audio.h"
#include "game/chrman.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/minigame_seq.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "ext_math.h"

#include "REL/m439data.h"
#include "version.h"
#include <string.h>

#ifndef __MWERKS__
#include "game/ClusterExec.h"
#include "game/EnvelopeExec.h"
#include "game/ShapeExec.h"
#endif

typedef struct CameraViewPreset {
    float zoom;
    Vec pos;
    Vec rot;
} CameraViewParams;

#if VERSION_NTSC
CameraViewParams cameraViewPresets[] = {
    { 4775.0f, -2800.0f, 0.0f, -7700.0f, 343.0f, 20.0f, 0.0f },
    { 5000.0f, 0.f, 0.0f, 0.0f, 290.0f, 0.0f, 0.0f },
    { 2800.0f, 0.f, 0.0f, -7600.0f, 347.0f, 0.0f, 0.0f },
    { 2800.0f, 0.f, 0.0f, -7600.0f, 290.0f, 0.0f, 0.0f },
};
#else
CameraViewParams cameraViewPresets[] = {
    { 4500.0f, -2800.0f, 0.0f, -7700.0f, 343.0f, 20.0f, 0.0f },
    { 5000.0f, 0.f, 0.0f, 0.0f, 290.0f, 0.0f, 0.0f },
    { 2800.0f, 0.f, 0.0f, -7600.0f, 347.0f, 0.0f, 0.0f },
    { 2800.0f, 0.f, 0.0f, -7600.0f, 290.0f, 0.0f, 0.0f },
};
#endif


typedef struct CourseRouteConnections {
    s32 modelIndex;
    s32 controllerPort;
    s32 buttonFlags;
    s32 characterId[2];
} CourseRouteConnections;

CourseRouteConnections courseRouteTable[16] = { { 0, -1, 1, { 1, 0 } }, { 1, 0, 2, { 2, 3 } }, { 2, 1, 1, { 4, 0 } }, { 2, 1, 1, { 5, 0 } },
    { 3, 2, 2, { 6, 7 } }, { 3, 3, 2, { 8, 9 } }, { 4, 4, 1, { 10, 0 } }, { 4, 4, 1, { 11, 0 } }, { 4, 5, 1, { 12, 0 } }, { 4, 5, 1, { 13, 0 } },
    { 5, 6, 0, { 0, 0 } }, { 5, 7, 0, { 0, 0 } }, { 5, 8, 0, { 0, 0 } }, { 5, 9, 0, { 0, 0 } } };

float courseSectionDepths[5] = { -700, -2200, -3600, -5200, -6600 };
s32 playerCameraMasks[4] = { 1, 2, 4, 8 };
Vec shadowLightPosition = { 700, 2500, -700 };
Vec worldOrigin = { 0, 0, 0 };
GXColor shadowLightColor = { 255, 255, 255, 255 };
Vec shadowCameraPosition = { 700, 2500, -700 };
Vec shadowCameraUp = { 0, 1, 0 };

OMOBJ *introSequenceObject;
OMOBJ *introSequenceObject;
OMOBJ *gameCameraObject;
u32 courseObjectCount;
u32 minigameState;
s32 finisherPlayerIndices[4];
s32 introStateTimer;
s32 initializedPlayerCount;
s32 initializedCourseModelCount;
s32 initializedCatcherCount;
s16 finishCoinAward;
s16 finisherCount;
s16 finisherIndices[4];
s16 finishCount;
s16 countdownValue;
s16 countdownTimer;
HUPROCESS *objectManager;
void *shadowBuffers[3];
Vec savedShadowCameraPositions[3];
Vec savedShadowCameraTargets[3];
Mtx savedShadowMatrices[3];
ANIMDATA *playerParticleAnimation;
Vec lightDirection;
s16 minigameSequenceId;
s16 countdownSequenceId;
s16 courseModelIds[16];
CourseRouteConnections *currentRouteConnections;
CourseRouteConnections *playerRouteConnections;
s16 reservedGlobalPadding[4];

void InitializeIntroSequence(OMOBJ *object);
void InitializePlayerObjects(HUPROCESS *objman);
void InitializeCatcherObjects(HUPROCESS *objman);
void InitializeCourseObjects(HUPROCESS *objman);
void InitializeShadows(HUPROCESS *objman);

void InitializeGameCamera(OMOBJ *object);

void ObjectSetup(void)
{
    HUPROCESS *objman;
    s16 light;

    HU3DCAMERA *camera = &Hu3DCamera[0];
    OSReport("******* M439ObjectSetup *********\n");
    objman = omInitObjMan(50, 8192);
    omGameSysInit(objman);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 10, 100, 15000, 1.2);
    Hu3DCameraViewportSet(1, 0, 0, 640, 480, 0, 1);
    gameCameraObject = omAddObjEx(objman, 32730, 0, 0, -1, omOutViewMulti);
    gameCameraObject->work[0] = 1;
    lightDirection.x = worldOrigin.x - shadowLightPosition.x;
    lightDirection.y = worldOrigin.y - shadowLightPosition.y;
    lightDirection.z = worldOrigin.z - shadowLightPosition.z;
    VECNormalize(&lightDirection, &lightDirection);
    light = Hu3DGLightCreateV(&shadowLightPosition, &lightDirection, &shadowLightColor);
    Hu3DGLightInfinitytSet(light);
    Hu3DGLightStaticSet(light, 1);
    Hu3DShadowCreate(20, 200, 10000);
    Hu3DShadowTPLvlSet(0.625f);
    Hu3DShadowPosSet(&shadowCameraPosition, &shadowCameraUp, &worldOrigin);
    HuAudSndGrpSet(65);
    introSequenceObject = omAddObjEx(objman, 0, 0, 0, -1, InitializeIntroSequence);
    introSequenceObject = omAddObjEx(objman, 10, 0, 0, -1, InitializeGameCamera);
    InitializePlayerObjects(objman);
    InitializeCatcherObjects(objman);
    InitializeCourseObjects(objman);
    InitializeShadows(objman);
}

void SetMinigameState(s32 state)
{
    minigameState = state;
}

s32 GetMinigameState(void)
{
    return minigameState;
}

void UpdateIntroSequence(OMOBJ *object);

void InitializeIntroSequence(OMOBJ *object)
{
    SetMinigameState(0);
    introStateTimer = REFRESH_RATE;
    countdownValue = 60;
    countdownTimer = REFRESH_RATE;
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    object->objFunc = UpdateIntroSequence;
}

s32 countdownSequenceId = -1;
s32 introSoundHandle = -1;

void StartGameplay(OMOBJ *object);

s32 AllPlayersFinished(void);
void AwardFinishCoins(void);

void UpdateIntroSequence(OMOBJ *object)
{
    switch (GetMinigameState()) {
        case 0:
            if (introStateTimer == REFRESH_RATE) {
                HuAudFXPlay(1797);
            }
            if (--introStateTimer == 0) {
                SetMinigameState(1);
                introStateTimer = 2 * REFRESH_RATE;
            }
            break;

        case 1:
            if (--introStateTimer == 0) {
                WipeColorSet(0, 0, 0);
                SetMinigameState(2);
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 30);
            }
            break;

        case 2:
            if (!WipeStatGet()) {
                SetMinigameState(3);
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 30);
                introSoundHandle = HuAudFXPlay(1798);
            }
            break;

        case 3:
            if (!WipeStatGet()) {
                SetMinigameState(4);
                minigameSequenceId = MGSeqStartCreate();
            }
            break;

        case 4:
            if (!MGSeqStatGet(minigameSequenceId)) {
                SetMinigameState(5);
                countdownSequenceId = MGSeqTimerCreate(60);
            }
            if (countdownSequenceId < 0 && (MGSeqStatGet(minigameSequenceId) & 0x10)) {
                countdownSequenceId = HuAudSeqPlay(75);
            }
            break;

        case 5:
            if (--countdownTimer == 0) {
                countdownValue--;
                MGSeqParamSet(countdownSequenceId, 1, countdownValue);
                countdownTimer = REFRESH_RATE;
            }
            if (finishCount >= 3 || countdownValue == 0) {
                SetMinigameState(6);
                minigameSequenceId = MGSeqFinishCreate();
                HuAudSeqFadeOut(countdownSequenceId, 100);
            }
            break;

        case 6:
            if (countdownSequenceId >= 0) {
                MGSeqParamSet(countdownSequenceId, 2, -1);
                countdownSequenceId = -1;
            }
            if (!MGSeqStatGet(minigameSequenceId)) {
                SetMinigameState(7);
                introStateTimer = REFRESH_RATE;
            }
            break;

        case 7:
            if (AllPlayersFinished() && --introStateTimer == 0) {
                AwardFinishCoins();
                SetMinigameState(9);
                if (finishCount) {
                    HuAudSStreamPlay(1);
                }
                else {
                    HuAudSStreamPlay(4);
                }
                introStateTimer = 3.5f * REFRESH_RATE;
            }
            break;

        case 9:
            if (--introStateTimer == 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                WipeColorSet(255, 255, 255);
                HuAudFXStop(introSoundHandle);
                object->objFunc = StartGameplay;
            }
            break;
    }
    if (finisherCount) {
        s32 i;
        for (i = 0; i < finisherCount; i++) {
            GWPlayerCoinWinSet(finisherIndices[i], finishCoinAward);
        }
        finishCoinAward += finisherCount;
        finishCount += finisherCount;
        finisherCount = 0;
    }
    if (omSysExitReq && !WipeStatGet()) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        WipeColorSet(255, 255, 255);
        object->objFunc = StartGameplay;
    }
}

void UpdatePlayerObjects(void);
void UpdateCourseObjects(void);

void StartGameplay(OMOBJ *object)
{
    if (!WipeStatGet()) {
        UpdatePlayerObjects();
        UpdateCourseObjects();
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
        OSReport("******* M439Exit *********\n");
    }
}

void InitializePlayer(OMOBJ *object);

void InitializePlayerObjects(HUPROCESS *objman)
{
    s32 i;
    finisherPlayerIndices[0] = finisherPlayerIndices[1] = finisherPlayerIndices[2] = finisherPlayerIndices[3] = -1;
    initializedPlayerCount = 0;
    finishCoinAward = 0;
    finisherCount = 0;
    finishCount = 0;
    omMakeGroupEx(objman, 0, 4);
    for (i = 0; i < 4; i++) {
        omAddObjEx(objman, 5, 3, 50, 0, InitializePlayer);
    }
    playerParticleAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M439, 0x16), HU_MEMNUM_OVL));
}

void UpdatePlayerObject(OMOBJ *object);

void UpdatePlayerObjects(void)
{
    OMOBJ **objList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    s32 i;
    for (i = 0; i < 4; i++) {
        UpdatePlayerObject(objList[i]);
    }
}

typedef struct PlayerWork {
    u16 reachedFinish : 1;
    u16 beingCarried : 1;
    u16 collisionPending : 1;
    u16 visibleAtStart : 1;
    u16 routeChanged : 1;
    u16 justCaught : 1;
    u16 avoidingCatch : 1;
    u16 finished : 1;
    u16 firstFinisher : 1;
    s16 playerIndex;
    s16 controllerPort;
    s8 stickX;
    s8 stickY;
    u16 buttonFlags;
    u16 buttonDownFlags;
    s16 characterId;
    s16 difficulty;
    s16 playerGroup;
    s16 movementState;
    Vec movementParams;
    Vec movementVelocity;
    s32 motionIndex;
    u32 motionAttributes;
    float motionSpeed;
    float motionBlend;
    s16 movementTimer;
    s16 stateTimer;
    float laneDepth;
    s16 roadSection;
    s16 previousRoadSection;
    float transitionStartDepth;
    float transitionEndDepth;
    Vec catchStartPosition;
    Vec catchPosition;
    Vec releasePosition;
    Vec targetPosition;
    u16 roadId;
    Vec roadOrigin;
    float roadDistance;
    float targetLaneBlend;
    Vec roadTargetPosition;
    float targetLaneBlendGoal;
    float reservedWorkValue;
} PlayerWork;

s32 AllPlayersFinished(void)
{
    OMOBJ **objList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    s32 i;
    for (i = 0; i < 4; i++) {
        PlayerWork *work = objList[i]->data;
        if (!work->finished) {
            return 0;
        }
    }
    return 1;
}

void AwardFinishCoins(void)
{
    OMOBJ **objList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    s32 i;
    if (finishCoinAward == 0) {
        for (i = 0; i < 4; i++) {
            GWPlayerCoinWinSet(i, 3);
        }
    }
    else {
        for (i = 0; i < 4; i++) {
            PlayerWork *work = objList[i]->data;
            if (!work->reachedFinish) {
                GWPlayerCoinWinSet(i, finishCoinAward);
            }
        }
    }
}

s32 playerMotionFiles[7] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x14), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x50), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x72) };

void UpdatePlayerParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

void UpdatePlayerIntroModel(OMOBJ *object);

void InitializePlayer(OMOBJ *object)
{
    PlayerWork *work = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerWork), HU_MEMNUM_OVL);
    u32 i;
    s32 charNo;
    object->data = work;
    memset(work, 0, sizeof(PlayerWork));
    work->playerIndex = initializedPlayerCount++;
    work->controllerPort = GWPlayerCfg[work->playerIndex].pad_idx;
    charNo = GWPlayerCfg[work->playerIndex].character;
    work->characterId = charNo;
    work->difficulty = GWPlayerCfg[work->playerIndex].diff;
    work->playerGroup = GWPlayerCfg[work->playerIndex].group;
    object->mdlId[0] = CharModelCreate(charNo, 8);
    if (work->playerIndex == 0) {
        Hu3DModelShadowSet(object->mdlId[0]);
    }
    Hu3DModelCameraSet(object->mdlId[0], playerCameraMasks[work->playerIndex]);
    work->motionIndex = 0;
    work->motionAttributes = HU3D_MOTATTR_LOOP;
    for (i = 0; i < 7; i++) {
        object->mtnId[i] = CharMotionCreate(work->characterId, playerMotionFiles[i]);
    }
    Hu3DMotionSet(object->mdlId[0], object->mtnId[0]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    CharMotionVoiceOnSet(work->characterId, object->mtnId[4], 0);
    CharModelDataClose(work->characterId);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M439, 0x12));
    Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
    Hu3DModelCameraSet(object->mdlId[1], playerCameraMasks[work->playerIndex]);
    work->visibleAtStart = 1;
    work->roadSection = 0;
    work->previousRoadSection = 0;
    work->laneDepth = 1000;
    work->movementParams.x = 0;
    work->targetLaneBlend = work->targetLaneBlendGoal = 0.5f;
    object->rot.y = 180;
    object->mdlId[2] = Hu3DParticleCreate(playerParticleAnimation, 10);
    Hu3DParticleHookSet(object->mdlId[2], UpdatePlayerParticle);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    Hu3DModelCameraSet(object->mdlId[2], playerCameraMasks[work->playerIndex]);
    object->objFunc = UpdatePlayerIntroModel;
}

float ApproachAngle(float currentAngle, float targetAngle, float approachFactor)
{
    float angleDifference = fmod(targetAngle - currentAngle, 360);
    float interpolatedAngle;
    if (angleDifference < 0.0f) {
        angleDifference += 360.0f;
    }
    if (angleDifference > 180.0f) {
        angleDifference -= 360.0f;
    }
    interpolatedAngle = fmod(currentAngle + (approachFactor * angleDifference), 360.0);
    if (interpolatedAngle < 0.0f) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

float innerCourseBoundary[11][2] = {
    { -180.0f, -230.0f },
    { -260.0f, -150.0f },
    { -300.0f, 0.0f },
    { -260.0f, 150.0f },
    { -150.0f, 260.0f },
    { -0.0f, 300.0f },
    { 150.0f, 260.0f },
    { 260.0f, 150.0f },
    { 300.0f, -0.0f },
    { 260.0f, -150.0f },
    { 180.0f, -230.0f },
};

s32 ConstrainToInnerCourse(Vec *position)
{
    float edgeVector[2];
    float pointOffset[2];
    float closestEdgePoint[2];
    float pointDistance;

    u32 i;
    for (i = 0; i < 10; i++) {
        s32 nextVertexIndex = i + 1;

        float edgeProjection;
        edgeVector[0] = innerCourseBoundary[nextVertexIndex][0] - innerCourseBoundary[i][0];
        edgeVector[1] = innerCourseBoundary[nextVertexIndex][1] - innerCourseBoundary[i][1];
        edgeProjection = (((position->z * edgeVector[1]) + ((position->x * edgeVector[0]) - (edgeVector[0] * innerCourseBoundary[i][0]))) - (edgeVector[1] * innerCourseBoundary[i][1]))
            / ((edgeVector[0] * edgeVector[0]) + (edgeVector[1] * edgeVector[1]));
        if (edgeProjection >= 0.0f && edgeProjection < 1.0f) {
            closestEdgePoint[0] = innerCourseBoundary[i][0] + (edgeVector[0] * edgeProjection);
            closestEdgePoint[1] = innerCourseBoundary[i][1] + (edgeVector[1] * edgeProjection);
            pointOffset[0] = position->x - closestEdgePoint[0];
            pointOffset[1] = position->z - closestEdgePoint[1];
            pointDistance = sqrtf((pointOffset[0] * pointOffset[0]) + (pointOffset[1] * pointOffset[1]));
            if (pointDistance < 130.0f) {
                if (pointDistance != 0.0f) {
                    pointOffset[0] /= pointDistance;
                    pointOffset[1] /= pointDistance;
                }
                else {
                    pointOffset[0] = 0;
                    pointOffset[1] = 1;
                }
                position->x = closestEdgePoint[0] + (130.0f * pointOffset[0]);
                position->z = closestEdgePoint[1] + (130.0f * pointOffset[1]);
            }
        }
    }
    for (i = 0; i < 11; i++) {
        edgeVector[0] = position->x - innerCourseBoundary[i][0];
        edgeVector[1] = position->z - innerCourseBoundary[i][1];
        pointDistance = sqrtf((edgeVector[0] * edgeVector[0]) + (edgeVector[1] * edgeVector[1]));
        if (pointDistance < 130.0f) {
            if (pointDistance != 0.0f) {
                edgeVector[0] /= pointDistance;
                edgeVector[1] /= pointDistance;
            }
            else {
                edgeVector[0] = 0;
                edgeVector[1] = 1;
            }
            position->x = innerCourseBoundary[i][0] + (130.0f * edgeVector[0]);
            position->z = innerCourseBoundary[i][1] + (130.0f * edgeVector[1]);
        }
    }
}

float outerCourseBoundary[3][2][2] = { -99.8, -1431.8, 77.2, -1437.8, -1499.8, -4571.8, -1322.8, -4577.8, 1300.2, -4571.8, 1477.2, -4577.8 };

s32 ConstrainToOuterCourse(Vec *position)
{
    s32 i;
    for (i = 0; i < 3; i++) {
        float edgeVector[2];
        float pointOffset[2];
        float closestEdgePoint[2];
        float pointDistance;
        float edgeProjection;
        s32 j;
        edgeVector[0] = outerCourseBoundary[i][1][0] - outerCourseBoundary[i][0][0];
        edgeVector[1] = outerCourseBoundary[i][1][1] - outerCourseBoundary[i][0][1];
        edgeProjection = (((position->z * edgeVector[1]) + ((position->x * edgeVector[0]) - (edgeVector[0] * outerCourseBoundary[i][0][0]))) - (edgeVector[1] * outerCourseBoundary[i][0][1]))
            / ((edgeVector[0] * edgeVector[0]) + (edgeVector[1] * edgeVector[1]));
        if (edgeProjection >= 0.0f && edgeProjection < 1.0f) {
            closestEdgePoint[0] = outerCourseBoundary[i][0][0] + (edgeVector[0] * edgeProjection);
            closestEdgePoint[1] = outerCourseBoundary[i][0][1] + (edgeVector[1] * edgeProjection);
            pointOffset[0] = position->x - closestEdgePoint[0];
            pointOffset[1] = position->z - closestEdgePoint[1];
            pointDistance = sqrtf((pointOffset[0] * pointOffset[0]) + (pointOffset[1] * pointOffset[1]));
            if (pointDistance < 50.0f) {
                if (pointDistance != 0.0f) {
                    pointOffset[0] /= pointDistance;
                    pointOffset[1] /= pointDistance;
                }
                else {
                    pointOffset[0] = 0;
                    pointOffset[1] = 1;
                }
                position->x = closestEdgePoint[0] + (50.0f * pointOffset[0]);
                position->z = closestEdgePoint[1] + (50.0f * pointOffset[1]);
            }
        }
        for (j = 0; j < 2; j++) {
            edgeVector[0] = position->x - outerCourseBoundary[i][j][0];
            edgeVector[1] = position->z - outerCourseBoundary[i][j][1];
            pointDistance = sqrtf((edgeVector[0] * edgeVector[0]) + (edgeVector[1] * edgeVector[1]));
            if (pointDistance < 50.0f) {
                if (pointDistance != 0.0f) {
                    edgeVector[0] /= pointDistance;
                    edgeVector[1] /= pointDistance;
                }
                else {
                    edgeVector[0] = 0;
                    edgeVector[1] = 1;
                }
                position->x = outerCourseBoundary[i][j][0] + (50.0f * edgeVector[0]);
                position->z = outerCourseBoundary[i][j][1] + (50.0f * edgeVector[1]);
            }
        }
    }
}

float GetCoursePathDistance(s32 roadId, Vec *roadOrigin, Vec *worldPosition)
{
    Vec *leftRailPoints;
    Vec *rightRailPoints;
    s32 i;
    Vec segmentVector;
    Vec pathOffset;
    s32 segmentIndex = -1;
    float minimumDistance = 1000;
    float *pathDistanceTable = courseDistanceTablesByRoad[roadId];
    VECSubtract(worldPosition, roadOrigin, &pathOffset);
    leftRailPoints = courseRailPointsByRoad[roadId][0];
    rightRailPoints = courseRailPointsByRoad[roadId][1];
    for (i = 0; i < courseRailPointCounts[roadId] - 1; i++, leftRailPoints++, rightRailPoints++) {
        float segmentLengthSquared;
        float averageProgress;
        float leftRailProgress;
        float rightRailProgress;
        VECSubtract(&leftRailPoints[1], &leftRailPoints[0], &segmentVector);
        segmentLengthSquared = VECMag2Point(&segmentVector);
        if (segmentLengthSquared == 0.0f) {
            continue;
        }
        leftRailProgress
            = ((pathOffset.x * segmentVector.x) - (leftRailPoints->x * segmentVector.x) + (pathOffset.y * segmentVector.y) - (leftRailPoints->y * segmentVector.y) + (pathOffset.z * segmentVector.z) - (leftRailPoints->z * segmentVector.z))
            / segmentLengthSquared;
        VECSubtract(&rightRailPoints[1], &rightRailPoints[0], &segmentVector);
        segmentLengthSquared = VECMag2Point(&segmentVector);
        if (segmentLengthSquared == 0.0f) {
            continue;
        }
        rightRailProgress
            = ((pathOffset.x * segmentVector.x) - (rightRailPoints->x * segmentVector.x) + (pathOffset.y * segmentVector.y) - (rightRailPoints->y * segmentVector.y) + (pathOffset.z * segmentVector.z) - (rightRailPoints->z * segmentVector.z))
            / segmentLengthSquared;
        averageProgress = 0.5f * (leftRailProgress + rightRailProgress);
        if (averageProgress < 0.0f || averageProgress >= 1.0f) {
            continue;
        }
        else {
            return pathDistanceTable[i] + (averageProgress * (pathDistanceTable[i + 1] - pathDistanceTable[i]));
        }
    }
    return -1;
}

void GetCoursePathPosition(s32 roadId, Vec *roadOrigin, float distanceAlongRoad, float laneBlend, Vec *targetPosition)
{
    float *pathDistanceTable = courseDistanceTablesByRoad[roadId];
    s32 i;
    for (i = 0; i < courseRailPointCounts[roadId] - 1; i++) {
        if (distanceAlongRoad < pathDistanceTable[i + 1]) {
            break;
        }
    }
    if (i < courseRailPointCounts[roadId] - 1) {
        float segmentProgress = (distanceAlongRoad - pathDistanceTable[i]) / (pathDistanceTable[i + 1] - pathDistanceTable[i]);
        Vec *leftRailPoints = &courseRailPointsByRoad[roadId][0][i];
        Vec *rightRailPoints = &courseRailPointsByRoad[roadId][1][i];
        Vec leftRailPosition;
        Vec rightRailPosition;
        Vec segmentVector;
        VECSubtract(&leftRailPoints[1], &leftRailPoints[0], &segmentVector);
        leftRailPosition.x = leftRailPoints->x + (segmentProgress * segmentVector.x);
        leftRailPosition.y = leftRailPoints->y + (segmentProgress * segmentVector.y);
        leftRailPosition.z = leftRailPoints->z + (segmentProgress * segmentVector.z);
        VECSubtract(&rightRailPoints[1], &rightRailPoints[0], &segmentVector);
        rightRailPosition.x = rightRailPoints->x + (segmentProgress * segmentVector.x);
        rightRailPosition.y = rightRailPoints->y + (segmentProgress * segmentVector.y);
        rightRailPosition.z = rightRailPoints->z + (segmentProgress * segmentVector.z);
        targetPosition->x = (roadOrigin->x + leftRailPosition.x) + (laneBlend * (rightRailPosition.x - leftRailPosition.x));
        targetPosition->y = (roadOrigin->y + leftRailPosition.y) + (laneBlend * (rightRailPosition.y - leftRailPosition.y));
        targetPosition->z = (roadOrigin->z + leftRailPosition.z) + (laneBlend * (rightRailPosition.z - leftRailPosition.z));
    }
}

float GetCoursePathCurvature(s32 roadId, float distanceAlongRoad)
{
    float *pathDistanceTable = courseDistanceTablesByRoad[roadId];
    s32 i;
    for (i = 0; i < courseRailPointCounts[roadId] - 1; i++) {
        if (distanceAlongRoad < pathDistanceTable[i + 1]) {
            break;
        }
    }
    if (i >= courseRailPointCounts[roadId] - 1) {
        return -1;
    }
    else {
        return pathDistanceTable[i + 1] - pathDistanceTable[i];
    }
}

s16 courseRoadModelMap[10][3] = { 1, 0, 0, 2, 1, 2, 2, 3, 4, 1, 5, 0, 1, 6, 0, 1, 7, 0, 1, 8, 0, 1, 9, 0, 1, 10, 0, 1, 11, 0 };

s32 catcherMotionFiles[8] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x01),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x14),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x50),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18),
};

typedef struct CourseModelWork {
    s16 modelIndex;
    Vec worldPosition;
    CourseRouteConnections routeConnections;
} CourseModelWork;

void UpdatePlayerCourseTarget(OMOBJ *object)
{
    float desiredSpeed;
    PlayerWork *playerWork;
    CourseModelWork *roadObjects;
    Vec targetDirection;
    s32 targetModelIndex;
    OMOBJ **roadObjectList;
    playerWork = object->data;
    if (playerWork->routeChanged) {
        playerWork->routeChanged = 0;
        roadObjectList = omGetGroupMemberListEx(HuPrcCurrentGet(), 1);
        roadObjects = roadObjectList[playerWork->playerIndex]->data;
        targetModelIndex = roadObjects[playerWork->roadSection].modelIndex;
        playerWork->roadId = courseRoadModelMap[targetModelIndex][((courseRoadModelMap[targetModelIndex][0] * frand8()) >> 8) + 1];
        playerWork->roadOrigin = roadObjects[playerWork->roadSection].worldPosition;
        playerWork->roadDistance = GetCoursePathDistance(playerWork->roadId, &playerWork->roadOrigin, &object->trans);
        if (playerWork->roadDistance < 0.0f) {
            playerWork->roadDistance = 105.0f;
        }
        playerWork->roadDistance += 20.0f;
    }
    else {
        if (playerWork->justCaught) {
            playerWork->targetLaneBlend = playerWork->targetLaneBlendGoal = 0.5f;
            playerWork->justCaught = 0;
            playerWork->avoidingCatch = 1;
        }
        else {
            VECSubtract(&playerWork->roadTargetPosition, &object->trans, &targetDirection);
            if (VECMag(&targetDirection) < 20.0f) {
                playerWork->roadDistance += 20.0f;
                playerWork->avoidingCatch = 0;
            }
        }
    }
    if (object->trans.z < -500.0f && !playerWork->avoidingCatch && frand8() == 0) {
        playerWork->targetLaneBlendGoal = (0.55f * ((((s32)frand() & 0x1FF) - 256.0f) / 256.0f)) + 0.5f;
    }
    playerWork->targetLaneBlend += 0.05f * (playerWork->targetLaneBlendGoal - playerWork->targetLaneBlend);
    GetCoursePathPosition(playerWork->roadId, &playerWork->roadOrigin, playerWork->roadDistance, playerWork->targetLaneBlend, &playerWork->roadTargetPosition);
    if (playerWork->roadTargetPosition.z > playerWork->laneDepth) {
        playerWork->roadTargetPosition.z = playerWork->laneDepth;
    }
    VECSubtract(&playerWork->roadTargetPosition, &object->trans, &targetDirection);
    VECNormalize(&targetDirection, &targetDirection);
    desiredSpeed = 10.0 + (((18.0 + (6.0f * playerWork->difficulty)) * GetCoursePathCurvature(playerWork->roadId, playerWork->roadDistance)) / 200.0);
    if (desiredSpeed > 72) {
        desiredSpeed = 72;
    }
    playerWork->stickX = targetDirection.x * desiredSpeed;
    playerWork->stickY = -targetDirection.z * desiredSpeed;
}

void UpdatePlayerMovement(OMOBJ *object);

s32 GetCourseSectionFromDepth(float worldDepth);

void UpdatePlayerIntroModel(OMOBJ *object)
{
    PlayerWork *work = object->data;
    Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
    Hu3DModelPosSet(object->mdlId[1], CenterM[work->playerIndex].x, CenterM[work->playerIndex].y, CenterM[work->playerIndex].z);
    if (GetMinigameState() == 3) {
        Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
        object->objFunc = UpdatePlayerMovement;
    }
}

typedef struct CatcherWork {
    u8 isPlayerCaptured : 1;
    s16 playerIndex;
    s16 eventState;
    float motionBlend[2];
    float fallSpeed;
    float eventTimers[4];
    s16 eventTimer;
    s16 eventDuration;
    Vec startPosition;
    Vec approachPosition;
    Vec launchVelocity;
} CatcherWork;

void UpdatePlayerFinish(OMOBJ *object);
s32 GetNextCourseSection(OMOBJ *object);
s32 FindCatchPositions(Vec *playerPosition, s32 playerIndex, s32 roadSectionIndex, Vec *catchPosition, Vec *targetPosition);

void UpdatePlayerMovement(OMOBJ *object)
{
    PlayerWork *playerWork;
    s32 roadSectionIndex;
    s32 motionIndex;
    OMOBJ *targetObject;
    u32 motionAttributes;
    OMOBJ **objectList;
    CatcherWork *eventWork;
    HU3DPARTICLE *particleData;
    float inputDirectionX;
    float inputDirectionZ;
    float eventProgress;
    float inputMagnitude;
    float motionSpeed;
    float movementSpeed;

    Vec cameraCenter;
    playerWork = object->data;
    motionIndex = playerWork->motionIndex;
    motionSpeed = playerWork->motionSpeed;
    motionAttributes = playerWork->motionAttributes;
    if (GetMinigameState() >= 6 && !playerWork->collisionPending) {
        playerWork->finished = 1;
        playerWork->movementState = 2;
        playerWork->stateTimer = 2 * REFRESH_RATE;
        object->objFunc = UpdatePlayerFinish;
        return;
    }
    if (GetMinigameState() < 5) {
        return;
    }
    switch (playerWork->movementState) {
        case 0:
            if (GWPlayerCfg[playerWork->playerIndex].iscom == 0) {
                playerWork->stickX = HuPadStkX[playerWork->controllerPort];
                playerWork->stickY = HuPadStkY[playerWork->controllerPort];
                playerWork->buttonFlags = HuPadBtn[playerWork->controllerPort];
                playerWork->buttonDownFlags = HuPadBtnDown[playerWork->controllerPort];
            }
            else {
                playerWork->buttonFlags = playerWork->buttonDownFlags = 0;
                playerWork->stickX = playerWork->stickY = 0;
                UpdatePlayerCourseTarget(object);
            }
            inputDirectionX = playerWork->stickX;
            inputDirectionZ = -(float)playerWork->stickY;
            inputMagnitude = VECMagPoint(inputDirectionX, inputDirectionZ, 0);
            if (inputMagnitude > 0.0f) {
                inputDirectionX /= inputMagnitude;
                inputDirectionZ /= inputMagnitude;
                if (inputMagnitude > 72) {
                    inputMagnitude = 72;
                }
                movementSpeed = ((600.0f / REFRESH_RATE) * inputMagnitude) / 72.0f;
                playerWork->movementVelocity.x = inputDirectionX * movementSpeed;
                playerWork->movementVelocity.z = inputDirectionZ * movementSpeed;
                playerWork->movementParams.y = movementSpeed;
                object->rot.y = ApproachAngle(object->rot.y, atan2d(playerWork->movementVelocity.x, playerWork->movementVelocity.z), 0.2f);
                if (playerWork->movementParams.y < (450.0f / REFRESH_RATE)) {
                    motionIndex = 1;
                    motionSpeed = playerWork->movementParams.y / (450.0f / REFRESH_RATE);
                }
                else {
                    motionIndex = 2;
                    motionSpeed = playerWork->movementParams.y / (600.0f / REFRESH_RATE);
                }
            }
            else {
                playerWork->movementVelocity.x = playerWork->movementVelocity.z = 0;
                motionIndex = 0;
                motionSpeed = 1;
            }
            VECAdd(&object->trans, &playerWork->movementVelocity, &object->trans);
            ConstrainToInnerCourse(&object->trans);
            ConstrainToOuterCourse(&object->trans);
            if (object->trans.z > playerWork->laneDepth) {
                object->trans.z = playerWork->laneDepth;
            }
            else {
                if (object->trans.z < -7300) {
                    playerWork->reachedFinish = 1;
                    playerWork->finished = 1;
                    playerWork->movementState = 0;
                    if (finishCount == 0) {
                        playerWork->firstFinisher = 1;
                    }
                    finisherIndices[finisherCount++] = playerWork->playerIndex;
                    object->objFunc = UpdatePlayerFinish;
                    return;
                }
            }
            if (FindCatchPositions(&object->trans, playerWork->playerIndex, playerWork->roadSection, &playerWork->catchPosition, &playerWork->targetPosition) && !playerWork->avoidingCatch) {
                playerWork->collisionPending = 1;
                playerWork->beingCarried = 1;
                playerWork->justCaught = 1;
                playerWork->visibleAtStart = 0;
                playerWork->catchStartPosition.x = object->trans.x;
                playerWork->catchStartPosition.y = object->trans.y;
                playerWork->catchStartPosition.z = object->trans.z;
                playerWork->releasePosition.y = -2000;
                playerWork->movementState = 2;
                playerWork->stateTimer = 0.2f * REFRESH_RATE;
                particleData = Hu3DData[object->mdlId[2]].hookData;
                particleData->dataCnt = 0;
                VECSubtract(&playerWork->catchPosition, &object->trans, &particleData->pos);
                Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
                Hu3DModelPosSet(object->mdlId[2], object->trans.x, object->trans.y, object->trans.z);
                objectList = omGetGroupMemberListEx(HuPrcCurrentGet(), 2);
                objectList[playerWork->playerIndex]->work[3] = 1;
            }
            else {
                cameraCenter.x = CenterM[playerWork->playerIndex].x;
                cameraCenter.z = CenterM[playerWork->playerIndex].z;
                cameraCenter.x += 0.1f * (object->trans.x - cameraCenter.x);
                cameraCenter.z += 0.1f * (object->trans.z - cameraCenter.z);
                roadSectionIndex = GetCourseSectionFromDepth(cameraCenter.z - 350.0f);
                if (roadSectionIndex != playerWork->previousRoadSection && roadSectionIndex > 1 && roadSectionIndex < 6) {
                    cameraCenter.z = courseSectionDepths[roadSectionIndex - 1] + 350.0f;
                }
                else {
                    roadSectionIndex = GetCourseSectionFromDepth(cameraCenter.z + 350.0f);
                    if (roadSectionIndex != playerWork->previousRoadSection && playerWork->previousRoadSection > 1 && playerWork->previousRoadSection < 6) {
                        cameraCenter.z = courseSectionDepths[playerWork->previousRoadSection - 1] - 350.0f;
                    }
                }
                CenterM[playerWork->playerIndex].x = cameraCenter.x;
                CenterM[playerWork->playerIndex].z = cameraCenter.z;
                roadSectionIndex = GetCourseSectionFromDepth(object->trans.z);
                if (roadSectionIndex != playerWork->previousRoadSection) {
                    playerWork->previousRoadSection = roadSectionIndex;
                    playerWork->roadSection = GetNextCourseSection(object);
                    playerWork->routeChanged = 01;
                    if (roadSectionIndex > 1 && roadSectionIndex < 6) {
                        playerWork->movementState++;
                        playerWork->stateTimer = 0.5f * REFRESH_RATE;
                        playerWork->transitionStartDepth = CenterM[playerWork->playerIndex].z;
                        playerWork->transitionEndDepth = object->trans.z - 350.0f;
                        playerWork->laneDepth = courseSectionDepths[playerWork->previousRoadSection - 1];
                    }
                }
            }
            break;

        case 1:
            if (--playerWork->stateTimer == 0) {
                playerWork->movementState = 0;
            }
            CenterM[playerWork->playerIndex].z = playerWork->transitionStartDepth + ((playerWork->transitionEndDepth - playerWork->transitionStartDepth) * cosd((playerWork->stateTimer * 90.0f) / (0.5f * REFRESH_RATE)));
            motionIndex = 0;
            motionSpeed = 1;
            break;

        case 2:
            objectList = omGetGroupMemberListEx(HuPrcCurrentGet(), 2);
            targetObject = objectList[playerWork->playerIndex];
            eventWork = targetObject->data;
            switch (eventWork->eventState) {
                case 1:
                    eventProgress = 1.0f - (eventWork->eventTimer / (1.5f * REFRESH_RATE));
                    object->trans.x = playerWork->catchStartPosition.x + (eventProgress * (playerWork->catchPosition.x - playerWork->catchStartPosition.x));
                    object->trans.y = 100 + (-2000.0f * (eventProgress - 0.1f)) * (eventProgress - 0.1f);
                    object->trans.z = playerWork->catchStartPosition.z + (eventProgress * (playerWork->catchPosition.z - playerWork->catchStartPosition.z));
                    motionIndex = 3;
                    motionAttributes = HU3D_MOTATTR_NONE;
                    break;

                case 2:
                case 3:
                    if (eventWork->isPlayerCaptured) {
                        object->trans.x = targetObject->trans.x;
                        object->trans.y = targetObject->trans.y - 150;
                        object->trans.z = targetObject->trans.z;
                        motionIndex = 4;
                        motionAttributes = HU3D_MOTATTR_NONE;
                    }
                    else {
                        if (playerWork->stateTimer) {
                            playerWork->stateTimer--;
                        }
                        eventProgress = sind((playerWork->stateTimer / (0.5f * REFRESH_RATE)) * 90.0f);
                        object->trans.x = playerWork->targetPosition.x - (eventProgress * (playerWork->targetPosition.x - playerWork->releasePosition.x));
                        object->trans.y = playerWork->targetPosition.y - (eventProgress * (playerWork->targetPosition.y - playerWork->releasePosition.y));
                        object->trans.z = playerWork->targetPosition.z - (eventProgress * (playerWork->targetPosition.z - playerWork->releasePosition.z));
                        motionIndex = 0;
                        motionAttributes = HU3D_MOTATTR_LOOP;
                    }
                    break;
            }
            if (!playerWork->beingCarried) {
                playerWork->collisionPending = 0;
                playerWork->movementState = 0;
                playerWork->movementVelocity.y = 0;
            }
            break;
    }
    if ((motionIndex >= 0 && motionIndex != playerWork->motionIndex) || motionAttributes != playerWork->motionAttributes) {
        playerWork->motionIndex = motionIndex;
        playerWork->motionAttributes = motionAttributes;
        CharMotionShiftSet(playerWork->characterId, object->mtnId[playerWork->motionIndex], 0, 8, motionAttributes);
    }
    CharMotionSpeedSet(playerWork->characterId, motionSpeed);
    playerWork->motionSpeed = motionSpeed;
    if (playerWork->playerIndex == 0) {
        Hu3DShadowData.camTarget.x = object->trans.x;
        Hu3DShadowData.camTarget.y = object->trans.y;
        Hu3DShadowData.camTarget.z = object->trans.z;
        Hu3DShadowData.camPos.x = 700 + Hu3DShadowData.camTarget.x;
        Hu3DShadowData.camPos.y = 2500 + Hu3DShadowData.camTarget.y;
        Hu3DShadowData.camPos.z = Hu3DShadowData.camTarget.z - 700;
    }
    else {
        savedShadowCameraTargets[playerWork->playerIndex - 1].x = object->trans.x;
        savedShadowCameraTargets[playerWork->playerIndex - 1].y = object->trans.y;
        savedShadowCameraTargets[playerWork->playerIndex - 1].z = object->trans.z;
        savedShadowCameraPositions[playerWork->playerIndex - 1].x = 700 + savedShadowCameraTargets[playerWork->playerIndex - 1].x;
        savedShadowCameraPositions[playerWork->playerIndex - 1].y = 2500 + savedShadowCameraTargets[playerWork->playerIndex - 1].y;
        savedShadowCameraPositions[playerWork->playerIndex - 1].z = savedShadowCameraTargets[playerWork->playerIndex - 1].z - 700;
    }
}

void UpdatePlayerFinish(OMOBJ *object)
{
    PlayerWork *playerWork;
    s32 motionIndex;
    u32 motionAttributes;
    float motionSpeed;
    playerWork = object->data;
    motionIndex = playerWork->motionIndex;
    motionSpeed = playerWork->motionSpeed;
    motionAttributes = playerWork->motionAttributes;
    switch (playerWork->movementState) {
        case 0:
            if (object->trans.z > -7600.0f) {
                object->trans.z -= 225.0f / REFRESH_RATE;
            }
            else {
                playerWork->movementState++;
            }
            object->rot.y = ApproachAngle(object->rot.y, 180.0f, 0.2f);
            motionIndex = 1;
            motionSpeed = 1.0f;
            motionAttributes = HU3D_MOTATTR_LOOP;
            break;

        case 1:
            if (GetMinigameState() >= 9) {
                motionIndex = (playerWork->firstFinisher) ? 5 : 6;
                motionSpeed = 1.0f;
                motionAttributes = HU3D_MOTATTR_NONE;
            }
            else {
                object->rot.y = ApproachAngle(object->rot.y, 0.0f, 0.2f);
                motionIndex = 0;
                motionSpeed = 1.0f;
                motionAttributes = HU3D_MOTATTR_LOOP;
            }
            break;

        case 2:
            if (--playerWork->stateTimer == 0 || GetMinigameState() == 7) {
                playerWork->movementState = 1;
            }
            motionIndex = 0;
            motionSpeed = 1.0f;
            motionAttributes = HU3D_MOTATTR_LOOP;
            break;
    }
    if (playerWork->reachedFinish) {
        CZoomM[playerWork->playerIndex] += (cameraViewPresets[2].zoom - CZoomM[playerWork->playerIndex]) * 0.1f;
        CenterM[playerWork->playerIndex].x += (object->trans.x - CenterM[playerWork->playerIndex].x) * 0.1f;
        CenterM[playerWork->playerIndex].y += ((object->trans.y + 100) - CenterM[playerWork->playerIndex].y) * 0.1f;
        CenterM[playerWork->playerIndex].z += (object->trans.z - CenterM[playerWork->playerIndex].z) * 0.1f;
        CRotM[playerWork->playerIndex].x += (cameraViewPresets[2].rot.x - CRotM[playerWork->playerIndex].x) * 0.1f;
        CRotM[playerWork->playerIndex].y += (cameraViewPresets[2].rot.y - CRotM[playerWork->playerIndex].y) * 0.1f;
        CRotM[playerWork->playerIndex].z += (cameraViewPresets[2].rot.z - CRotM[playerWork->playerIndex].z) * 0.1f;
        Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
        Hu3DModelPosSet(object->mdlId[1], object->trans.x, object->trans.y + 200, object->trans.z);
    }
    else {
        CZoomM[playerWork->playerIndex] += (cameraViewPresets[3].zoom - CZoomM[playerWork->playerIndex]) * 0.1f;
        CenterM[playerWork->playerIndex].x += (object->trans.x - CenterM[playerWork->playerIndex].x) * 0.1f;
        CenterM[playerWork->playerIndex].y += ((object->trans.y + 100) - CenterM[playerWork->playerIndex].y) * 0.1f;
        CenterM[playerWork->playerIndex].z += (object->trans.z - CenterM[playerWork->playerIndex].z) * 0.1f;
        CRotM[playerWork->playerIndex].x += (cameraViewPresets[3].rot.x - CRotM[playerWork->playerIndex].x) * 0.1f;
        CRotM[playerWork->playerIndex].y += (cameraViewPresets[3].rot.y - CRotM[playerWork->playerIndex].y) * 0.1f;
        CRotM[playerWork->playerIndex].z += (cameraViewPresets[3].rot.z - CRotM[playerWork->playerIndex].z) * 0.1f;
    }
    if ((motionIndex >= 0 && motionIndex != playerWork->motionIndex) || motionAttributes != playerWork->motionAttributes) {
        playerWork->motionIndex = motionIndex;
        playerWork->motionAttributes = motionAttributes;
        CharMotionShiftSet(playerWork->characterId, object->mtnId[playerWork->motionIndex], 0, 8, motionAttributes);
    }
    CharMotionSpeedSet(playerWork->characterId, motionSpeed);
    playerWork->motionSpeed = motionSpeed;
}

void UpdatePlayerParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float particleAngle;
    float particleSpeed;
    float emitterAngle;
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    s32 expiredParticleCount;
    if (particle->dataCnt == 0) {
        particle->dataCnt = 1;
        particleData = particle->data;
        emitterAngle = atan2d(particle->pos.x, particle->pos.z);
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleData++, particleIndex++) {
            particleData->scale = ((20.0f * frand8()) + 50.0f) / 256.0f;
            particleAngle = (frand8() * 360.0f) / 256.0f;
            particleData->pos.x = 100.0f * (0.2f * sind(particleAngle));
            particleData->pos.y = 0;
            particleData->pos.z = 100.0f * (0.2f * cosd(particleAngle));
            particleSpeed = ((frand8() * (150.0f / REFRESH_RATE)) / 256.0f) + (100.0f / REFRESH_RATE);
            particleAngle = emitterAngle + (30.0f * (((s32)frand() & 0x1FF) - 256.0f) / 256.0f);
            particleData->vel.x = particleSpeed * sind(particleAngle);
            particleData->vel.z = particleSpeed * cosd(particleAngle);
            particleData->vel.y = (frand8() * (1000.0f / REFRESH_RATE)) / 256.0f;
            particleData->color.r = particleData->color.g = particleData->color.b = particleData->color.a = 255;
        }
    }
    expiredParticleCount = 0;
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->pos.y < -2000.0f) {
            expiredParticleCount++;
        }
        else {
            particleData->vel.y += -(1.2f/REFRESH_RATE)*25; // -3.0f / REFRESH_RATE
            particleData->pos.x += particleData->vel.x;
            particleData->pos.y += particleData->vel.y;
            particleData->pos.z += particleData->vel.z;
        }
    }
    if (expiredParticleCount == particle->maxCnt) {
        model->attr |= 0x1;
    }
    DCStoreRangeNoSync(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void UpdatePlayerObject(OMOBJ *object)
{
    PlayerWork *work = object->data;
    if (object->data != NULL) {
        HuMemDirectFree(object->data);
        object->data = NULL;
    }
}

void InitializeCatcher(OMOBJ *object);

void InitializeCatcherObjects(HUPROCESS *objman)
{
    s32 i;
    initializedCatcherCount = 0;
    omMakeGroupEx(objman, 2, 4);
    for (i = 0; i < 4; i++) {
        omAddObjEx(objman, 6, 4, 50, 2, InitializeCatcher);
    }
}

void InitializeCourseModel(OMOBJ *object);

void UpdateCatcherObjects(void)
{
    OMOBJ **objList = omGetGroupMemberListEx(HuPrcCurrentGet(), 2);
    s32 i;
    for (i = 0; i < 4; i++) {
        InitializeCourseModel(objList[i]);
    }
}

s32 catcherJointMotionFiles[1] = { 0x460014 };

void UpdateCatcherIdle(OMOBJ *object);
void UpdateCatcherEvent(OMOBJ *object);

void InitializeCatcher(OMOBJ *object)
{
    CatcherWork *work;
    u32 i;
    work = HuMemDirectMallocNum(HEAP_HEAP, sizeof(CatcherWork), HU_MEMNUM_OVL);
    object->data = work;
    memset(work, 0, sizeof(CatcherWork));
    work->playerIndex = initializedCatcherCount;
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M439, 0x13));
    Hu3DModelCameraSet(object->mdlId[0], playerCameraMasks[initializedCatcherCount]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    for (i = 0; i < 1; i++) {
        object->mtnId[i] = Hu3DJointMotionFile(object->mdlId[0], catcherJointMotionFiles[i]);
    }
    Hu3DMotionSet(object->mdlId[0], object->mtnId[0]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    object->trans.y = 500.0f;
    object->scale.x = object->scale.y = object->scale.z = 0.25f;
    if (work->playerIndex == 0) {
        object->trans.x = -1800;
        object->trans.y = 50;
        object->trans.z = -8500;
        object->rot.y = -90;
        object->objFunc = UpdateCatcherIdle;
        Hu3DMotionSpeedSet(object->mdlId[0], 2.0f);
        work->eventTimer = REFRESH_RATE;
    }
    else {
        object->objFunc = UpdateCatcherEvent;
    }
    initializedCatcherCount++;
}

s32 remainingCelebrationSounds = 2;
s32 catcherSoundHandle = -1;

void UpdateCatcherIdle(OMOBJ *object)
{
    CatcherWork *work = object->data;
    float pan;
    Vec pos2D;
    if (REFRESH_RATE*0.7f == work->eventTimer) {
        HuAudFXPlay(1795);
    }
    if (work->eventTimer == 0) {
        object->trans.x -= 1500.0f / REFRESH_RATE;
    }
    else {
        if (--work->eventTimer == 0) {
            catcherSoundHandle = HuAudFXPlay(1796);
            remainingCelebrationSounds--;
        }
    }
    if (catcherSoundHandle >= 0) {
        if (HuAudFXStatusGet(catcherSoundHandle) == 0) {
            if (remainingCelebrationSounds > 0) {
                catcherSoundHandle = HuAudFXPlay(1796);
                remainingCelebrationSounds--;
            }
            else {
                catcherSoundHandle = -1;
            }
        }
        if (catcherSoundHandle >= 0) {
            Hu3D3Dto2D(&object->trans, 1, &pos2D);
            pan = 64.0f + ((32.0f * pos2D.x) / 576.0f);
            if (pan < 32.0f) {
                pan = 32.0f;
            }
            else if (pan > 96.0f) {
                pan = 96.0f;
            }
            HuAudFXPanning(catcherSoundHandle, pan);
        }
    }
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    if (GetMinigameState() == 3) {
        Hu3DMotionSpeedSet(object->mdlId[0], 1.0f);
        Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
        object->objFunc = UpdateCatcherEvent;
    }
}

void EvaluateBezierPosition(Vec *controlPoints, Vec *position, float progress);

void UpdateCatcherEvent(OMOBJ *object)
{
    CatcherWork *work = object->data;
    OMOBJ **playerObjectList;
    PlayerWork *playerWork;
    Vec directionToTarget;
    Vec interpolatedPosition;
    switch (work->eventState) {
        case 0:
            if (object->work[3]) {
                work->eventState++;
                work->eventTimer = work->eventDuration = 1.5f * REFRESH_RATE;
                playerObjectList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
                object->trans.x = work->startPosition.x = playerObjectList[work->playerIndex]->trans.x;
                object->trans.y = work->startPosition.y = 2000;
                object->trans.z = work->startPosition.z = playerObjectList[work->playerIndex]->trans.z - 100;
                object->rot.y = 180;
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
            }
            break;

        case 1:
            playerObjectList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
            work->approachPosition.x = playerObjectList[work->playerIndex]->trans.x;
            work->approachPosition.y = 1500;
            work->approachPosition.z = playerObjectList[work->playerIndex]->trans.z;
            work->launchVelocity.x = playerObjectList[work->playerIndex]->trans.x;
            work->launchVelocity.y = playerObjectList[work->playerIndex]->trans.y;
            work->launchVelocity.z = playerObjectList[work->playerIndex]->trans.z;
            EvaluateBezierPosition(&work->startPosition, &interpolatedPosition, 1.0f - ((float)work->eventTimer / (float)work->eventDuration));
            VECSubtract(&interpolatedPosition, &object->trans, &directionToTarget);
            object->rot.x = ApproachAngle(object->rot.x, atan2d(-directionToTarget.y, VECMagXZ(&directionToTarget)), 0.2f);
            object->rot.y = ApproachAngle(object->rot.y, atan2d(directionToTarget.x, directionToTarget.z), 0.2f);
            object->trans.x = interpolatedPosition.x;
            object->trans.y = interpolatedPosition.y;
            object->trans.z = interpolatedPosition.z;
            if (--work->eventTimer == 0) {
                work->isPlayerCaptured = 1;
                work->eventState++;
                work->eventTimer = 1.5f * REFRESH_RATE;
                work->fallSpeed = (200.0f - object->trans.y) / work->eventTimer;
                object->rot.x = 0;
            }
            if (1.0f * REFRESH_RATE == work->eventTimer) {
                HuAudFXPlay(1796);
            }
            break;

        case 2:
            object->trans.y += work->fallSpeed;
            if (--work->eventTimer == 0) {
                work->eventState++;
                work->eventTimer = work->eventDuration = 0.8f * REFRESH_RATE;
                work->startPosition.x = object->trans.x;
                work->startPosition.y = object->trans.y;
                work->startPosition.z = object->trans.z;
                playerObjectList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
                playerWork = playerObjectList[work->playerIndex]->data;
                work->approachPosition.x = playerWork->catchStartPosition.x;
                work->approachPosition.y = 250;
                work->approachPosition.z = playerWork->catchStartPosition.z;
                playerObjectList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
                playerWork = playerObjectList[work->playerIndex]->data;
                playerWork->visibleAtStart = 1;
                VECSubtract(&work->approachPosition, &work->startPosition, &directionToTarget);
                VECNormalize(&directionToTarget, &directionToTarget);
                VECScale(&directionToTarget, &work->launchVelocity, 2000.0f);
                work->launchVelocity.y = 300;
                VECAdd(&work->startPosition, &work->launchVelocity, &work->launchVelocity);
            }
            break;

        case 3:
            EvaluateBezierPosition(&work->startPosition, &interpolatedPosition, 1.0f - ((float)work->eventTimer / (float)work->eventDuration));
            VECSubtract(&interpolatedPosition, &object->trans, &directionToTarget);
            object->rot.x = ApproachAngle(object->rot.x, atan2d(-directionToTarget.y, VECMagXZ(&directionToTarget)), 0.2f);
            object->rot.y = ApproachAngle(object->rot.y, atan2d(directionToTarget.x, directionToTarget.z), 0.2f);
            object->trans.x = interpolatedPosition.x;
            object->trans.y = interpolatedPosition.y;
            object->trans.z = interpolatedPosition.z;
            if (--work->eventTimer == 0) {
                playerObjectList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
                playerWork = playerObjectList[work->playerIndex]->data;
                playerWork->beingCarried = 0;
                work->eventState = 0;
                object->work[3] = 0;
                Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
            }
            if (work->eventTimer == (int)(work->eventDuration - (0.2f * REFRESH_RATE))) {
                work->isPlayerCaptured = 0;
                playerObjectList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
                playerWork = playerObjectList[work->playerIndex]->data;
                playerWork->releasePosition.x = object->trans.x;
                playerWork->releasePosition.y = object->trans.y;
                playerWork->releasePosition.z = object->trans.z;
            }
            break;
    }
}

void InitializeCourseModel(OMOBJ *object) { }

s32 courseModelFiles[10] = { 0x00460000, 0x00460001, 0x00460002, 0x00460003, 0x00460004, 0x00460005, 0x00460006, 0x00460007, 0x00460008, 0x00460009 };

void InitializeCourse(OMOBJ *object);
void UpdateGoalModels(OMOBJ *object);

void InitializeCourseObjects(HUPROCESS *objman)
{
    s32 i;
    initializedCourseModelCount = 0;
    omMakeGroupEx(objman, 1, 4);
    for (i = 0; i < 4; i++) {
        omAddObjEx(objman, 10, 28, 0, 1, InitializeCourse);
    }
    for (i = 0; i < 10; i++) {
        courseModelIds[i] = Hu3DModelCreateFile(courseModelFiles[i]);
        Hu3DModelAttrSet(courseModelIds[i], HU3D_ATTR_DISPOFF);
    }
    omAddObjEx(objman, 10, 4, 0, 1, UpdateGoalModels);
}

void UpdateCourseObjects(void) { }

float courseModelPositions[][2] = { 0, 0, 0, -700, -1400, -2200, 1400, -2200, -1400, -3600, 1400, -3600, -2800, -5200, 0, -5200, 0, -5200, 2800, -5200,
    -2800, -6600, 0, -6600, 0, -6600, 2800, -6600 };

s16 courseModelIndexMap[] = { 0, 1, -1, -1, 2, 2, -1, -1, -1, -1, 3, 3, 3, 3 };

void UpdateCourseIntro(OMOBJ *object);

void InitializeCourse(OMOBJ *object)
{
    u16 randomModelPool[6] = { 4, 7, 5, 8, 6, 9 };
    char *shadowMapNames[10][2] = { "road0", NULL, "road1", "bridge1", "road2", "bridge2", "road3", NULL, "road4", NULL, "road5", NULL, "road6", NULL, "road7",
        NULL, "road8", NULL, "road9", NULL };
    CourseModelWork *work;
    s32 i;
    s32 swappedModelIndex;
    s32 shadowObjectIndex;
    s32 firstShuffleIndex;
    s32 roadModelIndex;
    s32 secondShuffleIndex;
    s32 modelPoolIndex;
    omSetStatBit(object, 0x100);
    work = HuMemDirectMallocNum(HEAP_HEAP, sizeof(CourseModelWork) * 14, HU_MEMNUM_OVL);
    object->data = work;
    memset(work, 0, sizeof(CourseModelWork) * 14);
    for (i = 0; i < 100; i++) {
        firstShuffleIndex = ((frand8() * 3) >> 8) * 2;
        secondShuffleIndex = ((frand8() * 3) >> 8) * 2;
        swappedModelIndex = randomModelPool[firstShuffleIndex];
        randomModelPool[firstShuffleIndex] = randomModelPool[secondShuffleIndex];
        randomModelPool[secondShuffleIndex] = swappedModelIndex;
        firstShuffleIndex = (((frand8() * 3) >> 8) * 2) + 1;
        secondShuffleIndex = (((frand8() * 3) >> 8) * 2) + 1;
        swappedModelIndex = randomModelPool[firstShuffleIndex];
        randomModelPool[firstShuffleIndex] = randomModelPool[secondShuffleIndex];
        randomModelPool[secondShuffleIndex] = swappedModelIndex;
    }
    for (i = 0; i < 100; i++) {
        firstShuffleIndex = ((frand8() * 3) >> 8) * 2;
        swappedModelIndex = randomModelPool[firstShuffleIndex + 1];
        randomModelPool[firstShuffleIndex + 1] = randomModelPool[firstShuffleIndex];
        randomModelPool[firstShuffleIndex] = swappedModelIndex;
    }
    modelPoolIndex = 0;
    for (i = 0; i < 14; i++, work++) {
        roadModelIndex = courseModelIndexMap[i];
        if (roadModelIndex < 0) {
            roadModelIndex = randomModelPool[modelPoolIndex++];
        }
        object->mdlId[i] = Hu3DModelLink(courseModelIds[roadModelIndex]);
        Hu3DModelPosSet(object->mdlId[i], courseModelPositions[i][0], 0, courseModelPositions[i][1]);
        Hu3DModelAttrReset(object->mdlId[i], HU3D_ATTR_DISPOFF);
        for (shadowObjectIndex = 0; shadowObjectIndex < 2; shadowObjectIndex++) {
            if (shadowMapNames[roadModelIndex][shadowObjectIndex]) {
                Hu3DModelShadowMapObjSet(object->mdlId[i], shadowMapNames[roadModelIndex][shadowObjectIndex]);
            }
        }
        work->modelIndex = roadModelIndex;
        work->worldPosition.x = courseModelPositions[i][0];
        work->worldPosition.y = 0;
        work->worldPosition.z = courseModelPositions[i][1];
        memcpy(&work->routeConnections, &courseRouteTable[i], sizeof(CourseRouteConnections));
    }
    object->work[0] = initializedCourseModelCount;
    initializedCourseModelCount++;
    object->objFunc = UpdateCourseIntro;
}

void UpdateCourseVisibility(OMOBJ *object);

void UpdateCourseIntro(OMOBJ *object)
{
    s32 i;
    for (i = 0; i < 14; i++) {
        if (object->work[0] == 0 && i == 10) {
            Hu3DModelCameraSet(object->mdlId[i], playerCameraMasks[0]);
        }
        else {
            Hu3DModelCameraSet(object->mdlId[i], 0);
        }
    }
    if (GetMinigameState() == 3) {
        object->objFunc = UpdateCourseVisibility;
    }
}

void UpdateCourseVisibility(OMOBJ *object)
{
    s32 i;
    PlayerWork *playerWork;
    OMOBJ **playerObjectList;
    CourseModelWork *work = object->data;
    s16 modelCameraMasks[14];
    for (i = 0; i < 14; i++) {
        modelCameraMasks[i] = 0;
    }
    playerObjectList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    playerWork = playerObjectList[object->work[0]]->data;
    currentRouteConnections = &work[playerWork->roadSection].routeConnections;
    modelCameraMasks[playerWork->roadSection] |= playerCameraMasks[object->work[0]];
    if (currentRouteConnections->alternateModelIndex >= 0) {
        modelCameraMasks[currentRouteConnections->alternateModelIndex] |= playerCameraMasks[object->work[0]];
    }
    for (i = 0; i < currentRouteConnections->connectionCount; i++) {
        modelCameraMasks[currentRouteConnections->connectedSections[i]] |= playerCameraMasks[object->work[0]];
    }
    for (i = 0; i < 14; i++) {
        Hu3DModelCameraSet(object->mdlId[i], modelCameraMasks[i]);
    }
}

s32 GetCourseSectionFromDepth(float worldDepth)
{
    s32 i;
    for (i = 0; i < 5; i++) {
        if (worldDepth > courseSectionDepths[i]) {
            break;
        }
    }
    return i;
}

s32 GetNextCourseSection(OMOBJ *object)
{
    PlayerWork *work = object->data;
    OMOBJ **playerObjectList = omGetGroupMemberListEx(HuPrcCurrentGet(), 1);
    CourseModelWork *roadObjects = playerObjectList[work->playerIndex]->data;
    s32 i;
    HU3DMODEL *candidateModel;
    s32 nearestSectionIndex;
    float horizontalDistance;
    float nearestDistance;
    playerRouteConnections = &roadObjects[work->roadSection].routeConnections;
    if (playerRouteConnections->connectionCount > 0 && work->previousRoadSection == roadObjects[playerRouteConnections->connectedSections[0]].routeConnections.modelIndex) {
        nearestSectionIndex = -1;
        nearestDistance = 9999;
        for (i = 0; i < playerRouteConnections->connectionCount; i++) {
            candidateModel = &Hu3DData[playerObjectList[work->playerIndex]->mdlId[playerRouteConnections->connectedSections[i]]];
            horizontalDistance = fabs(object->trans.x - candidateModel->pos.x);
            if (horizontalDistance < nearestDistance) {
                nearestDistance = horizontalDistance;
                nearestSectionIndex = playerRouteConnections->connectedSections[i];
            }
        }
        (void)i;
        return nearestSectionIndex;
    }
    else {
        if (work->previousRoadSection == roadObjects[playerRouteConnections->alternateModelIndex].routeConnections.modelIndex) {
            return playerRouteConnections->alternateModelIndex;
        }
        else {
            return -1;
        }
    }
}

s32 goalModelFiles[8] = { 0x0046000A, 0x0046000B, 0x0046000C, 0x0046000D, 0x0046000E, 0x0046000F, 0x00460010, 0x00460011 };

Vec goalPositions[4] = { { -2800, 0, -7700 }, { 0, 0, -7700 }, { 0, 0, -7700 }, { 2800, 0, -7700 } };

void UpdateGoalObjects(OMOBJ *object);

void UpdateGoalModels(OMOBJ *object)
{
    s32 i;
    omSetStatBit(object, 0x100);
    for (i = 0; i < object->mdlcnt; i++) {
        object->mdlId[i] = Hu3DModelCreateFile(goalModelFiles[GWPlayerCfg[i].character]);
        Hu3DModelCameraSet(object->mdlId[i], playerCameraMasks[i]);
        Hu3DModelClusterAttrSet(object->mdlId[i], 0, 0xC0000001);
        Hu3DModelAttrSet(object->mdlId[i], HU3D_ATTR_DISPOFF);
    }
    object->objFunc = UpdateGoalObjects;
}

void UpdateGoalVisibility(OMOBJ *object);

void UpdateGoalObjects(OMOBJ *object)
{
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelPosSet(object->mdlId[0], -2800, 0, -7700);
    if (GetMinigameState() == 3) {
        Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
        object->objFunc = UpdateGoalVisibility;
    }
}

void UpdateGoalVisibility(OMOBJ *object)
{
    OMOBJ **objList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    s32 i;
    for (i = 0; i < 4; i++) {
        PlayerWork *work = objList[i]->data;
        if (work->roadSection >= 10 && work->roadSection <= 13) {
            s32 goalNo = work->roadSection - 10;
            Hu3DModelPosSet(object->mdlId[i], goalPositions[goalNo].x, goalPositions[goalNo].y, goalPositions[goalNo].z);
            Hu3DModelAttrReset(object->mdlId[i], HU3D_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrSet(object->mdlId[i], HU3D_ATTR_DISPOFF);
        }
    }
}

s32 FindCatchPositions(Vec *playerPosition, s32 playerIndex, s32 roadSectionIndex, Vec *catchPosition, Vec *targetPosition)
{
    OMOBJ **roadObjectList = omGetGroupMemberListEx(HuPrcCurrentGet(), 1);
    CourseModelWork *roadObjects = roadObjectList[playerIndex]->data;
    s32 i;
    s32 modelIndex;

    Vec positionFromRoadOrigin;
    VECSubtract(playerPosition, &roadObjects[roadSectionIndex].worldPosition, &positionFromRoadOrigin);
    modelIndex = roadObjects[roadSectionIndex].modelIndex;
    for (i = 0; i < courseGeometryCounts02[modelIndex]; i++) {
        Vec *lowerRoadEdge = (courseGeometryData13[modelIndex])[i * 3];
        Vec *upperRoadEdge = *((&courseGeometryData13[modelIndex][i * 3]) + 1);
        Vec *farRoadEdge = *((&courseGeometryData13[modelIndex][i * 3]) + 2);
        s32 j;
        for (j = 0; j < courseGeometryData14[modelIndex][i] - 1; j++, lowerRoadEdge++, upperRoadEdge++, farRoadEdge++) {
            Vec edgeVector;
            Vec pointOffset;
            Vec surfaceNormal;
            float edgeProjection;
            float edgeLengthSquared;
            VECSubtract(&lowerRoadEdge[1], &lowerRoadEdge[0], &edgeVector);
            VECSubtract(&positionFromRoadOrigin, &lowerRoadEdge[1], &pointOffset);
            VECCrossProduct(&edgeVector, &pointOffset, &surfaceNormal);
            if (surfaceNormal.y < 0.0f) {
                continue;
            }
            VECSubtract(&upperRoadEdge[1], &lowerRoadEdge[1], &edgeVector);
            VECSubtract(&positionFromRoadOrigin, &upperRoadEdge[1], &pointOffset);
            VECCrossProduct(&edgeVector, &pointOffset, &surfaceNormal);
            if (surfaceNormal.y < 0.0f) {
                continue;
            }
            VECSubtract(&upperRoadEdge[0], &upperRoadEdge[1], &edgeVector);
            VECSubtract(&positionFromRoadOrigin, &upperRoadEdge[0], &pointOffset);
            VECCrossProduct(&edgeVector, &pointOffset, &surfaceNormal);
            if (surfaceNormal.y < 0.0f) {
                continue;
            }
            VECSubtract(&lowerRoadEdge[0], &upperRoadEdge[0], &edgeVector);
            VECSubtract(&positionFromRoadOrigin, &lowerRoadEdge[0], &pointOffset);
            VECCrossProduct(&edgeVector, &pointOffset, &surfaceNormal);
            if (surfaceNormal.y < 0.0f) {
                continue;
            }
            VECSubtract(&lowerRoadEdge[1], &lowerRoadEdge[0], &edgeVector);
            edgeLengthSquared = VECMag2Point(&edgeVector);
            if (0.0f == edgeLengthSquared) {
                continue;
            }
            edgeProjection = ((positionFromRoadOrigin.x * edgeVector.x) - (lowerRoadEdge->x * edgeVector.x) + (positionFromRoadOrigin.y * edgeVector.y) - (lowerRoadEdge->y * edgeVector.y) + (positionFromRoadOrigin.z * edgeVector.z)
                           - (lowerRoadEdge->z * edgeVector.z))
                / edgeLengthSquared;
            catchPosition->x = (upperRoadEdge[0].x + roadObjects[roadSectionIndex].worldPosition.x) + (edgeProjection * (upperRoadEdge[1].x - upperRoadEdge[0].x));
            catchPosition->y = (upperRoadEdge[0].y + roadObjects[roadSectionIndex].worldPosition.y) + (edgeProjection * (upperRoadEdge[1].y - upperRoadEdge[0].y));
            catchPosition->z = (upperRoadEdge[0].z + roadObjects[roadSectionIndex].worldPosition.z) + (edgeProjection * (upperRoadEdge[1].z - upperRoadEdge[0].z));
            targetPosition->x = (farRoadEdge[0].x + roadObjects[roadSectionIndex].worldPosition.x) + (edgeProjection * (farRoadEdge[1].x - farRoadEdge[0].x));
            targetPosition->y = (farRoadEdge[0].y + roadObjects[roadSectionIndex].worldPosition.y) + (edgeProjection * (farRoadEdge[1].y - farRoadEdge[0].y));
            targetPosition->z = (farRoadEdge[0].z + roadObjects[roadSectionIndex].worldPosition.z) + (edgeProjection * (farRoadEdge[1].z - farRoadEdge[0].z));
            return 1;
        }
    }
    return 0;
}

void SetCameraView(s32 cameraIndex, s32 viewPresetIndex);
void UpdateGameCamera(OMOBJ *object);

void InitializeGameCamera(OMOBJ *object)
{
    SetCameraView(0, 0);
    object->work[3] = espEntry(0x460015, 32768, 0);
    espPosSet(object->work[3], 288, 240);
    espDispOff(object->work[3]);
    espScaleSet(object->work[3], 2.0f, 2.0f);
    espColorSet(object->work[3], 0, 0, 0);
    object->objFunc = UpdateGameCamera;
}

void SetCameraView(s32 cameraIndex, s32 viewPresetIndex)
{
    CZoomM[cameraIndex] = cameraViewPresets[viewPresetIndex].zoom;
    CenterM[cameraIndex].x = cameraViewPresets[viewPresetIndex].pos.x;
    CenterM[cameraIndex].y = cameraViewPresets[viewPresetIndex].pos.y;
    CenterM[cameraIndex].z = cameraViewPresets[viewPresetIndex].pos.z;
    CRotM[cameraIndex].x = cameraViewPresets[viewPresetIndex].rot.x;
    CRotM[cameraIndex].y = cameraViewPresets[viewPresetIndex].rot.y;
    CRotM[cameraIndex].z = cameraViewPresets[viewPresetIndex].rot.z;
}

void UpdateGameCamera(OMOBJ *object)
{
    HU3DCAMERA *camera = &Hu3DCamera[0];
    s32 i;
    switch (object->work[0]) {
        case 0:
            if (GetMinigameState() <= 3) {
                CRotM[0].y -= 10.0f / REFRESH_RATE;
            }
            if (GetMinigameState() == 3) {
                Hu3DCameraCreate(15);
                Hu3DCameraPerspectiveSet(15, 10, 20, 25000, 1.2);
                Hu3DCameraViewportSet(1, 0, 0, 320, 240, 0, 1);
                Hu3DCameraViewportSet(2, 320, 0, 320, 240, 0, 1);
                Hu3DCameraViewportSet(4, 0, 240, 320, 240, 0, 1);
                Hu3DCameraViewportSet(8, 320, 240, 320, 240, 0, 1);
                Hu3DCameraScissorSet(1, 0, 0, 320, 240);
                Hu3DCameraScissorSet(2, 320, 0, 640, 240);
                Hu3DCameraScissorSet(4, 0, 240, 320, 480);
                Hu3DCameraScissorSet(8, 320, 240, 640, 480);
                for (i = 0; i < 4; i++) {
                    SetCameraView(i, 1);
                }
                gameCameraObject->work[0] = 4;
                object->work[0]++;
                espDispOn(object->work[3]);
            }
            break;
    }
}

float QuadraticBezier(float startValue, float controlValue, float endValue, float progress)
{
    float inverseProgress = 1.0f - progress;
    return (startValue * (inverseProgress * inverseProgress)) + ((2.0f * inverseProgress) * progress * controlValue) + (endValue * (progress * progress));
}

void EvaluateBezierPosition(Vec *controlPoints, Vec *position, float progress)
{
    position->x = QuadraticBezier(controlPoints[0].x, controlPoints[1].x, controlPoints[2].x, progress);
    position->y = QuadraticBezier(controlPoints[0].y, controlPoints[1].y, controlPoints[2].y, progress);
    position->z = QuadraticBezier(controlPoints[0].z, controlPoints[1].z, controlPoints[2].z, progress);
}

void UpdateShadows(s16 layer);

void InitializeShadows(HUPROCESS *objman)
{
    s32 i;
    s32 dataSize = Hu3DShadowData.size * Hu3DShadowData.size;
    objectManager = objman;
    for (i = 0; i < 3; i++) {
        shadowBuffers[i] = HuMemDirectMallocNum(HEAP_HEAP, dataSize, HU_MEMNUM_OVL);
        memset(shadowBuffers[i], 0, dataSize);
        savedShadowCameraTargets[i].x = Hu3DShadowData.camTarget.x;
        savedShadowCameraTargets[i].y = Hu3DShadowData.camTarget.y;
        savedShadowCameraTargets[i].z = Hu3DShadowData.camTarget.z;
        savedShadowCameraPositions[i].x = Hu3DShadowData.camPos.x;
        savedShadowCameraPositions[i].y = Hu3DShadowData.camPos.y;
        savedShadowCameraPositions[i].z = Hu3DShadowData.camPos.z;
    }
    Hu3DLayerHookSet(0, UpdateShadows);
}

void CaptureShadowTexture(s32 shadowNo);

void UpdateShadows(s16 layer)
{
    OMOBJ **playerList;
    s32 i;
    s32 dataSize = Hu3DShadowData.size * Hu3DShadowData.size;
    if (GetMinigameState() < 3) {
        return;
    }
    if (Hu3DCameraNo == 0) {
        PlayerWork *playerWork;
        GXDrawDone();
        playerList = omGetGroupMemberListEx(objectManager, 0);
        if (Hu3DData[playerList[0]->mdlId[0]].attr & 0x4) {
            Hu3DModelShadowReset(playerList[0]->mdlId[0]);
        }
        for (i = 0; i < 3; i++) {
            playerWork = playerList[i + 1]->data;
            if (playerWork->visibleAtStart) {
                Hu3DModelShadowSet(playerList[i + 1]->mdlId[0]);
            }
            CaptureShadowTexture(i);
            GXDrawDone();
            DCFlushRangeNoSync(shadowBuffers[i], dataSize);
            if (playerWork->visibleAtStart) {
                Hu3DModelShadowReset(playerList[i + 1]->mdlId[0]);
            }
        }
        playerWork = playerList[0]->data;
        if (playerWork->visibleAtStart) {
            Hu3DModelShadowSet(playerList[0]->mdlId[0]);
        }
        else {
            memset(Hu3DShadowData.buf, 0, dataSize);
            DCFlushRangeNoSync(Hu3DShadowData.buf, dataSize);
        }
        if (Hu3DShadowCamBit < 1) {
            Hu3DShadowCamBit = 1;
        }
    }
    else {
        MTXCopy(savedShadowMatrices[Hu3DCameraNo - 1], Hu3DShadowData.lookAtMtx);
        memcpy(Hu3DShadowData.buf, shadowBuffers[Hu3DCameraNo - 1], dataSize);
        DCFlushRangeNoSync(Hu3DShadowData.buf, dataSize);
    }
}

void CaptureShadowTexture(s32 shadowNo)
{
    HU3DMODEL *model;
    s32 dataSize;
    s16 i;
    Mtx transform[3];
    Mtx44 proj;
    GXColor clearColor = { 0, 0, 0, 0 };

    Hu3DDrawPreInit();
    GXSetCopyClear(clearColor, 0xFFFFFF);
    MTXPerspective(proj, Hu3DShadowData.fov, 1.2f, Hu3DShadowData.nnear, Hu3DShadowData.ffar);
    GXSetProjection(proj, GX_PERSPECTIVE);
    if (Hu3DShadowData.size <= 240) {
        GXSetScissor(2, 2, (Hu3DShadowData.size * 2) - 4, (Hu3DShadowData.size * 2) - 4);
        GXSetViewport(0, 0, Hu3DShadowData.size * 2, Hu3DShadowData.size * 2, 0, 1);
        dataSize = (Hu3DShadowData.size / 2) * (Hu3DShadowData.size / 2);
    }
    else {
        GXSetScissor(1, 1, Hu3DShadowData.size - 2, Hu3DShadowData.size - 2);
        GXSetViewport(0, 0, Hu3DShadowData.size, Hu3DShadowData.size, 0, 1);
        dataSize = Hu3DShadowData.size * Hu3DShadowData.size;
    }
    MTXLookAt(Hu3DCameraMtx, &savedShadowCameraPositions[shadowNo], &Hu3DShadowData.camUp, &savedShadowCameraTargets[shadowNo]);
    MTXCopy(Hu3DCameraMtx, savedShadowMatrices[shadowNo]);
    model = &Hu3DData[0];
    shadowModelDrawF = 1;
    GXInvalidateTexAll();
    GXSetFog(0, 0, 0, 0, 0, BGColor);
    for (i = 0; i < 512; i++, model++) {
        s32 clusterF;
        if (model->hsf && (model->attr & 0x4) && (model->attr & 0x1) == 0 && (model->attr & 0x8000) == 0) {
            if (model->attr & 0x8) {
                clusterF = 0;
                if (model->motId != -1) {
                    Hu3DMotionExec(i, model->motId, model->motWork.time, 0);
                }
                if (model->motIdShift != -1) {
                    Hu3DSubMotionExec(i);
                }
                if (model->motIdOvl != -1) {
                    Hu3DMotionExec(i, model->motIdOvl, model->motOvlWork.time, 1);
                }
                if (model->attr & 0x400) {
                    ClusterMotionExec(model);
                    clusterF = 1;
                }
                if (model->motIdShape != -1) {
                    if (model->motId == -1) {
                        Hu3DMotionExec(i, model->motIdShape, model->motShapeWork.time, 0);
                    }
                    else {
                        Hu3DMotionExec(i, model->motIdShape, model->motShapeWork.time, 1);
                    }
                }
                if ((model->attr & 0x90) == 0 || (model->motAttr & 0x40000002) == 0) {
                    clusterF = 1;
                    InitVtxParm(model->hsf);
                    if (model->motIdShape != -1) {
                        ShapeProc(model->hsf);
                    }
                    if (model->attr & 0x400) {
                        ClusterProc(model);
                    }
                    if (model->hsf->cenvNum) {
                        EnvelopeProc(model->hsf);
                    }
                    PPCSync();
                }
                model->attr |= 0x800;
            }
            mtxRot(transform[0], model->rot.x, model->rot.y, model->rot.z);
            MTXScale(transform[2], model->scale.x, model->scale.y, model->scale.z);
            MTXConcat(transform[0], transform[2], transform[2]);
            mtxTransCat(transform[2], model->pos.x, model->pos.y, model->pos.z);
            MTXConcat(Hu3DCameraMtx, transform[2], transform[1]);
            MTXConcat(transform[1], model->mtx, transform[1]);
            Hu3DDraw(model, transform[1], &model->scale);
        }
    }
    Hu3DDrawPost();
    shadowModelDrawF = 0;
    if (Hu3DShadowData.size <= 240) {
        GXSetTexCopySrc(0, 0, Hu3DShadowData.size * 2, Hu3DShadowData.size * 2);
        GXSetTexCopyDst(Hu3DShadowData.size, Hu3DShadowData.size, GX_CTF_R8, GX_TRUE);
    }
    else {
        GXSetTexCopySrc(0, 0, Hu3DShadowData.size, Hu3DShadowData.size);
        GXSetTexCopyDst(Hu3DShadowData.size, Hu3DShadowData.size, GX_CTF_R8, GX_FALSE);
    }
    GXCopyTex(shadowBuffers[shadowNo], GX_TRUE);
    GXSetViewport(0, 0, RenderMode->fbWidth, RenderMode->xfbHeight, 0, 1);
    GXSetScissor(0, 0, RenderMode->fbWidth, RenderMode->efbHeight);
    MTXOrtho(proj, 0, 1, 0, 1, 0, 1);
    GXSetProjection(proj, GX_ORTHOGRAPHIC);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_U8, 0);
    GXSetTevColor(GX_TEVREG0, BGColor);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumChans(0);
    MTXIdentity(transform[1]);
    GXLoadPosMtxImm(transform[1], GX_PNMTX0);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_TRUE);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3u8(0, 0, 0);
    GXPosition3u8(1, 0, 0);
    GXPosition3u8(1, 1, 0);
    GXPosition3u8(0, 1, 0);
}
