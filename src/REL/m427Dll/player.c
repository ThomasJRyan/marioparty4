#include "REL/m427Dll.h"

#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXStruct.h"
#include "dolphin/gx/GXVert.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/gamework_data.h"
#include "game/hsfformat.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "string.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

typedef struct PlayerCharacterState {
    /* 0x00 */ s32 playerIndex;
    /* 0x04 */ s32 teamIndex;
    /* 0x08 */ s32 controllerIndex;
    /* 0x0C */ s32 characterId;
    /* 0x10 */ s32 teamSlot;
    /* 0x14 */ s32 lightHandle;
    /* 0x18 */ s32 isComputer;
    /* 0x1C */ s32 difficulty;
    /* 0x20 */ s32 frameCounter;
    /* 0x24 */ s32 actionState;
    /* 0x28 */ s32 actionTimer;
    /* 0x2C */ s32 motionIndex;
    /* 0x30 */ s32 leftActionPending;
    /* 0x34 */ s32 rightActionPending;
    /* 0x38 */ s32 leftActionRequested;
    /* 0x3C */ s32 rightActionRequested;
    /* 0x40 */ s32 currentButtons;
    /* 0x44 */ s32 pressedButtons;
    /* 0x48 */ s32 previousButtons;
    /* 0x4C */ s32 previousPressedButtons;
    /* 0x50 */ s32 playerStateTimer;
    /* 0x54 */ s32 cameraTargetAge;
    /* 0x58 */ s32 cameraUpdateTimer;
    /* 0x5C */ s32 cameraUpdateJitter;
    /* 0x60 */ s32 cameraUpdateInterval;
    /* 0x64 */ s32 cameraTargetSegmentIndex;
    /* 0x68 */ float cameraTargetX;
    /* 0x6C */ float cameraTargetY;
    /* 0x70 */ float cameraTargetZ;
} PlayerCharacterState; /* size = 0x74 */

typedef struct TeamPlayerState {
    /* 0x00 */ OMOBJ *playerObjects[2];
    /* 0x08 */ PlayerCharacterState *playerStates[2];
    /* 0x10 */ s32 teamIndex;
    /* 0x14 */ s32 computerPlayerMask;
    /* 0x18 */ s32 frameCounter;
    /* 0x1C */ s32 playerState;
    /* 0x20 */ s32 stunTimer;
    /* 0x24 */ s32 effectTimer;
    /* 0x28 */ float hitReactionStrength;
    /* 0x2C */ float hitReactionAngle;
    /* 0x30 */ s32 modelIds[5];
    /* 0x34 */ char reservedModelData[8];
    /* 0x4C */ float inputVelocityX;
    /* 0x50 */ float inputStateValue;
    /* 0x54 */ float inputVelocityZ;
    /* 0x58 */ float laneSpeedLeft;
    /* 0x5C */ float laneSpeedRight;
    /* 0x60 */ s32 movementSpeed;
    /* 0x64 */ float facingAngle;
    /* 0x68 */ s32 effectCooldown;
    /* 0x6C */ char playerReservedBytes[4];
} TeamPlayerState; /* size = 0x70 */

typedef struct RouteCollisionState {
    /* 0x00 */ s32 collisionSlot;
    /* 0x04 */ s32 collisionSide;
    /* 0x08 */ s32 shapeType;
    /* 0x0C */ s32 collisionStateFlags;
    /* 0x10 */ s32 teamIndex;
    /* 0x14 */ s32 elapsedFrames;
    /* 0x18 */ s32 durationFrames;
    /* 0x1C */ Vec firstCorner;
    /* 0x28 */ Vec secondCorner;
    /* 0x34 */ float collisionRadius;
    /* 0x38 */ char collisionReservedBytes[4];
} RouteCollisionState; /* size = 0x3C */

typedef struct RouteSegment {
    /* 0x00 */ s32 segmentIndex;
    /* 0x04 */ s32 nextSegmentIndex;
    /* 0x08 */ s32 previousSegmentIndex;
    /* 0x0C */ float centerlineHeading;
    /* 0x10 */ float previousCenterlineHeading;
    /* 0x14 */ Vec leftEdgeControlPoints[2];
    /* 0x2C */ Vec rightEdgeControlPoints[2];
    /* 0x44 */ Vec centerlinePosition;
    /* 0x50 */ float centerlineTangentX;
    /* 0x54 */ float centerlineTangentY;
    /* 0x58 */ float centerlineTangentZ;
    /* 0x5C */ float leftEdgeWidthStart;
    /* 0x60 */ float rightEdgeWidthStart;
    /* 0x64 */ float leftEdgeWidthEnd;
    /* 0x68 */ float rightEdgeWidthEnd;
    /* 0x6C */ float centerlineSegmentLength;
    /* 0x70 */ float leftEdgeHeading;
    /* 0x74 */ float rightEdgeHeading;
    /* 0x78 */ Vec *leftEdgePosition;
    /* 0x7C */ Vec *rightEdgePosition;
    /* 0x80 */ Vec *centerlinePositionPtr;
} RouteSegment; /* size = 0x84 */

void UpdatePlayerController(OMOBJ *object);
void UpdatePlayerMovement(OMOBJ *object);
void UpdatePlayerCollision(OMOBJ *object);
void UpdatePlayerHitReaction(OMOBJ *object);
void UpdatePlayerAnimationState(OMOBJ *object);
void UpdatePlayerCamera(OMOBJ *object);
void EmptyPlayerObjectCallback(OMOBJ *object);
void UpdatePlayerRoutePosition(OMOBJ *object);
void UpdatePlayerRouteMotion(OMOBJ *object);
void InitializePlayerModels(void);
void UpdatePlayerModelLighting(void);
void EmptyPlayerModelCallback(void);
s32 CalculateRouteSegmentParameter(float positionX, float positionY, float positionZ, float *segmentFraction);
float CalculateRouteCurveValue(float curveParameter, float segmentLength);
void GetRoutePosition(float unusedWorldX, float worldZ, Vec *movementDirection);
s32 FindRouteSegmentForPosition(s32 preferredSegmentIndex, Vec *previousPosition, Vec *proposedPosition, Vec *resolvedPosition, Vec *collisionOffset);
void ResetPlayerRouteState(s32 teamIndex);
s32 FindNearestRoutePosition(s32 teamIndex, Vec *position, float searchRadius);
s32 FindPlayerRouteTarget(s32 teamIndex, Vec *routePositionContext, Vec *proposedPosition, Vec *resolvedPosition, float searchRadius);
float InterpolateRouteValue(float startValue, float endValue);
void InterpolateRouteVectors(Vec *startPosition, Vec *endPosition, Vec *outputVectors, s32 normalize);
void ProjectPositionOntoRouteDouble(Vec *routeVectors, float *routeParameter, Vec *position, Vec *projectedPosition);
void TransformRoutePosition(Mtx transformMatrix, Vec *position);
void ScaleModelMaterialLighting(s32 modelId, float brightnessScale);
s32 CopyModelLight(s32 sourceModelId, s32 sourceLightIndex, s32 targetModelId);
void SetPlayerObjectState(OMOBJ *object);
void UpdatePlayerObjectState(OMOBJ *object);
void UpdatePlayerAnimation(OMOBJ *object);

Vec leftRouteEdgePoints[0x1D] = {
    { -900.0f, 0.0f, 2000.0f },
    { -900.0f, 0.0f, 0.0f },
    { -900.0f, 0.0f, -2000.0f },
    { -900.0f, 0.0f, -4000.0f },
    { -1300.0f, 0.0f, -4500.0f },
    { -1600.0f, 0.0f, -5000.0f },
    { -1800.0f, 0.0f, -5500.0f },
    { -1900.0f, 0.0f, -6000.0f },
    { -1800.0f, 0.0f, -6500.0f },
    { -1600.0f, -100.0f, -7000.0f },
    { -1300.0f, -200.0f, -7500.0f },
    { -900.0f, -300.0f, -8000.0f },
    { -900.0f, -300.0f, -8500.0f },
    { -500.0f, -300.0f, -9000.0f },
    { -500.0f, -300.0f, -10000.0f },
    { -500.0f, -300.0f, -11000.0f },
    { -900.0f, -300.0f, -11500.0f },
    { -900.0f, -300.0f, -12000.0f },
    { -500.0f, -300.0f, -12500.0f },
    { -200.0f, -300.0f, -13000.0f },
    { -0.0f, -300.0f, -13500.0f },
    { 100.0f, -300.0f, -14000.0f },
    { 0.0f, -300.0f, -14500.0f },
    { -200.0f, -400.0f, -15000.0f },
    { -500.0f, -500.0f, -15500.0f },
    { -900.0f, -600.0f, -16000.0f },
    { -900.0f, -600.0f, -18000.0f },
    { -900.0f, -600.0f, -20000.0f },
    { -900.0f, -600.0f, -22000.0f },
};
Vec rightRouteEdgePoints[29] = {
    { 900.0f, 0.0f, 2000.0f },
    { 900.0f, 0.0f, -0.0f },
    { 900.0f, 0.0f, -2000.0f },
    { 900.0f, 0.0f, -4000.0f },
    { 500.0f, 0.0f, -4500.0f },
    { 200.0f, 0.0f, -5000.0f },
    { 0.0f, 0.0f, -5500.0f },
    { -100.0f, 0.0f, -6000.0f },
    { 0.0f, 0.0f, -6500.0f },
    { 200.0f, -100.0f, -7000.0f },
    { 500.0f, -200.0f, -7500.0f },
    { 900.0f, -300.0f, -8000.0f },
    { 900.0f, -300.0f, -8500.0f },
    { 500.0f, -300.0f, -9000.0f },
    { 500.0f, -300.0f, -10000.0f },
    { 500.0f, -300.0f, -11000.0f },
    { 900.0f, -300.0f, -11500.0f },
    { 900.0f, -300.0f, -12000.0f },
    { 1300.0f, -300.0f, -12500.0f },
    { 1600.0f, -300.0f, -13000.0f },
    { 1800.0f, -300.0f, -13500.0f },
    { 1900.0f, -300.0f, -14000.0f },
    { 1800.0f, -300.0f, -14500.0f },
    { 1600.0f, -400.0f, -15000.0f },
    { 1300.0f, -500.0f, -15500.0f },
    { 900.0f, -600.0f, -16000.0f },
    { 900.0f, -600.0f, -18000.0f },
    { 900.0f, -600.0f, -20000.0f },
    { 900.0f, -600.0f, -22000.0f },
};
s32 routeSegmentIndices[7] = { 0, 1, 2, 3, 4, 5, 6 };
Vec playerCollisionOffsets[2] = { { 0.0f, -25.0f, 50.0f }, { 0.0f, -25.0f, -50.0f } };
Vec playerCameraOffsets[2] = { { 0.0f, 0.0f, 1900.0f }, { 0.0f, 20.0f, 20.0f } };
Vec playerCameraRotation = { 0.0f, 240.00002f, 360.0f };
Vec playerCameraScale = { 0.0f, 1.0f, 1.5f };
GXColor playerWhiteColor = { 0xFF, 0xFF, 0xFF, 0xFF };

HUPROCESS *playerProcess;
RouteSegment *routeSegments;
Vec *leftRouteBoundary;
Vec *rightRouteBoundary;
Vec *routeCenterline;
s32 routeSegmentCount;
RouteCollisionState *routeCollisionSlots;
TeamPlayerState *teamPlayerWork;
PlayerCharacterState *playerWork;
OMOBJ *teamPlayerObjects[2];
char playerSystemState[0x10];
Vec *baseLeftRouteEdge;
Vec *baseRightRouteEdge;
Vec *baseRouteCenterline;

void InitializePlayerSystem(HUPROCESS *playerProcessInput)
{
    s32 startingPlayerIndex;
    OMOBJ *object;
    s32 playerIndex;
    s32 usesGroupConfig;
    s32 soloPlayerCount;
    s32 teamPlayerCount;
    s32 playerGroup;

    s8 firstTeamPlayerIndices[2] = { 0xFF, 0xFF };
    s8 secondTeamPlayerIndices[2] = { 0xFF, 0xFF };
    startingPlayerIndex = 0;
    OSReport("Init Player \n");
    playerProcess = playerProcessInput;
    object = omAddObjEx(playerProcess, 0x42, 0, 0, -1, SetPlayerObjectState);
    omMakeGroupEx(playerProcess, 0, 4);
    omGetGroupMemberListEx(playerProcess, 0);
    for (playerIndex = 0, soloPlayerCount = 0, teamPlayerCount = 0, usesGroupConfig = 0; playerIndex < 4; playerIndex++) {
        playerGroup = GWPlayerCfg[playerIndex].group;
        switch (playerGroup) {
            case 0:
                if (soloPlayerCount >= 2) {
                    usesGroupConfig = 1;
                }
                else {
                    firstTeamPlayerIndices[soloPlayerCount] = playerIndex;
                    soloPlayerCount++;
                }
                break;
            case 1:
                if (teamPlayerCount >= 2) {
                    usesGroupConfig = 1;
                }
                else {
                    secondTeamPlayerIndices[teamPlayerCount] = playerIndex;
                    teamPlayerCount++;
                }
                break;
            default:
                usesGroupConfig = 1;
                break;
        }
        if (usesGroupConfig != 0) {
            break;
        }
    }
    teamPlayerWork = HuMemDirectMallocNum(HEAP_HEAP, 2 * sizeof(TeamPlayerState), HU_MEMNUM_OVL);
    memset(teamPlayerWork, 0, 0xE0);
    playerWork = HuMemDirectMallocNum(HEAP_HEAP, 4 * sizeof(PlayerCharacterState), HU_MEMNUM_OVL);
    memset(playerWork, 0, 0x1D0);
    memset(teamPlayerObjects, 0, 8);
    memset(playerSystemState, 0, 0x10);
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        object = teamPlayerObjects[playerIndex] = omAddObjEx(playerProcess, 0x41, 0xE, 0, -1, UpdatePlayerObjectState);
        object->work[0] = playerIndex;
        if (usesGroupConfig != 0) {
            object->work[1] = playerIndex * 2;
            object->work[2] = (playerIndex * 2) + 1;
        }
        else if (playerIndex == 0) {
            object->work[1] = firstTeamPlayerIndices[0];
            object->work[2] = firstTeamPlayerIndices[1];
        }
        else {
            object->work[1] = secondTeamPlayerIndices[0];
            object->work[2] = secondTeamPlayerIndices[1];
        }
    }
    InitializePlayerModels();
}

