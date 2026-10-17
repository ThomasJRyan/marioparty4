#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXVert.h"
#include "dolphin/mtx/GeoTypes.h"
#include "ext_math.h"
#include "game/chrman.h"
#include "game/gamework_data.h"
#include "game/hsfload.h"
#include "game/hu3d.h"
#include "game/init.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include "string.h"

#include "REL/m428Dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#include "game/frand.h"
#endif

typedef struct M428DllProjectionState {
    /* 0x00 */ Float projectionOrViewportValueBuffer;
    /* 0x04 */ char projectionStatePadding[0x3C]; /* projection data padding */
} M428DllProjectionState; /* size = 0x40 */

typedef struct M428DllObstacleData {
    Vec obstacleCenter;
    Float collisionRadius;
} M428DllObstacleData; /* size = 0x10 */

typedef struct M428DllMotionTableEntry {
    /* 0x00 */ s32 characterMotionId;
    /* 0x04 */ s32 jointMotionTableIndex;
} M428DllMotionTableEntry; /* size = 0x08 */

typedef struct M428DllTeamWork {
    OMOBJ *playerObjects[2]; // M428DllCharacterWork
    s16 playerIds[2];
    s16 teamIndex;
    s16 animationState;
    s16 leadingPlayerIndex;
    s16 randomVariant;
    s32 roundResultFlag;
    s32 knockoutFlag;
    Float highestPlayerY;
    s32 roundCompleteFlag;
    s32 playerKnockedOut;
    s32 cameraFollowKnockout;
    Float knockoutY;
    Float jumpDistanceLimit;
    Float knockbackDistance;
    s32 collisionModelId;
    Vec collisionPoint;
    Float combinedRunSpeed;
    Float playerRunSpeeds[2];
    Float smoothedRunSpeeds[2];
    Float previousPlayer0Speed;
    Float previousPlayer1Speed;
    Vec cameraTargets[2];
    Vec player0CameraTarget;
    Vec player1CameraTarget;
    Vec player0ResultPosition;
    Vec player1ResultPosition;
    Vec knockoutPosition;
    s32 rumbleFrame;
    s32 rumbleMode;
    s32 rumbleRemainingFrames;
    s32 rumbleTotalFrames;
    Float rumbleAmplitude;
} M428DllTeamWork; /* size = 0xCC */

typedef struct M428DllCharacterWork {
    s16 playerId;
    s16 teamIndex;
    s16 teamPlayerIndex;
    s16 characterId;
    s16 controllerIndex;
    s16 isComputer;
    s16 computerDifficulty;
    s16 buttonsPressed;
    s16 buttonsHeld;
    Float stickX;
    Float stickY;
    s16 motionId;
    s16 animationBlendFrames;
    s16 motionState;
    s16 stateFrame;
    s16 jumpButtonHeld;
    s16 movementDirection;
    Float runSpeed;
    Float targetRunSpeed;
    s32 jumpState;
    s32 landedFrame;
    s32 motionFlags;
    Vec velocity;
    Vec worldPosition;
    Vec modelOffset;
    Mtx modelMatrix;
    Vec characterPosition;
    char hookName[0x100];
    s32 hookModelId;
    s32 idleDelayFrames;
    s32 footstepEffectId;
    s32 footstepFrame;
    Float footstepScale;
    s32 particleEffectId;
    s32 effectFrame;
    Vec effectOffset;
} M428DllCharacterWork; /* size = 0x1C4 */

typedef struct M428DllScoreDisplayWork {
    s32 scoreSprite;
    s32 teamProgressSprites[2];
    Float teamProgressValues[2][2];
    s32 teamHighlightSprites[2];
    Float teamHighlightAlpha[2][2];
    ANIMDATA *leftTeamAnimation;
    ANIMDATA *rightTeamAnimation;
    s32 leftTeamFlashFrames[2][2];
    s32 rightTeamFlashFrames[2][2];
} M428DllScoreDisplayWork; /* size = 0x5C */

typedef struct M428DllRopeSegmentWork {
    s16 segmentIndex;
    s16 endpointType;
    s32 intersectsObstacle;
    Vec segmentVertices[4];
} M428DllRopeSegmentWork; /* size = 0x38 */

typedef struct M428DllRopeWork {
    s16 hookModelId;
    s16 segmentCount;
    Float ropeWidth;
    Float segmentSpacing;
    s32 effectMode;
    s32 colorIndex;
    Float startAlpha;
    Float endAlpha;
    s32 useCenterControlPoint;
    Vec startPoint;
    Vec endPoint;
    Vec centerControlPoint;
    s32 centerSegmentIndex;
    Float reservedRopeData48;
    Float reservedRopeData4C;
    Float reservedRopeData50;
    M428DllRopeSegmentWork *segments;
} M428DllRopeWork; /* size = 0x58 */

typedef struct M428DllParticleWork {
    s16 particleReservedFrame;
    s16 particleReservedFlags;
    Float velocityX;
    Float velocityY;
    Float velocityZ;
    char particleReservedBytes[8];
    Float rotationAcceleration;
    char particleReservedData[0xC];
    Float animationFrame;
    Float frameAdvance;
    Float particleScale;
    Float particleRotation;
    Vec particleOffset;
    GXColor particleColor;
    s32 textureTileIndex;
} M428DllParticleWork; /* size = 0x4C */

typedef void (*M428DllPlayerHook)(HU3DMODEL *model, struct M428DllParticleHookWork *renderWork, Mtx matrix);

typedef struct M428DllParticleHookWork {
    s16 hookFlags;
    s16 hookState;
    char reservedHookData[0x1C];
    u8 renderFlags;
    u8 renderState;
    u8 renderOptions;
    u8 renderPadding;
    char reservedVertices[2];
    s16 particleCount;
    s32 displayListFrame;
    s32 lastRenderFrame;
    u32 displayListSize;
    u32 displayListCapacity;
    ANIMDATA *spriteAnimation;
    M428DllParticleWork *particles;
    Vec *particleVertices;
    Vec2f *particleTexCoords;
    void *displayList;
    M428DllPlayerHook renderHook;
    HU3DMODEL *nextHookModel;
} M428DllParticleHookWork; /* size = 0x54 */

typedef struct M428DllResultDisplayWork {
    s32 resultModelIds[2];
    s32 resultAnimationIds[2];
    ANIMDATA *resultAnimationData;
} M428DllResultDisplayWork; /* size = 0x14 */

void DrawPlayerParticleHook(HU3DMODEL *model, Mtx matrix);
void SetRopeEndpoint(s32 modelId, Vec *startPoint, Vec *endPoint);
void ConfigureRopeEndpoint(s32 modelId, s32 useCenterControlPoint, s32 centerSegmentIndex, Vec *centerControlPoint);
void SetRopeEndpointAlpha(s32 modelId, Float startAlpha, Float endAlpha);
s32 FindRopeCollisionPoint(s32 modelId, Vec *segmentStart, Vec *movementDelta, Vec *collisionPoint);
void InitializeScoreDisplay(void);
void UpdateScoreDisplay(OMOBJ *object);
void DestroyScoreDisplay(void);
void SetTeamProgress(s32 teamIndex, Float progress);
void InitializeResultDisplay(void);
void UpdateResultDisplay(OMOBJ *object);
void DestroyResultDisplay(void);
s32 CreatePairedParticleEffects(s32 teamIndex, Vec position, Vec velocity, Float particleScale, Float rotationAcceleration, Float particleSpacing, Float frameAdvance, GXColor color);
Float InterpolateWrappedAngle(Float progress, Float startAngle, Float maximumAngularStep);
void InterpolateVector(Float progress, Vec *startPoint, Vec *endPoint, Vec *interpolatedPoint);
void InterpolateQuadraticVector(Float progress, Vec *startPoint, Vec *controlPoint, Vec *endPoint, Vec *interpolatedPoint);
s16 CreateParticleModel(ANIMDATA *animationData, s16 particleCount);
void DrawParticleModel(HU3DMODEL *model, Mtx matrix);
void SetParticleModelVisible(s16 modelId, s16 particleIndex, s32 isVisible);
void SetParticleModelOffset(s16 modelId, s16 particleIndex, Vec particleOffset);
void SetParticleModelScale(s16 modelId, s16 particleIndex, Float particleScale);
s32 FindModelJointTransform(s16 modelId, char *jointName, Mtx parentMatrix, s32 includeParentMatrix);
void SearchModelJointTransform(HSFOBJECT *modelObject, Mtx parentMatrix, char *jointName, Mtx jointMatrix);
void UpdateTeamObject(OMOBJ *object);
void InitializeTeamPlayers(OMOBJ *object);
void UpdatePlayerResultState(OMOBJ *object);
void UpdatePlayerObject(OMOBJ *object);
void UpdatePlayerMovement(OMOBJ *object);
void UpdatePlayerEffect(OMOBJ *object);
void UpdatePlayerEffectCallback(OMOBJ *object);
s32 CheckSegmentObstacleCollision(Vec *segmentPosition, Vec *movementDelta, Float obstacleClearance);
s32 CheckPlayerObstacleCollision(Vec *playerPosition, Vec *movementDelta, Vec *resolvedPosition, Float obstacleClearance);
s32 CreateRopeModel(Vec *startPoint, Vec *endPoint, Float ropeWidth, s32 segmentCount, s32 colorIndex);

extern u32 GlobalCounter;

Vec playerStartPositions[4] = {
    { -140.0f, -400.0f, 0.0f },
    { 140.0f, -300.0f, 0.0f },
    { -140.0f, -300.0f, 0.0f },
    { 140.0f, -400.0f, 0.0f },
};
Vec playerModelOffsets[4] = {
    { -100.0f, -10.0f, 150.0f },
    { 100.0f, -10.0f, 150.0f },
    { -100.0f, -10.0f, 150.0f },
    { 100.0f, -10.0f, 150.0f },
};
Vec characterJointHookOffsets[4][0xE] = {
    {
        { -11.0f, 214.0f, 39.0f },
        { -11.0f, 400.0f, 46.0f },
        { -11.0f, 668.0f, 56.0f },
        { -11.0f, 931.0f, 42.0f },
        { 7.0f, 1131.0f, 65.0f },
        { -452.0f, 1449.0f, 57.0f },
        { -478.0f, 1763.0f, 48.0f },
        { -452.0f, 2001.0f, 44.0f },
        { -311.0f, 2120.0f, 55.0f },
        { -85.0f, 2442.0f, 42.0f },
        { -32.0f, 2876.0f, 43.0f },
        { -12.0f, 3300.0f, 48.0f },
        { -42.0f, 4500.0f, 48.0f },
        { -42.0f, 4500.0f, 48.0f },
    },
    {
        { 16.0f, 203.0f, 40.0f },
        { 0.0f, 456.0f, 50.0f },
        { 15.0f, 650.0f, 58.0f },
        { -268.0f, 935.0f, 51.0f },
        { -463.0f, 1205.0f, 66.0f },
        { -663.0f, 1313.0f, 69.0f },
        { -460.0f, 1600.0f, 59.0f },
        { -333.0f, 1946.0f, 46.0f },
        { -82.0f, 2188.0f, 44.0f },
        { -76.0f, 2374.0f, 40.0f },
        { -108.0f, 2620.0f, 43.0f },
        { -95.0f, 2990.0f, 46.0f },
        { -270.0f, 3445.0f, 46.0f },
        { -42.0f, 4500.0f, 48.0f },
    },
    {
        { 26.0f, 354.0f, 46.0f },
        { 29.0f, 604.0f, 57.0f },
        { 30.0f, 877.0f, 39.0f },
        { 51.0f, 1086.0f, 64.0f },
        { 360.0f, 1196.0f, 46.0f },
        { 398.0f, 1416.0f, 39.0f },
        { 468.0f, 1658.0f, 43.0f },
        { 468.0f, 1944.0f, 53.0f },
        { 135.0f, 2174.0f, 47.0f },
        { 107.0f, 2430.0f, 61.0f },
        { 98.0f, 2698.0f, 43.0f },
        { 90.0f, 2995.0f, 50.0f },
        { 0.0f, 4500.0f, 48.0f },
        { 0.0f, 4500.0f, 48.0f },
    },
    {
        { -24.0f, 313.0f, 41.0f },
        { -11.0f, 529.0f, 51.0f },
        { -33.0f, 799.0f, 44.0f },
        { 7.0f, 990.0f, 53.0f },
        { 135.0f, 1196.0f, 57.0f },
        { 393.0f, 1379.0f, 39.0f },
        { 429.0f, 1457.0f, 42.0f },
        { 420.0f, 1984.0f, 54.0f },
        { 190.0f, 2239.0f, 53.0f },
        { 70.0f, 2532.0f, 55.0f },
        { 75.0f, 2933.0f, 44.0f },
        { 75.0f, 4500.0f, 48.0f },
        { 75.0f, 4500.0f, 48.0f },
        { 75.0f, 4500.0f, 48.0f },
    },
};
Vec characterHookOffsets[4] = {
    { -30.000002f, -10.0f, -90.0f },
    { 20.0f, -10.0f, -70.0f },
    { 0.0f, -10.0f, 0.0f },
    { 0.0f, -10.0f, 0.0f },
};
Vec characterHookRotations[4] = {
    { 15.0f, 150.0f, 0.0f },
    { 15.0f, 210.0f, 0.0f },
    { 0.0f, 180.0f, 0.0f },
    { 0.0f, 180.0f, 0.0f },
};
M428DllObstacleData obstacleCollisionData[5] = {
    { { -400.0f, 700.0f, 0.0f }, 200.0f },
    { { 400.0f, 900.0f, 0.0f }, 200.0f },
    { { 0.0f, 1800.0f, 0.0f }, 220.0f },
    { { -600.0f, 3100.0f, 0.0f }, 300.0f },
    { { 600.0f, 2880.0f, 0.0f }, 300.0f },
};
M428DllMotionTableEntry characterMotionTable[0xC] = {
    { 0x5F0000, 0 },
    { 0x5F0002, 0 },
    { 0x5F0003, 0 },
    { -1, 0 },
    { -1, 1 },
    { -1, 2 },
    { -1, 3 },
    { 0x5F0006, 0 },
    { 0x5F0014, 0 },
    { 0x5F005B, 0 },
    { 0x5F0017, 0 },
    { 0x5F0018, 0 },
};
s32 jointMotionFileIds[4][8] = {
    {
        DATA_MAKE_NUM(DATADIR_M428, 0x11),
        DATA_MAKE_NUM(DATADIR_M428, 0x12),
        DATA_MAKE_NUM(DATADIR_M428, 0x13),
        DATA_MAKE_NUM(DATADIR_M428, 0x14),
        DATA_MAKE_NUM(DATADIR_M428, 0x15),
        DATA_MAKE_NUM(DATADIR_M428, 0x16),
        DATA_MAKE_NUM(DATADIR_M428, 0x17),
        DATA_MAKE_NUM(DATADIR_M428, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_M428, 0x19),
        DATA_MAKE_NUM(DATADIR_M428, 0x1A),
        DATA_MAKE_NUM(DATADIR_M428, 0x1B),
        DATA_MAKE_NUM(DATADIR_M428, 0x1C),
        DATA_MAKE_NUM(DATADIR_M428, 0x1D),
        DATA_MAKE_NUM(DATADIR_M428, 0x1E),
        DATA_MAKE_NUM(DATADIR_M428, 0x1F),
        DATA_MAKE_NUM(DATADIR_M428, 0x20),
    },
    {
        DATA_MAKE_NUM(DATADIR_M428, 0x21),
        DATA_MAKE_NUM(DATADIR_M428, 0x22),
        DATA_MAKE_NUM(DATADIR_M428, 0x23),
        DATA_MAKE_NUM(DATADIR_M428, 0x24),
        DATA_MAKE_NUM(DATADIR_M428, 0x25),
        DATA_MAKE_NUM(DATADIR_M428, 0x26),
        DATA_MAKE_NUM(DATADIR_M428, 0x27),
        DATA_MAKE_NUM(DATADIR_M428, 0x28),
    },
    {
        DATA_MAKE_NUM(DATADIR_M428, 0x29),
        DATA_MAKE_NUM(DATADIR_M428, 0x2A),
        DATA_MAKE_NUM(DATADIR_M428, 0x2B),
        DATA_MAKE_NUM(DATADIR_M428, 0x2C),
        DATA_MAKE_NUM(DATADIR_M428, 0x2D),
        DATA_MAKE_NUM(DATADIR_M428, 0x2E),
        DATA_MAKE_NUM(DATADIR_M428, 0x2F),
        DATA_MAKE_NUM(DATADIR_M428, 0x30),
    },
};
Vec characterModelOffsets[8] = {
    { 0.0f, -10.0f, -20.0f },
    { 0.0f, -10.0f, -20.0f },
    { 0.0f, -30.000002f, -20.0f },
    { 0.0f, -10.0f, -20.0f },
    { 0.0f, -10.0f, -20.0f },
    { 0.0f, 10.0f, -10.0f },
    { 0.0f, -30.000002f, -20.0f },
    { 0.0f, -10.0f, -20.0f },
};
s32 controllerRumbleMasks[4] = { 0x80000000, 0x40000000, 0x20000000, 0x10000000 };
GXColor ropePrimaryColor = { 0xFF, 0xFF, 0xFF, 0x20 };
GXColor ropeSecondaryColor = { 0xFF, 0xFF, 0xFF, 0x40 };

HUPROCESS *objectManager;
// M428DllTeamWork
OMOBJ *teamObjects[2];
OMOBJ *playerObjects[4];
// M428DllScoreDisplayWork
OMOBJ *scoreObject;
// M428DllResultDisplayWork
OMOBJ *resultObject;
M428DllTeamWork *teamWorkArray;
M428DllCharacterWork *playerWorkArray;
s32 roundEndFlag;
s32 teamRumbleModes[2];
Mtx cameraViewMatrices[2];
M428DllProjectionState cameraProjectionStates[2];
M428DllProjectionState cameraViewportStates[2];
s32 printObjectNames;
s32 useCurrentObjectTransforms;
s32 matchedObjectFound;

OMOBJ *CreatePlayerTeamObjects(HUPROCESS *process)
{
    s32 teamIndex;

    objectManager = process;
    roundEndFlag = 0;
    teamWorkArray = HuMemDirectMallocNum(HEAP_HEAP, 2 * sizeof(M428DllTeamWork), HU_MEMNUM_OVL);
    memset(teamWorkArray, 0, 2 * sizeof(M428DllTeamWork));
    playerWorkArray = HuMemDirectMallocNum(HEAP_HEAP, 4 * sizeof(M428DllCharacterWork), HU_MEMNUM_OVL);
    memset(playerWorkArray, 0, 4 * sizeof(M428DllCharacterWork));
    for (teamIndex = 0; teamIndex < 2; teamIndex++) {
        teamObjects[teamIndex] = omAddObjEx(process, 0x40, 1, 0, -1, UpdateTeamObject);
        teamObjects[teamIndex]->work[0] = teamIndex;
    }
    InitializeScoreDisplay();
    SetScoreDisplayVisible(0);
    InitializeResultDisplay();
    teamRumbleModes[0] = teamRumbleModes[1] = 0;
#ifdef NON_MATCHING
    return NULL; // the return value is never used anyways and teamIndex is too high, causing an error
#else
    return teamObjects[teamIndex];
#endif
}

void DestroyPlayerModuleObjects(void)
{
    s32 teamIndex;

    DestroyScoreDisplay();
    DestroyResultDisplay();
    for (teamIndex = 0; teamIndex < 2; teamIndex++) {
        UpdatePlayerResultState(teamObjects[teamIndex]);
    }
}

void UpdateTeamObject(OMOBJ *object)
{
    M428DllTeamWork *teamWork;
    s32 teamPlayerIndex;
    s32 teamIndex;
    s32 playerId;

    teamIndex = object->teamWork[0];
    teamWork = &teamWorkArray[teamIndex];
    object->data = teamWork;
    teamWork->teamIndex = teamIndex;
    teamWork->animationState = 0;
    teamWork->leadingPlayerIndex = 0;
    teamWork->randomVariant = (u8)frand() % 4;
    teamWork->roundResultFlag = 0;
    teamWork->knockoutFlag = 0;
    teamWork->highestPlayerY = 0.0f;
    teamWork->roundCompleteFlag = 0;
    teamWork->playerKnockedOut = 0;
    teamWork->cameraFollowKnockout = 0;
    teamWork->knockoutY = 0.0f;
    teamWork->jumpDistanceLimit = 0.0f;
    teamWork->knockbackDistance = 0.0f;
    teamWork->rumbleFrame = 0;
    teamWork->rumbleMode = -1;
    teamWork->rumbleRemainingFrames = teamWork->rumbleTotalFrames = 0;
    teamWork->rumbleAmplitude = 0.0f;
    teamWork->collisionModelId = -1;
    teamWork->playerRunSpeeds[0] = teamWork->playerRunSpeeds[1] = 0.0f;
    teamWork->combinedRunSpeed = 400.0f;
    teamWork->smoothedRunSpeeds[0] = teamWork->previousPlayer0Speed = 200.0f;
    teamWork->smoothedRunSpeeds[1] = teamWork->previousPlayer1Speed = 200.0f;
    teamWork->cameraTargets[0].x = 0.5f * (playerStartPositions[0].x + playerStartPositions[1].x);
    teamWork->cameraTargets[0].y = 0.5f * (playerStartPositions[0].y + playerStartPositions[1].y);
    teamWork->cameraTargets[0].z = 0.5f * (playerStartPositions[0].z + playerStartPositions[1].z);
    teamWork->cameraTargets[1] = teamWork->cameraTargets[0];
    teamWork->player0CameraTarget = teamWork->cameraTargets[0];
    teamWork->player1CameraTarget = teamWork->cameraTargets[1];
    teamWork->player0ResultPosition.x = teamWork->player0ResultPosition.y = teamWork->player0ResultPosition.z = 0.0f;
    teamWork->player1ResultPosition.x = teamWork->player1ResultPosition.y = teamWork->player1ResultPosition.z = 0.0f;
    teamWork->knockoutPosition.x = teamWork->knockoutPosition.y = teamWork->knockoutPosition.z = 0.0f;
    for (teamPlayerIndex = 0; teamPlayerIndex < 2; teamPlayerIndex++) {
        playerId = playerIdsByTeam[teamPlayerIndex + (teamIndex * 2)];
        teamWork->playerIds[teamPlayerIndex] = playerId;
        teamWork->playerObjects[teamPlayerIndex] = playerObjects[playerId] = omAddObjEx(objectManager, 0x41, 2, 0xC, -1, UpdatePlayerObject);
        teamWork->playerObjects[teamPlayerIndex]->data = NULL;
        teamWork->playerObjects[teamPlayerIndex]->teamWork[0] = playerId;
        teamWork->playerObjects[teamPlayerIndex]->teamWork[1] = teamWork->teamIndex;
        teamWork->playerObjects[teamPlayerIndex]->teamWork[2] = teamPlayerIndex;
    }
    object->mdlId[0] = CreateRopeModel(&playerStartPositions[0], &playerStartPositions[1], 400.0f, 0x30, teamWork->teamIndex);
    if (teamWork->teamIndex == 0) {
        Hu3DModelCameraSet(object->mdlId[0], 1);
    }
    else {
        Hu3DModelCameraSet(object->mdlId[0], 2);
    }
    object->objFunc = InitializeTeamPlayers;
}

void InitializeTeamPlayers(OMOBJ *object)
{
    Vec firstPlayerPosition;
    Vec secondPlayerPosition;
    Vec collisionPoint;
    Vec teamCenterPosition;
    Vec playerSeparation;
    Vec cameraPosition;
    Vec cameraRotation;
    Float combinedRunSpeed;
    Float cameraBlend;
    Float playerSeparationDistance;
    Float cameraCenterX;
    Float knockoutBlend;
    Float cameraCenterY;
    Float highestPlayerY;
    M428DllTeamWork *teamWork;
    M428DllCharacterWork *firstPlayer;
    M428DllCharacterWork *secondPlayer;
    s32 knockedOutPlayer;
    s32 cameraSpeed;
    s32 leadingPlayer;
    s32 teamHasKnockout;
    s32 collisionChanged;
    s32 cameraNeedsFastUpdate;
    s32 collisionModelId;

    cameraNeedsFastUpdate = 0;
    teamHasKnockout = 0;
    collisionChanged = 0;
    teamWork = object->data;
    firstPlayer = teamWork->playerObjects[0]->data;
    secondPlayer = teamWork->playerObjects[1]->data;
    if (!firstPlayer || !secondPlayer) {
        return;
    }
    if ((GetGamePhase() < 7) || (roundEndFlag == 0)) {
        firstPlayerPosition.x = firstPlayer->characterPosition.x;
        firstPlayerPosition.y = firstPlayer->characterPosition.y;
        firstPlayerPosition.z = firstPlayer->characterPosition.z;
        secondPlayerPosition.x = secondPlayer->characterPosition.x;
        secondPlayerPosition.y = secondPlayer->characterPosition.y;
        secondPlayerPosition.z = secondPlayer->characterPosition.z;
        VECAdd(&firstPlayerPosition, &secondPlayerPosition, &teamCenterPosition);
        VECScale(&teamCenterPosition, &teamCenterPosition, 0.5f);
        collisionModelId = FindRopeCollisionPoint(object->mdlId[0], &firstPlayerPosition, &secondPlayerPosition, &collisionPoint);
        if (collisionModelId != -1) {
            if (teamWork->collisionModelId != collisionModelId) {
                teamWork->collisionModelId = collisionModelId;
                teamWork->collisionPoint = collisionPoint;
                collisionChanged = 1;
            }
            else {
                collisionChanged = 0;
            }
            teamWork->cameraTargets[1] = teamWork->collisionPoint;
            teamWork->cameraTargets[0] = teamWork->cameraTargets[1];
            combinedRunSpeed = teamWork->playerRunSpeeds[0] + teamWork->playerRunSpeeds[1];
            if (combinedRunSpeed >= 400.0f) {
                if (collisionChanged != 0) {
                    teamWork->combinedRunSpeed = combinedRunSpeed;
                }
                else {
                    teamWork->combinedRunSpeed + (400.0f - teamWork->combinedRunSpeed) * 0.01f; // bug
                    (void)cameraNeedsFastUpdate;
                }
                combinedRunSpeed = 1.0f / combinedRunSpeed;
                teamWork->smoothedRunSpeeds[0] = teamWork->combinedRunSpeed * (teamWork->playerRunSpeeds[0] * combinedRunSpeed);
                teamWork->smoothedRunSpeeds[1] = teamWork->combinedRunSpeed * (teamWork->playerRunSpeeds[1] * combinedRunSpeed);
            }
            else {
                combinedRunSpeed = 1.0f / combinedRunSpeed;
                teamWork->smoothedRunSpeeds[0] = 400.0f * (teamWork->playerRunSpeeds[0] * combinedRunSpeed);
                teamWork->smoothedRunSpeeds[1] = 400.0f * (teamWork->playerRunSpeeds[1] * combinedRunSpeed);
                teamWork->combinedRunSpeed = 400.0f;
            }
            combinedRunSpeed = teamWork->smoothedRunSpeeds[0] + teamWork->smoothedRunSpeeds[1];
            cameraSpeed = 48.0f * (teamWork->smoothedRunSpeeds[0] / combinedRunSpeed);
            if (cameraSpeed < 1) {
                cameraSpeed = 1;
            }
            else if (cameraSpeed > 0x2E) {
                cameraSpeed = 0x2E;
            }
            ConfigureRopeEndpoint(object->mdlId[0], 1, cameraSpeed, &teamWork->collisionPoint);
            combinedRunSpeed = (teamWork->playerRunSpeeds[0] + teamWork->playerRunSpeeds[1]) / teamWork->combinedRunSpeed;
            if (combinedRunSpeed > 1.0f) {
                combinedRunSpeed = 1.0f;
            }
            SetRopeEndpointAlpha(object->mdlId[0], 1.0f - combinedRunSpeed, 1.0f - combinedRunSpeed);
        }
        else {
            teamWork->collisionModelId = -1;
            ConfigureRopeEndpoint(object->mdlId[0], 0, 0x18, &teamWork->collisionPoint);
            combinedRunSpeed = (teamWork->playerRunSpeeds[0] + teamWork->playerRunSpeeds[1]) / 400.0f;
            if (combinedRunSpeed > 1.0f) {
                combinedRunSpeed = 1.0f;
            }
            SetRopeEndpointAlpha(object->mdlId[0], 1.0f - combinedRunSpeed, 1.0f - combinedRunSpeed);
            teamWork->combinedRunSpeed = 400.0f;
            teamWork->cameraTargets[1] = teamCenterPosition;
            teamWork->cameraTargets[0] = teamWork->cameraTargets[1];
            teamWork->smoothedRunSpeeds[0] = 200.0f;
            teamWork->smoothedRunSpeeds[1] = 200.0f;
        }
        teamWork->smoothedRunSpeeds[0] = teamWork->previousPlayer0Speed + (0.01f * (teamWork->smoothedRunSpeeds[0] - teamWork->previousPlayer0Speed));
        teamWork->smoothedRunSpeeds[1] = teamWork->previousPlayer1Speed + (0.01f * (teamWork->smoothedRunSpeeds[1] - teamWork->previousPlayer1Speed));
        InterpolateVector(0.1f, &teamWork->cameraTargets[2], &teamWork->cameraTargets[0], &teamWork->cameraTargets[0]);
        InterpolateVector(0.1f, &teamWork->player1CameraTarget, &teamWork->cameraTargets[1], &teamWork->cameraTargets[1]);
        teamWork->previousPlayer0Speed = teamWork->smoothedRunSpeeds[0];
        teamWork->previousPlayer1Speed = teamWork->smoothedRunSpeeds[1];
        teamWork->player0CameraTarget = teamWork->cameraTargets[0];
        teamWork->player1CameraTarget = teamWork->cameraTargets[1];
        if (teamWork->collisionModelId != -1) {
            teamWork->player0ResultPosition = teamWork->cameraTargets[0];
            teamWork->player1ResultPosition = teamWork->cameraTargets[1];
        }
        else if (firstPlayer->motionState == 4) {
            teamWork->player0ResultPosition = secondPlayer->characterPosition;
            teamWork->player1ResultPosition = firstPlayer->characterPosition;
        }
        else if (secondPlayer->motionState == 4) {
            teamWork->player0ResultPosition = firstPlayer->characterPosition;
            teamWork->player1ResultPosition = secondPlayer->characterPosition;
        }
        else {
            teamWork->player0ResultPosition = teamWork->cameraTargets[0];
            teamWork->player1ResultPosition = teamWork->cameraTargets[1];
        }
        if (teamRumbleModes[teamWork->teamIndex] == 5) {
            teamHasKnockout = 1;
        }
        if ((GetGamePhase() != 4) || (teamWork->knockoutFlag != 0)) {
            teamHasKnockout = 0;
        }
        if (teamHasKnockout != 0) {
            knockedOutPlayer = -1;
            if ((firstPlayer->motionState == 2) && (secondPlayer->motionState == 3)) {
                knockedOutPlayer = 0;
            }
            else if ((firstPlayer->motionState == 3) && (secondPlayer->motionState == 2)) {
                knockedOutPlayer = 1;
            }
            else if ((firstPlayer->motionState == 2) && (secondPlayer->motionState == 2)) {
                if (firstPlayerPosition.y > secondPlayerPosition.y) {
                    knockedOutPlayer = 0;
                }
                else {
                    knockedOutPlayer = 1;
                }
            }
            if (knockedOutPlayer == 0) {
                firstPlayer->motionState = 4;
                firstPlayer->stateFrame = 0;
                firstPlayer->landedFrame = 0;
                firstPlayer->motionFlags = 0;
                firstPlayer->velocity.x = 0.0f;
                firstPlayer->velocity.y = firstPlayer->velocity.z = 0.0f;
                teamWork->knockoutY = firstPlayerPosition.y;
                teamWork->knockoutPosition = firstPlayerPosition;
                teamWork->knockbackDistance = 0.0f;
            }
            else if (knockedOutPlayer == 1) {
                secondPlayer->motionState = 4;
                secondPlayer->stateFrame = 0;
                secondPlayer->landedFrame = 0;
                secondPlayer->motionFlags = 0;
                secondPlayer->velocity.x = 0.0f;
                secondPlayer->velocity.y = secondPlayer->velocity.z = 0.0f;
                teamWork->knockoutY = secondPlayerPosition.y;
                teamWork->knockoutPosition = secondPlayerPosition;
                teamWork->knockbackDistance = 0.0f;
            }
        }
        if ((firstPlayer->motionState == 4) || (secondPlayer->motionState == 4)) {
            teamWork->playerKnockedOut = 1;
            teamWork->cameraFollowKnockout = 1;
        }
        else {
            teamWork->playerKnockedOut = 0;
        }
        if (teamRumbleModes[teamWork->teamIndex] != 5) {
            teamWork->cameraFollowKnockout = 0;
        }
        firstPlayerPosition.x = firstPlayer->characterPosition.x;
        firstPlayerPosition.y = firstPlayer->characterPosition.y;
        firstPlayerPosition.z = firstPlayer->characterPosition.z;
        secondPlayerPosition.x = secondPlayer->characterPosition.x;
        secondPlayerPosition.y = secondPlayer->characterPosition.y;
        secondPlayerPosition.z = secondPlayer->characterPosition.z;
        SetRopeEndpoint(object->mdlId[0], &firstPlayerPosition, &secondPlayerPosition);
        if (teamWork->playerObjects[0]->trans.y > teamWork->playerObjects[1]->trans.y) {
            teamWork->highestPlayerY = teamWork->playerObjects[0]->trans.y;
            teamWork->leadingPlayerIndex = 0;
        }
        else if (teamWork->playerObjects[1]->trans.y > teamWork->playerObjects[0]->trans.y) {
            teamWork->highestPlayerY = teamWork->playerObjects[1]->trans.y;
            teamWork->leadingPlayerIndex = 1;
        }
        else {
            leadingPlayer = frand() & 1;
            teamWork->highestPlayerY = teamWork->playerObjects[leadingPlayer]->trans.y;
            teamWork->leadingPlayerIndex = leadingPlayer;
        }
        SetTeamProgress(teamWork->teamIndex, teamWork->highestPlayerY / 3800.0f);
        if (GetGamePhase() == 4) {
            firstPlayerPosition.x = teamWork->playerObjects[0]->trans.x;
            firstPlayerPosition.y = teamWork->playerObjects[0]->trans.y;
            firstPlayerPosition.z = teamWork->playerObjects[0]->trans.z;
            secondPlayerPosition.x = teamWork->playerObjects[1]->trans.x;
            secondPlayerPosition.y = teamWork->playerObjects[1]->trans.y;
            secondPlayerPosition.z = teamWork->playerObjects[1]->trans.z;
            VECAdd(&firstPlayerPosition, &secondPlayerPosition, &teamCenterPosition);
            VECScale(&teamCenterPosition, &teamCenterPosition, 0.5f);
            VECSubtract(&firstPlayerPosition, &secondPlayerPosition, &playerSeparation);
            playerSeparation.z = 0.0f;
            playerSeparationDistance = VECMag(&playerSeparation);
            if (playerSeparationDistance < 100.0f) {
                knockoutBlend = 0.0f;
            }
            else if (playerSeparationDistance >= 300.0f) {
                knockoutBlend = 1.0f;
            }
            else {
                knockoutBlend = (playerSeparationDistance - 100.0f) / 200.0f;
            }
            playerSeparationDistance = 1500.0f + (500.0f * knockoutBlend);
            cameraCenterX = teamCenterPosition.x;
            if (cameraCenterX > 400.0f) {
                cameraCenterX = 400.0f;
            }
            else if (cameraCenterX < -400.0f) {
                cameraCenterX = -400.0f;
            }
            cameraCenterY = teamCenterPosition.y;
            if (cameraCenterY < 400.0f) {
                cameraCenterY = 400.0f;
            }
            highestPlayerY = teamWork->highestPlayerY;
            if (highestPlayerY < 20.0f) {
                cameraBlend = 0.0f;
            }
            else {
                cameraBlend = (highestPlayerY - 20.0f) / 3780.0f;
                if (cameraBlend >= 1.0f) {
                    cameraBlend = 1.0f;
                }
            }
            cameraPosition.x = cameraCenterX;
            cameraPosition.y = cameraCenterY;
            cameraPosition.z = 0.0f;
            cameraRotation.x = 70.0f * cameraBlend;
            cameraRotation.y = 0.0f;
            cameraRotation.z = 0.0f;
            if (teamWork->rumbleTotalFrames > 0) {
                cameraBlend = teamWork->rumbleAmplitude * ((Float)teamWork->rumbleRemainingFrames / teamWork->rumbleTotalFrames);
                leadingPlayer = (teamWork->rumbleRemainingFrames * 0x2D) % 360;
                cameraBlend = cameraBlend * sind(leadingPlayer);
                cameraBlend = cameraBlend;
                cameraPosition.x += cameraBlend;
                cameraPosition.y += cameraBlend;
                if (--teamWork->rumbleRemainingFrames <= 0) {
                    teamWork->rumbleRemainingFrames = teamWork->rumbleTotalFrames = 0;
                }
                cameraNeedsFastUpdate = 1;
            }
            if (cameraNeedsFastUpdate == 0) {
                ConfigureCameraTransition(teamWork->teamIndex, (REFRESH_RATE_F / 5), &cameraPosition, &cameraRotation, playerSeparationDistance);
            }
            else {
                ConfigureCameraTransition(teamWork->teamIndex, (REFRESH_RATE_F / 20), &cameraPosition, &cameraRotation, playerSeparationDistance);
            }
        }
        if (teamWork->rumbleMode != -1) {
            if (teamWork->rumbleMode == 0) {
                teamWork->rumbleFrame++;
                if (teamWork->rumbleFrame == 1) {
                    omVibrate(teamWork->playerIds[0], 0xC, 6, 6);
                    omVibrate(teamWork->playerIds[1], 0xC, 6, 6);
                    return;
                }
                if (teamWork->rumbleFrame >= (REFRESH_RATE_F * 0.75f)) {
                    teamWork->rumbleFrame = 0;
                }
            }
            else {
                teamWork->rumbleFrame++;
                if (teamWork->rumbleFrame == 1) {
                    omVibrate(teamWork->playerIds[0], 0xC, 0xC, 0);
                    omVibrate(teamWork->playerIds[1], 0xC, 0xC, 0);
                    return;
                }
                if (teamWork->rumbleFrame >= REFRESH_RATE_F / 10.0f) {
                    teamWork->rumbleFrame = 0;
                }
            }
        }
    }
    else {
        switch (GetGamePhase()) {
            case 7:
                if (roundEndFlag != 0) {
                    if (teamWork->roundResultFlag != 0) {
                        firstPlayer->motionState = 7;
                        firstPlayer->stateFrame = 0;
                        secondPlayer->motionState = 7;
                        secondPlayer->stateFrame = 0;
                    }
                    else {
                        firstPlayer->motionState = 9;
                        firstPlayer->stateFrame = 0;
                        secondPlayer->motionState = 9;
                        secondPlayer->stateFrame = 0;
                    }
                    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
                }
                break;
            case 8:
                break;
            case 9:
                if ((teamWork->roundResultFlag != 0) && (roundEndFlag != 0)) {
                    firstPlayer->motionState = 8;
                    firstPlayer->stateFrame = 0;
                    secondPlayer->motionState = 8;
                    secondPlayer->stateFrame = 0;
                }
                break;
        }
    }
}