s32 playerModelAssetIds[7] = { 0x3A001C, 0x3A001D, 0x3A001E, 0x3A001F, 0x3A0020, 0x3A0026, 0x3A0027 };
s32 alternatePlayerModelAssetIds[7] = { 0x3A0021, 0x3A0022, 0x3A0023, 0x3A0024, 0x3A0025, 0x3A0026, 0x3A0027 };
s32 teamLeftModelIndices[2] = { 1, 3 };
s32 teamRightModelIndices[2] = { 2, 4 };

void DestroyPlayerSystem(void)
{
    EmptyPlayerModelCallback();
}

void SetPlayerObjectState(OMOBJ *object)
{
    UpdatePlayerModelLighting();
}

void UpdatePlayerObjectState(OMOBJ *object)
{
    TeamPlayerState *work;
    s32 playerIndex;
    s32 modelIndex;
    s32 lightHandle;

    work = &teamPlayerWork[object->work[0]];
    object->data = work;
    work->teamIndex = object->work[0];
    work->computerPlayerMask = 0;
    work->frameCounter = 0;
    work->playerState = 0;
    work->stunTimer = 0;
    work->effectTimer = 0;
    work->hitReactionStrength = 0.0f;
    work->hitReactionAngle = 0.0f;
    work->inputVelocityX = 0.0f;
    work->inputStateValue = 0.0f;
    work->inputVelocityZ = 0.0f;
    work->laneSpeedLeft = 0.0f;
    work->laneSpeedRight = 0.0f;
    work->movementSpeed = 0;
    work->facingAngle = 180.0f;
    work->effectCooldown = 0;
    object->trans.x = playerCameraOffsets->x;
    object->trans.y = playerCameraOffsets->y;
    object->trans.z = playerCameraOffsets->z;
    for (playerIndex = 0; playerIndex < 7; playerIndex++) {
        if (work->teamIndex == 0) {
            object->mdlId[playerIndex] = Hu3DModelCreateFile(playerModelAssetIds[playerIndex]);
        }
        else {
            object->mdlId[playerIndex] = Hu3DModelCreateFile(alternatePlayerModelAssetIds[playerIndex]);
        }
        Hu3DModelLayerSet(object->mdlId[playerIndex], 2);
        if (work->teamIndex == 0) {
            Hu3DModelCameraSet(object->mdlId[playerIndex], 1);
        }
        else {
            Hu3DModelCameraSet(object->mdlId[playerIndex], 2);
        }
        if (playerIndex < 5) {
            Hu3DMotionSpeedSet(object->mdlId[playerIndex], 2.0f);
        }
    }
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DMotionSpeedSet(object->mdlId[0], 2.0f);
    Hu3DModelLayerSet(object->mdlId[5], 5);
    Hu3DModelAttrSet(object->mdlId[5], HU3D_MOTATTR_LOOP);
    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        if (playerIndex == 0) {
            modelIndex = object->mdlId[playerIndex];
            lightHandle = Hu3DLLightCreateV(modelIndex, &playerCameraRotation, &playerCameraScale, &playerWhiteColor);
            work->modelIds[playerIndex] = lightHandle;
            Hu3DLLightStaticSet(modelIndex, lightHandle, 1);
            Hu3DLLightPointSet(modelIndex, lightHandle, 80.0f, 0.95f, 3);
            Hu3DLLightColorSet(modelIndex, lightHandle, 0xFF, 0xF0, 0xDC, 0xFF);
            Hu3DLLightPosSet(
                modelIndex, lightHandle, playerCameraRotation.x, playerCameraRotation.y, playerCameraRotation.z, playerCameraScale.x, playerCameraScale.y, playerCameraScale.z);
        }
        else {
            modelIndex = object->mdlId[playerIndex];
            work->modelIds[playerIndex] = CopyModelLight(object->mdlId[0], work->modelIds[0], modelIndex);
        }
        ScaleModelMaterialLighting(modelIndex, 0.1f);
    }
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        work->playerObjects[playerIndex] = omAddObjEx(playerProcess, 0x40, 2, 4, -1, UpdatePlayerAnimationState);
        work->playerObjects[playerIndex]->work[0] = object->work[playerIndex + 1];
        work->playerObjects[playerIndex]->work[1] = work->teamIndex;
        work->playerObjects[playerIndex]->work[2] = playerIndex;
    }
    object->objFunc = UpdatePlayerAnimation;
}

void UpdatePlayerAnimation(OMOBJ *object)
{
    Mtx cameraMatrix;
    Vec playerPosition;
    Vec collisionPosition;
    Vec collisionNormal;
    Vec movementDirection;
    float movementX;
    float movementY;
    float movementZ;
    TeamPlayerState *work;
    s32 playerIndex;
    s32 movementState;

    work = object->data;
    if ((work->playerObjects[0] == NULL) || (work->playerStates[0] == NULL)) {
        return;
    }
    playerPosition.x = object->trans.x;
    playerPosition.y = object->trans.y;
    playerPosition.z = object->trans.z;
    switch (work->playerState) {
        case 0:
            object->rot.y = work->facingAngle = 180.0f;
            for (playerIndex = 0; playerIndex < 2; playerIndex++) {
                if ((work->playerStates[playerIndex]->leftActionPending != 0) && (Hu3DMotionEndCheck(object->mdlId[teamLeftModelIndices[playerIndex]]) != 0)) {
                    PlayTeamRouteEffect(playerIndex * 2, work->teamIndex);
                    Hu3DMotionTimeSet(object->mdlId[teamLeftModelIndices[playerIndex]], 0.0f);
                    if (work->teamIndex == 0) {
                        HuAudFXPlay(0x655);
                    }
                    else {
                        HuAudFXPlay(0x656);
                    }
                }
                if ((work->playerStates[playerIndex]->rightActionPending != 0) && (Hu3DMotionEndCheck(object->mdlId[teamRightModelIndices[playerIndex]]) != 0)) {
                    PlayTeamRouteEffect((playerIndex * 2) + 1, work->teamIndex);
                    Hu3DMotionTimeSet(object->mdlId[teamRightModelIndices[playerIndex]], 0.0f);
                    if (work->teamIndex == 0) {
                        HuAudFXPlay(0x655);
                    }
                    else {
                        HuAudFXPlay(0x656);
                    }
                }
            }
            if (playerPosition.z != object->trans.z) {
                work->movementSpeed = (VERSION_PAL) ? 3 : 2;
            }
            else {
                work->movementSpeed = 0;
            }
            if (getGameplayState() == 3) {
                work->playerState = 1;
            }
            break;
        case 1:
            if (playerPosition.z != object->trans.z) {
                work->movementSpeed = (VERSION_PAL) ? 3 : 2;
            }
            else {
                work->movementSpeed = 0;
            }
            if (getGameplayState() == 4) {
                work->playerState = 2;
            }
            break;
        case 2:
            for (playerIndex = 0; playerIndex < 2; playerIndex++) {
                if (work->playerStates[playerIndex]->leftActionPending != 0) {
                    work->laneSpeedLeft += 0.6f / (REFRESH_RATE_F * 0.1f);
                    if (Hu3DMotionEndCheck(object->mdlId[teamLeftModelIndices[playerIndex]]) != 0) {
                        PlayTeamRouteEffect(playerIndex * 2, work->teamIndex);
                        Hu3DMotionTimeSet(object->mdlId[teamLeftModelIndices[playerIndex]], 0.0f);
                        if (work->teamIndex == 0) {
                            HuAudFXPlay(0x655);
                        }
                        else {
                            HuAudFXPlay(0x656);
                        }
                    }
                }
                if (work->playerStates[playerIndex]->rightActionPending != 0) {
                    work->laneSpeedRight += 0.6f / (REFRESH_RATE_F * 0.1f);
                    if (Hu3DMotionEndCheck(object->mdlId[teamRightModelIndices[playerIndex]]) != 0) {
                        PlayTeamRouteEffect((playerIndex * 2) + 1, work->teamIndex);
                        Hu3DMotionTimeSet(object->mdlId[teamRightModelIndices[playerIndex]], 0.0f);
                        if (work->teamIndex == 0) {
                            HuAudFXPlay(0x655);
                        }
                        else {
                            HuAudFXPlay(0x656);
                        }
                    }
                }
            }
            GetRoutePosition(object->trans.x, object->trans.z, &movementDirection);
            work->inputVelocityX += (9.0f / REFRESH_RATE_F) * movementDirection.x;
            work->inputVelocityZ += (9.0f / REFRESH_RATE_F) * movementDirection.z;
#if VERSION_PAL
            work->laneSpeedLeft *= 0.989f;
            work->laneSpeedRight *= 0.989f;
            work->inputVelocityX *= 0.989f;
            work->inputVelocityZ *= 0.989f;
#else
            work->laneSpeedLeft *= 0.995f;
            work->laneSpeedRight *= 0.995f;
            work->inputVelocityX *= 0.99f;
            work->inputVelocityZ *= 0.99f;
#endif
            work->movementSpeed = work->laneSpeedLeft + work->laneSpeedRight;
#if VERSION_PAL
            if (work->movementSpeed < -6.0f) {
                work->movementSpeed = -6;
            }
            else if (work->movementSpeed > 6.0f) {
                work->movementSpeed = 6;
            }
#else
            if (work->movementSpeed < -5.0f) {
                work->movementSpeed = -5;
            }
            else if (work->movementSpeed > 5.0f) {
                work->movementSpeed = 5;
            }
#endif
            movementY = (15.0f / REFRESH_RATE_F) * (work->laneSpeedLeft - work->laneSpeedRight);
            if (movementY > 1200.0f / REFRESH_RATE_F) {
                movementY = 1200.0f / REFRESH_RATE_F;
            }
            else if (movementY < -1200.0f / REFRESH_RATE_F) {
                movementY = -1200.0f / REFRESH_RATE_F;
            }
            work->facingAngle += movementY;
            if (work->facingAngle > 360.0f) {
                work->facingAngle -= 360.0f;
            }
            else if (work->facingAngle < 0.0f) {
                work->facingAngle += 360.0f;
            }
            if (work->facingAngle > 270.0f) {
                work->facingAngle = 270.0f;
            }
            else if (work->facingAngle < 90.0f) {
                work->facingAngle = 90.0f;
            }
            object->rot.y = work->facingAngle;
            movementX = work->movementSpeed;
#if VERSION_PAL
            object->trans.x = object->trans.x + (2.4000000953674316 * (movementX * sind(work->facingAngle)));
            object->trans.z = object->trans.z + (2.4000000953674316 * (movementX * cosd(work->facingAngle)));
#else
            object->trans.x = object->trans.x + (2.0 * (movementX * sind(work->facingAngle)));
            object->trans.z = object->trans.z + (2.0 * (movementX * cosd(work->facingAngle)));
#endif
            object->trans.x += work->inputVelocityX;
            object->trans.z += work->inputVelocityZ;
            if (work->effectCooldown > 0) {
                work->effectCooldown--;
            }
            for (playerIndex = 0, movementState = 1; (movementState == 1) && (playerIndex < 6); playerIndex++) {
                movementState = 0;
                collisionPosition.x = object->trans.x;
                collisionPosition.y = object->trans.y;
                collisionPosition.z = object->trans.z;
                if (FindRouteSegmentForPosition(-1, &playerPosition, &collisionPosition, &collisionNormal, &movementDirection) != 0) {
                    movementState = 1;
                    playerPosition = collisionPosition;
                    object->trans.x = collisionNormal.x;
                    object->trans.y = collisionNormal.y;
                    object->trans.z = collisionNormal.z;
                    if ((work->effectCooldown == 0) && (getGameplayState() <= 4)) {
                        if (work->teamIndex == 0) {
                            HuAudFXPlay(0x65C);
                        }
                        else {
                            HuAudFXPlay(0x65D);
                        }
                    }
                    work->effectCooldown = 5;
                }
                collisionPosition.x = object->trans.x;
                collisionPosition.y = object->trans.y;
                collisionPosition.z = object->trans.z;
                if (FindPlayerRouteTarget(work->teamIndex, &playerPosition, &collisionPosition, &collisionNormal, 200.0f) != 0) {
                    movementState = 1;
                    playerPosition = collisionPosition;
                    object->trans.x = collisionNormal.x;
                    object->trans.y = collisionNormal.y;
                    object->trans.z = collisionNormal.z;
                }
            }
            if (work->stunTimer <= 0) {
                collisionPosition.x = object->trans.x;
                collisionPosition.y = object->trans.y;
                collisionPosition.z = object->trans.z;
                if (FindNearestRoutePosition(work->teamIndex, &collisionPosition, 200.0f) != 0) {
                    omVibrate(work->playerStates[0]->playerIndex, 0xC, 6, 6);
                    omVibrate(work->playerStates[1]->playerIndex, 0xC, 6, 6);
                    CharFXPlay(work->playerStates[0]->characterId, 0x123);
                    CharFXPlay(work->playerStates[1]->characterId, 0x123);
                    work->effectTimer = REFRESH_RATE * 2;
                    work->playerState = 3;
                    work->hitReactionStrength = 5.0f;
                    work->hitReactionAngle = 360.0f * ((frand() & 0x7F) / 127.0f);
                    work->frameCounter = 0;
                }
            }
            else {
                work->stunTimer--;
            }
            UpdatePlayerHitReaction(object);
            if (getGameplayState() >= 5) {
                work->playerState = 4;
            }
            break;
        case 3:
            work->laneSpeedLeft = 0.0f;
            work->laneSpeedRight = 0.0f;
            work->inputVelocityX = 0.0f;
            work->inputVelocityZ = 0.0f;
            work->movementSpeed = work->laneSpeedLeft + work->laneSpeedRight;
#if VERSION_PAL
            if (work->movementSpeed < -6.0f) {
                work->movementSpeed = -6;
            }
            else if (work->movementSpeed > 6.0f) {
                work->movementSpeed = 6;
            }
#else
            if (work->movementSpeed < -5.0f) {
                work->movementSpeed = -5;
            }
            else if (work->movementSpeed > 5.0f) {
                work->movementSpeed = 5;
            }
#endif
            movementX = work->movementSpeed / (300.0f / REFRESH_RATE_F);
            movementX = (300.0f / REFRESH_RATE_F) * (movementX * movementX);
#if VERSION_PAL
            object->trans.x = object->trans.x + (2.4000000953674316 * (movementX * sind(work->facingAngle)));
            object->trans.z = object->trans.z + (2.4000000953674316 * (movementX * cosd(work->facingAngle)));
#else
            object->trans.x = object->trans.x + (2.0 * (movementX * sind(work->facingAngle)));
            object->trans.z = object->trans.z + (2.0 * (movementX * cosd(work->facingAngle)));
#endif
            object->trans.x += work->inputVelocityX;
            object->trans.z += work->inputVelocityZ;
            for (playerIndex = 0, movementState = 1; (movementState == 1) && (playerIndex < 6); playerIndex++) {
                movementState = 0;
                collisionPosition.x = object->trans.x;
                collisionPosition.y = object->trans.y;
                collisionPosition.z = object->trans.z;
                if (FindRouteSegmentForPosition(-1, &playerPosition, &collisionPosition, &collisionNormal, &movementDirection) != 0) {
                    movementState = 1;
                    playerPosition = collisionPosition;
                    object->trans.x = collisionNormal.x;
                    object->trans.y = collisionNormal.y;
                    object->trans.z = collisionNormal.z;
                }
                collisionPosition.x = object->trans.x;
                collisionPosition.y = object->trans.y;
                collisionPosition.z = object->trans.z;
                if (FindPlayerRouteTarget(work->teamIndex, &playerPosition, &collisionPosition, &collisionNormal, 200.0f) != 0) {
                    movementState = 1;
                    playerPosition = collisionPosition;
                    object->trans.x = collisionNormal.x;
                    object->trans.y = collisionNormal.y;
                    object->trans.z = collisionNormal.z;
                }
            }
            if ((--work->effectTimer > 0) && ((work->frameCounter & 3) == 0)) {
                omVibrate(work->playerStates[0]->playerIndex, 0xC, 6, 6);
                omVibrate(work->playerStates[1]->playerIndex, 0xC, 6, 6);
            }
            if (work->frameCounter >= REFRESH_RATE_F * 2) {
                work->playerState = 2;
                work->stunTimer = REFRESH_RATE * 3;
                work->frameCounter = 0;
            }
            UpdatePlayerHitReaction(object);
            if (getGameplayState() >= 7) {
                work->stunTimer = 0;
                work->frameCounter = 0;
                work->playerState = 4;
                object->objFunc = UpdatePlayerController;
            }
            break;
        case 4:
            work->laneSpeedLeft *= 0.993f;
            work->laneSpeedRight *= 0.993f;
            work->inputVelocityX *= 0.993f;
            work->inputVelocityZ *= 0.993f;
            work->movementSpeed = work->laneSpeedLeft + work->laneSpeedRight;
#if VERSION_PAL
            if (work->movementSpeed < -6.0f) {
                work->movementSpeed = -6;
            }
            else if (work->movementSpeed > 6.0f) {
                work->movementSpeed = 6;
            }
#else
            if (work->movementSpeed < -5.0f) {
                work->movementSpeed = -5;
            }
            else if (work->movementSpeed > 5.0f) {
                work->movementSpeed = 5;
            }
#endif
            movementX = work->movementSpeed / (300.0f / REFRESH_RATE_F);
            movementX = (300.0f / REFRESH_RATE_F) * (movementX * movementX);
#if VERSION_PAL
            object->trans.x = object->trans.x + (2.4000000953674316 * (movementX * sind(work->facingAngle)));
            object->trans.z = object->trans.z + (2.4000000953674316 * (movementX * cosd(work->facingAngle)));
#else
            object->trans.x = object->trans.x + (2.0 * (movementX * sind(work->facingAngle)));
            object->trans.z = object->trans.z + (2.0 * (movementX * cosd(work->facingAngle)));
#endif
            object->trans.x += work->inputVelocityX;
            object->trans.z += work->inputVelocityZ;
            for (playerIndex = 0, movementState = 1; (movementState == 1) && (playerIndex < 6); playerIndex++) {
                movementState = 0;
                collisionPosition.x = object->trans.x;
                collisionPosition.y = object->trans.y;
                collisionPosition.z = object->trans.z;
                if (FindRouteSegmentForPosition(-1, &playerPosition, &collisionPosition, &collisionNormal, &movementDirection) != 0) {
                    movementState = 1;
                    playerPosition = collisionPosition;
                    object->trans.x = collisionNormal.x;
                    object->trans.y = collisionNormal.y;
                    object->trans.z = collisionNormal.z;
                }
                collisionPosition.x = object->trans.x;
                collisionPosition.y = object->trans.y;
                collisionPosition.z = object->trans.z;
                if (FindPlayerRouteTarget(work->teamIndex, &playerPosition, &collisionPosition, &collisionNormal, 200.0f) != 0) {
                    movementState = 1;
                    playerPosition = collisionPosition;
                    object->trans.x = collisionNormal.x;
                    object->trans.y = collisionNormal.y;
                    object->trans.z = collisionNormal.z;
                }
            }
            work->stunTimer = 0;
            if (getGameplayState() >= 7) {
                work->frameCounter = 0;
                work->playerState = 4;
                object->objFunc = UpdatePlayerController;
            }
            break;
        case 5:
            if (playerPosition.z != object->trans.z) {
                work->movementSpeed = (VERSION_PAL) ? 3 : 2;
            }
            else {
                work->movementSpeed = 0;
            }
            break;
        case 6:
            break;
        case 7:
            break;
    }
    movementX = work->movementSpeed / (300.0f / REFRESH_RATE_F);
    Hu3DMotionSpeedSet(object->mdlId[0], 2.0f * movementX);
    if (work->playerState == 3) {
        work->hitReactionAngle += (frand() & 3) + 2;
        object->rot.x = 0.0f;
        object->rot.z = 0.0f;
        work->hitReactionStrength = 5.0f - (5.0f * (0.5f * REFRESH_FREQ * work->frameCounter));
        movementZ = 7.0 * (work->hitReactionStrength * sind((10.0f * work->frameCounter)));
        object->rot.x = object->rot.x + (movementZ * sind(work->hitReactionAngle));
        object->rot.z = object->rot.z + (movementZ * cosd(work->hitReactionAngle));
    }
    if (((work->playerState == 3) || (work->stunTimer != 0)) && ((work->frameCounter & 1) != 0)) {
        for (playerIndex = 0; playerIndex < 7; playerIndex++) {
            Hu3DModelAttrSet(object->mdlId[playerIndex], HU3D_ATTR_DISPOFF);
        }
        for (playerIndex = 0; playerIndex < 2; playerIndex++) {
            if (work->playerObjects[playerIndex]->mdlId[0] != -1) {
                Hu3DModelAttrSet(*work->playerObjects[playerIndex]->mdlId, HU3D_ATTR_DISPOFF);
            }
        }
    }
    else {
        for (playerIndex = 0; playerIndex < 7; playerIndex++) {
            Hu3DModelAttrReset(object->mdlId[playerIndex], HU3D_ATTR_DISPOFF);
        }
        for (playerIndex = 0; playerIndex < 2; playerIndex++) {
            if (work->playerObjects[playerIndex]->mdlId[0] != -1) {
                Hu3DModelAttrReset(*work->playerObjects[playerIndex]->mdlId, HU3D_ATTR_DISPOFF);
            }
        }
    }
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        work->playerStates[playerIndex]->actionState = work->playerState;
    }
    object->trans.y = calculateTeamMarkerOpacity(work->teamIndex, object->trans.x, object->trans.z);
    mtxRot(cameraMatrix, object->rot.x, object->rot.y, object->rot.z);
    cameraMatrix[1][0] *= object->scale.y;
    cameraMatrix[1][1] *= object->scale.y;
    cameraMatrix[1][2] *= object->scale.z;
    collisionPosition.x = 0.0f;
    collisionPosition.y = 0.0f;
    collisionPosition.z = 100.0f;
    TransformRoutePosition(cameraMatrix, &collisionPosition);
    collisionPosition.x += object->trans.x;
    collisionPosition.y += object->trans.y;
    collisionPosition.z += object->trans.z;
    SetBannerAnimationOffset(work->teamIndex, collisionPosition.x, collisionPosition.y, collisionPosition.z, 180.0f + object->rot.y);
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        collisionPosition = playerCollisionOffsets[playerIndex];
        TransformRoutePosition(cameraMatrix, &collisionPosition);
        work->playerObjects[playerIndex]->trans.x = object->trans.x + collisionPosition.x;
        work->playerObjects[playerIndex]->trans.y = object->trans.y + collisionPosition.y;
        work->playerObjects[playerIndex]->trans.z = object->trans.z + collisionPosition.z;
        work->playerObjects[playerIndex]->rot.x = object->rot.x;
        work->playerObjects[playerIndex]->rot.y = object->rot.y;
        work->playerObjects[playerIndex]->rot.z = object->rot.z;
    }
    for (playerIndex = 0; playerIndex < 7; playerIndex++) {
        Hu3DModelPosSet(object->mdlId[playerIndex], object->trans.x, object->trans.y, object->trans.z);
        Hu3DModelRotSet(object->mdlId[playerIndex], object->rot.x, object->rot.y, object->rot.z);
    }
    UpdatePlayerMovement(object);
    if ((getResultCameraState() == 0) && (object->objFunc != UpdatePlayerController) && ((work->playerState == 2) || (work->playerState == 3))) {
        UpdatePlayerCollision(object);
    }
    work->frameCounter++;
}