void UpdatePlayerResultState(OMOBJ *object)
{
    s32 playerIndex;
    M428DllTeamWork *teamWork = object->data;
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        UpdatePlayerEffect(teamWork->playerObjects[playerIndex]);
        teamWork->playerObjects[playerIndex] = NULL;
    }
}

void UpdatePlayerObject(OMOBJ *object)
{
    M428DllCharacterWork *characterWork;
    s32 motionIndex;
    s32 motionSlot;
    u32 playerId;
    M428DllTeamWork *teamWork;
    u32 teamIndex;
    u32 teamPlayerIndex;

    playerId = object->work[0];
    teamIndex = object->work[1];
    teamPlayerIndex = object->work[2];
    teamWork = &teamWorkArray[teamIndex];
    characterWork = object->data = &playerWorkArray[playerId];
    characterWork->playerId = playerId;
    characterWork->teamIndex = teamIndex;
    characterWork->teamPlayerIndex = teamPlayerIndex;
    characterWork->characterId = GWPlayerCfg[playerId].character;
    characterWork->controllerIndex = GWPlayerCfg[playerId].pad_idx;
    characterWork->isComputer = GWPlayerCfg[playerId].iscom;
    characterWork->computerDifficulty = GWPlayerCfg[playerId].diff;
    if ((characterWork->isComputer == 0) && ((controllerRumbleMasks[characterWork->controllerIndex] & HuPadRumbleGet()) == 0)) {
        rumbleState = 0;
    }
    if (characterWork->isComputer != 0) {
        teamWork->animationState = teamWork->animationState | (1 << characterWork->teamPlayerIndex);
    }
    characterWork->buttonsPressed = characterWork->buttonsHeld = 0;
    characterWork->stickX = characterWork->stickY = 0.0f;
    characterWork->motionId = -1;
    characterWork->animationBlendFrames = 4;
    characterWork->motionState = 0;
    characterWork->stateFrame = characterWork->jumpButtonHeld = 0;
    characterWork->movementDirection = 0;
    characterWork->runSpeed = 200.0f;
    characterWork->targetRunSpeed = characterWork->runSpeed;
    characterWork->jumpState = 0;
    characterWork->landedFrame = 0;
    characterWork->motionFlags = 0;
    characterWork->velocity.x = characterWork->velocity.y = characterWork->velocity.z = 0.0f;
    characterWork->worldPosition.x = characterWork->worldPosition.y = characterWork->worldPosition.z = 0.0f;
    characterWork->modelOffset.x = 0.0f;
    characterWork->modelOffset.z = 0.0f;
    characterWork->modelOffset.y = 75.0f;
    mtxRot(characterWork->modelMatrix, 0.0f, 0.0f, 0.0f);
    mtxTransCat(characterWork->modelMatrix, characterModelOffsets[characterWork->characterId].x, characterModelOffsets[characterWork->characterId].y, characterModelOffsets[characterWork->characterId].z);
    characterWork->characterPosition.x = playerStartPositions[characterWork->teamPlayerIndex].x;
    characterWork->characterPosition.y = 80.0f + playerStartPositions[characterWork->teamPlayerIndex].y;
    characterWork->characterPosition.z = playerStartPositions[characterWork->teamPlayerIndex].z;
    strcpy(characterWork->hookName, MakeObjectName(CharModelItemHookGet(characterWork->characterId, 4, 4)));
    characterWork->hookModelId = -1;
    characterWork->idleDelayFrames = (REFRESH_RATE_F / 2) * ((frand() & 0x7FFF) % 5);
    characterWork->footstepEffectId = 0;
    characterWork->footstepFrame = 0;
    characterWork->footstepScale = 0.0f;
    characterWork->particleEffectId = 0;
    characterWork->effectFrame = 0;
    characterWork->effectOffset.x = 0.5f * (100.0f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));
    characterWork->effectOffset.y = 0.5f * (100.0f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));
    characterWork->effectOffset.z = 0.5f * (100.0f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));
    motionIndex = CharModelCreate(characterWork->characterId, 4);
    object->mdlId[0] = motionIndex;
    if (characterWork->teamIndex == 0) {
        Hu3DModelCameraSet(motionIndex, 1);
        AddModelToCameraShadows(motionIndex, 1);
    }
    else {
        Hu3DModelCameraSet(motionIndex, 2);
        AddModelToCameraShadows(motionIndex, 2);
    }
    Hu3DModelLayerSet(motionIndex, 2);
    CharModelStepFxSet(characterWork->characterId, 0);
    if (characterWork->teamIndex == 0) {
        motionIndex = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M428, 0x07));
        object->mdlId[1] = motionIndex;
        Hu3DModelCameraSet(motionIndex, 1);
    }
    else {
        motionIndex = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M428, 0x08));
        object->mdlId[1] = motionIndex;
        Hu3DModelCameraSet(motionIndex, 2);
    }
    Hu3DModelLayerSet(motionIndex, 2);
    for (motionSlot = 0; motionSlot < 0xC; motionSlot++) {
        if (characterMotionTable[motionSlot].characterMotionId != 0xFFFFFFFF) {
            object->mtnId[motionSlot] = CharMotionCreate(characterWork->characterId, characterMotionTable[motionSlot].characterMotionId);
        }
        else {
            object->mtnId[motionSlot] = Hu3DJointMotionFile(object->mdlId[0], jointMotionFileIds[characterMotionTable[motionSlot].jointMotionTableIndex][characterWork->characterId]);
        }
    }
    characterWork->motionId = 3;
    CharMotionSet(characterWork->characterId, object->mtnId[characterWork->motionId]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    CharMotionDataClose(characterWork->characterId);
    object->trans.x = characterWork->worldPosition.x = playerStartPositions[characterWork->teamPlayerIndex].x;
    object->trans.y = characterWork->worldPosition.y = playerStartPositions[characterWork->teamPlayerIndex].y;
    object->trans.z = characterWork->worldPosition.z = playerStartPositions[characterWork->teamPlayerIndex].z;
    object->rot.x = object->rot.z = 0.0f;
    object->rot.y = 180.0f;
    object->objFunc = UpdatePlayerMovement;
}