s32 teamPlayerModelIndices[2] = { 1, 3 };
s32 teamOpponentModelIndices[2] = { 2, 4 };
s32 playerMotionAssetIds[4] = { 0x3A002F, 0x3A0037, 0x3A003F, 0x3A0047 };
s32 playerMotionDurations[4] = { REFRESH_RATE / 4, REFRESH_RATE / 6, REFRESH_RATE / 10, REFRESH_RATE / 12 };

void UpdatePlayerController(OMOBJ *object)
{
    Mtx playerMatrix;
    Vec playerPosition;
    s32 modelIndex;
    TeamPlayerState *work;
    s32 animationIndex;
    s32 playerState;

    work = object->data;
    switch (work->playerState) {
        case 4:
            playerState = getWinningTeamResult();
            if (playerState == 3) {
                break;
            }
            if (playerState == work->teamIndex) {
                for (modelIndex = 0; modelIndex < 2; modelIndex++) {
                    Hu3DModelCameraSet(*work->playerObjects[modelIndex]->mdlId, 1);
                    Hu3DModelLayerSet(*work->playerObjects[modelIndex]->mdlId, 3);
                    ScaleModelMaterialLighting(work->playerObjects[modelIndex]->mdlId[0], 5.0f);
                }
                for (modelIndex = 0; modelIndex < 7; modelIndex++) {
                    Hu3DModelCameraSet(object->mdlId[modelIndex], 1);
                    Hu3DModelLayerSet(object->mdlId[modelIndex], 3);
                    if (modelIndex < 5) {
                        ScaleModelMaterialLighting(object->mdlId[modelIndex], 5.0f);
                    }
                }
                Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
                for (modelIndex = 0; modelIndex < 2; modelIndex++) {
                    work->playerObjects[modelIndex]->mdlId[1] = Hu3DModelLink(*work->playerObjects[modelIndex]->mdlId);
                    Hu3DModelCameraSet(work->playerObjects[modelIndex]->mdlId[1], 1);
                    Hu3DModelLayerSet(work->playerObjects[modelIndex]->mdlId[1], 1);
                }
                for (modelIndex = 0; modelIndex < 7; modelIndex++) {
                    object->mdlId[modelIndex + 7] = Hu3DModelLink(object->mdlId[modelIndex]);
                    Hu3DModelCameraSet(object->mdlId[modelIndex + 7], 1);
                    Hu3DModelLayerSet(object->mdlId[modelIndex + 7], 1);
                }
                object->trans.x = object->trans.y = object->trans.z = 0.0f;
                object->rot.x = object->rot.z = 0.0f;
                object->rot.y = 180.0f;
                work->playerState = 5;
                break;
            }
            for (modelIndex = 0; modelIndex < 2; modelIndex++) {
                Hu3DModelAttrSet(work->playerObjects[modelIndex]->mdlId[0], HU3D_ATTR_DISPOFF);
                work->playerObjects[modelIndex]->objFunc = NULL;
            }
            for (modelIndex = 0; modelIndex < 7; modelIndex++) {
                Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
            }
            object->objFunc = NULL;
            return;
        case 5:
            for (modelIndex = 0; modelIndex < 2; modelIndex++) {
                animationIndex = teamPlayerModelIndices[modelIndex];
                if ((work->playerStates[modelIndex]->leftActionPending != 0) && (Hu3DMotionEndCheck(object->mdlId[animationIndex]) != 0)) {
                    PlayTeamRouteEffect(modelIndex * 2, work->teamIndex);
                    Hu3DMotionTimeSet(object->mdlId[animationIndex], 0.0f);
                    if (work->teamIndex == 0) {
                        HuAudFXPlay(0x655);
                    }
                    else {
                        HuAudFXPlay(0x656);
                    }
                }
                animationIndex = teamOpponentModelIndices[modelIndex];
                if ((work->playerStates[modelIndex]->rightActionPending != 0) && (Hu3DMotionEndCheck(object->mdlId[animationIndex]) != 0)) {
                    PlayTeamRouteEffect((modelIndex * 2) + 1, work->teamIndex);
                    Hu3DMotionTimeSet(object->mdlId[animationIndex], 0.0f);
                    if (work->teamIndex == 0) {
                        HuAudFXPlay(0x655);
                    }
                    else {
                        HuAudFXPlay(0x656);
                    }
                }
            }
            if (getGameplayState() >= 0xC) {
                work->playerState = 6;
            }
            break;
        case 6:
        case 7:
            break;
    }
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        work->playerStates[modelIndex]->actionState = work->playerState;
    }
    mtxRot(playerMatrix, object->rot.x, object->rot.y, object->rot.z);
    playerMatrix[1][0] *= object->scale.y;
    playerMatrix[1][1] *= object->scale.y;
    playerMatrix[1][2] *= object->scale.z;
    playerPosition.x = 0.0f;
    playerPosition.y = 0.0f;
    playerPosition.z = 100.0f;
    TransformRoutePosition(playerMatrix, &playerPosition);
    playerPosition.x += object->trans.x;
    playerPosition.y += object->trans.y;
    playerPosition.z += object->trans.z;
    SetBannerAnimationOffset(work->teamIndex, playerPosition.x, playerPosition.y, playerPosition.z, 180.0f + object->rot.y);
    object->trans.y = calculateTeamMarkerOpacity(work->teamIndex, object->trans.x, object->trans.z);
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        playerPosition = playerCollisionOffsets[modelIndex];
        TransformRoutePosition(playerMatrix, &playerPosition);
        work->playerObjects[modelIndex]->trans.x = object->trans.x + playerPosition.x;
        work->playerObjects[modelIndex]->trans.y = object->trans.y + playerPosition.y;
        work->playerObjects[modelIndex]->trans.z = object->trans.z + playerPosition.z;
        work->playerObjects[modelIndex]->rot.x = object->rot.x;
        work->playerObjects[modelIndex]->rot.y = object->rot.y;
        work->playerObjects[modelIndex]->rot.z = object->rot.z;
        if (work->playerObjects[modelIndex]->mdlId[1] != -1) {
            Hu3DModelPosSet(
                work->playerObjects[modelIndex]->mdlId[1], -work->playerObjects[modelIndex]->trans.x, work->playerObjects[modelIndex]->trans.y, work->playerObjects[modelIndex]->trans.z);
            Hu3DModelRotSet(
                work->playerObjects[modelIndex]->mdlId[1], work->playerObjects[modelIndex]->rot.x, -work->playerObjects[modelIndex]->rot.y, work->playerObjects[modelIndex]->rot.z);
        }
    }
    for (modelIndex = 0; modelIndex < 0xE; modelIndex++) {
        if (object->mdlId[modelIndex] != -1) {
            if (modelIndex < 7) {
                Hu3DModelPosSet(object->mdlId[modelIndex], object->trans.x, object->trans.y, object->trans.z);
                Hu3DModelRotSet(object->mdlId[modelIndex], object->rot.x, object->rot.y, object->rot.z);
            }
            else {
                Hu3DModelPosSet(object->mdlId[modelIndex], -object->trans.x, object->trans.y, object->trans.z);
                Hu3DModelRotSet(object->mdlId[modelIndex], object->rot.x, -object->rot.y, object->rot.z);
            }
        }
    }
    work->facingAngle = 200.0f;
    UpdatePlayerMovement(object);
    work->frameCounter++;
}