void UpdatePlayerMovement(OMOBJ *object)
{
    Mtx modelTransform;
    Vec characterPosition;
    Vec positionDelta;
    Vec movementVector;
    Float movementMagnitude;
    Float playerSeparation;
    Float horizontalInput;
    Float motionBlendScratch;
    Float collisionResolutionScratch;
    Float verticalInput;
    Float particleSizeScale;
    Float targetRotation;
    M428DllCharacterWork *characterWork;
    M428DllTeamWork *teamWork;
    OMOBJ *teammateObject;
    M428DllCharacterWork *teammateWork;
    s32 targetMotionId;
    HU3DMODEL *model;
    s32 particleIndex;
    s32 previousMotionId;
    s32 teammateIndex;
    s32 playersSeparated;
    s32 movementCorrected;

#ifdef NON_MATCHING
    movementCorrected = 0;
#endif

    characterWork = object->data;
    teamWork = &teamWorkArray[characterWork->teamIndex];
    teammateIndex = characterWork->teamPlayerIndex ^ 1;
    teammateObject = teamWork->playerObjects[teammateIndex];
    teammateWork = teammateObject->data;
    if (GetGamePhase() == 4) {
        if (characterWork->isComputer != 0) {
            characterWork->buttonsPressed = characterWork->buttonsHeld = 0;
            characterWork->stickX = characterWork->stickY = 0.0f;
            UpdatePlayerEffectCallback(object);
        }
        else {
            characterWork->buttonsPressed = HuPadBtnDown[characterWork->controllerIndex];
            characterWork->buttonsHeld = HuPadBtn[characterWork->controllerIndex];
            characterWork->stickX = HuPadStkX[characterWork->controllerIndex];
            characterWork->stickY = HuPadStkY[characterWork->controllerIndex];
        }
    }
    else {
        characterWork->buttonsPressed = characterWork->buttonsHeld = 0;
        characterWork->stickX = characterWork->stickY = 0.0f;
    }
    switch (characterWork->motionState) {
        case 0:
            characterWork->motionState = 1;
            characterWork->stateFrame = 0;
            break;
        case 1:
            if (object->trans.y < 0.0f) {
                object->trans.y += 1.6f;
                targetMotionId = 3;
                if ((targetMotionId != characterWork->motionId) && (CharMotionShiftIDGet(characterWork->characterId) == -1)) {
                    characterWork->motionId = targetMotionId;
                    CharMotionShiftSet(characterWork->characterId, object->mtnId[characterWork->motionId], 0.0f, 8.0f, 0x40000001);
                }
            }
            else {
                object->trans.y = 0.0f;
                CharMotionSpeedSet(characterWork->characterId, 0.0f);
            }
            characterPosition = characterWork->characterPosition;
            VECSubtract(&characterPosition, &teamWork->cameraTargets[characterWork->teamPlayerIndex], &positionDelta);
            characterWork->jumpState = 0;
            movementMagnitude = VECMag(&positionDelta);
            teamWork->playerRunSpeeds[characterWork->teamPlayerIndex] = movementMagnitude;
            characterPosition.x = object->trans.x + characterWork->modelOffset.x;
            characterPosition.y = object->trans.y + characterWork->modelOffset.y;
            characterPosition.z = object->trans.z + characterWork->modelOffset.z;
            if (IsPositionInCameraBounds(&characterPosition) != 0) {
                object->trans.z = 50.0f + characterPosition.z;
            }
            if (GetGamePhase() >= 4) {
                characterWork->motionState = 2;
                characterWork->stateFrame = 0;
            }
            break;
        case 2:
            if (((characterWork->buttonsPressed & 0x100) != 0) || (teamWork->playerKnockedOut != 0) || (teamWork->cameraFollowKnockout != 0)) {
                if (characterWork->motionId == 4) {
                    characterWork->animationBlendFrames = 5;
                }
                else {
                    characterWork->animationBlendFrames = 4;
                }
                characterWork->movementDirection = 0;
                characterWork->motionState = 3;
                characterWork->stateFrame = 0;
            }
            else {
                if (fabs(characterWork->stickX) >= 8.0) {
                    horizontalInput = 0.025f * characterWork->stickX;
                }
                else {
                    horizontalInput = 0.0f;
                }
                if (fabs(characterWork->stickY) >= 8.0) {
                    verticalInput = 0.025f * characterWork->stickY;
                }
                else {
                    verticalInput = 0.0f;
                }
#if VERSION_PAL
                movementVector.x = horizontalInput * 1.2f;
                movementVector.y = verticalInput * 1.2f;
#else
                movementVector.x = horizontalInput;
                movementVector.y = verticalInput;
#endif
                movementVector.z = 0.0f;
                if (VECMag(&movementVector) > 0.0f) {
                    characterPosition.x = object->trans.x + characterWork->modelOffset.x;
                    characterPosition.y = object->trans.y + characterWork->modelOffset.y;
                    characterPosition.z = object->trans.z + characterWork->modelOffset.z;
                    if (CheckSegmentObstacleCollision(&characterPosition, &movementVector, 75.0f) != 0) {
                        object->trans.x += movementVector.x;
                        object->trans.y += movementVector.y;
                    }
                    else {
                        object->trans.x = characterPosition.x - characterWork->modelOffset.x;
                        object->trans.y = characterPosition.y - characterWork->modelOffset.y;
                    }
                    if ((teammateWork->motionState == 2) || (teammateWork->motionState == 3)) {
                        movementVector.x = (object->trans.x + characterWork->modelOffset.x) - (teammateObject->trans.x + teammateWork->modelOffset.x);
                        movementVector.y = (object->trans.y + characterWork->modelOffset.y) - (teammateObject->trans.y + teammateWork->modelOffset.y);
                        movementVector.z = (object->trans.z + characterWork->modelOffset.z) - (teammateObject->trans.z + teammateWork->modelOffset.z);
                        playerSeparation = VECMag(&movementVector);
                        if (playerSeparation <= 200.0f) {
                            if (playerSeparation <= 0.0f) {
                                movementVector.y = -1.0f;
                            }
                            VECNormalize(&movementVector, &movementVector);
                            VECScale(&movementVector, &movementVector, 200.0f);
                            object->trans.x = movementVector.x + (teammateObject->trans.x + teammateWork->modelOffset.x);
                            object->trans.y = movementVector.y + (teammateObject->trans.y + teammateWork->modelOffset.x);
                            object->trans.z = movementVector.z + (teammateObject->trans.z + teammateWork->modelOffset.x);
                        }
                    }
                }
                characterPosition = characterWork->characterPosition;
                VECSubtract(&characterPosition, &teamWork->cameraTargets[characterWork->teamPlayerIndex], &positionDelta);
                characterWork->jumpState = 0;
                movementMagnitude = VECMag(&positionDelta);
                teamWork->playerRunSpeeds[characterWork->teamPlayerIndex] = movementMagnitude;
                if (movementMagnitude > teamWork->smoothedRunSpeeds[characterWork->teamPlayerIndex]) {
                    characterWork->jumpState = 1;
                    VECNormalize(&positionDelta, &positionDelta);
                    VECScale(&positionDelta, &positionDelta, teamWork->smoothedRunSpeeds[characterWork->teamPlayerIndex]);
                    VECAdd(&teamWork->cameraTargets[characterWork->teamPlayerIndex], &positionDelta, &characterPosition);
                    object->trans.x += characterPosition.x - characterWork->characterPosition.x;
                    object->trans.y += characterPosition.y - characterWork->characterPosition.y;
                    object->trans.z += characterPosition.z - characterWork->characterPosition.z;
                    movementCorrected = 1;
                }
                if ((movementCorrected != 0) || (object->trans.x != characterWork->worldPosition.x) || (object->trans.y != characterWork->worldPosition.y)) {
                    characterPosition.x = characterWork->worldPosition.x + characterWork->modelOffset.x;
                    characterPosition.y = characterWork->worldPosition.y + characterWork->modelOffset.y;
                    characterPosition.z = characterWork->worldPosition.z + characterWork->modelOffset.z;
                    movementVector.x = (object->trans.x + characterWork->modelOffset.x) - characterPosition.x;
                    movementVector.y = (object->trans.y + characterWork->modelOffset.y) - characterPosition.y;
                    movementVector.z = (object->trans.z + characterWork->modelOffset.z) - characterPosition.z;
                    if (CheckSegmentObstacleCollision(&characterPosition, &movementVector, 75.0f) == 0) {
                        object->trans.x = characterPosition.x - characterWork->modelOffset.x;
                        object->trans.y = characterPosition.y - characterWork->modelOffset.y;
                        movementCorrected = 1;
                    }
                }
                if (object->trans.x < -700.0f) {
                    object->trans.x = -700.0f;
                }
                else if (object->trans.x > 700.0f) {
                    object->trans.x = 700.0f;
                }
                if (object->trans.y < -200.0f) {
                    object->trans.y = -200.0f;
                }
                characterPosition.x = object->trans.x + characterWork->modelOffset.x;
                characterPosition.y = object->trans.y + characterWork->modelOffset.y;
                characterPosition.z = object->trans.z + characterWork->modelOffset.z;
                if (IsPositionInCameraBounds(&characterPosition) != 0) {
                    object->trans.z = 50.0f + characterPosition.z;
                }
                movementMagnitude = sqrtf((characterWork->stickX * characterWork->stickX) + (characterWork->stickY * characterWork->stickY));
                movementMagnitude *= 0.02f;
                CharMotionSpeedSet(characterWork->characterId, movementMagnitude);
                if ((horizontalInput == 0.0f) && (verticalInput == 0.0f)) {
                    CharMotionSpeedSet(characterWork->characterId, 0.0f);
                    if ((characterWork->motionId != 8) && (characterWork->motionId != 9) && (characterWork->motionId != 6)) {
                        targetMotionId = characterWork->motionId;
                    }
                    else {
                        targetMotionId = 4;
                    }
                    targetRotation = object->rot.z;
                }
                else {
                    if (fabs(horizontalInput) > (2.0 * fabs(verticalInput))) {
                        if (horizontalInput > 0.0f) {
                            targetMotionId = 4;
                        }
                        else {
                            targetMotionId = 5;
                        }
                        targetRotation = 0.0f;
                    }
                    else {
                        targetMotionId = 3;
                        if (verticalInput > 0.0f) {
                            targetRotation = -30.0f * (horizontalInput / 1.6f);
                        }
                        else {
                            targetRotation = -(-30.0f * (horizontalInput / 1.6f));
                            CharMotionSpeedSet(characterWork->characterId, -movementMagnitude);
                        }
                    }
                }
                if ((targetMotionId != characterWork->motionId) && (CharMotionShiftIDGet(characterWork->characterId) == -1)) {
                    previousMotionId = characterWork->motionId;
                    characterWork->motionId = targetMotionId;
                    if (previousMotionId != 6) {
                        CharMotionShiftSet(characterWork->characterId, object->mtnId[characterWork->motionId], 0.0f, 8.0f, 0x40000001);
                    }
                    else {
                        CharMotionShiftSet(characterWork->characterId, object->mtnId[characterWork->motionId], 0.0f, 25.0f, 0x40000001);
                    }
                }
                else {
                    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
                }
                object->rot.x = 0.0f;
                object->rot.y = 180.0f;
                object->rot.z = InterpolateWrappedAngle(targetRotation, object->rot.z, 1.25f);
            }
            break;
        case 3:
            characterPosition = characterWork->characterPosition;
            VECSubtract(&characterPosition, &teamWork->cameraTargets[characterWork->teamPlayerIndex], &positionDelta);
            characterWork->jumpState = 0;
            movementMagnitude = VECMag(&positionDelta);
            teamWork->playerRunSpeeds[characterWork->teamPlayerIndex] = movementMagnitude;
            if (GetGamePhase() != 4) {
                characterWork->buttonsHeld = 0;
                characterWork->movementDirection = 0;
            }
            if (((characterWork->buttonsHeld & 0x100) != 0) || (teamWork->playerKnockedOut != 0) || (teamWork->cameraFollowKnockout != 0) || (characterWork->movementDirection != 0)) {
                if (characterWork->movementDirection > 0) {
                    characterWork->movementDirection--;
                }
                if (teamWork->playerKnockedOut != 0) {
                    if (fabs(teamWork->knockbackDistance) > 0.0) {
                        characterPosition.x = object->trans.x;
                        characterPosition.y = object->trans.y;
                        characterPosition.z = object->trans.z;
                        teamWork->player1ResultPosition.z = object->trans.z;
                        VECSubtract(&teamWork->player1ResultPosition, &characterPosition, &positionDelta);
                        playerSeparation = VECMag(&positionDelta);
                        if (playerSeparation < teamWork->knockbackDistance) {
                            teamWork->knockbackDistance = playerSeparation;
                        }
                        if (playerSeparation > 0.0f) {
                            VECNormalize(&positionDelta, &positionDelta);
                            VECScale(&positionDelta, &positionDelta, teamWork->knockbackDistance);
                            VECAdd(&characterPosition, &characterWork->modelOffset, &characterPosition);
                            if (CheckSegmentObstacleCollision(&characterPosition, &positionDelta, 75.0f) != 0) {
                                for (particleIndex = 0; particleIndex < 6; particleIndex++) {
                                    characterPosition.x = object->trans.x + (0.5f * (100.0f * ((0.007874016f * (frand() & 0x7F)) - 0.5f)));
                                    characterPosition.y = 100.0f + object->trans.y + 0.5f * (100.0f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));
                                    characterPosition.z = object->trans.z - 47.5f;
                                    movementMagnitude = -22.5f + (45.0f * (0.007874016f * (frand() & 0x7F)));
                                    movementVector.x = 2.5 * sind(movementMagnitude);
                                    movementVector.y = 2.5 * cosd(movementMagnitude);
                                    movementVector.z = 0.0f;
                                    CreatePairedParticleEffects(teamWork->teamIndex, characterPosition, movementVector, 50.0f, 5.0f, 100.0f, 0.5f, ropePrimaryColor);
                                }
                                object->trans.x += positionDelta.x;
                                object->trans.y += positionDelta.y;
                            }
                        }
                    }
                }
                characterPosition.x = object->trans.x;
                characterPosition.y = object->trans.y;
                characterPosition.z = object->trans.z;
                if (IsPositionInCameraBounds(&characterPosition) != 0) {
                    object->trans.z = 50.0f + characterPosition.z;
                }
                if (object->trans.x < -700.0f) {
                    object->trans.x = -700.0f;
                }
                else if (object->trans.x > 700.0f) {
                    object->trans.x = 700.0f;
                }
                if (object->trans.y < -200.0f) {
                    object->trans.y = -200.0f;
                }
                if (teamRumbleModes[teamWork->teamIndex] == 5) {
                    targetMotionId = 6;
                    if ((object->rot.z > 180.0f) || (object->rot.z < 45.0f)) {
                        object->rot.z += 5.0f;
                    }
                    if (object->rot.z >= 360.0f) {
                        object->rot.z -= 360.0f;
                    }
                }
                else {
                    targetMotionId = characterWork->animationBlendFrames;
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                }
                if ((targetMotionId != characterWork->motionId) && (targetMotionId != -1) && (CharMotionShiftIDGet(characterWork->characterId) == -1)) {
                    previousMotionId = characterWork->motionId;
                    characterWork->motionId = targetMotionId;
                    if (characterWork->motionId != 6) {
                        if (previousMotionId != 6) {
                            CharMotionShiftSet(characterWork->characterId, object->mtnId[characterWork->motionId], 0.0f, 8.0f, 0);
                        }
                        else {
                            CharMotionShiftSet(characterWork->characterId, object->mtnId[characterWork->motionId], 0.0f, 25.0f, 0);
                        }
                        CharMotionSpeedSet(characterWork->characterId, 0.0f);
                    }
                    else {
                        CharMotionShiftSet(characterWork->characterId, object->mtnId[characterWork->motionId], 0.0f, 8.0f, 0x40000001);
                    }
                }
                if ((characterWork->motionId != 6) && (CharMotionShiftIDGet(characterWork->characterId) == -1)) {
                    CharMotionTimeSet(characterWork->characterId, 0.0f);
                    CharMotionSpeedSet(characterWork->characterId, 0.0f);
                }
                if (characterWork->motionId == 6) {
                    CharMotionSpeedSet(characterWork->characterId, 2.0f);
                }
            }
            else {
                characterWork->motionState = 2;
                characterWork->stateFrame = 0;
            }
            break;
        case 4:
            characterPosition = characterWork->characterPosition;
            VECSubtract(&characterPosition, &teamWork->cameraTargets[characterWork->teamPlayerIndex], &positionDelta);
            characterWork->jumpState = 0;
            movementMagnitude = VECMag(&positionDelta);
            teamWork->playerRunSpeeds[characterWork->teamPlayerIndex] = movementMagnitude;
            characterPosition.x = object->trans.x;
            characterPosition.y = object->trans.y;
            characterPosition.z = object->trans.z;
            characterWork->velocity.y -= (VERSION_PAL) ? 0.9408001f : 0.65333337f; // 2352.0 * REFRESH_FREQ * REFRESH_FREQ
            if (teamWork->collisionModelId == -1) {
                teamWork->jumpDistanceLimit = 400.0f;
                teamWork->player0ResultPosition.x = teammateObject->trans.x + teammateWork->modelOffset.x;
                teamWork->player0ResultPosition.y = teammateObject->trans.y + teammateWork->modelOffset.y;
                teamWork->player0ResultPosition.z = teammateObject->trans.z + teammateWork->modelOffset.z;
            }
            else {
                teamWork->jumpDistanceLimit = teamWork->combinedRunSpeed - teamWork->smoothedRunSpeeds[teammateIndex];
            }
            VECAdd(&characterPosition, &characterWork->modelOffset, &characterPosition);
            if ((CheckSegmentObstacleCollision(&characterPosition, &characterWork->velocity, 75.0f) != 0) && ((object->trans.y + characterWork->velocity.y) >= -200.0f)) {
                characterPosition.x = object->trans.x + characterWork->velocity.x;
                characterPosition.y = object->trans.y + characterWork->velocity.y;
                characterPosition.z = object->trans.z + characterWork->velocity.z;
                if (object->trans.x < -700.0f) {
                    object->trans.x = -700.0f;
                }
                else if (object->trans.x > 700.0f) {
                    object->trans.x = 700.0f;
                }
                if (object->trans.y < -200.0f) {
                    object->trans.y = -200.0f;
                }
                VECSubtract(&characterPosition, &teamWork->player0ResultPosition, &positionDelta);
                movementMagnitude = VECMag(&positionDelta);
                if (movementMagnitude > teamWork->jumpDistanceLimit) {
                    VECNormalize(&positionDelta, &positionDelta);
                    VECScale(&positionDelta, &positionDelta, teamWork->jumpDistanceLimit);
                    VECAdd(&teamWork->player0ResultPosition, &positionDelta, &characterPosition);
                    if (fabs(positionDelta.x) < 180.0) {
                        characterWork->landedFrame = 1;
                    }
                }
                object->trans.x = characterPosition.x;
                object->trans.y = characterPosition.y;
                object->trans.z = characterPosition.z;
            }
            else {
                object->trans.x = characterPosition.x - characterWork->modelOffset.x;
                object->trans.y = characterPosition.y - characterWork->modelOffset.y;
                object->trans.z = characterPosition.z - characterWork->modelOffset.z;
                if ((object->trans.y + characterWork->velocity.y) >= -200.0f) {
                    for (particleIndex = 0; particleIndex < 6; particleIndex++) {
                        playerSeparation = -90.0f + (36.0f * particleIndex);
                        characterPosition.x = object->trans.x + (12.5 * sind(playerSeparation));
                        characterPosition.z = object->trans.z + 12.5 * cosd(playerSeparation);
                        characterPosition.y = (object->trans.y - 25.0f) + (0.25f * (100.0f * ((0.007874016f * (frand() & 0x7F)) - 0.5f)));
                        movementMagnitude = -22.5f + (45.0f * (0.007874016f * (frand() & 0x7F)));
                        particleSizeScale = 0.3f + (0.7f * (0.007874016f * (frand() & 0x7F)));
                        movementVector.x = particleSizeScale * ((2.5 * sind(playerSeparation)) + (2.5 * sind(movementMagnitude)));
                        movementVector.y = particleSizeScale * (2.5 * cosd(movementMagnitude));
                        movementVector.z = particleSizeScale * (2.5 * cosd(playerSeparation));
                        CreatePairedParticleEffects(teamWork->teamIndex, characterPosition, movementVector, 100.0f, 5.0f, 100.0f, 0.5f * particleSizeScale, ropeSecondaryColor);
                    }
                }
                characterPosition.x = object->trans.x;
                characterPosition.y = object->trans.y;
                characterPosition.z = object->trans.x;
                characterWork->motionFlags = 1;
                characterWork->velocity.x = characterWork->velocity.y = characterWork->velocity.z = 0.0f;
                teamWork->knockbackDistance = 0.0f;
                characterWork->animationBlendFrames = 4;
                characterWork->movementDirection = REFRESH_RATE * 0.3;
                characterWork->motionState = 5;
                characterWork->stateFrame = 0;
                break;
            }
            if (characterWork->landedFrame == 0) {
                teamWork->knockbackDistance = 0.0f;
                characterWork->stateFrame++;
                characterWork->jumpButtonHeld = 0;
                movementMagnitude = 2.0f * characterWork->stateFrame;
                if (movementMagnitude >= 90.0f) {
                    movementMagnitude = 90.0f;
                }
                if ((object->trans.z = object->trans.z + (10.0 * cosd(movementMagnitude))) >= (150.0f + teamWork->knockoutPosition.z)) {
                    object->trans.z = 150.0f + teamWork->knockoutPosition.z;
                }
                characterPosition.x = object->trans.x;
                characterPosition.y = object->trans.y;
                characterPosition.z = object->trans.z;
                if (IsPositionInCameraBounds(&characterPosition) != 0) {
                    characterPosition.z += 50.0f;
                }
                else {
                    characterPosition.z = 50.0f;
                }
                if (object->trans.z < characterPosition.z) {
                    object->trans.z = characterPosition.z;
                }
                targetMotionId = 8;
                if ((targetMotionId != characterWork->motionId) && (CharMotionShiftIDGet(characterWork->characterId) == -1)) {
                    characterWork->motionId = targetMotionId;
                    CharMotionShiftSet(characterWork->characterId, object->mtnId[characterWork->motionId], 0.0f, 8.0f, 0);
                    CharMotionSpeedSet(characterWork->characterId, 0.0f);
                }
            }
            else {
                characterPosition.x = object->trans.x;
                characterPosition.y = object->trans.y;
                characterPosition.z = object->trans.z;
                VECSubtract(&characterPosition, &teamWork->player0ResultPosition, &positionDelta);
                if (fabs(positionDelta.x) < 100.0) {
                    if ((characterWork->velocity.y += (VERSION_PAL) ? 0.9408001f : 0.65333337f) > 0.0f) {
                        characterWork->velocity.y = 0.0f;
                    }
                    characterWork->velocity.x *= 0.96f;
                    characterWork->velocity.y *= 0.97f;
                }
                else {
                    characterWork->stateFrame++;
                }
                teamWork->knockbackDistance = fabs((27.0f / REFRESH_RATE_F) * sqrtf((characterWork->velocity.x * characterWork->velocity.x) + (characterWork->velocity.y * characterWork->velocity.y)));
                characterPosition.x = object->trans.x;
                characterPosition.y = object->trans.y;
                characterPosition.z = object->trans.z;
                if (IsPositionInCameraBounds(&characterPosition) != 0) {
                    characterPosition.z += 50.0f;
                }
                else {
                    characterPosition.z = 50.0f;
                }
                if ((object->trans.z -= 5.0f) < characterPosition.z) {
                    object->trans.z = 50.0f + characterPosition.z;
                }
                targetMotionId = 9;
                if ((targetMotionId != characterWork->motionId) && (CharMotionShiftIDGet(characterWork->characterId) == -1)) {
                    characterWork->motionId = targetMotionId;
                    CharMotionShiftSet(characterWork->characterId, object->mtnId[characterWork->motionId], 0.0f, 8.0f, 0);
                    CharMotionSpeedSet(characterWork->characterId, 0.0f);
                }
                if (++characterWork->jumpButtonHeld > characterWork->stateFrame) {
                    characterWork->animationBlendFrames = 4;
                    characterWork->movementDirection = REFRESH_RATE * 0.3;
                    characterWork->motionState = 5;
                    characterWork->stateFrame = 0;
                }
            }

            break;
        case 5:
            characterPosition = characterWork->characterPosition;
            VECSubtract(&characterPosition, &teamWork->cameraTargets[characterWork->teamPlayerIndex], &positionDelta);
            characterWork->jumpState = 0;
            movementMagnitude = VECMag(&positionDelta);
            teamWork->playerRunSpeeds[characterWork->teamPlayerIndex] = movementMagnitude;
            if (characterWork->motionFlags != 0) {
                targetMotionId = 7;
                if ((targetMotionId != characterWork->motionId) && (CharMotionShiftIDGet(characterWork->characterId) == -1)) {
                    characterWork->motionId = targetMotionId;
                    CharMotionShiftSet(characterWork->characterId, object->mtnId[characterWork->motionId], 0.0f, 8.0f, 0);
                    CharMotionSpeedSet(characterWork->characterId, 0.0f);
                }
            }
            playersSeparated = 0;
            if ((teammateWork->motionState == 2) || (teammateWork->motionState == 3)) {
                movementVector.x = (object->trans.x + characterWork->modelOffset.x) - (teammateObject->trans.x + teammateWork->modelOffset.x);
                movementVector.y = (object->trans.y + characterWork->modelOffset.y) - (teammateObject->trans.y + teammateWork->modelOffset.y);
                movementVector.z = (object->trans.z + characterWork->modelOffset.z) - (teammateObject->trans.z + teammateWork->modelOffset.z);
                playerSeparation = VECMag(&movementVector);
                if (playerSeparation <= 200.0f) {
                    if (object->trans.x > teammateObject->trans.x) {
                        playerSeparation = 10.0f;
                    }
                    else {
                        playerSeparation = -10.0f;
                    }
                    characterPosition.x = object->trans.x + characterWork->modelOffset.x;
                    characterPosition.y = object->trans.y + characterWork->modelOffset.y;
                    characterPosition.z = object->trans.z + characterWork->modelOffset.z;
                    movementVector.x = playerSeparation;
                    movementVector.y = movementVector.z = 0.0f;
                    if (CheckSegmentObstacleCollision(&characterPosition, &movementVector, 75.0f) != 0) {
                        object->trans.x += movementVector.x;
                        object->trans.y += movementVector.y;
                        object->trans.z += movementVector.z;
                    }
                    else {
                        object->trans.x = characterPosition.x - characterWork->modelOffset.x;
                        object->trans.y = characterPosition.y - characterWork->modelOffset.y;
                        object->trans.z = characterPosition.z - characterWork->modelOffset.z;
                    }
                    characterPosition.x = teammateObject->trans.x + teammateWork->modelOffset.x;
                    characterPosition.y = teammateObject->trans.y + teammateWork->modelOffset.y;
                    characterPosition.z = teammateObject->trans.z + teammateWork->modelOffset.z;
                    movementVector.x = -playerSeparation;
                    movementVector.y = movementVector.z = 0.0f;
                    if (CheckSegmentObstacleCollision(&characterPosition, &movementVector, 75.0f) != 0) {
                        teammateObject->trans.x += movementVector.x;
                        teammateObject->trans.y += movementVector.y;
                        teammateObject->trans.z += movementVector.z;
                    }
                    else {
                        teammateObject->trans.x = characterPosition.x - teammateWork->modelOffset.x;
                        teammateObject->trans.y = characterPosition.y - teammateWork->modelOffset.y;
                        teammateObject->trans.z = characterPosition.z - teammateWork->modelOffset.z;
                    }
                    playersSeparated = 1;
                }
            }
            if (object->trans.x < -700.0f) {
                object->trans.x = -700.0f;
            }
            else if (object->trans.x > 700.0f) {
                object->trans.x = 700.0f;
            }
            if (object->trans.y < -200.0f) {
                object->trans.y = -200.0f;
            }
            characterPosition.x = object->trans.x;
            characterPosition.y = object->trans.y;
            characterPosition.z = object->trans.z;
            if (IsPositionInCameraBounds(&characterPosition) != 0) {
                characterPosition.z += 50.0f;
            }
            else {
                characterPosition.z = 50.0f;
            }
            if ((object->trans.z - characterPosition.z) > 30.000002f) {
                object->trans.z -= 30.000002f;
            }
            else {
                object->trans.z = characterPosition.z;
                if (playersSeparated == 0) {
                    characterWork->motionState = 3;
                    characterWork->stateFrame = 0;
                }
            }
            break;
        case 7:
            object->trans.x = playerModelOffsets[characterWork->teamPlayerIndex].x;
            object->trans.y = playerModelOffsets[characterWork->teamPlayerIndex].y;
            object->trans.z = playerModelOffsets[characterWork->teamPlayerIndex].z;
            object->rot.x = object->rot.y = object->rot.z = 0.0f;
            model = &Hu3DData[object->mdlId[0]];
            model->mtx[0][3] = 0.0f;
            model->mtx[1][3] = 0.0f;
            model->mtx[2][3] = 0.0f;
            model = &Hu3DData[object->mdlId[1]];
            MTXIdentity(model->mtx);
            Hu3DModelPosSet(
                object->mdlId[1], characterHookOffsets[characterWork->teamPlayerIndex].x, characterHookOffsets[characterWork->teamPlayerIndex].y, characterHookOffsets[characterWork->teamPlayerIndex].z);
            Hu3DModelRotSet(
                object->mdlId[1], characterHookRotations[characterWork->teamPlayerIndex].x, characterHookRotations[characterWork->teamPlayerIndex].y, characterHookRotations[characterWork->teamPlayerIndex].z);
            if (characterWork->teamIndex == 0) {
                AddModelToCameraShadows(object->mdlId[1], 1);
            }
            else {
                AddModelToCameraShadows(object->mdlId[1], 2);
            }
            targetMotionId = 0;
            if ((targetMotionId != characterWork->motionId) && (CharMotionShiftIDGet(characterWork->characterId) == -1)) {
                characterWork->motionId = targetMotionId;
                CharMotionShiftSet(characterWork->characterId, object->mtnId[characterWork->motionId], 0.0f, 8.0f, 0x40000001);
            }
            break;
        case 8:
            targetMotionId = 0xA;
            if ((targetMotionId != characterWork->motionId) && (CharMotionShiftIDGet(characterWork->characterId) == -1)) {
                characterWork->motionId = targetMotionId;
                CharMotionShiftSet(characterWork->characterId, object->mtnId[characterWork->motionId], 0.0f, 8.0f, 0);
            }
            break;
        case 9:
            Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
            break;
    }
    characterWork->worldPosition.x = object->trans.x;
    characterWork->worldPosition.y = object->trans.y;
    characterWork->worldPosition.z = object->trans.z;
    if (characterWork->motionState < 7) {
        if (FindModelJointTransform(object->mdlId[0], characterWork->hookName, modelTransform, 0) != 0) {
            model = &Hu3DData[object->mdlId[1]];
            MTXConcat(modelTransform, characterWork->modelMatrix, modelTransform);
            MTXCopy(modelTransform, model->mtx);
            characterWork->characterPosition.x = modelTransform[0][3];
            characterWork->characterPosition.y = modelTransform[1][3];
            characterWork->characterPosition.z = modelTransform[2][3];
        }
        else {
            OSReport("NO OBJECT %d %s \n", characterWork->characterId, CharModelItemHookGet(characterWork->characterId, 4, 4));
        }
        characterPosition.x = 0.0f;
        characterPosition.y = 100.0f;
        characterPosition.z = 0.0f;
        movementVector.x = characterPosition.y * sind((180.0f + object->rot.z));
        movementVector.y = characterPosition.y * sind((90.0f + object->rot.z));
        movementVector.z = 0.0f;
        VECSubtract(&movementVector, &characterPosition, &positionDelta);
        model = &Hu3DData[object->mdlId[0]];
        model->mtx[0][3] = positionDelta.x;
        model->mtx[1][3] = positionDelta.y;
        model->mtx[2][3] = positionDelta.z;
    }
}

void UpdatePlayerEffect(OMOBJ *object)
{
    object->objFunc = NULL;
}