void UpdatePlayerCameraTarget(OMOBJ *object)
{
    s32 playerIndex;
    TeamPlayerState *work = object->data;
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        EmptyPlayerObjectCallback(work->playerObjects[playerIndex]);
    }
}

s32 FindCharacterModelIndex(s32 teamIndex, s32 *modelIds)
{
    s32 modelIndex;
    TeamPlayerState *teamWork;
    OMOBJ *teamObject;

    teamObject = teamPlayerObjects[teamIndex];
    teamWork = &teamPlayerWork[teamIndex];
    if ((teamObject == NULL) || (teamWork == NULL)) {
        return -1;
    }
    if ((teamWork->playerObjects[0] == NULL) || (teamWork->playerStates[0] == NULL)) {
        return -1;
    }
    for (modelIndex = 0; modelIndex < 7; modelIndex++) {
        if (modelIndex < 5) {
            modelIds[modelIndex] = teamObject->mdlId[modelIndex];
        }
        else if (modelIndex < 6) {
            modelIds[modelIndex] = teamWork->playerObjects[0]->mdlId[0];
        }
        else {
            modelIds[modelIndex] = teamWork->playerObjects[1]->mdlId[0];
        }
    }
    return modelIndex;
}

void GetTeamCharacterIds(s32 teamIndex, s32 *characterIds)
{
    s32 playerIndex;
    TeamPlayerState *teamWork;
    OMOBJ *teamObject;

    teamObject = teamPlayerObjects[teamIndex];
    teamWork = &teamPlayerWork[teamIndex];
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        characterIds[playerIndex] = 0;
    }
    if ((teamObject == NULL) || (teamWork == NULL)) {
        return;
    }
    if ((teamWork->playerObjects[0] == NULL) || (teamWork->playerStates[0] == NULL)) {
        return;
    }
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        characterIds[playerIndex] = teamWork->playerStates[playerIndex]->characterId;
    }
}

void GetTeamPlayerIndices(s32 teamIndex, s32 *playerIndices)
{
    s32 playerIndex;
    TeamPlayerState *teamWork;
    OMOBJ *teamObject;

    teamObject = teamPlayerObjects[teamIndex];
    teamWork = &teamPlayerWork[teamIndex];
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        playerIndices[playerIndex] = 0;
    }
    if ((teamObject == NULL) || (teamWork == NULL)) {
        return;
    }
    if ((teamWork->playerObjects[0] == NULL) || (teamWork->playerStates[0] == NULL)) {
        return;
    }
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        playerIndices[playerIndex] = teamWork->playerStates[playerIndex]->playerIndex;
    }
}

void SetPlayerModelOpacity(float modelDepthFactor)
{
    float blendProgress;
    OMOBJ *object;
    s32 playerIndex;

    if (getGameplayState() < 4) {
        for (playerIndex = 0; playerIndex < 2; playerIndex++) {
            object = teamPlayerObjects[playerIndex];
            if (object) {
                object->trans.z = 1900.0f + (-1800.0f * modelDepthFactor);
            }
        }
        return;
    }
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        object = teamPlayerObjects[playerIndex];
        if (object) {
            object->trans.z = -1800.0f * modelDepthFactor;
            if (object->trans.z < -1500.0f) {
                blendProgress = (object->trans.z - -1500.0f) / 300.0f;
                object->rot.y = 180.0f - (30.0f * blendProgress);
            }
        }
    }
}

s32 GetTeamComputerPlayerCount(s32 teamIndex)
{
    TeamPlayerState *work;
    OMOBJ *object;

    object = teamPlayerObjects[teamIndex];
    work = object->data;
    if (work->computerPlayerMask == 3) {
        return 1;
    }
    return 0;
}

void UpdatePlayerMovement(OMOBJ *object)
{
    Mtx lightMatrix;
    Vec lightPosition;
    Vec lightDirection;
    TeamPlayerState *work;
    s32 playerIndex;

    work = object->data;
    mtxRot(lightMatrix, 0.0f, work->facingAngle, 0.0f);
    lightPosition = playerCameraRotation;
    TransformRoutePosition(lightMatrix, &lightPosition);
    lightPosition.x += object->trans.x;
    lightPosition.y += object->trans.y;
    lightPosition.z += object->trans.z;
    lightDirection.x = lightDirection.z = 0.0f;
    lightDirection.y = 1.0f;
    Hu3DLLightPosSet(object->mdlId[0], work->modelIds[0], lightPosition.x, lightPosition.y, lightPosition.z, lightDirection.x, lightDirection.y, lightDirection.z);
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        if (*work->playerObjects[playerIndex]->mdlId != -1) {
            Hu3DLLightPosSet(work->playerObjects[playerIndex]->mdlId[0], work->playerStates[playerIndex]->lightHandle, lightPosition.x, lightPosition.y, lightPosition.z, lightDirection.x, lightDirection.y, lightDirection.z);
        }
    }
}

void UpdatePlayerCollision(OMOBJ *object)
{
    Vec cameraTarget;
    float segmentProgress;
    float cameraHeading;
    float cameraEdgeWidth;
    float cameraXOffset;
    RouteSegment *trackSegment;
    TeamPlayerState *work;
    s32 segmentIndex;

    work = object->data;
    segmentIndex = CalculateRouteSegmentParameter(object->trans.x, object->trans.y, object->trans.z, &segmentProgress);
    if ((segmentIndex < 0) || (segmentIndex >= (routeSegmentCount - 1))) {
        return;
    }
    trackSegment = &routeSegments[segmentIndex];
    cameraTarget.x = trackSegment->centerlinePosition.x + (segmentProgress * trackSegment->centerlineTangentX);
    cameraTarget.y = 275.0f + (trackSegment->centerlinePosition.y + (segmentProgress * trackSegment->centerlineTangentY));
    cameraTarget.z = trackSegment->centerlinePosition.z + (segmentProgress * trackSegment->centerlineTangentZ);
    cameraXOffset = object->trans.x - cameraTarget.x;
    cameraTarget.x += 0.4f * cameraXOffset;
    if (cameraXOffset < 0.0f) {
        cameraEdgeWidth = fabs(trackSegment->leftEdgeWidthStart + (segmentProgress * (trackSegment->leftEdgeWidthEnd - trackSegment->leftEdgeWidthStart)));
    }
    else {
        cameraEdgeWidth = fabs(trackSegment->rightEdgeWidthStart + (segmentProgress * (trackSegment->rightEdgeWidthEnd - trackSegment->rightEdgeWidthStart)));
    }
    CenterM[work->teamIndex].x = CenterM[work->teamIndex].x + (0.06666667f * (cameraTarget.x - CenterM[work->teamIndex].x));
    CenterM[work->teamIndex].y = CenterM[work->teamIndex].y + (0.06666667f * (cameraTarget.y - CenterM[work->teamIndex].y));
    CenterM[work->teamIndex].z = CenterM[work->teamIndex].z + (0.06666667f * (cameraTarget.z - CenterM[work->teamIndex].z));
    cameraHeading = trackSegment->centerlineHeading;
    if (cameraHeading < 0.0f) {
        cameraHeading += 360.0f;
    }
    cameraHeading = 180.0f + (0.25f * (cameraHeading - 180.0f));
    if (cameraEdgeWidth != 0.0f) {
        cameraEdgeWidth = cameraXOffset / cameraEdgeWidth;
    }
    cameraHeading -= 30.0f * cameraEdgeWidth;
    CRotM[work->teamIndex].y = InterpolateAngle(0.020000001f, CRotM[work->teamIndex].y, 180.0f + cameraHeading);
}

void UpdatePlayerHitReaction(OMOBJ *object)
{
    float trackProgress;
    float cameraTrackProgress;
    RouteSegment *endSegment;
    TeamPlayerState *work;
    RouteSegment *startSegment;

    work = object->data;
    endSegment = &routeSegments[1];
    startSegment = &routeSegments[27];
    trackProgress = startSegment->centerlinePositionPtr->z - endSegment->centerlinePositionPtr->z;
    cameraTrackProgress = object->trans.z - endSegment->centerlinePositionPtr->z;
    SetTeamProgress(work->teamIndex, cameraTrackProgress / trackProgress);
}

void UpdatePlayerAnimationState(OMOBJ *object)
{
    PlayerCharacterState *playerState;
    s32 motionIndex;
    s32 lightHandle;
    s32 characterId;
    TeamPlayerState *teamWork;

    playerState = &playerWork[object->work[0]];
    object->data = playerState;
    teamWork = &teamPlayerWork[object->work[1]];
    teamPlayerWork[object->work[1]].playerStates[object->work[2]] = playerState;
    playerState->playerIndex = object->work[0];
    playerState->teamIndex = object->work[1];
    playerState->controllerIndex = GWPlayerCfg[playerState->playerIndex].pad_idx;
    playerState->characterId = GWPlayerCfg[playerState->playerIndex].character;
    playerState->teamSlot = object->work[2];
    playerState->isComputer = GWPlayerCfg[playerState->playerIndex].iscom;
    playerState->difficulty = GWPlayerCfg[playerState->playerIndex].diff;
    playerState->frameCounter = 0;
    playerState->actionState = 0;
    playerState->actionTimer = 0;
    playerState->leftActionPending = 0;
    playerState->rightActionPending = 0;
    playerState->leftActionRequested = 0;
    playerState->rightActionRequested = 0;
    playerState->currentButtons = playerState->pressedButtons = 0;
    playerState->previousButtons = playerState->previousPressedButtons = 0;
    playerState->playerStateTimer = 0;
    playerState->cameraTargetAge = 0;
    playerState->cameraUpdateInterval = playerMotionDurations[playerState->difficulty];
    playerState->cameraUpdateTimer = frand() % playerState->cameraUpdateInterval;
    playerState->cameraUpdateJitter = frand() % 7;
    playerState->cameraTargetSegmentIndex = -1;
    playerState->cameraTargetX = playerState->cameraTargetY = playerState->cameraTargetZ = 0.0f;
    if (playerState->isComputer != 0) {
        teamWork->computerPlayerMask |= 1 << playerState->teamSlot;
    }
    characterId = playerState->characterId;
    object->mdlId[0] = CharModelCreate(characterId, 4);
    Hu3DModelLayerSet(object->mdlId[0], 2);
    if (playerState->teamIndex == 0) {
        Hu3DModelCameraSet(object->mdlId[0], 1);
    }
    else {
        Hu3DModelCameraSet(object->mdlId[0], 2);
    }
    if (playerState->teamSlot != 0) {
        ScaleModelMaterialLighting(object->mdlId[0], 0.3f);
    }
    else {
        ScaleModelMaterialLighting(object->mdlId[0], 0.3f);
    }
    CharModelStepFxSet(characterId, 3);
    for (motionIndex = 0; motionIndex < 4; motionIndex++) {
        object->mtnId[motionIndex] = Hu3DJointMotionFile(object->mdlId[0], playerState->characterId + playerMotionAssetIds[motionIndex]);
    }
    playerState->motionIndex = 0;
    CharMotionSet(playerState->characterId, object->mtnId[0]);
    CharMotionDataClose(characterId);
    lightHandle = Hu3DLLightCreateV(object->mdlId[0], &playerCameraRotation, &playerCameraScale, &playerWhiteColor);
    playerState->lightHandle = lightHandle;
    Hu3DLLightStaticSet(object->mdlId[0], lightHandle, 1);
    if (playerState->teamSlot == 0) {
        Hu3DLLightPointSet(object->mdlId[0], lightHandle, 80.0f, 0.95f, 3);
    }
    else {
        Hu3DLLightPointSet(object->mdlId[0], lightHandle, 80.0f, 0.95f, 3);
    }
    Hu3DLLightColorSet(object->mdlId[0], lightHandle, 0xFF, 0xFF, 0xFF, 0xFF);
    Hu3DLLightPosSet(
        object->mdlId[0], lightHandle, playerCameraRotation.x, playerCameraRotation.y, playerCameraRotation.z, playerCameraScale.x, playerCameraScale.y, playerCameraScale.z);
    object->objFunc = UpdatePlayerCamera;
    (void)motionIndex;
}

void UpdatePlayerCamera(OMOBJ *object)
{
    PlayerCharacterState *playerState;
    s32 motionIndex;
    TeamPlayerState *teamWork;
    OMOBJ *teamObject;

    playerState = object->data;
    teamWork = &teamPlayerWork[playerState->teamIndex];
    teamObject = teamPlayerObjects[playerState->teamIndex];
    playerState->currentButtons = HuPadBtn[playerState->controllerIndex];
    playerState->pressedButtons = HuPadBtnDown[playerState->controllerIndex];
    if ((playerState->isComputer != 0) && (playerState->actionState == 2)) {
        if (teamWork->computerPlayerMask == 3) {
            UpdatePlayerRouteMotion(object);
        }
        else {
            UpdatePlayerRoutePosition(object);
        }
    }
    playerState->leftActionRequested = playerState->rightActionRequested = 0;
    if (playerState->leftActionPending != 0) {
        playerState->leftActionRequested = 1;
    }
    if (playerState->rightActionPending != 0) {
        playerState->rightActionRequested = 1;
    }
    playerState->leftActionPending = playerState->rightActionPending = 0;
    switch (playerState->actionState) {
        case 0:
            if (((playerState->frameCounter & 0xF) == 0) && (teamObject->trans.z > 100.0f)) {
                playerState->leftActionPending = 1;
                playerState->rightActionPending = 1;
            }
            motionIndex = 1;
            if (motionIndex != playerState->motionIndex) {
                playerState->motionIndex = motionIndex;
                CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, 0x40000001);
            }
            break;
        case 1:
            motionIndex = 0;
            if ((motionIndex != playerState->motionIndex) && (CharMotionShiftIDGet(playerState->characterId) == -1)) {
                playerState->motionIndex = motionIndex;
                CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, 0x40000001);
                playerState->actionTimer = 0;
            }
            break;
        case 2:
            if (getResultCameraState() == 0) {
                if ((playerState->pressedButtons & 0x100) != 0) {
                    playerState->leftActionPending = 1;
                }
                if ((playerState->pressedButtons & 0x200) != 0) {
                    playerState->rightActionPending = 1;
                }
            }
            playerState->previousButtons = playerState->currentButtons;
            playerState->previousPressedButtons = playerState->pressedButtons;
            if ((playerState->leftActionPending != 0) || (playerState->rightActionPending != 0)) {
                motionIndex = 1;
            }
            else {
                motionIndex = 0;
            }
            if ((CharMotionShiftIDGet(playerState->characterId) == -1)
                && ((motionIndex != playerState->motionIndex) || (CharMotionEndCheck(playerState->characterId) == 1))) {
                if (playerState->motionIndex == motionIndex) {
                    CharMotionTimeSet(playerState->characterId, 0.0f);
                }
                else {
                    playerState->motionIndex = motionIndex;
                    CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, 0);
                }
            }
            break;
        case 3:
            motionIndex = 2;
            if ((motionIndex != playerState->motionIndex) && (CharMotionShiftIDGet(playerState->characterId) == -1)) {
                playerState->motionIndex = motionIndex;
                CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, 0x40000001);
                playerState->actionTimer = 0;
            }
            if (playerState->motionIndex == 2) {
                if (++playerState->actionTimer >= REFRESH_RATE_F * 4) {
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                }
            }
            break;
        case 4:
            motionIndex = 0;
            if ((motionIndex != playerState->motionIndex) && (CharMotionShiftIDGet(playerState->characterId) == -1)) {
                playerState->motionIndex = motionIndex;
                if (object->mdlId[1] != -1) {
                    Hu3DMotionShiftSet(object->mdlId[1], object->mtnId[playerState->motionIndex], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                }
                else {
                    CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, 0x40000001);
                }
                playerState->actionTimer = 0;
            }
            break;
        case 5:
            if (object->trans.z > -1300.0f) {
                if ((playerState->frameCounter & 0xF) == 0) {
                    playerState->leftActionPending = 1;
                    playerState->rightActionPending = 1;
                }
                motionIndex = 1;
            }
            else {
                motionIndex = 0;
            }
            if (playerState->motionIndex == 1) {
                Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
            }
            if ((CharMotionShiftIDGet(playerState->characterId) == -1) && (motionIndex != playerState->motionIndex)) {
                playerState->motionIndex = motionIndex;
                CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, 0x40000001);
            }
            break;
        case 6:
            motionIndex = 3;
            if ((motionIndex != playerState->motionIndex) && (CharMotionShiftIDGet(playerState->characterId) == -1)) {
                playerState->motionIndex = motionIndex;
                CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, 0);
                CharFXPlay(playerState->characterId, 0x124);
                playerState->actionTimer = 0;
            }
            break;
        case 7:
            break;
    }
    playerState->frameCounter++;
}

void EmptyPlayerObjectCallback(OMOBJ *object) { }

void UpdatePlayerRoutePosition(OMOBJ *object)
{
    TeamPlayerState *teamWork;
    PlayerCharacterState *playerState;
    PlayerCharacterState *partnerState;

    playerState = object->data;
    teamWork = &teamPlayerWork[playerState->teamIndex];
    if (teamWork->computerPlayerMask == 1) {
        partnerState = teamWork->playerStates[1];
    }
    else {
        partnerState = teamWork->playerStates[0];
    }
    playerState->currentButtons = partnerState->previousButtons;
    playerState->pressedButtons = partnerState->previousPressedButtons;
}

void UpdatePlayerRouteMotion(OMOBJ *object)
{
    float distanceX;
    float distanceZ;
    float routeDistance;
    float movementSpeed;
    float movementDirection;
    PlayerCharacterState *playerState;
    RouteSegment *playerIndex;
    OMOBJ *routeSegmentIndex;
    TeamPlayerState *teamWork;
    s32 playerObject;
    s32 routeSegment;
    s32 targetLaneIndex;

    playerState = object->data;
    teamWork = &teamPlayerWork[playerState->teamIndex];
    routeSegmentIndex = teamPlayerObjects[playerState->teamIndex];
    playerState->currentButtons = 0;
    playerState->pressedButtons = 0;
    routeSegment = CalculateRouteSegmentParameter(routeSegmentIndex->trans.x, routeSegmentIndex->trans.y, routeSegmentIndex->trans.z, NULL);
    if ((playerState->cameraTargetSegmentIndex != routeSegment) || (playerState->cameraTargetAge > REFRESH_RATE_F * 3)) {
        playerState->cameraTargetSegmentIndex = routeSegment;
        switch (playerState->difficulty) {
            case 0:
                playerObject = 2;
                break;
            case 1:
                playerObject = 2;
                break;
            case 2:
                playerObject = 3;
                break;
            case 3:
                playerObject = 4;
                break;
        }
        if ((playerState->cameraTargetSegmentIndex >= 0) && (playerState->cameraTargetSegmentIndex < (routeSegmentCount - playerObject)) && (playerState->cameraTargetSegmentIndex <= (routeSegmentCount - 2))) {
            routeSegment = playerState->cameraTargetSegmentIndex + playerObject;
            playerIndex = &routeSegments[routeSegment];
            distanceX = 0.5f * (playerIndex->leftEdgeWidthEnd + playerIndex->rightEdgeWidthEnd);
            distanceZ = (0.6f - (0.1f * playerState->difficulty)) - (0.1f * playerState->difficulty * ((frand() & 0x7F) / 127.0f));
            if (distanceZ > 1.0f) {
                distanceZ = 1.0f;
            }
            else if (distanceZ < 0.0f) {
                distanceZ = 0.0f;
            }
            distanceX *= distanceZ;
            if ((frand() & 1) != 0) {
                distanceX *= -1.0f;
            }
            playerState->cameraTargetX = playerIndex->centerlinePositionPtr->x + distanceX;
            playerState->cameraTargetY = playerIndex->centerlinePositionPtr->y;
            playerState->cameraTargetZ = playerIndex->centerlinePositionPtr->z;
        }
        else {
            playerIndex = &routeSegments[routeSegmentCount - 1];
            playerState->cameraTargetX = playerIndex->centerlinePositionPtr->x;
            playerState->cameraTargetY = playerIndex->centerlinePositionPtr->y;
            playerState->cameraTargetZ = playerIndex->centerlinePositionPtr->z;
        }
        playerState->cameraTargetAge = 0;
    }
    targetLaneIndex = playerState->cameraUpdateInterval + playerState->cameraUpdateJitter;
    if (teamWork->stunTimer != 0) {
        targetLaneIndex = (VERSION_PAL) ? 4 : 5;
    }
    if (++playerState->cameraUpdateTimer > targetLaneIndex) {
        playerState->cameraUpdateTimer = 0;
        playerState->cameraUpdateJitter = frand() % 7;
        movementSpeed = playerState->cameraTargetX - routeSegmentIndex->trans.x;
        movementDirection = playerState->cameraTargetZ - routeSegmentIndex->trans.z;
        distanceX = InterpolateRouteValue(atan2d(movementSpeed, movementDirection), teamWork->facingAngle);
        if (fabs(distanceX) < (30.0f - (5.0f * playerState->difficulty))) {
            if (playerState->difficulty > 2) {
                routeDistance = (15.0f / REFRESH_RATE_F) * (teamWork->laneSpeedLeft - teamWork->laneSpeedRight);
                if (routeDistance > 600.0f / REFRESH_RATE_F) {
                    playerState->pressedButtons |= 0x100;
                }
                else if (routeDistance < -600.0f / REFRESH_RATE_F) {
                    playerState->pressedButtons |= 0x200;
                }
                else {
                    playerState->pressedButtons |= 0x300;
                }
            }
            else {
                playerState->pressedButtons |= 0x300;
            }
        }
        else if (distanceX < 0.0f) {
            playerState->pressedButtons |= 0x200;
        }
        else {
            playerState->pressedButtons |= 0x100;
        }
    }
    playerState->playerStateTimer++;
    playerState->cameraTargetAge++;
}