void UpdatePlayerEffectCallback(OMOBJ *object)
{
    Vec characterPosition;
    Vec targetPosition;
    Vec targetDelta;
    s32 particlePathIndex;
    Float aiSteeringScale;
    Float targetDistance;
    Float targetAngle;
    M428DllCharacterWork *characterWork;
    M428DllTeamWork *teamWork;
    OMOBJ *teammateObject;
    s32 pathPointIndex;
    s32 teammateIsLeading;
    s32 aiActionRoll;
    s32 teammateIndex;

    characterWork = object->data;
    teamWork = &teamWorkArray[characterWork->teamIndex];
    teammateIndex = characterWork->teamPlayerIndex ^ 1;
    teammateObject = teamWork->playerObjects[teammateIndex];
    if ((teamWork->animationState == 3) && (teamWork->leadingPlayerIndex == characterWork->teamPlayerIndex)) {
        teammateIsLeading = 1;
    }
    else {
        teammateIsLeading = 0;
    }
    if ((teamRumbleModes[characterWork->teamIndex] == 6) || (characterWork->hookModelId == -1)) {
        if (teamWork->animationState != 3) {
            characterWork->hookModelId = 2;
        }
        else {
            aiActionRoll = (s32)(50.0f * (0.007874016f * (frand() & 0x7F))) + characterWork->computerDifficulty * 0xF + 5;
            characterWork->idleDelayFrames = (u8)frand() % (s32)(REFRESH_RATE * 1.5);
            if (teammateIsLeading == 0) {
                characterWork->idleDelayFrames = (u8)frand() % REFRESH_RATE;
            }
            if (aiActionRoll < 0x2D) {
                characterWork->hookModelId = 0;
            }
            else if (aiActionRoll < 0x4B) {
                characterWork->hookModelId = 1;
            }
            else {
                characterWork->hookModelId = 2;
            }
        }
    }
    aiSteeringScale = 1.0f;
    switch (characterWork->hookModelId) {
        case 0:
            break;
        case 1:
            if (teamRumbleModes[characterWork->teamIndex] != 0) {
                if (characterWork->idleDelayFrames > 0) {
                    characterWork->idleDelayFrames--;
                }
                else {
                    characterWork->buttonsPressed |= 0x100;
                    characterWork->buttonsHeld |= 0x100;
                }
            }
            break;
        case 2:
            if ((teamRumbleModes[characterWork->teamIndex] == 3) || (teamRumbleModes[characterWork->teamIndex] == 5)) {
                characterWork->buttonsPressed |= 0x100;
                characterWork->buttonsHeld |= 0x100;
            }
            break;
    }
    if (teammateIsLeading != 0) {
        for (pathPointIndex = 0; pathPointIndex < 0x10; pathPointIndex++) {
            if ((characterJointHookOffsets[teamWork->randomVariant][pathPointIndex].y - 100.0f) > object->trans.y) {
                break;
            }
        }
        characterWork->particleEffectId = pathPointIndex;
        targetPosition = characterJointHookOffsets[teamWork->randomVariant][characterWork->particleEffectId];
        particlePathIndex = characterWork->particleEffectId;
        characterPosition.x = object->trans.x;
        characterPosition.y = object->trans.y;
        characterPosition.z = object->trans.z;
        VECSubtract(&characterWork->worldPosition, &characterPosition, &targetDelta);
        if ((VECMag(&targetDelta) < 1.0f) && (teamRumbleModes[characterWork->teamIndex] != 0)) {
            characterWork->effectFrame++;
        }
        if (characterWork->effectFrame >= REFRESH_RATE_F * 3) {
            if (characterWork->effectFrame < REFRESH_RATE_F * 5) {
                if ((characterWork->particleEffectId >= 2) && (characterWork->particleEffectId <= 3)) {
                    if (fabs(object->trans.x) < 500.0) {
                        if (object->trans.x > 0.0f) {
                            characterWork->stickX = 64.0f;
                        }
                        else {
                            characterWork->stickX = -64.0f;
                        }
                    }
                }
                else {
                    if (fabs(object->trans.x) > 50.0) {
                        if (object->trans.x > 0.0f) {
                            characterWork->stickX = -64.0f;
                        }
                        else {
                            characterWork->stickX = 64.0f;
                        }
                    }
                }
            }
            else if (characterWork->particleEffectId > 0) {
                characterWork->particleEffectId--;
                characterWork->effectFrame = 0;
            }
        }
        else {
            characterWork->effectFrame = 0;
            characterPosition.x = object->trans.x;
            characterPosition.y = object->trans.y;
            characterPosition.z = object->trans.z;
            VECAdd(&targetPosition, &characterWork->effectOffset, &targetPosition);
            VECSubtract(&targetPosition, &characterPosition, &targetDelta);
            targetDistance = VECMag(&targetDelta);
            VECNormalize(&targetDelta, &targetDelta);
            characterWork->stickX = 64.0f * (targetDelta.x * aiSteeringScale);
            characterWork->stickY = 64.0f * (targetDelta.y * aiSteeringScale);
            if (++characterWork->footstepEffectId > REFRESH_RATE_F * 3) {
                characterWork->effectOffset.x = 0.5f * (100.0f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));
                characterWork->effectOffset.y = 0.5f * (100.0f * (0.007874016f * (frand() & 0x7F)));
                characterWork->effectOffset.z = 0.5f * (100.0f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));
                characterWork->footstepEffectId++;
            }
        }
    }
    else {
        characterPosition.x = object->trans.x;
        characterPosition.y = object->trans.y;
        characterPosition.z = object->trans.z;
        targetPosition.x = teammateObject->trans.x;
        targetPosition.y = teammateObject->trans.y;
        targetPosition.z = teammateObject->trans.z;
        VECSubtract(&targetPosition, &characterPosition, &targetDelta);
        targetDistance = VECMag(&targetDelta);
        if (targetDistance != 0.0f) {
            targetAngle = atan2d(targetDelta.x, targetDelta.y);
        }
        else {
            targetDelta.y = 1.0f;
            targetAngle = 0.0;
        }
        if (targetDistance < 230.0f) {
            targetDelta.x *= -1.0f;
            targetDelta.y *= -1.0f;
            targetAngle += 180.0f;
        }
        if ((targetDistance >= 250.0f) || (targetDistance <= 230.0f)) {
            VECNormalize(&targetDelta, &targetDelta);
            if (targetDistance >= 250.0f) {
                aiSteeringScale = (targetDistance - 250.0f) / 20.0f;
                if (aiSteeringScale > 1.0f) {
                    aiSteeringScale = 1.0f;
                }
            }
            else {
                aiSteeringScale = 1.0f;
            }
            characterPosition.x = object->trans.x;
            characterPosition.y = object->trans.y;
            characterPosition.z = object->trans.z;
            VECAdd(&characterPosition, &characterWork->modelOffset, &characterPosition);
            targetAngle = targetAngle + characterWork->footstepScale;
            targetPosition.x = 64.0 * sind(targetAngle);
            targetPosition.y = 64.0 * cosd(targetAngle);
            targetPosition.z = 0.0f;
            characterWork->stickX = targetPosition.x * aiSteeringScale;
            characterWork->stickY = targetPosition.y * aiSteeringScale;
            characterWork->footstepFrame = 0;
        }
    }
    characterWork->footstepEffectId++;
}

s32 CheckPointObstacleCollision(Vec *position, Vec *movementDelta, Float obstacleClearance)
{
    Vec candidatePosition;
    Vec lastValidPosition;
    Vec stepDirection;
    Float stepAngle;
    Float movementAngle;
    s32 collisionProbe;

    if (VECMag(movementDelta) <= 0.0f) {
        return 0;
    }
    movementAngle = atan2d(movementDelta->x, movementDelta->y);
    movementAngle = movementAngle;
    candidatePosition = *position;
    for (collisionProbe = 0; collisionProbe < 6; collisionProbe++) {
        stepAngle = -45.0f + (15.0f * collisionProbe);
        stepAngle += movementAngle;
        stepDirection.x = sind(stepAngle);
        stepDirection.y = cosd(stepAngle);
        stepDirection.z = 0.0f;
        VECNormalize(&stepDirection, &stepDirection);
        VECScale(&stepDirection, &stepDirection, obstacleClearance);
        VECAdd(position, &stepDirection, &candidatePosition);
        if (IsPositionInCameraBounds(&candidatePosition) != 0) {
            if (collisionProbe == 3) {
                lastValidPosition = candidatePosition;
            }
        }
        else {
            break;
        }
    }
    if (collisionProbe < 6) {
        return 0;
    }
    return 1;
}

s32 CheckSegmentObstacleCollision(Vec *segmentPosition, Vec *movementDelta, Float obstacleClearance)
{
    Vec candidatePosition;
    Vec movementDelta;
    Vec resolvedPosition;
    s32 collisionResult;

    candidatePosition = *segmentPosition;
    movementDelta = *movementDelta;
    collisionResult = CheckPlayerObstacleCollision(&candidatePosition, &movementDelta, &resolvedPosition, obstacleClearance);
    if (collisionResult != 0) {
        *segmentPosition = resolvedPosition;
        return 0;
    }
    *segmentPosition = candidatePosition;
    return 1;
}

s32 GetPlayerCount(void)
{
    s32 playerIndex;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (playerObjects[playerIndex]->trans.y < 0.0f) {
            break;
        }
    }
    if (playerIndex < 4) {
        return 0;
    }
    return 1;
}

s32 GetTeamPlayerCount(s32 teamIndex)
{
    M428DllTeamWork *teamWork = &teamWorkArray[teamIndex];
    if (teamWork->highestPlayerY >= 3800.0f) {
        return teamWork->knockoutFlag = 1;
    }
    return teamWork->knockoutFlag = 0;
}

void SetRoundEndFlag(s32 teamIndex)
{
    M428DllTeamWork *teamWork = &teamWorkArray[teamIndex];
    teamWork->roundResultFlag = 1;
    roundEndFlag = 1;
}

void SetTeamRumbleMode(s32 teamIndex, s32 rumbleMode)
{
    teamRumbleModes[teamIndex] = rumbleMode;
}

void SetTeamRumbleModeValue(s32 teamIndex, s32 rumbleMode)
{
    if (rumbleState != 0) {
        M428DllTeamWork *teamWork = &teamWorkArray[teamIndex];
        teamWork->rumbleFrame = 0;
        teamWork->rumbleMode = rumbleMode;
    }
}

void StartTeamRumble(s32 teamIndex, s32 rumbleDuration, Float rumbleStrength)
{
    M428DllTeamWork *teamWork = &teamWorkArray[teamIndex];
    teamWork->rumbleTotalFrames = rumbleDuration;
    teamWork->rumbleRemainingFrames = rumbleDuration;
    teamWork->rumbleAmplitude = rumbleStrength;
}

s32 GetTeamRumbleMode(s32 teamIndex)
{
    M428DllTeamWork *teamWork = &teamWorkArray[teamIndex];
    if (teamWork->highestPlayerY >= 500.0f) {
        return 1;
    }
    return 0;
}

s32 CheckPlayerObstacleCollision(Vec *playerPosition, Vec *movementDelta, Vec *resolvedPosition, Float obstacleClearance)
{
    Vec obstacleCenter;
    Vec candidatePosition;
    Vec obstacleOffset;
    Vec collisionNormal;
    Vec edgeNormal;
    Float distanceToObstacle;
    Float edgeIntersectionX;
    M428DllObstacleData *obstacle;
    s32 obstacleIndex;

    *resolvedPosition = *playerPosition;
    obstacle = obstacleCollisionData;
    for (obstacleIndex = 0; obstacleIndex < 5; obstacleIndex++, obstacle++) {
        obstacleCenter = obstacle->obstacleCenter;
        VECAdd(playerPosition, movementDelta, &candidatePosition);
        if ((playerPosition->y >= (obstacleCenter.y + obstacleClearance)) && (candidatePosition.y <= (obstacleCenter.y + obstacleClearance))) {
            edgeNormal.x = candidatePosition.y - playerPosition->y;
            edgeNormal.y = playerPosition->x - candidatePosition.x;
            edgeNormal.z = -((edgeNormal.x * playerPosition->x) + (edgeNormal.y * playerPosition->y));
            edgeIntersectionX = -((edgeNormal.z + (edgeNormal.y * (obstacleCenter.y + obstacleClearance))) / edgeNormal.x);
            if ((edgeIntersectionX >= (obstacleCenter.x - (obstacle->collisionRadius + obstacleClearance))) && (edgeIntersectionX <= (obstacleCenter.x + (obstacle->collisionRadius + obstacleClearance)))) {
                resolvedPosition->x = playerPosition->x + movementDelta->x;
                resolvedPosition->y = 0.1f + (obstacleCenter.y + obstacleClearance);
                resolvedPosition->z = playerPosition->z + movementDelta->z;
                return 1;
            }
        }
        VECSubtract(&candidatePosition, &obstacleCenter, &obstacleOffset);
        distanceToObstacle = VECMag(&obstacleOffset);
        if (distanceToObstacle == 0.0f) {
            obstacleOffset.y = 1.0f;
        }
        collisionNormal = obstacleOffset;
        if ((distanceToObstacle < (obstacle->collisionRadius + obstacleClearance)) && (obstacleOffset.y < obstacleClearance)) {
            obstacleCenter = obstacle->obstacleCenter;
            candidatePosition = *playerPosition;
            VECSubtract(&candidatePosition, &obstacleCenter, &obstacleOffset);
            distanceToObstacle = VECMag(&obstacleOffset);
            if (distanceToObstacle == 0.0f) {
                obstacleOffset.y = 1.0f;
            }
            if (obstacleOffset.y <= obstacleClearance) {
                VECNormalize(&collisionNormal, &obstacleOffset);
                VECScale(&obstacleOffset, &obstacleOffset, obstacle->collisionRadius + obstacleClearance);
                VECAdd(&obstacleCenter, &obstacleOffset, resolvedPosition);
                return 1;
            }
            resolvedPosition->x = playerPosition->x + movementDelta->x;
            resolvedPosition->y = 0.1f + (obstacleCenter.y + obstacleClearance);
            resolvedPosition->z = playerPosition->z + movementDelta->z;
            return 1;
        }
    }
    return 0;
}

s32 CheckSegmentPairCollision(Vec *segmentPosition, Vec *segmentDelta, Vec *resolvedPosition)
{
    Vec obstacleCenter;
    Vec candidatePosition;
    Vec obstacleOffset;
    Vec edgeNormal;
    Float distanceToObstacle;
    Float edgeIntersectionX;
    M428DllObstacleData *obstacle;
    s32 obstacleIndex;

    *resolvedPosition = *segmentPosition;
    obstacle = obstacleCollisionData;
    for (obstacleIndex = 0; obstacleIndex < 5; obstacleIndex++, obstacle++) {
        obstacleCenter = obstacle->obstacleCenter;
        VECAdd(segmentPosition, segmentDelta, &candidatePosition);
        if ((segmentPosition->y >= obstacleCenter.y) && (candidatePosition.y <= obstacleCenter.y)) {
            edgeNormal.x = candidatePosition.y - segmentPosition->y;
            edgeNormal.y = segmentPosition->x - candidatePosition.x;
            edgeNormal.z = -((edgeNormal.x * segmentPosition->x) + (edgeNormal.y * segmentPosition->y));
            edgeIntersectionX = -((edgeNormal.z + (edgeNormal.y * obstacleCenter.y)) / edgeNormal.x);
            if ((edgeIntersectionX >= (obstacleCenter.x - obstacle->collisionRadius)) && (edgeIntersectionX <= (obstacleCenter.x + obstacle->collisionRadius))) {
                resolvedPosition->x = segmentPosition->x + segmentDelta->x;
                resolvedPosition->y = obstacleCenter.y;
                resolvedPosition->z = segmentPosition->z + segmentDelta->z;
                return 1;
            }
        }
        VECSubtract(&candidatePosition, &obstacleCenter, &obstacleOffset);
        distanceToObstacle = VECMag(&obstacleOffset);
        if (distanceToObstacle == 0.0f) {
            obstacleOffset.y = 1.0f;
        }
        if ((distanceToObstacle < obstacle->collisionRadius) && (obstacleOffset.y <= 0.0f)) {
            obstacleCenter = obstacle->obstacleCenter;
            candidatePosition = *segmentPosition;
            VECSubtract(&candidatePosition, &obstacleCenter, &obstacleOffset);
            distanceToObstacle = VECMag(&obstacleOffset);
            if (distanceToObstacle == 0.0f) {
                obstacleOffset.y = 1.0f;
            }
            if (obstacleOffset.y <= 0.0f) {
                VECNormalize(&obstacleOffset, &obstacleOffset);
                VECScale(&obstacleOffset, &obstacleOffset, obstacle->collisionRadius);
                VECAdd(&obstacleCenter, &obstacleOffset, resolvedPosition);
                return obstacleIndex;
            }
            resolvedPosition->x = segmentPosition->x + segmentDelta->x;
            resolvedPosition->y = obstacleCenter.y;
            resolvedPosition->z = segmentPosition->z + segmentDelta->z;
            return obstacleIndex;
        }
    }
    return -1;
}

s32 CreateRopeModel(Vec *startPoint, Vec *endPoint, Float ropeWidth, s32 segmentCount, s32 colorIndex)
{
    Float segmentProgress;
    M428DllRopeWork *ropeWork;
    M428DllRopeSegmentWork *ropeSegment;
    s32 segmentIndex;
    HU3DMODEL *model;
    s32 hookModelId;

    hookModelId = Hu3DHookFuncCreate(DrawPlayerParticleHook);
    model = &Hu3DData[hookModelId];
    Hu3DModelLayerSet(hookModelId, 2);
    ropeWork = HuMemDirectMallocNum(HEAP_MODEL, sizeof(M428DllRopeWork), model->mallocNo);
    model->hookData = ropeWork;
    memset(ropeWork, 0, sizeof(M428DllRopeWork));
    ropeSegment = HuMemDirectMallocNum(HEAP_MODEL, segmentCount * sizeof(M428DllRopeSegmentWork), model->mallocNo);
    ropeWork->segments = ropeSegment;
    memset(ropeSegment, 0, segmentCount * sizeof(M428DllRopeSegmentWork));
    ropeWork->hookModelId = hookModelId;
    ropeWork->segmentCount = segmentCount;
    ropeWork->ropeWidth = ropeWidth;
    ropeWork->segmentSpacing = ropeWidth / (segmentCount - 1);
    ropeWork->effectMode = 0;
    ropeWork->colorIndex = colorIndex;
    ropeWork->startAlpha = 0.0f;
    ropeWork->endAlpha = 0.0f;
    ropeWork->useCenterControlPoint = 0;
    ropeWork->startPoint = *startPoint;
    ropeWork->endPoint = *endPoint;
    ropeWork->centerControlPoint.x = ropeWork->centerControlPoint.y = ropeWork->centerControlPoint.z = 0.0f;
    ropeWork->centerSegmentIndex = -1;
    ropeWork->reservedRopeData48 = ropeWork->reservedRopeData4C = ropeWork->reservedRopeData50 = 0.0f;
    for (segmentIndex = 0; segmentIndex < segmentCount; segmentIndex++, ropeSegment++) {
        ropeSegment->segmentIndex = segmentIndex;
        if (segmentIndex == 0) {
            ropeSegment->endpointType = 0;
        }
        else if (segmentIndex == (segmentCount - 1)) {
            ropeSegment->endpointType = 1;
        }
        else {
            ropeSegment->endpointType = -1;
        }
        ropeSegment->intersectsObstacle = 0;
        segmentProgress = (Float)segmentIndex / (segmentCount - 1);
        ropeSegment->segmentVertices[0].x = ropeWork->startPoint.x + (segmentProgress * (ropeWork->endPoint.x - ropeWork->startPoint.x));
        ropeSegment->segmentVertices[0].y = ropeWork->startPoint.y + (segmentProgress * (ropeWork->endPoint.y - ropeWork->startPoint.y));
        ropeSegment->segmentVertices[0].z = ropeWork->startPoint.z + (segmentProgress * (ropeWork->endPoint.z - ropeWork->startPoint.z));
        ropeSegment->segmentVertices[1] = ropeSegment->segmentVertices[0];
        ropeSegment->segmentVertices[2] = ropeSegment->segmentVertices[0];
        ropeSegment->segmentVertices[3].x = ropeSegment->segmentVertices[3].y = ropeSegment->segmentVertices[3].z = 0.0f;
    }
    return hookModelId;
}