void InitializePlayerModels(void)
{
    Vec routePoint;
    float routeTangent;
    float routeNormal;
    float rightEdgePoint;
    float leftEdgePoint;
    float routePosition;
    float routeDirection;
    float segmentLength;
    float routeWidth;
    float routeHeight;
    float leftEdgeDistance;
    float rightEdgeDistance;
    float segmentHeading;
    float routeAngle;
    float routeProgress;
    float laneWidth;
    float curveDistance;
    float routeDistance;
    float curveProgress;
    float leftEdgeHeading;
    float rightEdgeHeading;
    RouteSegment *routeSegment;
    s32 routePointIndex;
    RouteCollisionState *segmentIndex;

    routeSegmentCount = 0x1D;
    routeCenterline = HuMemDirectMallocNum(HEAP_HEAP, routeSegmentCount * sizeof(Vec), HU_MEMNUM_OVL);
    memset(routeCenterline, 0, routeSegmentCount * sizeof(Vec));
    for (routePointIndex = 0; routePointIndex < routeSegmentCount; routePointIndex++) {
        routeCenterline[routePointIndex].x = 0.5f * (leftRouteEdgePoints[routePointIndex].x + rightRouteEdgePoints[routePointIndex].x);
        routeCenterline[routePointIndex].y = 0.5f * (leftRouteEdgePoints[routePointIndex].y + rightRouteEdgePoints[routePointIndex].y);
        routeCenterline[routePointIndex].z = 0.5f * (leftRouteEdgePoints[routePointIndex].z + rightRouteEdgePoints[routePointIndex].z);
    }
    leftRouteBoundary = HuMemDirectMallocNum(HEAP_HEAP, routeSegmentCount * sizeof(Vec), HU_MEMNUM_OVL);
    memset(leftRouteBoundary, 0, routeSegmentCount * sizeof(Vec));
    for (routePointIndex = 0; routePointIndex < routeSegmentCount; routePointIndex++) {
        VECSubtract(&routeCenterline[routePointIndex], &leftRouteEdgePoints[routePointIndex], &leftRouteBoundary[routePointIndex]);
        VECNormalize(&leftRouteBoundary[routePointIndex], &leftRouteBoundary[routePointIndex]);
        VECScale(&leftRouteBoundary[routePointIndex], &leftRouteBoundary[routePointIndex], 200.0f);
        VECAdd(&leftRouteBoundary[routePointIndex], &leftRouteEdgePoints[routePointIndex], &leftRouteBoundary[routePointIndex]);
    }
    rightRouteBoundary = HuMemDirectMallocNum(HEAP_HEAP, routeSegmentCount * sizeof(Vec), HU_MEMNUM_OVL);
    memset(rightRouteBoundary, 0, routeSegmentCount * sizeof(Vec));
    for (routePointIndex = 0; routePointIndex < routeSegmentCount; routePointIndex++) {
        VECSubtract(&routeCenterline[routePointIndex], &rightRouteEdgePoints[routePointIndex], &rightRouteBoundary[routePointIndex]);
        VECNormalize(&rightRouteBoundary[routePointIndex], &rightRouteBoundary[routePointIndex]);
        VECScale(&rightRouteBoundary[routePointIndex], &rightRouteBoundary[routePointIndex], 200.0f);
        VECAdd(&rightRouteBoundary[routePointIndex], &rightRouteEdgePoints[routePointIndex], &rightRouteBoundary[routePointIndex]);
    }
    routeSegment = routeSegments = HuMemDirectMallocNum(HEAP_HEAP, routeSegmentCount * sizeof(RouteSegment), HU_MEMNUM_OVL);
    memset(routeSegments, 0, routeSegmentCount * sizeof(RouteSegment));
    for (routePointIndex = 0; routePointIndex < routeSegmentCount; routePointIndex++, routeSegment++) {
        routeSegment->segmentIndex = routePointIndex;
        routeSegment->nextSegmentIndex = routePointIndex + 1;
        routeSegment->previousSegmentIndex = routePointIndex - 1;
        if (routeSegment->nextSegmentIndex >= routeSegmentCount) {
            routeSegment->nextSegmentIndex = -1;
        }
        if (routeSegment->previousSegmentIndex >= routeSegmentCount) {
            routeSegment->previousSegmentIndex = -1;
        }
        routeSegment->leftEdgePosition = &leftRouteBoundary[routePointIndex];
        routeSegment->rightEdgePosition = &rightRouteBoundary[routePointIndex];
        routeSegment->centerlinePositionPtr = &routeCenterline[routePointIndex];
        if (routeSegment->nextSegmentIndex < 0) {
            routeSegment->centerlineHeading = 180.0f;
            routeSegment->leftEdgeHeading = 180.0f;
            routeSegment->rightEdgeHeading = 180.0f;
        }
        else {
            routeSegment->centerlineHeading
                = atan2d(routeCenterline[routeSegment->nextSegmentIndex].x - routeCenterline[routePointIndex].x, routeCenterline[routeSegment->nextSegmentIndex].z - routeCenterline[routePointIndex].z);
            routeSegment->leftEdgeHeading
                = atan2d(leftRouteBoundary[routeSegment->nextSegmentIndex].x - leftRouteBoundary[routePointIndex].x, leftRouteBoundary[routeSegment->nextSegmentIndex].z - leftRouteBoundary[routePointIndex].z);
            routeSegment->rightEdgeHeading
                = atan2d(rightRouteBoundary[routeSegment->nextSegmentIndex].x - rightRouteBoundary[routePointIndex].x, rightRouteBoundary[routeSegment->nextSegmentIndex].z - rightRouteBoundary[routePointIndex].z);
        }
        if (routeSegment->previousSegmentIndex < 0) {
            routeSegment->previousCenterlineHeading = 0.0f;
        }
        else {
            routeSegment->previousCenterlineHeading
                = atan2d(routeCenterline[routeSegment->previousSegmentIndex].x - routeCenterline[routePointIndex].x, routeCenterline[routeSegment->previousSegmentIndex].z - routeCenterline[routePointIndex].z);
        }
        if (routeSegment->nextSegmentIndex < 0) {
            routeSegment->centerlineSegmentLength = 0.0f;
            routePoint.x = leftRouteBoundary[routePointIndex].x + routeSegments[routePointIndex - 1].leftEdgeControlPoints[1].x;
            routePoint.y = leftRouteBoundary[routePointIndex].y + routeSegments[routePointIndex - 1].leftEdgeControlPoints[1].y;
            routePoint.z = leftRouteBoundary[routePointIndex].z + routeSegments[routePointIndex - 1].leftEdgeControlPoints[1].z;
            InterpolateRouteVectors(&leftRouteEdgePoints[routePointIndex], &routePoint, &routeSegment->leftEdgeControlPoints[0], 0);
            routePoint.x = rightRouteBoundary[routePointIndex].x + routeSegments[routePointIndex - 1].rightEdgeControlPoints[1].x;
            routePoint.y = rightRouteBoundary[routePointIndex].y + routeSegments[routePointIndex - 1].rightEdgeControlPoints[1].y;
            routePoint.z = rightRouteBoundary[routePointIndex].z + routeSegments[routePointIndex - 1].rightEdgeControlPoints[1].z;
            InterpolateRouteVectors(&rightRouteEdgePoints[routePointIndex], &routePoint, &routeSegment->rightEdgeControlPoints[0], 0);
            routePoint.x = routeCenterline[routePointIndex].x + routeSegments[routePointIndex - 1].centerlineTangentX;
            routePoint.y = routeCenterline[routePointIndex].y + routeSegments[routePointIndex - 1].centerlineTangentY;
            routePoint.z = routeCenterline[routePointIndex].z + routeSegments[routePointIndex - 1].centerlineTangentZ;
            InterpolateRouteVectors(&routeCenterline[routePointIndex], &routePoint, &routeSegment->centerlinePosition, 0);
        }
        else {
            routePoint.x = routeCenterline[routePointIndex + 1].x - routeCenterline[routePointIndex].x;
            routePoint.y = routeCenterline[routePointIndex + 1].y - routeCenterline[routePointIndex].y;
            routePoint.z = routeCenterline[routePointIndex + 1].z - routeCenterline[routePointIndex].z;
            routeSegment->centerlineSegmentLength = sqrtf((routePoint.z * routePoint.z) + ((routePoint.x * routePoint.x) + (routePoint.y * routePoint.y)));
            InterpolateRouteVectors(&leftRouteBoundary[routePointIndex], &leftRouteBoundary[routeSegment->nextSegmentIndex], &routeSegment->leftEdgeControlPoints[0], 0);
            InterpolateRouteVectors(&rightRouteBoundary[routePointIndex], &rightRouteBoundary[routeSegment->nextSegmentIndex], &routeSegment->rightEdgeControlPoints[0], 0);
            InterpolateRouteVectors(&routeCenterline[routePointIndex], &routeCenterline[routeSegment->nextSegmentIndex], &routeSegment->centerlinePosition, 0);
        }
        routePoint.x = leftRouteBoundary[routePointIndex].x - routeCenterline[routePointIndex].x;
        routePoint.z = leftRouteBoundary[routePointIndex].z - routeCenterline[routePointIndex].z;
        routePoint.y = 0.0f;
        routeSegment->leftEdgeWidthStart = sqrtf((routePoint.z * routePoint.z) + ((routePoint.x * routePoint.x) + (routePoint.y * routePoint.y)));
        routePoint.x = rightRouteBoundary[routePointIndex].x - routeCenterline[routePointIndex].x;
        routePoint.z = rightRouteBoundary[routePointIndex].z - routeCenterline[routePointIndex].z;
        routePoint.y = 0.0f;
        routeSegment->rightEdgeWidthStart = sqrtf((routePoint.z * routePoint.z) + ((routePoint.x * routePoint.x) + (routePoint.y * routePoint.y)));
        if (routeSegment->nextSegmentIndex < 0) {
            routeSegment->leftEdgeWidthEnd = routeSegment->leftEdgeWidthStart;
            routeSegment->rightEdgeWidthEnd = routeSegment->rightEdgeWidthStart;
        }
        else {
            routePoint.x = leftRouteBoundary[routeSegment->nextSegmentIndex].x - routeCenterline[routeSegment->nextSegmentIndex].x;
            routePoint.z = leftRouteBoundary[routeSegment->nextSegmentIndex].z - routeCenterline[routeSegment->nextSegmentIndex].z;
            routePoint.y = 0.0f;
            routeSegment->leftEdgeWidthEnd = sqrtf((routePoint.z * routePoint.z) + ((routePoint.x * routePoint.x) + (routePoint.y * routePoint.y)));
            routePoint.x = rightRouteBoundary[routeSegment->nextSegmentIndex].x - routeCenterline[routeSegment->nextSegmentIndex].x;
            routePoint.z = rightRouteBoundary[routeSegment->nextSegmentIndex].z - routeCenterline[routeSegment->nextSegmentIndex].z;
            routePoint.y = 0.0f;
            routeSegment->rightEdgeWidthEnd = sqrtf((routePoint.z * routePoint.z) + ((routePoint.x * routePoint.x) + (routePoint.y * routePoint.y)));
        }
    }
    baseLeftRouteEdge = leftRouteEdgePoints;
    baseRightRouteEdge = rightRouteEdgePoints;
    baseRouteCenterline = routeCenterline;
    segmentIndex = HuMemDirectMallocNum(HEAP_HEAP, 0x40 * sizeof(RouteCollisionState), HU_MEMNUM_OVL);
    routeCollisionSlots = segmentIndex;
    memset(routeCollisionSlots, 0, 0x40 * sizeof(RouteCollisionState));
    for (routePointIndex = 0; routePointIndex < 0x40; routePointIndex++, segmentIndex++) {
        segmentIndex->collisionSlot = routePointIndex;
        segmentIndex->collisionSide = -1;
        segmentIndex->shapeType = -1;
        segmentIndex->collisionStateFlags = 0;
        segmentIndex->teamIndex = 0;
        segmentIndex->elapsedFrames = segmentIndex->durationFrames = 0;
    }
}

void UpdatePlayerModelLighting(void)
{
    RouteCollisionState *routeSegment;
    s32 segmentIndex;

    for (routeSegment = routeCollisionSlots, segmentIndex = 0; segmentIndex < 0x40; segmentIndex += 1, routeSegment++) {
        if ((routeSegment->collisionSide >= 0) && (routeSegment->shapeType >= 0) && (routeSegment->durationFrames >= 0)) {
            if (++routeSegment->elapsedFrames > routeSegment->durationFrames) {
                ResetPlayerRouteState(segmentIndex);
            }
        }
    }
}

void EmptyPlayerModelCallback(void) { }

GXColor playerModelWhiteColor = { 0xFF, 0xFF, 0xFF, 0xFF };
GXColor playerModelRedColor = { 0xFF, 0, 0, 0xFF };
GXColor playerModelYellowColor = { 0xFF, 0xFF, 0, 0xFF };

void UpdatePlayerModelRendering(HU3DMODEL *model, Mtx matrix)
{
    Mtx clipMatrix;
    float routeProgress;
    float edgeX;
    float edgeZ;
    RouteCollisionState *obstacle;
    RouteSegment *obstacleIndex;
    s16 sideIndex;
    s16 vertexIndex;
    TeamPlayerState *routeSide;
    PlayerCharacterState *playerState;
    s32 *obstacleSide;
    OMOBJ *triangleIndex;

    obstacleSide = model->hookData;
    routeSide = &teamPlayerWork[*obstacleSide];
    triangleIndex = teamPlayerObjects[*obstacleSide];
    GXLoadPosMtxImm(matrix, 0);
    PSMTXInvXpose(matrix, clipMatrix);
    GXLoadNrmMtxImm(clipMatrix, 0);
    GXSetNumTevStages(1);
    GXSetNumTexGens(1);
    GXSetNumChans(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, GX_FALSE, 0x7D);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG0);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG0);
    GXSetChanCtrl(GX_COLOR0A0, 1, GX_SRC_VTX, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_SPEC);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GXSetZCompLoc(GX_FALSE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
    GXSetLineWidth(0x10, GX_TO_ZERO);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_TEX_ST, GX_RGBA6, 0);
    GXSetArray(GX_VA_POS, leftRouteEdgePoints, sizeof(Vec));
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_TEX_ST, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, &playerModelWhiteColor, sizeof(GXColor));
    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 0x1D);
    for (sideIndex = 0; sideIndex < 0x1DU; sideIndex++) {
        GXPosition1x16(sideIndex);
        GXColor1x16(0);
    }
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_POS, &rightRouteEdgePoints, sizeof(Vec));
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, &playerModelWhiteColor, sizeof(GXColor));
    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 0x1D);
    for (sideIndex = 0; sideIndex < 0x1DU; sideIndex++) {
        GXPosition1x16(sideIndex);
        GXColor1x16(0);
    }
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_POS, routeCenterline, sizeof(Vec));
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, &playerModelYellowColor, sizeof(GXColor));
    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 0x1D);
    for (sideIndex = 0; sideIndex < routeSegmentCount; sideIndex++) {
        GXPosition1x16(sideIndex);
        GXColor1x16(0);
    }
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXBegin(GX_LINES, GX_VTXFMT0, routeSegmentCount * 4);
    for (obstacleIndex = routeSegments, sideIndex = 0; sideIndex < routeSegmentCount; sideIndex++, obstacleIndex++) {
        GXPosition3f32(obstacleIndex->leftEdgePosition->x, obstacleIndex->leftEdgePosition->y, obstacleIndex->leftEdgePosition->z);
        GXColor4u8(playerModelRedColor.r, playerModelRedColor.g, playerModelRedColor.b, playerModelRedColor.a);
        GXPosition3f32(obstacleIndex->centerlinePositionPtr->x, obstacleIndex->centerlinePositionPtr->y, obstacleIndex->centerlinePositionPtr->z);
        GXColor4u8(playerModelRedColor.r, playerModelRedColor.g, playerModelRedColor.b, playerModelRedColor.a);
        GXPosition3f32(obstacleIndex->rightEdgePosition->x, obstacleIndex->rightEdgePosition->y, obstacleIndex->rightEdgePosition->z);
        GXColor4u8(playerModelRedColor.r, playerModelRedColor.g, playerModelRedColor.b, playerModelRedColor.a);
        GXPosition3f32(obstacleIndex->centerlinePositionPtr->x, obstacleIndex->centerlinePositionPtr->y, obstacleIndex->centerlinePositionPtr->z);
        GXColor4u8(playerModelRedColor.r, playerModelRedColor.g, playerModelRedColor.b, playerModelRedColor.a);
    }
    for (sideIndex = 0, obstacle = routeCollisionSlots; sideIndex < 0x40; sideIndex++, obstacle++) {
        if ((obstacle->collisionSide >= 0) && (obstacle->shapeType >= 0) && (obstacle->teamIndex == *obstacleSide) && (obstacle->collisionSide == 1)) {
            switch (obstacle->shapeType) {
                case 0:
                    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 5);
                    routeProgress = 200.0f + CalculateRouteCurveValue(obstacle->firstCorner.x, obstacle->firstCorner.z);
                    GXPosition3f32(obstacle->firstCorner.x, routeProgress, obstacle->firstCorner.z);
                    GXColor4u8(0, 0xFF, 0, 0xFF);
                    GXPosition3f32(obstacle->secondCorner.x, routeProgress, obstacle->firstCorner.z);
                    GXColor4u8(0, 0xFF, 0, 0xFF);
                    GXPosition3f32(obstacle->secondCorner.x, routeProgress, obstacle->secondCorner.z);
                    GXColor4u8(0, 0xFF, 0, 0xFF);
                    GXPosition3f32(obstacle->firstCorner.x, routeProgress, obstacle->secondCorner.z);
                    GXColor4u8(0, 0xFF, 0, 0xFF);
                    GXPosition3f32(obstacle->firstCorner.x, routeProgress, obstacle->firstCorner.z);
                    GXColor4u8(0, 0xFF, 0, 0xFF);
                    break;
                case 1:
                    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 0xA);
                    routeProgress = 200.0f + CalculateRouteCurveValue(obstacle->firstCorner.x, obstacle->firstCorner.z);
                    for (vertexIndex = 0; vertexIndex < 0xA; vertexIndex++) {
                        edgeX = obstacle->firstCorner.x + (obstacle->collisionRadius * sind((40.0f * vertexIndex)));
                        edgeZ = obstacle->firstCorner.z + (obstacle->collisionRadius * cosd((40.0f * vertexIndex)));
                        GXPosition3f32(edgeX, routeProgress, edgeZ);
                        GXColor4u8(0, 0xFF, 0, 0xFF);
                    }
                    break;
            }
        }
    }
    for (sideIndex = 0, obstacle = routeCollisionSlots; sideIndex < 0x40; sideIndex++, obstacle++) {
        if ((obstacle->collisionSide >= 0) && (obstacle->shapeType >= 0) && (obstacle->teamIndex == *obstacleSide) && (obstacle->collisionSide == 0)) {
            switch (obstacle->shapeType) {
                case 0:
                    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 5);
                    routeProgress = 200.0f + CalculateRouteCurveValue(obstacle->firstCorner.x, obstacle->firstCorner.z);
                    GXPosition3f32(obstacle->firstCorner.x, routeProgress, obstacle->firstCorner.z);
                    GXColor4u8(0, 0, 0xFF, 0xFF);
                    GXPosition3f32(obstacle->secondCorner.x, routeProgress, obstacle->firstCorner.z);
                    GXColor4u8(0, 0, 0xFF, 0xFF);
                    GXPosition3f32(obstacle->secondCorner.x, routeProgress, obstacle->secondCorner.z);
                    GXColor4u8(0, 0, 0xFF, 0xFF);
                    GXPosition3f32(obstacle->firstCorner.x, routeProgress, obstacle->secondCorner.z);
                    GXColor4u8(0, 0, 0xFF, 0xFF);
                    GXPosition3f32(obstacle->firstCorner.x, routeProgress, obstacle->firstCorner.z);
                    GXColor4u8(0, 0, 0xFF, 0xFF);
                    break;
                case 1:
                    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 0xA);
                    routeProgress = 200.0f + CalculateRouteCurveValue(obstacle->firstCorner.x, obstacle->firstCorner.z);
                    for (vertexIndex = 0; vertexIndex < 0xA; vertexIndex++) {
                        edgeX = obstacle->firstCorner.x + (obstacle->collisionRadius * sind((40.0f * vertexIndex)));
                        edgeZ = obstacle->firstCorner.z + (obstacle->collisionRadius * cosd((40.0f * vertexIndex)));
                        GXPosition3f32(edgeX, routeProgress, edgeZ);
                        GXColor4u8(0, 0, 0xFF, 0xFF);
                    }
                    break;
            }
        }
    }
    if ((triangleIndex == NULL) || (routeSide == NULL)) {
        return;
    }
    if ((routeSide->playerObjects[0] == NULL) || (routeSide->playerStates[0] == NULL)) {
        return;
    }
    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 0xA);
    routeProgress = 100.0f + triangleIndex->trans.y;
    for (vertexIndex = 0; vertexIndex < 0xA; vertexIndex++) {
        edgeX = triangleIndex->trans.x + (200.0 * sind((40.0f * vertexIndex)));
        edgeX = edgeX;
        edgeZ = triangleIndex->trans.z + (200.0 * cosd((40.0f * vertexIndex)));
        edgeZ = edgeZ;
        GXPosition3f32(edgeX, routeProgress, edgeZ);
        GXColor4u8(0xFF, 0, 0xFF, 0xFF);
    }
    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 4);
    playerState = routeSide->playerStates[0];
    edgeX = playerState->cameraTargetX;
    edgeZ = playerState->cameraTargetZ;
    routeProgress = 100.0f + CalculateRouteCurveValue(edgeX, edgeZ);
    GXPosition3f32(edgeX, routeProgress, edgeZ);
    GXColor4u8(0xFF, 0x80, 0, 0xFF);
    GXPosition3f32(100.0f + edgeX, 200.0f + routeProgress, edgeZ);
    GXColor4u8(0xFF, 0x80, 0, 0xFF);
    GXPosition3f32(edgeX - 100.0f, 200.0f + routeProgress, edgeZ);
    GXColor4u8(0xFF, 0x80, 0, 0xFF);
    GXPosition3f32(edgeX, routeProgress, edgeZ);
    GXColor4u8(0xFF, 0x80, 0, 0xFF);
    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 4);
    playerState = routeSide->playerStates[1];
    edgeX = playerState->cameraTargetX;
    edgeZ = playerState->cameraTargetZ;
    routeProgress = 100.0f + CalculateRouteCurveValue(edgeX, edgeZ);
    GXPosition3f32(edgeX, routeProgress, edgeZ);
    GXColor4u8(0, 0x80, 0xFF, 0xFF);
    GXPosition3f32(100.0f + edgeX, 200.0f + routeProgress, edgeZ);
    GXColor4u8(0, 0x80, 0xFF, 0xFF);
    GXPosition3f32(edgeX - 100.0f, 200.0f + routeProgress, edgeZ);
    GXColor4u8(0, 0x80, 0xFF, 0xFF);
    GXPosition3f32(edgeX, routeProgress, edgeZ);
    GXColor4u8(0, 0x80, 0xFF, 0xFF);
}

void EmptyPlayerCollisionCallback(void) { }

s32 TestPointInRouteBounds(float positionX, float positionY, float positionZ)
{
    float segmentFraction;
    float distanceX;
    float distanceZ;
    float crossProduct;
    float segmentLength;
    s32 trackSegment;
    s32 segmentIndex;

    segmentIndex = -1;
    segmentLength = 1e7f;
    for (trackSegment = 0; trackSegment < routeSegmentCount; trackSegment++) {
        segmentFraction = routeSegments[trackSegment].centerlinePositionPtr->x - positionX;
        distanceX = routeSegments[trackSegment].centerlinePositionPtr->y - positionY;
        distanceZ = routeSegments[trackSegment].centerlinePositionPtr->z - positionZ;
        crossProduct = (distanceZ * distanceZ) + ((segmentFraction * segmentFraction) + (distanceX * distanceX));
        if (crossProduct < segmentLength) {
            segmentIndex = trackSegment;
            segmentLength = crossProduct;
        }
    }
    return segmentIndex;
}

s32 CalculateRouteSegmentParameter(float positionX, float positionY, float positionZ, float *segmentFraction)
{
    float segmentDistance;
    float projectionFraction;
    double segmentLength;
    double closestPointDistance;
    RouteSegment *trackSegment;
    s32 segmentIndex;
    RouteSegment *bestSegmentIndex;

    for (segmentIndex = 0; segmentIndex < (routeSegmentCount - 1); segmentIndex++) {
        trackSegment = &routeSegments[segmentIndex];
        bestSegmentIndex = &routeSegments[segmentIndex + 1];
        if ((positionZ < trackSegment->centerlinePositionPtr->z) && (positionZ >= bestSegmentIndex->centerlinePositionPtr->z)) {
            segmentDistance = bestSegmentIndex->centerlinePositionPtr->z - trackSegment->centerlinePositionPtr->z;
            if ((segmentFraction != NULL) && (segmentDistance != 0.0f)) {
                *segmentFraction = fabs((positionZ - trackSegment->centerlinePositionPtr->z) / segmentDistance);
            }
            break;
        }
    }
    return segmentIndex;
}

float CalculateRouteCurveValue(float curveParameter, float segmentLength)
{
    float routePosition;
    RouteSegment *routeSegment;
    s32 segmentIndex;
    RouteSegment *nextSegmentIndex;

    segmentIndex = CalculateRouteSegmentParameter(0.0f, 0.0f, segmentLength, &routePosition);
    routeSegment = &routeSegments[segmentIndex];
    if ((segmentIndex < 0) || (segmentIndex >= (routeSegmentCount - 1))) {
        return routeSegment->centerlinePositionPtr->y;
    }
    nextSegmentIndex = &routeSegments[segmentIndex + 1];
    return routeSegment->centerlinePositionPtr->y + (routePosition * (nextSegmentIndex->centerlinePositionPtr->y - routeSegment->centerlinePositionPtr->y));
}

void GetRoutePosition(float unusedWorldX, float worldZ, Vec *movementDirection)
{
    float segmentPosition;
    RouteSegment *segmentIndex;
    RouteSegment *nextSegmentIndex;
    s32 trackSegment;

    movementDirection->x = movementDirection->y = movementDirection->z = 0.0f;
    trackSegment = CalculateRouteSegmentParameter(0.0f, 0.0f, worldZ, &segmentPosition);
    segmentIndex = &routeSegments[trackSegment];
    if ((trackSegment < 0) || (trackSegment >= (routeSegmentCount - 1))) {
        return;
    }
    nextSegmentIndex = &routeSegments[trackSegment + 1];
    if (segmentIndex->centerlinePositionPtr->y > nextSegmentIndex->centerlinePositionPtr->y) {
        movementDirection->x = nextSegmentIndex->centerlinePositionPtr->x - segmentIndex->centerlinePositionPtr->x;
        movementDirection->z = nextSegmentIndex->centerlinePositionPtr->z - segmentIndex->centerlinePositionPtr->z;
        VECNormalize(movementDirection, movementDirection);
    }
}

s32 FindRouteSegmentForPosition(s32 preferredSegmentIndex, Vec *previousPosition, Vec *proposedPosition, Vec *resolvedPosition, Vec *collisionOffset)
{
    Vec movementDelta;
    Vec correctedPosition;
    Vec collisionPosition;
    Vec collisionNormal;
    Vec routePoint;
    float segmentFraction;
    float distanceX;
    float distanceZ;
    RouteSegment *segmentIndex;

    *resolvedPosition = *proposedPosition;
    collisionOffset->x = collisionOffset->y = collisionOffset->z = 0.0f;
    if (preferredSegmentIndex < 0) {
        preferredSegmentIndex = CalculateRouteSegmentParameter(0.0f, 0.0f, proposedPosition->z, &segmentFraction);
    }
    if ((preferredSegmentIndex < 0) || (preferredSegmentIndex >= (routeSegmentCount - 1))) {
        return 0;
    }
    segmentIndex = &routeSegments[preferredSegmentIndex];
    movementDelta.x = proposedPosition->x - previousPosition->x;
    movementDelta.y = proposedPosition->y - previousPosition->y;
    movementDelta.z = proposedPosition->z - previousPosition->z;
    VECNormalize(&movementDelta, &movementDelta);
    routePoint.x = segmentIndex->centerlinePosition.x + (segmentFraction * segmentIndex->centerlineTangentX);
    routePoint.y = segmentIndex->centerlinePosition.y + (segmentFraction * segmentIndex->centerlineTangentY);
    routePoint.z = segmentIndex->centerlinePosition.z + (segmentFraction * segmentIndex->centerlineTangentZ);
    distanceZ = fabs(proposedPosition->x - routePoint.x);
    if (proposedPosition->x < routePoint.x) {
        distanceX = segmentIndex->leftEdgeWidthStart + (segmentFraction * (segmentIndex->leftEdgeWidthEnd - segmentIndex->leftEdgeWidthStart));
        if (distanceZ > distanceX) {
            correctedPosition = *proposedPosition;
            ProjectPositionOntoRouteDouble(segmentIndex->leftEdgeControlPoints, NULL, &correctedPosition, &collisionPosition);
            resolvedPosition->x = collisionPosition.x;
            resolvedPosition->y = collisionPosition.y;
            resolvedPosition->z = collisionPosition.z;
            collisionOffset->x = resolvedPosition->x - proposedPosition->x;
            collisionOffset->y = resolvedPosition->y - proposedPosition->y;
            collisionOffset->z = resolvedPosition->z - proposedPosition->z;
            return 1;
        }
        goto block18;
    }
    distanceX = segmentIndex->rightEdgeWidthStart + (segmentFraction * (segmentIndex->rightEdgeWidthEnd - segmentIndex->rightEdgeWidthStart));
    if (distanceZ > distanceX) {
        correctedPosition = *proposedPosition;
        ProjectPositionOntoRouteDouble(segmentIndex->rightEdgeControlPoints, NULL, &correctedPosition, &collisionNormal);
        resolvedPosition->x = collisionNormal.x;
        resolvedPosition->y = collisionNormal.y;
        resolvedPosition->z = collisionNormal.z;
        collisionOffset->x = resolvedPosition->x - proposedPosition->x;
        collisionOffset->y = resolvedPosition->y - proposedPosition->y;
        collisionOffset->z = resolvedPosition->z - proposedPosition->z;
        return 1;
    }
block18:
    return 0;
}

s32 TestRouteSegmentCollision(Vec firstRoutePoint, Vec secondRoutePoint, s32 routeIndex, s32 collisionShape, s32 collisionSide)
{
    RouteCollisionState *collisionSlot;
    s32 collisionSlotIndex;

    for (collisionSlot = routeCollisionSlots, collisionSlotIndex = 0; collisionSlotIndex < 0x40; collisionSlotIndex++, collisionSlot++) {
        if ((collisionSlot->collisionSide < 0) || (collisionSlot->shapeType < 0)) {
            break;
        }
    }
    if (collisionSlotIndex >= 0x40) {
        return -1;
    }
    collisionSlot->collisionSide = routeIndex;
    collisionSlot->shapeType = 0;
    collisionSlot->collisionStateFlags = 0;
    collisionSlot->teamIndex = collisionSide;
    collisionSlot->durationFrames = collisionShape;
    collisionSlot->elapsedFrames = 0;
    collisionSlot->firstCorner = firstRoutePoint;
    collisionSlot->secondCorner = secondRoutePoint;
    return collisionSlotIndex;
}

s32 TestRouteCircleCollision(Vec circleCenter, float collisionRadius, s32 routeIndex, s32 collisionShape, s32 collisionSide)
{
    RouteCollisionState *collisionSlot;
    s32 collisionSlotIndex;

    for (collisionSlot = routeCollisionSlots, collisionSlotIndex = 0; collisionSlotIndex < 0x40; collisionSlotIndex++, collisionSlot++) {
        if ((collisionSlot->collisionSide < 0) || (collisionSlot->shapeType < 0)) {
            break;
        }
    }
    if (collisionSlotIndex >= 0x40) {
        return -1;
    }
    collisionSlot->collisionSide = routeIndex;
    collisionSlot->shapeType = 1;
    collisionSlot->collisionStateFlags = 0;
    collisionSlot->teamIndex = collisionSide;
    collisionSlot->durationFrames = collisionShape;
    collisionSlot->elapsedFrames = 0;
    collisionSlot->firstCorner = circleCenter;
    collisionSlot->collisionRadius = collisionRadius;
    return collisionSlotIndex;
}

void ResetPlayerRouteState(s32 teamIndex)
{
    RouteCollisionState *collisionSlot;

    if ((teamIndex < 0) || (teamIndex >= 0x40)) {
        return;
    }
    collisionSlot = &routeCollisionSlots[teamIndex];
    collisionSlot->collisionSide = -1;
    collisionSlot->shapeType = -1;
    collisionSlot->elapsedFrames = collisionSlot->durationFrames = 0;
}