void UpdateRopeModel(M428DllRopeWork *ropeWork)
{
    Vec segmentDelta;
    Vec collisionPoint;
    Vec curveControlPoint;
    s32 collisionResult;
    Float segmentProgress;
    Float previousSegmentZ;
    M428DllRopeSegmentWork *ropeSegment;
    s32 segmentIndex;

    if (ropeWork->useCenterControlPoint == 0) {
        InterpolateVector(0.5f, &ropeWork->startPoint, &ropeWork->endPoint, &curveControlPoint);
        curveControlPoint.y -= ropeWork->startAlpha * (0.75f * ropeWork->ropeWidth);
        for (ropeSegment = ropeWork->segments, segmentIndex = 0; segmentIndex < ropeWork->segmentCount; segmentIndex++, ropeSegment++) {
            segmentProgress = (Float)segmentIndex / (ropeWork->segmentCount - 1);
            InterpolateQuadraticVector(segmentProgress, &ropeWork->startPoint, &curveControlPoint, &ropeWork->endPoint, &ropeSegment->segmentVertices[0]);
            InterpolateVector(segmentProgress, &ropeWork->startPoint, &ropeWork->endPoint, &ropeSegment->segmentVertices[2]);
        }
    }
    else {
        ropeWork->centerControlPoint.z = 0.5f * (ropeWork->startPoint.z + ropeWork->endPoint.z);
        InterpolateVector(0.5f, &ropeWork->startPoint, &ropeWork->centerControlPoint, &curveControlPoint);
        curveControlPoint.y -= ropeWork->startAlpha * (0.375f * ropeWork->ropeWidth);
        for (ropeSegment = ropeWork->segments, segmentIndex = 0; segmentIndex < ropeWork->centerSegmentIndex; segmentIndex++, ropeSegment++) {
            segmentProgress = (Float)segmentIndex / (ropeWork->centerSegmentIndex - 1);
            InterpolateQuadraticVector(segmentProgress, &ropeWork->startPoint, &curveControlPoint, &ropeWork->centerControlPoint, &ropeSegment->segmentVertices[0]);
            InterpolateVector(segmentProgress, &ropeWork->startPoint, &ropeWork->centerControlPoint, &ropeSegment->segmentVertices[2]);
        }
        InterpolateVector(0.1f, &ropeWork->centerControlPoint, &ropeWork->endPoint, &curveControlPoint);
        curveControlPoint.y -= ropeWork->endAlpha * (0.375f * ropeWork->ropeWidth);
        for (ropeSegment = &ropeWork->segments[ropeWork->centerSegmentIndex], segmentIndex = ropeWork->centerSegmentIndex; segmentIndex < ropeWork->segmentCount; segmentIndex++, ropeSegment++) {
            segmentProgress = (Float)(segmentIndex - ropeWork->centerSegmentIndex) / ((ropeWork->segmentCount - ropeWork->centerSegmentIndex) - 1);
            InterpolateQuadraticVector(segmentProgress, &ropeWork->centerControlPoint, &curveControlPoint, &ropeWork->endPoint, &ropeSegment->segmentVertices[0]);
            InterpolateVector(segmentProgress, &ropeWork->centerControlPoint, &ropeWork->endPoint, &ropeSegment->segmentVertices[2]);
        }
    }
    for (segmentIndex = 0, ropeSegment = ropeWork->segments; segmentIndex < ropeWork->segmentCount; segmentIndex++, ropeSegment++) {
        PSVECSubtract(&ropeSegment->segmentVertices[0], &ropeSegment->segmentVertices[2], &segmentDelta);
        previousSegmentZ = ropeSegment->segmentVertices[0].z;
        if ((collisionResult = CheckSegmentPairCollision(&ropeSegment->segmentVertices[2], &segmentDelta, &collisionPoint)) != -1) {
            ropeSegment->segmentVertices[0] = collisionPoint;
            ropeSegment->segmentVertices[0].z = previousSegmentZ;
            ropeSegment->intersectsObstacle = 1;
        }
        else {
            ropeSegment->intersectsObstacle = 0;
        }
        if (ropeSegment->endpointType == -1) {
            InterpolateVector(0.9f, &ropeSegment->segmentVertices[1], &ropeSegment->segmentVertices[0], &ropeSegment->segmentVertices[0]);
        }
        ropeSegment->segmentVertices[1] = ropeSegment->segmentVertices[0];
    }
}

s32 ropeSegmentColors[2] = { 0xFFA040FF, 0xC8FFFF };
s16 scoreProgressXOffsets[2] = { -0x10, 0x10 };
s16 scoreProgressYBounds[2] = { 0x1A4, 0x60 };
Vec particleQuadCorners[4] = {
    { -0.5f, 0.5f, 0.0f },
    { 0.5f, 0.5f, 0.0f },
    { 0.5f, -0.5f, 0.0f },
    { -0.5f, -0.5f, 0.0f },
};
Vec2f particleTextureCornerOffsets[4] = { { 0.0f, 0.0f }, { 0.25f, 0.0f }, { 0.25f, 0.25f }, { 0.0f, 0.25f } };

void DrawPlayerParticleHook(HU3DMODEL *model, Mtx matrix)
{
    Mtx normalMatrix;
    s16 vertexIndex;

    M428DllRopeWork *ropeWork = model->hookData;
    if (omPauseChk() == 0) {
        UpdateRopeModel(ropeWork);
    }
    GXLoadPosMtxImm(matrix, 0);
    MTXInvXpose(matrix, normalMatrix);
    GXLoadNrmMtxImm(normalMatrix, 0);
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
    GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_VTX, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_SPEC);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GXSetZCompLoc(GX_FALSE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
    GXSetLineWidth(0x10, GX_TO_ZERO);
    // bug, it should be ropeWork->segments
    DCFlushRange(&ropeWork->segments, ropeWork->segmentCount * sizeof(M428DllRopeSegmentWork));
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_POS, ropeWork->segments->segmentVertices, sizeof(M428DllRopeSegmentWork));
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, &ropeSegmentColors[ropeWork->colorIndex], 4);
    GXBegin(GX_LINESTRIP, GX_VTXFMT0, ropeWork->segmentCount);
    for (vertexIndex = 0; vertexIndex < ropeWork->segmentCount; vertexIndex++) {
        GXPosition1x16(vertexIndex);
        GXColor1x16(0);
    }
}

void SetRopeEndpoint(s32 modelId, Vec *startPoint, Vec *endPoint)
{
    HU3DMODEL *model = &Hu3DData[modelId];
    M428DllRopeWork *ropeWork = model->hookData;
    ropeWork->startPoint = *startPoint;
    ropeWork->endPoint = *endPoint;
}

void ConfigureRopeEndpoint(s32 modelId, s32 useCenterControlPoint, s32 centerSegmentIndex, Vec *centerControlPoint)
{
    HU3DMODEL *model = &Hu3DData[modelId];
    M428DllRopeWork *ropeWork = model->hookData;
    ropeWork->useCenterControlPoint = useCenterControlPoint;
    if (useCenterControlPoint != 0) {
        ropeWork->centerControlPoint = *centerControlPoint;
        ropeWork->centerSegmentIndex = centerSegmentIndex;
    }
}

void SetRopeEndpointAlpha(s32 modelId, Float startAlpha, Float endAlpha)
{
    HU3DMODEL *model = &Hu3DData[modelId];
    M428DllRopeWork *ropeWork = model->hookData;
    if (startAlpha > 1.0f) {
        startAlpha = 1.0f;
    }
    else if (startAlpha < 0.0f) {
        startAlpha = 0.0f;
    }
    if (endAlpha > 1.0f) {
        endAlpha = 1.0f;
    }
    else if (endAlpha < 0.0f) {
        endAlpha = 0.0f;
    }
    ropeWork->startAlpha = startAlpha;
    ropeWork->endAlpha = endAlpha;
}

s32 FindRopeSegmentIntersection(s32 modelId, s32 reverseOrder, Vec *intersectionPoint, Float *distanceAlongRope)
{
    M428DllRopeSegmentWork *ropeSegment;
    s32 segmentIndex;

    HU3DMODEL *model = &Hu3DData[modelId];
    M428DllRopeWork *ropeWork = model->hookData;
    Float segmentDistance = 0.0f;
    for (segmentIndex = 0; segmentIndex < ropeWork->segmentCount; segmentDistance += ropeWork->segmentSpacing, segmentIndex++) {
        if (reverseOrder == 0) {
            ropeSegment = &ropeWork->segments[segmentIndex];
        }
        else {
            ropeSegment = &ropeWork->segments[(ropeWork->segmentCount - 1) - segmentIndex];
        }
        if (ropeSegment->intersectsObstacle != 0) {
            *intersectionPoint = ropeSegment->segmentVertices[0];
            *distanceAlongRope = segmentDistance;
            return 1;
        }
    }
    return 0;
}

s32 FindRopeCollisionPoint(s32 modelId, Vec *segmentStart, Vec *movementDelta, Vec *collisionPoint)
{
    Vec obstacleLeftEdge;
    Vec obstacleRightEdge;
    Vec movementEdge;
    Vec obstacleEdge;
    HU3DMODEL *model;
    Float lineIntersectionX;
    Float minY;
    Float maxY;
    Float obstacleY;
    Float minX;
    Float maxX;
    M428DllObstacleData *obstacle;
    s32 obstacleIndex;
    s32 intersectsObstacle;
    s32 intersectsMovement;

    model = &Hu3DData[modelId];
    movementEdge.x = movementDelta->y - segmentStart->y;
    movementEdge.y = segmentStart->x - movementDelta->x;
    movementEdge.z = -((movementEdge.x * segmentStart->x) + (movementEdge.y * segmentStart->y));
    if (segmentStart->x < movementDelta->x) {
        minX = segmentStart->x;
        maxX = movementDelta->x;
    }
    else {
        minX = movementDelta->x;
        maxX = segmentStart->x;
    }
    if (segmentStart->y < movementDelta->y) {
        minY = segmentStart->y;
        maxY = movementDelta->y;
    }
    else {
        minY = movementDelta->y;
        maxY = segmentStart->y;
    }
    for (obstacleIndex = 0, obstacle = obstacleCollisionData; obstacleIndex < 5; obstacleIndex++, obstacle++) {
        obstacleLeftEdge = obstacle->obstacleCenter;
        obstacleLeftEdge.x -= obstacle->collisionRadius;
        obstacleRightEdge = obstacle->obstacleCenter;
        obstacleRightEdge.x += obstacle->collisionRadius;
        obstacleEdge.x = obstacleRightEdge.y - obstacleLeftEdge.y;
        obstacleEdge.y = obstacleLeftEdge.x - obstacleRightEdge.x;
        obstacleEdge.z = -((obstacleEdge.x * obstacleLeftEdge.x) + (obstacleEdge.y * obstacleLeftEdge.y));
        if ((obstacle->obstacleCenter.y >= minY) && (obstacle->obstacleCenter.y <= maxY) && (movementEdge.x != 0.0f)) {
            lineIntersectionX = -((movementEdge.z + (movementEdge.y * obstacle->obstacleCenter.y)) / movementEdge.x);
            obstacleY = obstacle->obstacleCenter.y;
            if ((lineIntersectionX >= obstacleLeftEdge.x) && (lineIntersectionX <= obstacleRightEdge.x)) {
                intersectsObstacle = 1;
            }
            else {
                intersectsObstacle = 0;
            }
            if ((lineIntersectionX >= minX) && (lineIntersectionX <= maxX) && (obstacleY >= minY) && (obstacleY <= maxY)) {
                intersectsMovement = 1;
            }
            else {
                intersectsMovement = 0;
            }
            if ((intersectsObstacle != 0) && (intersectsMovement != 0)) {
                if (lineIntersectionX < obstacle->obstacleCenter.x) {
                    *collisionPoint = obstacleLeftEdge;
                }
                else {
                    *collisionPoint = obstacleRightEdge;
                }
                return obstacleIndex;
            }
        }
    }
    return -1;
}

void InitializeScoreDisplay(void)
{
    M428DllScoreDisplayWork *scoreWork;
    OMOBJ *scoreObject;
    s32 spriteId;
    M428DllParticleHookWork *animationData;
    HU3DMODEL *model;
    s32 teamIndex;
    s32 playerIndex;

    scoreObject = scoreObject = omAddObjEx(objectManager, 0x42, 0xA, 0, -1, UpdateScoreDisplay);
    scoreWork = scoreObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M428DllScoreDisplayWork), HU_MEMNUM_OVL);
    memset(scoreWork, 0, sizeof(M428DllScoreDisplayWork));
    spriteId = espEntry(DATA_MAKE_NUM(DATADIR_M428, 0x0E), 0x80, 0);
    scoreWork->scoreSprite = spriteId;
    espDispOn(spriteId);
    espDrawNoSet(spriteId, 0);
    espAttrSet(spriteId, HUSPR_ATTR_NOANIM);
    espPosSet(spriteId, 288.0, 240.0);
    spriteId = espEntry(DATA_MAKE_NUM(DATADIR_M428, 0x0C), 0x50, 0);
    scoreWork->teamProgressSprites[0] = spriteId;
    espDispOn(spriteId);
    espDrawNoSet(spriteId, 0);
    espAttrSet(spriteId, HUSPR_ATTR_NOANIM);
    espPosSet(spriteId, 288.0, 240.0);
    spriteId = espEntry(DATA_MAKE_NUM(DATADIR_M428, 0x0D), 0x50, 0);
    scoreWork->teamProgressSprites[1] = spriteId;
    espDispOn(spriteId);
    espDrawNoSet(spriteId, 0);
    espAttrSet(spriteId, HUSPR_ATTR_NOANIM);
    espPosSet(spriteId, 288.0, 240.0);
    scoreWork->teamProgressValues[0][0] = scoreWork->teamProgressValues[0][1] = 0.0f;
    scoreWork->teamProgressValues[1][0] = scoreWork->teamProgressValues[1][1] = 0.0f;
    spriteId = espEntry(DATA_MAKE_NUM(DATADIR_M428, 0x10), 0x100, 0);
    scoreWork->teamHighlightSprites[0] = spriteId;
    espDispOn(spriteId);
    espDrawNoSet(spriteId, 0);
    espAttrSet(spriteId, HUSPR_ATTR_NOANIM);
    espPosSet(spriteId, 144.0, 240.0);
    espScaleSet(spriteId, 36.0, 60.0);
    espColorSet(spriteId, 0xFF, 0xFF, 0xFF);
    espTPLvlSet(spriteId, 0.0);
    spriteId = espEntry(DATA_MAKE_NUM(DATADIR_M428, 0x10), 0x100, 0);
    scoreWork->teamHighlightSprites[1] = spriteId;
    espDispOn(spriteId);
    espDrawNoSet(spriteId, 0);
    espAttrSet(spriteId, HUSPR_ATTR_NOANIM);
    espPosSet(spriteId, 432.0, 240.0);
    espScaleSet(spriteId, 36.0, 60.0);
    espColorSet(spriteId, 0xFF, 0xFF, 0xFF);
    espTPLvlSet(spriteId, 0.0);
    scoreWork->teamHighlightAlpha[0][0] = scoreWork->teamHighlightAlpha[0][1] = 0.0f;
    scoreWork->teamHighlightAlpha[1][0] = scoreWork->teamHighlightAlpha[1][1] = 0.0f;
    scoreWork->leftTeamAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M428, 0x0A), HU_MEMNUM_OVL));
    scoreWork->rightTeamAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M428, 0x0B), HU_MEMNUM_OVL));
    scoreObject->mdlId[0] = CreateParticleModel(scoreWork->leftTeamAnimation, 2);
    Hu3DModelCameraSet(scoreObject->mdlId[0], 1);
    Hu3DModelLayerSet(scoreObject->mdlId[0], 5);
    SetParticleModelVisible(scoreObject->mdlId[0], -1, 0);
    model = &Hu3DData[scoreObject->mdlId[0]];
    animationData = model->hookData;
    animationData->renderOptions |= 3;
    scoreObject->mdlId[1] = CreateParticleModel(scoreWork->rightTeamAnimation, 2);
    Hu3DModelCameraSet(scoreObject->mdlId[1], 1);
    Hu3DModelLayerSet(scoreObject->mdlId[1], 5);
    SetParticleModelVisible(scoreObject->mdlId[1], -1, 0);
    model = &Hu3DData[scoreObject->mdlId[1]];
    animationData = model->hookData;
    animationData->renderOptions |= 3;
    scoreObject->mdlId[2] = CreateParticleModel(scoreWork->leftTeamAnimation, 2);
    Hu3DModelCameraSet(scoreObject->mdlId[2], 2);
    Hu3DModelLayerSet(scoreObject->mdlId[2], 5);
    SetParticleModelVisible(scoreObject->mdlId[2], -1, 0);
    model = &Hu3DData[scoreObject->mdlId[2]];
    animationData = model->hookData;
    animationData->renderOptions |= 3;
    scoreObject->mdlId[3] = CreateParticleModel(scoreWork->rightTeamAnimation, 2);
    Hu3DModelCameraSet(scoreObject->mdlId[3], 2);
    Hu3DModelLayerSet(scoreObject->mdlId[3], 5);
    SetParticleModelVisible(scoreObject->mdlId[3], -1, 0);
    model = &Hu3DData[scoreObject->mdlId[3]];
    animationData = model->hookData;
    animationData->renderOptions |= 3;
    for (teamIndex = 0; teamIndex < 2; teamIndex++) {
        for (playerIndex = 0; playerIndex < 2; playerIndex++) {
            scoreWork->leftTeamFlashFrames[teamIndex][playerIndex] = scoreWork->rightTeamFlashFrames[teamIndex][playerIndex] = -1;
        }
    }
}

void UpdateScoreDisplay(OMOBJ *object)
{
    Vec playerPosition;
    Float flashCurve;
    Float progressY;
    s32 teamIndex;
    s32 playerIndex;
    OMOBJ *playerObject;
    M428DllTeamWork *teamWork;

    M428DllScoreDisplayWork *scoreWork = object->data;
    for (teamIndex = 0; teamIndex < 2; teamIndex++) {
        scoreWork->teamProgressValues[teamIndex][0] = scoreWork->teamProgressValues[teamIndex][0] + (0.05f * (scoreWork->teamProgressValues[teamIndex][1] - scoreWork->teamProgressValues[teamIndex][0]));
        progressY = scoreProgressYBounds[0] + (scoreWork->teamProgressValues[teamIndex][0] * (scoreProgressYBounds[1] - scoreProgressYBounds[0]));
        espPosSet(scoreWork->teamProgressSprites[teamIndex], 288.0f + scoreProgressXOffsets[teamIndex], progressY);
    }
    for (teamIndex = 0; teamIndex < 2; teamIndex++) {
        scoreWork->teamHighlightAlpha[teamIndex][0] = scoreWork->teamHighlightAlpha[teamIndex][0] + (0.05f * (scoreWork->teamHighlightAlpha[teamIndex][1] - scoreWork->teamHighlightAlpha[teamIndex][0]));
        espTPLvlSet(scoreWork->teamHighlightSprites[teamIndex], scoreWork->teamHighlightAlpha[teamIndex][0]);
    }
    for (teamIndex = 0; teamIndex < 2; teamIndex++) {
        for (playerIndex = 0; playerIndex < 2; playerIndex++) {
            teamWork = teamObjects[teamIndex]->data;
            playerObject = teamWork->playerObjects[playerIndex];
            if (scoreWork->leftTeamFlashFrames[teamIndex][playerIndex] > 0) {
                flashCurve = ((REFRESH_RATE_F / 2) - scoreWork->leftTeamFlashFrames[teamIndex][playerIndex]) / (REFRESH_RATE_F / 10);
                if (flashCurve < 0.0f) {
                    flashCurve = 0.0f;
                }
                else if (flashCurve > 1.0f) {
                    flashCurve = 1.0f;
                }
                flashCurve = sind((180.0f * flashCurve));
                playerPosition.x = playerObject->trans.x;
                playerPosition.y = 200.0f + playerObject->trans.y;
                playerPosition.z = playerObject->trans.z;
                SetParticleModelOffset(object->mdlId[teamIndex * 2], playerIndex, playerPosition);
                SetParticleModelScale(object->mdlId[teamIndex * 2], playerIndex, 125.0f * (1.0f + flashCurve));
                scoreWork->leftTeamFlashFrames[teamIndex][playerIndex] -= 1;
            }
            else {
                SetParticleModelVisible(object->mdlId[teamIndex * 2], playerIndex, 0);
            }
            if (scoreWork->rightTeamFlashFrames[teamIndex][playerIndex] > 0) {
                flashCurve = ((REFRESH_RATE_F / 2) - scoreWork->rightTeamFlashFrames[teamIndex][playerIndex]) / (REFRESH_RATE_F / 10);
                if (flashCurve < 0.0f) {
                    flashCurve = 0.0f;
                }
                else if (flashCurve > 1.0f) {
                    flashCurve = 1.0f;
                }
                flashCurve = sind((180.0f * flashCurve));
                playerPosition.x = playerObject->trans.x;
                playerPosition.y = 200.0f + playerObject->trans.y;
                playerPosition.z = playerObject->trans.z;
                SetParticleModelOffset(object->mdlId[(teamIndex * 2) + 1], playerIndex, playerPosition);
                SetParticleModelScale(object->mdlId[(teamIndex * 2) + 1], playerIndex, 125.0f * (1.0f + flashCurve));
                scoreWork->rightTeamFlashFrames[teamIndex][playerIndex] -= 1;
            }
            else {
                SetParticleModelVisible(object->mdlId[(teamIndex * 2) + 1], playerIndex, 0);
            }
        }
    }
}

void DestroyScoreDisplay(void)
{
    M428DllScoreDisplayWork *scoreWork = scoreObject->data;
    espKill(scoreWork->scoreSprite);
    espKill(scoreWork->teamProgressSprites[0]);
    espKill(scoreWork->teamProgressSprites[1]);
    espKill(scoreWork->teamHighlightSprites[0]);
    espKill(scoreWork->teamHighlightSprites[1]);
    Hu3DModelKill(scoreObject->mdlId[0]);
    Hu3DModelKill(scoreObject->mdlId[1]);
    Hu3DModelKill(scoreObject->mdlId[2]);
    Hu3DModelKill(scoreObject->mdlId[3]);
    HuSprAnimKill(scoreWork->leftTeamAnimation);
    HuSprAnimKill(scoreWork->rightTeamAnimation);
}

void SetTeamScoreFlash(s32 teamIndex, s32 flashType)
{
    if (scoreObject) {
        s32 teamIndex;
        M428DllScoreDisplayWork *scoreWork = scoreObject->data;
        for (teamIndex = 0; teamIndex < 2; teamIndex++) {
            if (flashType == 0) {
                scoreWork->leftTeamFlashFrames[teamIndex][teamIndex] = REFRESH_RATE / 2;
            }
            else {
                scoreWork->rightTeamFlashFrames[teamIndex][teamIndex] = REFRESH_RATE / 2;
            }
        }
    }
}

void SetTeamScoreAlpha(s32 teamIndex, Float targetAlpha)
{
    if (scoreObject) {
        M428DllScoreDisplayWork *scoreWork = scoreObject->data;
        scoreWork->teamHighlightAlpha[teamIndex][1] = targetAlpha;
    }
}

void SetScoreDisplayVisible(s32 showSprites)
{
    s32 teamIndex;
    s32 playerIndex;

    if (scoreObject) {
        M428DllScoreDisplayWork *scoreWork = scoreObject->data;
        if (showSprites == 0) {
            espDispOff(scoreWork->scoreSprite);
            espDispOff(scoreWork->teamProgressSprites[0]);
            espDispOff(scoreWork->teamProgressSprites[1]);
            espDispOff(scoreWork->teamHighlightSprites[0]);
            espDispOff(scoreWork->teamHighlightSprites[1]);
        }
        else {
            espDispOn(scoreWork->scoreSprite);
            espDispOn(scoreWork->teamProgressSprites[0]);
            espDispOn(scoreWork->teamProgressSprites[1]);
            espDispOn(scoreWork->teamHighlightSprites[0]);
            espDispOn(scoreWork->teamHighlightSprites[1]);
        }
        for (teamIndex = 0; teamIndex < 2; teamIndex++) {
            for (playerIndex = 0; playerIndex < 2; playerIndex++) {
                scoreWork->leftTeamFlashFrames[teamIndex][playerIndex] = scoreWork->rightTeamFlashFrames[teamIndex][playerIndex] = -1;
            }
        }
    }
}

void SetTeamProgress(s32 teamIndex, Float progress)
{
    M428DllScoreDisplayWork *scoreWork;
    if (scoreObject) {
        scoreWork = scoreObject->data;
        if (progress < 0.0f) {
            progress = 0.0f;
        }
        else if (progress > 1.0f) {
            progress = 1.0f;
        }
        scoreWork->teamProgressValues[teamIndex][1] = progress;
    }
}

void InitializeResultDisplay(void)
{
    M428DllResultDisplayWork *resultWork;
    s32 teamIndex;
    s32 resultModelId;
    OMOBJ *resultObject;
    M428DllParticleHookWork *modelHook;
    HU3DMODEL *resultModel;
    ANIMDATA *animationData;

    resultObject = resultObject = omAddObjEx(objectManager, 0xBF, 0, 0, -1, UpdateResultDisplay);
    resultWork = resultObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M428DllResultDisplayWork), HU_MEMNUM_OVL);
    memset(resultWork, 0, sizeof(M428DllResultDisplayWork));
    animationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M428, 0x0F), HU_MEMNUM_OVL));
    resultWork->resultAnimationData = animationData;
    for (teamIndex = 0; teamIndex < 2; teamIndex++) {
        resultModelId = CreateParticleModel(animationData, 0x200);
        resultWork->resultModelIds[teamIndex] = resultModelId;
        if (teamIndex == 0) {
            Hu3DModelCameraSet(resultModelId, 1);
        }
        else {
            Hu3DModelCameraSet(resultModelId, 2);
        }
        Hu3DModelLayerSet(resultModelId, 3);
        resultWork->resultAnimationIds[teamIndex] = 0;
        resultModel = &Hu3DData[resultModelId];
        modelHook = resultModel->hookData;
        modelHook->renderFlags = 0;
    }
}

void UpdateResultDisplay(OMOBJ *object)
{
    s32 resultAnimationScratch;
    M428DllParticleWork *particle;
    M428DllResultDisplayWork *resultWork;
    s32 teamIndex;
    M428DllParticleHookWork *renderWork;
    s32 particleIndex;
    HU3DMODEL *model;

    resultWork = object->data;
    for (teamIndex = 0; teamIndex < 2; teamIndex++) {
        if (resultWork->resultAnimationIds[teamIndex] <= 0) {
            Hu3DModelAttrSet(resultWork->resultModelIds[teamIndex], HU3D_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrReset(resultWork->resultModelIds[teamIndex], HU3D_ATTR_DISPOFF);
            model = &Hu3DData[resultWork->resultModelIds[teamIndex]];
            renderWork = model->hookData;
            particle = renderWork->particles;
            renderWork->renderPadding = 0;
            for (particleIndex = 0; particleIndex < renderWork->particleCount; particleIndex++, particle++) {
                if (!(particle->particleScale <= 0.0f)) {
                    particle->particleOffset.x += particle->velocityX;
                    particle->particleOffset.y += particle->velocityY;
                    particle->particleOffset.z += particle->velocityZ;
                    particle->particleRotation = particle->particleRotation + particle->rotationAcceleration;
                    if (particle->particleRotation >= 360.0f) {
                        particle->particleRotation = particle->particleRotation - 360.0f;
                    }
                    particle->textureTileIndex = particle->animationFrame;
                    particle->animationFrame = particle->animationFrame + particle->frameAdvance;
                    if (particle->textureTileIndex >= 0x10) {
                        particle->textureTileIndex = 0;
                        particle->particleReservedFrame = 0;
                        particle->particleScale = 0.0f;
                        resultWork->resultAnimationIds[teamIndex] -= 1;
                    }
                }
            }
        }
    }
}

void DestroyResultDisplay(void)
{

    OMOBJ *resultObject = resultObject;
    M428DllResultDisplayWork *resultWork = resultObject->data;
    Hu3DModelAttrSet(resultWork->resultModelIds[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(resultWork->resultModelIds[1], HU3D_ATTR_DISPOFF);
    HuSprAnimKill(resultWork->resultAnimationData);
    resultObject->objFunc = NULL;
}

s32 CreateParticleEffect(s32 teamIndex, Vec position, Vec velocity, GXColor color, Float particleScale, Float rotationAcceleration, Float frameAdvance)
{
    M428DllParticleWork *particle;
    M428DllParticleHookWork *renderWork;
    s32 particleIndex;
    M428DllResultDisplayWork *resultWork;
    OMOBJ *resultObject;
    HU3DMODEL *resultModel;

    resultObject = resultObject;
    resultWork = resultObject->data;
    resultModel = &Hu3DData[resultWork->resultModelIds[teamIndex]];
    renderWork = resultModel->hookData;
    for (particle = renderWork->particles, particleIndex = 0; particleIndex < renderWork->particleCount; particleIndex++, particle++) {
        if (particle->particleScale <= 0.0f) {
            break;
        }
    }
    if (particleIndex >= renderWork->particleCount) {
        return -1;
    }
    particle->particleReservedFrame = particle->particleReservedFlags = 0;
    particle->particleOffset.x = position.x;
    particle->particleOffset.y = position.y;
    particle->particleOffset.z = position.z;
    particle->velocityX = velocity.x;
    particle->velocityY = velocity.y;
    particle->velocityZ = velocity.z;
    particle->rotationAcceleration = rotationAcceleration;
    particle->particleScale = particleScale;
    particle->particleColor.r = color.r;
    particle->particleColor.g = color.g;
    particle->particleColor.b = color.b;
    particle->particleColor.a = color.a;
    particle->particleRotation = 0.0f;
    particle->textureTileIndex = 0;
    particle->particleReservedFrame = 0;
    particle->animationFrame = 0.0f;
    particle->frameAdvance = frameAdvance;
    resultWork->resultAnimationIds[teamIndex]++;
    return particleIndex;
}

static inline s32 CreateparticleeffectInlined(s32 teamIndex, Vec position, Vec velocity, GXColor color, Float particleScale, Float rotationAcceleration, Float frameAdvance)
{
    M428DllParticleWork *particle;
    M428DllParticleHookWork *renderWork;
    s32 particleIndex;
    M428DllResultDisplayWork *resultWork;
    OMOBJ *resultObject;
    HU3DMODEL *resultModel;

    resultObject = resultObject;
    resultWork = resultObject->data;
    resultModel = &Hu3DData[resultWork->resultModelIds[teamIndex]];
    renderWork = resultModel->hookData;
    for (particle = renderWork->particles, particleIndex = 0; particleIndex < renderWork->particleCount; particleIndex++, particle++) {
        if (particle->particleScale <= 0.0f) {
            break;
        }
    }
    if (particleIndex >= renderWork->particleCount) {
        return -1;
    }
    particle->particleReservedFrame = particle->particleReservedFlags = 0;
    particle->particleOffset.x = position.x;
    particle->particleOffset.y = position.y;
    particle->particleOffset.z = position.z;
    particle->velocityX = velocity.x;
    particle->velocityY = velocity.y;
    particle->velocityZ = velocity.z;
    particle->rotationAcceleration = rotationAcceleration;
    particle->particleScale = particleScale;
    particle->particleColor.r = color.r;
    particle->particleColor.g = color.g;
    particle->particleColor.b = color.b;
    particle->particleColor.a = color.a;
    particle->particleRotation = 0.0f;
    particle->textureTileIndex = 0;
    particle->particleReservedFrame = 0;
    particle->animationFrame = 0.0f;
    particle->frameAdvance = frameAdvance;
    resultWork->resultAnimationIds[teamIndex]++;
    return particleIndex;
}

s32 CreatePairedParticleEffects(s32 teamIndex, Vec position, Vec velocity, Float particleScale, Float rotationAcceleration, Float particleSpacing, Float frameAdvance, GXColor color)
{
    Vec particlePositionA;
    Vec particleDirection;
    s32 particleIdA;
    s32 particleIdB;
    Float halfParticleSpacing;

    particleDirection.x = velocity.z;
    particleDirection.z = velocity.x;
    particleDirection.y = 0.0f;
    VECNormalize(&particleDirection, &particleDirection);
    halfParticleSpacing = 0.5f * particleSpacing;
    particlePositionA.x = position.x + (particleDirection.x * halfParticleSpacing);
    particlePositionA.y = position.y + (particleDirection.y * halfParticleSpacing);
    particlePositionA.z = position.z + (particleDirection.z * halfParticleSpacing);

    particleIdA = CreateparticleeffectInlined(teamIndex, particlePositionA, velocity, color, particleScale, rotationAcceleration, frameAdvance);
    particlePositionA.x = position.x - (particleDirection.x * halfParticleSpacing);
    particlePositionA.y = position.y - (particleDirection.y * halfParticleSpacing);
    particlePositionA.z = position.z - (particleDirection.z * halfParticleSpacing);

    particleIdB = CreateparticleeffectInlined(teamIndex, particlePositionA, velocity, color, particleScale, -rotationAcceleration, frameAdvance);
    return (particleIdA << 0x10) | particleIdB;
}

Float GetAngleDifference(Float firstAngle, Float secondAngle)
{
    Float angleDifference;

    if (firstAngle >= 360.0f) {
        firstAngle -= 360.0f;
    }
    else if (firstAngle < 0.0f) {
        firstAngle += 360.0f;
    }
    if (secondAngle >= 360.0f) {
        secondAngle -= 360.0f;
    }
    else if (secondAngle < 0.0f) {
        secondAngle += 360.0f;
    }
    angleDifference = firstAngle - secondAngle;
    if (angleDifference <= -180.0f) {
        angleDifference += 360.0f;
    }
    else if (angleDifference >= 180.0f) {
        angleDifference -= 360.0f;
    }
    return angleDifference;
}

Float InterpolateWrappedAngle(Float progress, Float startAngle, Float maximumAngularStep)
{
    Float interpolatedAngle;
    Float wrappedAngleDifference;

    if (progress >= 360.0) {
        progress -= 360.0;
    }
    else if (progress < 0.0) {
        progress += 360.0;
    }
    if (startAngle >= 360.0) {
        startAngle -= 360.0;
    }
    else if (startAngle < 0.0) {
        startAngle += 360.0;
    }
    wrappedAngleDifference = 360.0 + (progress - startAngle);
    if (fabs(wrappedAngleDifference) >= 360.0) {
        wrappedAngleDifference = fmod(wrappedAngleDifference, 360.0);
    }
    if (wrappedAngleDifference < 180.0) {
        if (wrappedAngleDifference <= maximumAngularStep) {
            interpolatedAngle = wrappedAngleDifference;
        }
        else {
            interpolatedAngle = maximumAngularStep;
        }
    }
    else if (360.0 - wrappedAngleDifference <= maximumAngularStep) {
        interpolatedAngle = -(360.0 - wrappedAngleDifference);
    }
    else {
        interpolatedAngle = -maximumAngularStep;
    }
    interpolatedAngle += startAngle;
    if (interpolatedAngle >= 360.0) {
        interpolatedAngle -= 360.0;
    }
    else if (interpolatedAngle < 0.0) {
        interpolatedAngle += 360.0;
    }
    return interpolatedAngle;
}

Float InterpolateAngle(Float progress, Float startAngle, Float targetAngle)
{
    Float angleDifference;
    Float rotationDelta;

    rotationDelta = GetAngleDifference(targetAngle, startAngle);
    angleDifference = fabs(rotationDelta * progress);
    return InterpolateWrappedAngle(targetAngle, startAngle, angleDifference);
}

void InterpolateVector(Float progress, Vec *startPoint, Vec *endPoint, Vec *interpolatedPoint)
{
    Vec interpolatedVector;
    interpolatedVector.x = startPoint->x + (progress * (endPoint->x - startPoint->x));
    interpolatedVector.y = startPoint->y + (progress * (endPoint->y - startPoint->y));
    interpolatedVector.z = startPoint->z + (progress * (endPoint->z - startPoint->z));
    *interpolatedPoint = interpolatedVector;
}

Float EvaluateQuadraticCurve(Float progress, Float startValue, Float controlValue, Float endValue)
{
    Float inverseBlend;
    Float quadraticInterpolation;

    inverseBlend = 1.0 - progress;
    quadraticInterpolation = (endValue * (progress * progress)) + ((startValue * (inverseBlend * inverseBlend)) + (2.0 * (controlValue * (inverseBlend * progress))));
    return quadraticInterpolation;
}

void InterpolateQuadraticVector(Float progress, Vec *startPoint, Vec *controlPoint, Vec *endPoint, Vec *interpolatedPoint)
{
    interpolatedPoint->x = EvaluateQuadraticCurve(progress, startPoint->x, controlPoint->x, endPoint->x);
    interpolatedPoint->y = EvaluateQuadraticCurve(progress, startPoint->y, controlPoint->y, endPoint->y);
    interpolatedPoint->z = EvaluateQuadraticCurve(progress, startPoint->z, controlPoint->z, endPoint->z);
}

s16 CreateParticleModel(ANIMDATA *animationData, s16 particleCount)
{
    s16 particleIndex;
    M428DllParticleWork *particle;
    Vec2f *textureCoordinate;
    Vec *vertex;
    void *displayListBuffer;
    void *displayListData;

    s16 modelId = Hu3DHookFuncCreate(DrawParticleModel);
    HU3DMODEL *model = &Hu3DData[modelId];
    M428DllParticleHookWork *renderWork = HuMemDirectMallocNum(HEAP_MODEL, sizeof(M428DllParticleHookWork), model->mallocNo);
    model->hookData = renderWork;
    renderWork->spriteAnimation = animationData;
    renderWork->particleCount = particleCount;
    renderWork->renderFlags = 0;
    renderWork->renderOptions = 0;
    renderWork->renderHook = 0;
    renderWork->nextHookModel = 0;
    renderWork->displayListFrame = 0;
    renderWork->renderState = 0;
    renderWork->renderPadding = 0;
    renderWork->displayListSize = 0;
    renderWork->hookFlags = renderWork->hookState = 0;
    particle = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(M428DllParticleWork), model->mallocNo);
    renderWork->particles = particle;
    memset(particle, 0, particleCount * sizeof(M428DllParticleWork));
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++, particle++) {
        particle->particleScale = 0.0f;
        particle->particleRotation = 0.0f;
        particle->animationFrame = 0.0f;
        particle->frameAdvance = 1.0f;
        particle->particleOffset.x = ((frand() & 0x7F) - 0x40) * 0x14;
        particle->particleOffset.y = ((frand() & 0x7F) - 0x40) * 0x1E;
        particle->particleOffset.z = ((frand() & 0x7F) - 0x40) * 0x14;
        particle->particleColor.r = particle->particleColor.g = particle->particleColor.b = particle->particleColor.a = 0xFF;
        particle->textureTileIndex = 0;
    }
    vertex = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(Vec) * 4, model->mallocNo);
    renderWork->particleVertices = vertex;
    for (particleIndex = 0; particleIndex < (particleCount * 4); particleIndex++, vertex++) {
        vertex->x = vertex->y = vertex->z = 0.0f;
    }
    textureCoordinate = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(Vec2f) * 4, model->mallocNo);
    renderWork->particleTexCoords = textureCoordinate;
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++) {
        textureCoordinate->x = 0.0f;
        textureCoordinate->y = 0.0f;
        textureCoordinate++;
        textureCoordinate->x = 1.0f;
        textureCoordinate->y = 0.0f;
        textureCoordinate++;
        textureCoordinate->x = 1.0f;
        textureCoordinate->y = 1.0f;
        textureCoordinate++;
        textureCoordinate->x = 0.0f;
        textureCoordinate->y = 1.0f;
        textureCoordinate++;
    }
    displayListBuffer = HuMemDirectMallocNum(HEAP_MODEL, 0x10000, model->mallocNo);
    displayListData = displayListBuffer;
    DCFlushRange(displayListBuffer, 0x10000);
    GXBeginDisplayList(displayListData, 0x10000);
    GXBegin(GX_QUADS, GX_VTXFMT0, particleCount * 4);
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++) {
        GXUnknownu16(particleIndex * 4);
        GXUnknownu16(particleIndex);
        GXUnknownu16(particleIndex * 4);
        GXUnknownu16(particleIndex * 4 + 1);
        GXUnknownu16(particleIndex);
        GXUnknownu16(particleIndex * 4 + 1);
        GXUnknownu16(particleIndex * 4 + 2);
        GXUnknownu16(particleIndex);
        GXUnknownu16(particleIndex * 4 + 2);
        GXUnknownu16(particleIndex * 4 + 3);
        GXUnknownu16(particleIndex);
        GXUnknownu16(particleIndex * 4 + 3);
    }
    renderWork->displayListCapacity = GXEndDisplayList();
    renderWork->displayList = HuMemDirectMallocNum(HEAP_MODEL, renderWork->displayListCapacity, model->mallocNo);
    memcpy(renderWork->displayList, displayListBuffer, renderWork->displayListCapacity);
    DCFlushRange(renderWork->displayList, renderWork->displayListCapacity);
    HuMemDirectFree(displayListBuffer);
    (void)particleIndex;
    (void)particleCount;
    return modelId;
}