s32 FindNearestRoutePosition(s32 teamIndex, Vec *position, float searchRadius)
{
    float collisionDeltaX;
    float collisionDeltaZ;
    float expandedRadius;
    RouteCollisionState *collisionSlot;
    s32 collisionSlotIndex;

    for (collisionSlot = routeCollisionSlots, collisionSlotIndex = 0; collisionSlotIndex < 0x40; collisionSlotIndex++, collisionSlot++) {
        if ((collisionSlot->collisionSide >= 0) && (collisionSlot->shapeType >= 0) && (collisionSlot->teamIndex == teamIndex) && ((collisionSlot->collisionSide == 1) || (collisionSlot->collisionSide == 2))) {
            switch (collisionSlot->shapeType) {
                case 0:
                    collisionDeltaX = position->x - collisionSlot->firstCorner.x;
                    collisionDeltaZ = position->z - collisionSlot->firstCorner.z;
                    if ((position->x >= (collisionSlot->firstCorner.x - searchRadius)) && (position->x <= (collisionSlot->secondCorner.x + searchRadius)) && (position->z >= (collisionSlot->firstCorner.z - searchRadius))
                        && (position->z <= (collisionSlot->secondCorner.z + searchRadius))) {
                        return 1;
                    }
                    break;
                case 1:
                    collisionDeltaX = position->x - collisionSlot->firstCorner.x;
                    collisionDeltaZ = position->z - collisionSlot->firstCorner.z;
                    expandedRadius = collisionSlot->collisionRadius + searchRadius;
                    if (((collisionDeltaX * collisionDeltaX) + (collisionDeltaZ * collisionDeltaZ)) < (expandedRadius * expandedRadius)) {
                        return 1;
                    }
                    break;
            }
        }
    }
    return 0;
}

s32 FindPlayerRouteTarget(s32 teamIndex, Vec *routePositionContext, Vec *proposedPosition, Vec *resolvedPosition, float searchRadius)
{
    float collisionDeltaX;
    float collisionDeltaZ;
    float expandedRadius;
    RouteCollisionState *collisionSlot;
    s32 collisionSlotIndex;

    *resolvedPosition = *proposedPosition;
    for (collisionSlot = routeCollisionSlots, collisionSlotIndex = 0; collisionSlotIndex < 0x40; collisionSlotIndex++, collisionSlot++) {
        if ((collisionSlot->collisionSide >= 0) && (collisionSlot->shapeType >= 0) && (collisionSlot->teamIndex == teamIndex) && ((collisionSlot->collisionSide == 0) || (collisionSlot->collisionSide == 2))) {
            switch (collisionSlot->shapeType) {
                case 0:
                    collisionDeltaX = proposedPosition->x - collisionSlot->firstCorner.x;
                    collisionDeltaZ = proposedPosition->z - collisionSlot->firstCorner.z;
                    if ((proposedPosition->x >= (collisionSlot->firstCorner.x - searchRadius)) && (proposedPosition->x <= (collisionSlot->secondCorner.x + searchRadius)) && (proposedPosition->z >= (collisionSlot->firstCorner.z - searchRadius))
                        && (proposedPosition->z <= (collisionSlot->secondCorner.z + searchRadius))) {
                        return 1;
                    }
                    break;
                case 1:
                    collisionDeltaX = proposedPosition->x - collisionSlot->firstCorner.x;
                    collisionDeltaZ = proposedPosition->z - collisionSlot->firstCorner.z;
                    expandedRadius = collisionSlot->collisionRadius + searchRadius;
                    if (((collisionDeltaX * collisionDeltaX) + (collisionDeltaZ * collisionDeltaZ)) < (expandedRadius * expandedRadius)) {
                        expandedRadius = sqrtf((collisionDeltaX * collisionDeltaX) + (collisionDeltaZ * collisionDeltaZ));
                        if (expandedRadius == 0.0f) {
                            collisionDeltaX = 0.0f;
                            collisionDeltaZ = 1.0f;
                        }
                        else {
                            collisionDeltaX /= expandedRadius;
                            collisionDeltaZ /= expandedRadius;
                        }
                        expandedRadius = collisionSlot->collisionRadius + searchRadius;
                        resolvedPosition->x = collisionSlot->firstCorner.x + (collisionDeltaX * expandedRadius);
                        resolvedPosition->z = collisionSlot->firstCorner.z + (collisionDeltaZ * expandedRadius);
                        resolvedPosition->y = proposedPosition->y;
                        return 1;
                    }
                    break;
            }
        }
    }
    return 0;
}

float InterpolateRouteValue(float startValue, float endValue)
{
    float normalizedAngle;

    if (startValue >= 360.0f) {
        startValue -= 360.0f;
    }
    else if (startValue < 0.0f) {
        startValue += 360.0f;
    }
    if (endValue >= 360.0f) {
        endValue -= 360.0f;
    }
    else if (endValue < 0.0f) {
        endValue += 360.0f;
    }
    normalizedAngle = startValue - endValue;
    if (normalizedAngle <= -180.0f) {
        normalizedAngle += 360.0f;
    }
    else if (normalizedAngle >= 180.0f) {
        normalizedAngle -= 360.0f;
    }
    return normalizedAngle;
}

float CalculateRouteHeading(float positionX, float positionZ, float routeParameter)
{
    float angleDelta;
    float rotationStep;
    float targetAngle;

    if (positionX >= 360.0) {
        positionX -= 360.0;
    }
    else if (positionX < 0.0) {
        positionX += 360.0;
    }
    if (positionZ >= 360.0) {
        positionZ -= 360.0;
    }
    else if (positionZ < 0.0) {
        positionZ += 360.0;
    }
    rotationStep = 360.0 + (positionX - positionZ);
    if (fabs(rotationStep) >= 360.0) {
        rotationStep = fmod(rotationStep, 360.0);
    }
    if (rotationStep < 180.0) {
        if (rotationStep <= routeParameter) {
            angleDelta = rotationStep;
        }
        else {
            angleDelta = routeParameter;
        }
    }
    else if ((360.0 - rotationStep) <= routeParameter) {
        angleDelta = -(360.0 - rotationStep);
    }
    else {
        angleDelta = -routeParameter;
    }
    angleDelta += positionZ;
    if (angleDelta >= 360.0) {
        angleDelta -= 360.0;
    }
    else if (angleDelta < 0.0) {
        angleDelta += 360.0;
    }
    return angleDelta;
}

float InterpolateAngle(float interpolationRate, float currentAngle, float targetAngle)
{
    float angleDelta;
    float rotationStep;

    rotationStep = InterpolateRouteValue(targetAngle, currentAngle);
    angleDelta = fabs(rotationStep * interpolationRate);
    return CalculateRouteHeading(targetAngle, currentAngle, angleDelta);
}

void BuildRouteSegmentVectors(Vec *segmentStart, Vec *segmentEnd, Vec *controlPoints, Vec *segmentTangent)
{
    Vec transformedVector;
    Vec rotationMatrix;

    transformedVector.x = segmentEnd->x - segmentStart->x;
    transformedVector.y = segmentEnd->y - segmentStart->y;
    transformedVector.z = segmentEnd->z - segmentStart->z;
    rotationMatrix.x = controlPoints->x - segmentStart->x;
    rotationMatrix.y = controlPoints->y - segmentStart->y;
    rotationMatrix.z = controlPoints->z - segmentStart->z;
    segmentTangent->x = (transformedVector.y * rotationMatrix.z) - (transformedVector.z * rotationMatrix.y);
    segmentTangent->y = (transformedVector.z * rotationMatrix.x) - (transformedVector.x * rotationMatrix.z);
    segmentTangent->z = (transformedVector.x * rotationMatrix.y) - (transformedVector.y * rotationMatrix.x);
    VECNormalize(segmentTangent, segmentTangent);
    segmentTangent[1].x = (segmentTangent->z * segmentStart->z) + ((segmentTangent->x * segmentStart->x) + (segmentTangent->y * segmentStart->y));
}

void InterpolateRouteVectors(Vec *startPosition, Vec *endPosition, Vec *outputVectors, s32 normalize)
{
    Vec interpolatedVector;

    interpolatedVector.x = endPosition->x - startPosition->x;
    interpolatedVector.y = endPosition->y - startPosition->y;
    interpolatedVector.z = endPosition->z - startPosition->z;
    if (normalize != 0) {
        VECNormalize(&interpolatedVector, &interpolatedVector);
    }
    outputVectors->x = startPosition->x;
    outputVectors->y = startPosition->y;
    outputVectors->z = startPosition->z;
    outputVectors[1].x = interpolatedVector.x;
    outputVectors[1].y = interpolatedVector.y;
    outputVectors[1].z = interpolatedVector.z;
}

void ProjectPositionOntoRoute(Vec *routeVectors, float *routeParameter, Vec *position, Vec *projectedPosition)
{
    float rotationX;
    float rotationY;
    float rotationZ;

    rotationY = (routeVectors[1].z * routeVectors[1].z) + ((routeVectors[1].x * routeVectors[1].x) + (routeVectors[1].y * routeVectors[1].y));
    rotationZ = (-routeVectors[0].z * routeVectors[1].z) + (position->z * routeVectors[1].z)
        + ((-routeVectors[0].x * routeVectors[1].x) + (position->x * routeVectors[1].x) + ((-routeVectors[0].y * routeVectors[1].y) + (position->y * routeVectors[1].y)));
    if (rotationY != 0.0f) {
        rotationX = rotationZ / rotationY;
    }
    else {
        rotationX = 0.0f;
    }
    if (projectedPosition != NULL) {
        projectedPosition->x = routeVectors[0].x + (rotationX * routeVectors[1].x);
        projectedPosition->y = routeVectors[0].y + (rotationX * routeVectors[1].y);
        projectedPosition->z = routeVectors[0].z + (rotationX * routeVectors[1].z);
    }
    if (routeParameter != NULL) {
        *routeParameter = rotationX;
    }
}

typedef struct DoubleVector3 {
    double x;
    double y;
    double z;
} DoubleVector3;

void ProjectPositionOntoRouteDouble(Vec *routeVectors, float *routeParameter, Vec *position, Vec *projectedPosition)
{
    DoubleVector3 transformedVector;
    DoubleVector3 sourceVector;
    DoubleVector3 translationVector;
    double projectionX;
    double projectionY;
    double projectionZ;

    sourceVector.x = 0.0;
    sourceVector.y = 0.0;
    sourceVector.z = 0.0;
    transformedVector.x = routeVectors[1].x;
    transformedVector.y = routeVectors[1].y;
    transformedVector.z = routeVectors[1].z;
    translationVector.x = position->x - routeVectors->x;
    translationVector.y = position->y - routeVectors->y;
    translationVector.z = position->z - routeVectors->z;

    projectionY = (transformedVector.z * routeVectors[1].z) + ((transformedVector.x * routeVectors[1].x) + (transformedVector.y * routeVectors[1].y));
    projectionZ = (-sourceVector.z * transformedVector.z) + (translationVector.z * transformedVector.z) + ((-sourceVector.x * transformedVector.x) + (translationVector.x * transformedVector.x) + ((-sourceVector.y * transformedVector.y) + (translationVector.y * transformedVector.y)));
    if (0.0 != projectionY) {
        projectionX = projectionZ / projectionY;
    }
    else {
        projectionX = 0.0;
    }
    if (projectedPosition != NULL) {
        projectedPosition->x = routeVectors->x + (float)(sourceVector.x + (projectionX * transformedVector.x));
        projectedPosition->y = routeVectors->y + (float)(sourceVector.y + (projectionX * transformedVector.y));
        projectedPosition->z = routeVectors->z + (float)(sourceVector.z + (projectionX * transformedVector.z));
    }
    if (routeParameter != NULL) {
        *routeParameter = projectionX;
    }
}

void EmptyPlayerModelHook(void) { }

void TransformRoutePosition(Mtx transformMatrix, Vec *position)
{
    Vec transformedVector;

    transformedVector.x = (transformMatrix[0][2] * position->z) + ((transformMatrix[0][0] * position->x) + (transformMatrix[0][1] * position->y));
    transformedVector.y = (transformMatrix[1][2] * position->z) + ((transformMatrix[1][0] * position->x) + (transformMatrix[1][1] * position->y));
    transformedVector.z = (transformMatrix[2][2] * position->z) + ((transformMatrix[2][0] * position->x) + (transformMatrix[2][1] * position->y));
    *position = transformedVector;
}

void ScaleModelMaterialLighting(s32 modelId, float brightnessScale)
{
    HSFMATERIAL *vertexIndex;
    s32 modelIndex;
    HSFDATA *vertexCount;
    s32 modelDataIndex;
    HU3DMODEL *textureWidth;
    s32 textureHeight;

    textureWidth = &Hu3DData[modelId];
    vertexCount = textureWidth->hsf;
    vertexIndex = vertexCount->material;
    textureHeight = vertexCount->materialNum;
    for (modelDataIndex = 0; modelDataIndex < textureHeight; modelDataIndex++, vertexIndex++) {
        vertexIndex->vtxMode = 1;
        modelIndex = vertexIndex->litColor[0] * brightnessScale;
        if (modelIndex > 0xFF) {
            modelIndex = 0xFF;
        }
        vertexIndex->litColor[0] = modelIndex;
        modelIndex = vertexIndex->litColor[1] * brightnessScale;
        if (modelIndex > 0xFF) {
            modelIndex = 0xFF;
        }
        vertexIndex->litColor[1] = modelIndex;
        modelIndex = vertexIndex->litColor[2] * brightnessScale;
        if (modelIndex > 0xFF) {
            modelIndex = 0xFF;
        }
        vertexIndex->litColor[2] = modelIndex;
    }
}

s32 CopyModelLight(s32 sourceModelId, s32 sourceLightIndex, s32 targetModelId)
{
    s32 motionIndex;
    HU3DMODEL *modelIndex;
    HU3DMODEL *motionDataIndex;

    motionDataIndex = &Hu3DData[sourceModelId];
    modelIndex = &Hu3DData[targetModelId];
    for (motionIndex = 0; motionIndex < 8; motionIndex++) {
        if (modelIndex->lLightId[motionIndex] == -1) {
            break;
        }
    }
    if (motionIndex == 8) {
        return -1;
    }
    modelIndex->lLightId[motionIndex] = motionDataIndex->lLightId[sourceLightIndex];
    modelIndex->attr |= 0x1000;
    return motionIndex;
}