s16 CloneParticleModel(s16 sourceModelId)
{
    M428DllParticleHookWork *newRenderWork;
    M428DllParticleWork *particle;
    Vec2f *texCoord;
    s16 particleIndex;
    HU3DMODEL *newModel;
    M428DllParticleHookWork *sourceRenderWork;
    Vec *vertex;
    s32 particleCount;
    HU3DMODEL *sourceModel;
    s16 newModelId;

    sourceModel = &Hu3DData[sourceModelId];
    sourceRenderWork = sourceModel->hookData;
    newModelId = Hu3DHookFuncCreate(sourceModel->hookFunc);
    newModel = &Hu3DData[newModelId];
    newRenderWork = HuMemDirectMallocNum(HEAP_MODEL, sizeof(M428DllParticleHookWork), newModel->mallocNo);
    newModel->hookData = newRenderWork;
    newRenderWork->spriteAnimation = sourceRenderWork->spriteAnimation;
    newRenderWork->particleCount = sourceRenderWork->particleCount;
    particleCount = newRenderWork->particleCount;
    newRenderWork->renderFlags = 0;
    newRenderWork->renderOptions = sourceRenderWork->renderOptions;
    newRenderWork->renderHook = NULL;
    newRenderWork->nextHookModel = NULL;
    newRenderWork->displayListFrame = 0;
    newRenderWork->renderState = 0;
    newRenderWork->displayListSize = 0;
    newRenderWork->hookFlags = newRenderWork->hookState = 0;
    particle = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(M428DllParticleWork), newModel->mallocNo);
    newRenderWork->particles = particle;
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++, particle++) {
        particle->particleScale = 0.0f;
        particle->particleRotation = 0.0f;
        particle->animationFrame = 0.0f;
        particle->frameAdvance = 1.0f;
        particle->particleOffset.x = ((frand() & 0x7F) - 0x40) * 0x14;
        particle->particleOffset.y = ((frand() & 0x7F) - 0x40) * 0x1E;
        particle->particleOffset.z = ((frand() & 0x7F) - 0x40) * 0x14;
        particle->particleColor.r = particle->particleColor.g = particle->particleColor.b = particle->particleColor.a = 0xFF;
        particle->textureTileIndex = 0;
    }
    vertex = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(Vec) * 4, newModel->mallocNo);
    newRenderWork->particleVertices = vertex;
    for (particleIndex = 0; particleIndex < (particleCount * 4); particleIndex++, vertex++) {
        vertex->x = vertex->y = vertex->z = 0.0f;
    }
    texCoord = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(Vec2f) * 4, newModel->mallocNo);
    newRenderWork->particleTexCoords = texCoord;
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++) {
        texCoord->x = 0.0f;
        texCoord->y = 0.0f;
        texCoord++;
        texCoord->x = 1.0f;
        texCoord->y = 0.0f;
        texCoord++;
        texCoord->x = 1.0f;
        texCoord->y = 1.0f;
        texCoord++;
        texCoord->x = 0.0f;
        texCoord->y = 1.0f;
        texCoord++;
    }
    newRenderWork->displayListCapacity = sourceRenderWork->displayListCapacity;
    newRenderWork->displayList = sourceRenderWork->displayList;
    return newModelId;
}

void DrawParticleModel(HU3DMODEL *model, Mtx modelMatrix)
{
    Mtx inverseModelMatrix;
    Mtx rotatedParticleMatrix;
    Mtx rotationMatrix;
    Vec worldQuad[4];
    Vec rotatedQuad[4];
    Vec modelQuad[4];
    ROMtx reorderedMatrix;
    M428DllParticleHookWork *renderWork;
    Vec *vertex;
    M428DllParticleWork *particle;
    Vec2f *texCoord;
    Vec *quadVertex;
    s16 particleIndex;
    s16 cornerIndex;
    Vec *modelVertex;
    s16 textureFormat;
    s16 tileColumn;
    s16 tileRow;
    M428DllPlayerHook renderHook;

    renderWork = model->hookData;
    if ((renderWork->lastRenderFrame != GlobalCounter) || (shadowModelDrawF != 0)) {
        if ((renderWork->nextHookModel) && (renderWork->nextHookModel != model)) {
            DrawParticleModel(renderWork->nextHookModel, modelMatrix);
        }
        GXLoadPosMtxImm(modelMatrix, 0);
        GXSetNumTevStages(1);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, GX_FALSE, 0x7D);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        if (shadowModelDrawF != 0) {
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ONE, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
            GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
        }
        else {
            textureFormat = renderWork->spriteAnimation->bmp->dataFmt & 0xF;
            if ((textureFormat == 7) || (textureFormat == 8)) {
                GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
            }
            else {
                GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
            }
            if ((renderWork->renderOptions & 1) != 0) {
                GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
            }
            else if ((model->attr & 2) != 0) {
                GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
            }
            else {
                GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
            }
        }
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
        HuSprTexLoad(renderWork->spriteAnimation, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
        GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
        GXSetZCompLoc(0);
        switch (renderWork->renderFlags) {
            case 0:
                GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
                break;
            case 1:
                GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
                break;
            case 2:
                GXSetBlendMode(GX_BM_BLEND, GX_BL_ZERO, GX_BL_INVDSTCLR, GX_LO_NOOP);
                break;
        }
        if (HmfInverseMtxF3X3(modelMatrix, &inverseModelMatrix[0]) == 0) {
            MTXIdentity(inverseModelMatrix);
        }
        PSMTXReorder(inverseModelMatrix, reorderedMatrix);
        if (renderWork->renderHook) {
            renderHook = renderWork->renderHook;
            renderHook(model, renderWork, modelMatrix);
        }
        particle = renderWork->particles;
        vertex = renderWork->particleVertices;
        texCoord = renderWork->particleTexCoords;
        PSMTXROMultVecArray(reorderedMatrix, particleQuadCorners, &modelQuad[0], 4);
        for (particleIndex = 0; particleIndex < renderWork->particleCount; particleIndex++, particle++) {
            if (!particle->particleScale) {
                vertex->x = vertex->y = vertex->z = 0.0f;
                vertex++;
                vertex->x = vertex->y = vertex->z = 0.0f;
                vertex++;
                vertex->x = vertex->y = vertex->z = 0.0f;
                vertex++;
                vertex->x = vertex->y = vertex->z = 0.0f;
                vertex++;
            }
            else if (!particle->particleRotation) {
                quadVertex = &worldQuad[0];
                modelVertex = &modelQuad[0];
                VECScale(modelVertex++, quadVertex, particle->particleScale);
                VECAdd(quadVertex++, &particle->particleOffset, vertex++);
                VECScale(modelVertex++, quadVertex, particle->particleScale);
                VECAdd(quadVertex++, &particle->particleOffset, vertex++);
                VECScale(modelVertex++, quadVertex, particle->particleScale);
                VECAdd(quadVertex++, &particle->particleOffset, vertex++);
                VECScale(modelVertex++, quadVertex, particle->particleScale);
                VECAdd(quadVertex++, &particle->particleOffset, vertex++);
            }
            else {
                VECScale(particleQuadCorners, &worldQuad[0], particle->particleScale);
                VECScale(&particleQuadCorners[1], &worldQuad[1], particle->particleScale);
                VECScale(&particleQuadCorners[2], &worldQuad[2], particle->particleScale);
                VECScale(&particleQuadCorners[3], &worldQuad[3], particle->particleScale);
                MTXRotRad(rotationMatrix, 0x5A, particle->particleRotation);
                MTXConcat(&inverseModelMatrix[0], rotationMatrix, rotatedParticleMatrix);
                MTXMultVecArray(rotatedParticleMatrix, &worldQuad[0], &rotatedQuad[0], 4);
                VECAdd(&rotatedQuad[0], &particle->particleOffset, vertex++);
                VECAdd(&rotatedQuad[1], &particle->particleOffset, vertex++);
                VECAdd(&rotatedQuad[2], &particle->particleOffset, vertex++);
                VECAdd(&rotatedQuad[3], &particle->particleOffset, vertex++);
            }
        }
        particle = renderWork->particles;
        texCoord = renderWork->particleTexCoords;
        if ((renderWork->renderOptions & 2) == 0) {
            for (particleIndex = 0; particleIndex < renderWork->particleCount; particleIndex++, particle++) {
                tileColumn = particle->textureTileIndex & 3;
                tileRow = (particle->textureTileIndex >> 2) & 3;
                for (cornerIndex = 0; cornerIndex < 4; cornerIndex++, texCoord++) {
                    texCoord->x = (0.25f * tileColumn) + particleTextureCornerOffsets[cornerIndex].x;
                    texCoord->y = (0.25f * tileRow) + particleTextureCornerOffsets[cornerIndex].y;
                }
            }
        }
        else {
            for (particleIndex = 0; particleIndex < renderWork->particleCount; particleIndex++, particle++) {
                for (cornerIndex = 0; cornerIndex < 4; cornerIndex++, texCoord++) {
                    texCoord->x = 4.0f * particleTextureCornerOffsets[cornerIndex].x;
                    texCoord->y = 4.0f * particleTextureCornerOffsets[cornerIndex].y;
                }
            }
        }
        DCFlushRangeNoSync(renderWork->particleVertices, renderWork->particleCount * 0xC * 4);
        DCFlushRangeNoSync(renderWork->particleTexCoords, renderWork->particleCount * 8 * 4);
        DCFlushRangeNoSync(renderWork->particles, renderWork->particleCount * 0x4C);
        PPCSync();
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GXSetArray(GX_VA_POS, renderWork->particleVertices, sizeof(Vec));
        GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
        GXSetArray(GX_VA_CLR0, &renderWork->particles->particleColor, sizeof(M428DllParticleWork)); // bug?
        GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
        GXSetArray(GX_VA_TEX0, renderWork->particleTexCoords, sizeof(Vec2f));
        GXCallDisplayList(renderWork->displayList, renderWork->displayListCapacity);
        if (shadowModelDrawF == 0) {
            if ((renderWork->renderState & 2) == 0) {
                renderWork->displayListFrame++;
            }
            if ((renderWork->displayListSize != 0) && (renderWork->displayListSize <= renderWork->displayListFrame)) {
                if ((renderWork->renderState & 1) != 0) {
                    renderWork->displayListFrame = 0;
                }
                renderWork->displayListFrame = renderWork->displayListSize;
            }
            renderWork->lastRenderFrame = GlobalCounter;
        }
    }
}

void SetParticleModelVisible(s16 modelId, s16 particleIndex, s32 isVisible)
{
    if (isVisible == 0) {
        SetParticleModelScale(modelId, particleIndex, 0.0f);
        return;
    }
    SetParticleModelScale(modelId, particleIndex, 1.0f);
}

void SetParticleModelOffset(s16 modelId, s16 particleIndex, Vec particleOffset)
{
    M428DllParticleWork *particle;
    M428DllParticleHookWork *renderWork;
    s32 particleIndex;
    HU3DMODEL *model;

    model = &Hu3DData[modelId];
    renderWork = model->hookData;
    if (particleIndex != -1) {
        particle = &renderWork->particles[particleIndex];
        particle->particleOffset = particleOffset;
        return;
    }
    for (particle = renderWork->particles, particleIndex = 0; particleIndex < renderWork->particleCount; particleIndex++, particle++) {
        particle->particleOffset = particleOffset;
    }
}

void SetParticleModelScale(s16 modelId, s16 particleIndex, Float particleScale)
{
    M428DllParticleWork *particle;
    M428DllParticleHookWork *renderWork;
    s32 particleIndex;
    HU3DMODEL *model;

    model = &Hu3DData[modelId];
    renderWork = model->hookData;
    if (particleIndex != -1) {
        particle = &renderWork->particles[particleIndex];
        particle->particleScale = particleScale;
        return;
    }
    for (particle = renderWork->particles, particleIndex = 0; particleIndex < renderWork->particleCount; particleIndex++, particle++) {
        particle->particleScale = particleScale;
    }
}

s32 FindModelJointTransform(s16 modelId, char *jointName, Mtx parentMatrix, s32 includeParentMatrix)
{
    Mtx modelTransform;
    Mtx rotationMatrix;
    HU3DMODEL *model;
    HSFDATA *hsfData;

    model = &Hu3DData[modelId];
    hsfData = model->hsf;
    matchedObjectFound = 0;
    if (model->motId != -1) {
        useCurrentObjectTransforms = 1;
    }
    else {
        useCurrentObjectTransforms = 0;
    }
    mtxRot(rotationMatrix, model->rot.x, model->rot.y, model->rot.z);
    MTXScale(modelTransform, model->scale.x, model->scale.y, model->scale.z);
    MTXConcat(rotationMatrix, modelTransform, modelTransform);
    mtxTransCat(modelTransform, model->pos.x, model->pos.y, model->pos.z);
    MTXConcat(modelTransform, model->mtx, modelTransform);
    if (includeParentMatrix != 0) {
        MTXConcat(parentMatrix, modelTransform, modelTransform);
    }
    SearchModelJointTransform(hsfData->root, modelTransform, jointName, parentMatrix);
    return matchedObjectFound;
}

void SearchModelJointTransform(HSFOBJECT *modelObject, Mtx parentMatrix, char *jointName, Mtx jointMatrix)
{
    Mtx objectTransform;
    Mtx localTransform;
    Mtx childTransform;
    Mtx meshTransform;
    Mtx rootTransform;
    Mtx combinedTransform;
    HSFTRANSFORM *objectTransformData;
    HSFTRANSFORM *childTransformData;
    HSFTRANSFORM *baseTransformData;
    HSFOBJECT *object;
    HSFOBJECT *childObject;
    u32 childIndex;
    u32 meshIndex;
    u32 submeshIndex;

    if (matchedObjectFound == 0) {
        if (useCurrentObjectTransforms != 0) {
            objectTransformData = &modelObject->mesh.curr;
        }
        else {
            objectTransformData = &modelObject->mesh.base;
        }
        if ((objectTransformData->scale.x <= 0.0f) && (objectTransformData->scale.y <= 0.0f) && (objectTransformData->scale.z <= 0.0f)) {
            MTXCopy(parentMatrix, objectTransform);
            OSReport("OBJ MTX CHECK ERROR %s \n", modelObject->name);
        }
        else {
            mtxRot(localTransform, objectTransformData->rot.x, objectTransformData->rot.y, objectTransformData->rot.z);
            MTXScale(objectTransform, objectTransformData->scale.x, objectTransformData->scale.y, objectTransformData->scale.z);
            MTXConcat(localTransform, objectTransform, objectTransform);
            mtxTransCat(objectTransform, objectTransformData->pos.x, objectTransformData->pos.y, objectTransformData->pos.z);
            MTXConcat(parentMatrix, objectTransform, objectTransform);
        }
        if (printObjectNames != 0) {
            OSReport("%s \n", modelObject->name);
        }
        if (strcmp(jointName, modelObject->name) == 0) {
            MTXCopy(objectTransform, jointMatrix);
            matchedObjectFound = 1;
        }
        for (childIndex = 0; childIndex < modelObject->mesh.childrenCount; childIndex++) {
            object = modelObject->mesh.children[childIndex];
            if (matchedObjectFound == 0) {
                if (useCurrentObjectTransforms != 0) {
                    childTransformData = &object->mesh.curr;
                }
                else {
                    childTransformData = &object->mesh.base;
                }
                if ((childTransformData->scale.x <= 0.0f) && (childTransformData->scale.y <= 0.0f) && (childTransformData->scale.z <= 0.0f)) {
                    MTXCopy(objectTransform, meshTransform);
                    OSReport("OBJ MTX CHECK ERROR %s \n", object->name);
                }
                else {
                    mtxRot(&childTransform[0], childTransformData->rot.x, childTransformData->rot.y, childTransformData->rot.z);
                    MTXScale(meshTransform, childTransformData->scale.x, childTransformData->scale.y, childTransformData->scale.z);
                    MTXConcat(&childTransform[0], meshTransform, meshTransform);
                    mtxTransCat(meshTransform, childTransformData->pos.x, childTransformData->pos.y, childTransformData->pos.z);
                    MTXConcat(objectTransform, meshTransform, meshTransform);
                }
                if (printObjectNames != 0) {
                    OSReport("%s \n", object->name);
                }
                if (strcmp(jointName, object->name) == 0) {
                    MTXCopy(meshTransform, jointMatrix);
                    matchedObjectFound = 1;
                }
                for (meshIndex = 0; meshIndex < object->mesh.childrenCount; meshIndex++) {
                    childObject = object->mesh.children[meshIndex];
                    if (matchedObjectFound == 0) {
                        if (useCurrentObjectTransforms != 0) {
                            baseTransformData = &childObject->mesh.curr;
                        }
                        else {
                            baseTransformData = &childObject->mesh.base;
                        }
                        if ((baseTransformData->scale.x <= 0.0f) && (baseTransformData->scale.y <= 0.0f) && (baseTransformData->scale.z <= 0.0f)) {
                            MTXCopy(meshTransform, combinedTransform);
                            OSReport("OBJ MTX CHECK ERROR %s \n", childObject->name);
                        }
                        else {
                            mtxRot(rootTransform, baseTransformData->rot.x, baseTransformData->rot.y, baseTransformData->rot.z);
                            MTXScale(combinedTransform, baseTransformData->scale.x, baseTransformData->scale.y, baseTransformData->scale.z);
                            MTXConcat(rootTransform, combinedTransform, combinedTransform);
                            mtxTransCat(combinedTransform, baseTransformData->pos.x, baseTransformData->pos.y, baseTransformData->pos.z);
                            MTXConcat(meshTransform, combinedTransform, combinedTransform);
                        }
                        if (printObjectNames != 0) {
                            OSReport("%s \n", childObject->name);
                        }
                        if (strcmp(jointName, childObject->name) == 0) {
                            MTXCopy(combinedTransform, jointMatrix);
                            matchedObjectFound = 1;
                        }
                        for (submeshIndex = 0; submeshIndex < childObject->mesh.childrenCount; submeshIndex++) {
                            SearchModelJointTransform(childObject->mesh.children[submeshIndex], combinedTransform, jointName, jointMatrix);
                        }
                    }
                }
            }
        }
    }
}

void CaptureCameraProjection(s16 cameraIndex)
{
    HU3DCAMERA *camera = &Hu3DCamera[cameraIndex];
    C_MTXLookAt(cameraViewMatrices[cameraIndex], &camera->pos, &camera->up, &camera->target);
    GXGetProjectionv(&cameraProjectionStates[cameraIndex].projectionOrViewportValues);
    GXGetViewportv(&cameraViewportStates[cameraIndex].projectionOrViewportValues);
}

void ProjectWorldPointToScreen(s16 cameraIndex, Vec *worldPosition, Float *screenPosition)
{
    GXProject(worldPosition->x, worldPosition->y, worldPosition->z, cameraViewMatrices[cameraIndex], &cameraProjectionStates[cameraIndex].projectionOrViewportValues, &cameraViewportStates[cameraIndex].projectionOrViewportValues, screenPosition,
        &screenPosition[1], &screenPosition[2]);
}
