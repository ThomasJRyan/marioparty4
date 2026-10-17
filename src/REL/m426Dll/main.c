#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/esprite.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"
#include "string.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

typedef struct TeamPlayerGroup {
    /* 0x00 */ s32 teamIndex;
    /* 0x04 */ OMOBJ *playerObjects[2];
    /* 0x0C */ u8 playerIndices[2];
    /* 0x0E */ u8 playerMotionSlots[6];
} TeamPlayerGroup; /* size = 0x14 */

typedef struct LaneObjectState {
    /* 0x00 */ s32 laneIndex;
    /* 0x04 */ s32 crossingDirection;
    /* 0x08 */ s32 laneSide;
    /* 0x0C */ s32 crossingFrame;
    /* 0x10 */ s32 shadowModelId;
    /* 0x14 */ s32 upperLeftModelId;
    /* 0x18 */ s32 upperRightModelId;
    /* 0x1C */ s32 lowerLeftInnerModelId;
    /* 0x20 */ s32 lowerLeftOuterModelId;
    /* 0x24 */ s32 lowerRightOuterModelId;
    /* 0x28 */ s32 lowerRightInnerModelId;
    /* 0x2C */ char unreferencedAlignmentBytes[8];
    /* 0x34 */ s32 animationHandle;
    /* 0x38 */ float laneZPosition;
} LaneObjectState; /* size = 0x3C */

typedef struct PlayerState {
    /* 0x00 */ s32 teamIndex;
    /* 0x04 */ u32 playerIndex;
    /* 0x08 */ u32 teamSlot;
    /* 0x0C */ s32 characterId;
    /* 0x10 */ s32 controllerIndex;
    /* 0x14 */ s32 isComputer;
    /* 0x18 */ s32 difficulty;
    /* 0x1C */ s32 unreferencedPlayerStateWord1c;
    /* 0x20 */ s32 unreferencedPlayerStateWord20;
    /* 0x24 */ s32 playerActionState;
    /* 0x28 */ s32 motionIndex;
    /* 0x2C */ s32 actionTimer;
    /* 0x30 */ s32 inputLockTimer;
    /* 0x34 */ s32 visibilityToggle;
    /* 0x38 */ s32 visibilityTimer;
    /* 0x3C */ s32 collisionCooldown;
    /* 0x40 */ float spawnPositionX;
    /* 0x44 */ float facingAngle;
    /* 0x48 */ OMOBJ *teamObject;
    /* 0x4C */ OMOBJ *partnerObject;
    /* 0x50 */ float movementHistory[15];
    /* 0x8C */ s32 movementHistoryIndex;
    /* 0x90 */ s32 currentLaneIndex;
    /* 0x94 */ s32 targetLaneIndex;
    /* 0x98 */ float jumpHeight;
    /* 0x9C */ Vec movementDirection;
    /* 0xA8 */ s32 effectSlot;
    /* 0xAC */ s32 effectTimer;
    /* 0xB0 */ s32 aiTargetLane;
    /* 0xB4 */ s32 aiDecisionTimer;
    /* 0xB8 */ s32 aiWaitTimer;
    /* 0xBC */ s32 aiState;
} PlayerState; /* size = 0xC0 */

typedef struct CrossingEffectState {
    /* 0x00 */ s32 modelId;
    /* 0x04 */ char unreferencedModelBytes[4];
    /* 0x08 */ u32 idleMotionId;
    /* 0x0C */ s32 actionMotionId;
    /* 0x10 */ char unreferencedMotionBytes[8];
    /* 0x18 */ s32 displayLayer;
    /* 0x0C */ s32 previousDisplayLayer;
    /* 0x20 */ s32 effectSlot;
    /* 0x24 */ s32 playerIndex;
    /* 0x28 */ s32 effectState;
    /* 0x2C */ s32 stateTimer;
    /* 0x30 */ s32 stateFrame;
    /* 0x34 */ float movementSpeed;
    /* 0x38 */ s32 isHidden;
    /* 0x3C */ float positionX;
    /* 0x40 */ float positionY;
    /* 0x44 */ float positionZ;
    /* 0x48 */ float facingAngle;
    /* 0x4C */ float sideOffset;
} CrossingEffectState; /* size = 0x50 */

typedef struct ParticleEffectState {
    /* 0x00 */ s32 effectSlot;
    /* 0x04 */ s32 modelId;
    /* 0x08 */ ANIMDATA *spriteAnimation;
    /* 0x0C */ s32 isActive;
    /* 0x10 */ s32 remainingFrames;
    /* 0x14 */ Vec position;
    /* 0x20 */ float scale;
} ParticleEffectState; /* size = 0x24 */

typedef struct ImpactEffectState {
    /* 0x00 */ s32 modelId;
    /* 0x04 */ s32 isActive;
    /* 0x08 */ s32 elapsedFrames;
    /* 0x0C */ float positionX;
    /* 0x10 */ float positionY;
    /* 0x14 */ float positionZ;
    /* 0x18 */ float scale;
} ImpactEffectState; /* size = 0x1C */

typedef struct ScoreboardState {
    /* 0x00 */ s32 scorePositionX;
    /* 0x04 */ s32 scorePositionY;
    /* 0x08 */ s32 displayedScore;
    /* 0x0C */ s32 scaleAnimationFrames;
    /* 0x10 */ s32 isVisible;
    /* 0x14 */ s32 digitSprites[6];
    /* 0x2C */ s32 digitBasePositions[6][3];
    /* 0x74 */ s32 digitScaleOffsets[6][3];
    /* 0xBC */ s32 digitSpacingX;
    /* 0xC0 */ s32 digitSpacingY;
} ScoreboardState; /* size = 0xC4 */

typedef struct ParticleVertex {
    /* 0x00 */ s16 animationFrame;
    /* 0x02 */ s16 reservedFlag;
    /* 0x04 */ Vec velocity;
    /* 0x10 */ char unreferencedVertexBytes[0x18];
    /* 0x28 */ float alpha;
    /* 0x2C */ float rotation;
    /* 0x30 */ Vec position;
    /* 0x3C */ GXColor color;
    /* 0x40 */ s32 textureFrame;
} ParticleVertex; /* size = 0x44 */

typedef struct JointState {
    /* 0x00 */ char unreferencedJointStateBytes[0x14];
    /* 0x14 */ s32 jointState;
} JointState; /* size = unknown */

typedef void (*ParticleModelDrawHook)(struct Hu3DModel_s *, struct ParticleModelData *, Mtx);

typedef struct ParticleModelData {
    /* 0x00 */ s16 unreferencedInitialFrame;
    /* 0x02 */ s16 reservedParticleFlag;
    /* 0x04 */ char unreferencedHookHeader[0x1C];
    /* 0x20 */ u8 blendMode;
    /* 0x21 */ u8 reservedHookByte;
    /* 0x22 */ char unreferencedHookBytes[2];
    /* 0x24 */ s16 particleCount;
    /* 0x26 */ char reservedParticleBytes[2];
    /* 0x28 */ s32 animationFrame;
    /* 0x2C */ s32 lastDrawFrame;
    /* 0x30 */ u32 animationFrameLimit;
    /* 0x34 */ u32 displayListSize;
    /* 0x38 */ ANIMDATA *animationData;
    /* 0x3C */ ParticleVertex *particles;
    /* 0x40 */ Vec *particlePositions;
    /* 0x44 */ Vec2f *particleTextureCoordinates;
    /* 0x48 */ void *displayListBuffer;
    /* 0x4C */ ParticleModelDrawHook drawHook;
    /* 0x50 */ HU3DMODEL *relatedModel;
} ParticleModelData; /* size = 0x54 */

typedef struct ModelNode {
    /* 0x00 */ char unreferencedNodeHeader[4];
    /* 0x04 */ s32 nodeType;
    /* 0x08 */ char unreferencedNodeMetadata[4];
    /* 0x0C */ s32 processedFlags;
    /* 0x10 */ char unreferencedNodeMetadata10[4];
    /* 0x14 */ s32 childCount;
    /* 0x18 */ struct ModelNode **children;
    /* 0x1C */ char unreferencedNodeBytes[0x48];
    /* 0x64 */ struct ModelNode *singleChild;
} ModelNode; /* size = unknown */

typedef struct FaceAttributeSet {
    /* 0x00 */ u16 faceAttributeWordA;
    /* 0x04 */ u16 faceAttributeWordB;
    /* 0x08 */ u16 faceAttributeWordC;
    /* 0x0C */ u16 faceAttributeWordD;
} FaceAttributeSet; /* size = 0x10 */

typedef struct ModelFaceAttributes {
    /* 0x00 */ s16 faceCommandFlags;
    /* 0x02 */ char unreferencedFacePadding[0x0A];
    /* 0x0C */ FaceAttributeSet firstFaceAttributes;
    /* 0x14 */ FaceAttributeSet secondFaceAttributes;
    /* 0x1C */ char unreferencedFaceTail[0x14];
} ModelFaceAttributes; /* size = 0x30 */

typedef struct CharacterMotionResource {
    /* 0x00 */ s32 motionResourceId;
    /* 0x04 */ u32 jointDataIndex;
} CharacterMotionResource; /* size = 0x08 */

void InitializeTeam(OMOBJ *object);
void UpdateTeam(OMOBJ *object);
void InitializePlayer(OMOBJ *object);
void UpdatePlayer(OMOBJ *object);
void InitializeGameplay(OMOBJ *object);
void ClearObjectCallback(OMOBJ *object);
void DestroyTeams(void);
s32 ArePlayersReady(void);
void UpdateComputerPlayerInput(OMOBJ *object, float *object, float *stickX, s32 *stickY);
void UpdateGameplay(OMOBJ *object);
void CreateLaneObjects(void);
void InitializeLaneObjects(OMOBJ *object);
void UpdateLaneObjects(OMOBJ *object);
void RemoveLaneObjects(void);
void SetLaneSide(s32 laneIndex, s32 laneSide);
s32 GetLaneSide(s32 laneIndex);
s32 GetNearestLane(float worldZ);
float GetLanePosition(s32 laneIndex);
s32 TryCrossLane(Vec *worldPosition, float facingAngle, s32 startCrossing);
void CreateCrossingEffects(void);
void InitializeCrossingEffects(OMOBJ *object);
void UpdateCrossingEffects(OMOBJ *object);
void RemoveCrossingEffects(void);
s32 SpawnCrossingEffect(s32 playerIndex, s32 delayFrames, Vec *spawnPosition);
s32 GetCrossingWinner(void);
void CreateParticleEffects(void);
void InitializeParticleEffects(OMOBJ *object);
void UpdateParticleEffects(OMOBJ *object);
void RemoveParticleEffects(void);
s32 AllocateParticleEffect(Vec *position);
void SetParticleEffectPosition(s32 effectIndex, Vec *position);
s32 SpawnParticle(s32 effectIndex, Vec *position, Vec *velocity, float alpha, GXColor color);
void CreateImpactEffects(void);
void InitializeImpactEffects(OMOBJ *object);
void UpdateImpactEffects(OMOBJ *object);
void RemoveImpactEffects(void);
s32 SpawnImpactEffect(Vec *position);
void CreateScoreboards(void);
void InitializeScoreboards(OMOBJ *object);
void UpdateScoreboards(OMOBJ *object);
void RemoveScoreboards(void);
void SetScoreboardsVisible(s32 isVisible);
void InitializeCamera(void);
void InitializeCameraController(OMOBJ *object);
void UpdateCameraController(OMOBJ *object);
void RemoveCameraController(void);
float GetAngleDifference(float targetAngle, float currentAngle);
float ApproachAngle(float targetAngle, float currentAngle, float maximumStep);
s16 CreateParticleModel(ANIMDATA *animationData, s16 particleCount);
s16 CloneParticleModel(s16 sourceModelId);
void DrawParticleModel(HU3DMODEL *model, Mtx modelMatrix);
void DrawBillboardSprite(HU3DMODEL *model, Mtx modelMatrix);
void DestroySpriteAnimation(ANIMDATA *animationData);
void DrawCameraOverlay(HU3DMODEL *model, Mtx modelMatrix);
void UpdateGroupModelNode(s32 modelIndex, ModelNode *node, s32 clearMask);
void UpdateContainerModelNode(s32 modelIndex, ModelNode *node, s32 clearMask);
void UpdateRecursiveModelNode(s32 modelIndex, ModelNode *node, s32 clearMask);
void UpdateSingleChildModelNode(s32 modelIndex, ModelNode *node, s32 clearMask);
void UpdateChildModelNodeTypeFour(s32 modelIndex, ModelNode *node, s32 clearMask);
void UpdateChildModelNodeTypeFive(s32 modelIndex, ModelNode *node, s32 clearMask);
float InterpolateCameraParameter(float progress, float startValue, float controlValue, float endValue);
void FinishMinigame(OMOBJ *object);
void CheckMinigameExit(OMOBJ *object);
void AssignTeamPlayers(void);

extern u32 GlobalCounter;

Vec playerSpawnPositions[4] = {
    { -600.0f, 0.0f, -285.0f },
    { -600.0f, 0.0f, 135.0f },
    { 600.0f, 0.0f, -285.0f },
    { 600.0f, 0.0f, 135.0f },
};

Vec globalLightPosition = { -200.0f, 2500.0f, 1200.0f };

Vec globalLightDirection = { 0.2f, -0.8f, -0.2f };

GXColor globalLightColor = { 143, 143, 143, 143 };

float cameraSetupParameters[3][7] = {
    { 315.0f, 0.0f, 0.0f, 0.0f, 130.0f, -180.0f, 1625.0f },
    { 339.0f, 0.0f, 0.0f, 0.0f, 5.0f, -1446.0f, 1625.0f },
    { 339.0f, 0.0f, 0.0f, 0.0f, 5.0f, -380.0f, 1625.0f },
};

Vec shadowCameraPosition = { 0.0f, 2500.0f, -500.0f };

Vec shadowCameraUp = { 0.0f, 1.0f, 0.0f };

Vec shadowCameraTarget = { 0.0f, 0.0f, 0.0f };

HUPROCESS *objectManager;
OMOBJ *mainGameObject;
OMOBJ *teamObjects[2];
OMOBJ *playerObjects[4];
OMOBJ *laneObjects;
OMOBJ *crossingEffectObject;
OMOBJ *particleEffectObject;
OMOBJ *impactEffectObject;
OMOBJ *scoreboardObject;
OMOBJ *cameraControllerObject;
s32 cameraUpdatesDisabled;
s32 gameplayFrame;
s32 gameStateTimer;
s32 gameState;
s32 gameSequenceId;
s32 countdownSequenceId;
s32 musicSequenceHandle;
s32 roundResult;
s32 playerControlsEnabled;
s32 crossingEffectsEnabled;
s32 roundStarted;
float crossingEffectSpeed;
s32 laneAnimationTimer;
s32 globalLightId;
float globalLightFade;
ANIMDATA *particleSpriteAnimation;
s32 teamScores[2];
u8 playerOrder[4];
Vec collisionPosition;
Vec particlePosition;

void ObjectSetup(void)
{
    HUPROCESS *objectManager;
    HU3DLIGHT *light;
    s32 globalLightId;

    OSReport("******* M426ObjectSetup *********\n");
    Hu3DLighInit();
    globalLightId = Hu3DGLightCreateV(&globalLightPosition, &globalLightDirection, &globalLightColor);
    globalLightId = globalLightId;
    Hu3DGLightInfinitytSet(globalLightId);
    light = Hu3DGlobalLight;
    light->type |= 32768;
    globalLightFade = 0.0f;
    objectManager = omInitObjMan(50, 8192);
    gameObjectManager = objectManager;
    omGameSysInit(objectManager);
    HuAudSndGrpSet(51);
    InitializeCamera();
    mainGameObject = omAddObjEx(objectManager, 255, 2, 0, -1, InitializeGameplay);
    Hu3DShadowCreate(35.0f, 2000.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    Hu3DShadowPosSet(&shadowCameraPosition, &shadowCameraUp, &shadowCameraTarget);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
}

void InitializeGameplay(OMOBJ *object)
{
    object->mdlId[0] = Hu3DHookFuncCreate(DrawCameraOverlay);
    Hu3DModelLayerSet(object->mdlId[0], 1);
    AssignTeamPlayers();
    CreateCrossingEffects();
    CreateLaneObjects();
    CreateParticleEffects();
    CreateImpactEffects();
    CreateScoreboards();
    gameplayFrame = 0;
    gameStateTimer = 0;
    gameState = 0;
    musicSequenceHandle = -1;
    roundResult = 1;
    playerControlsEnabled = 0;
    crossingEffectsEnabled = 0;
    roundStarted = 0;
    crossingEffectSpeed = 2.0f;
    teamScores[0] = teamScores[1] = 10;
    object->objFunc = UpdateGameplay;
}

void UpdateGameplay(OMOBJ *object)
{
    float cameraParameters[16];

    float cameraBlend;

    s32 cameraParameterIndex;
    TeamPlayerGroup *playerGroup;
    PlayerState *playerState;
    s32 winnerIndex;
    s32 teamIndex;
    s32 sequenceStatus;
    s32 countdownSeconds;

    winnerIndex = -1;
    {
        s32 sp8[2] = { 0, 1 };
        switch (gameState) {
            case 0:
                if (!WipeStatGet()) {
                    gameState = 1;
                    gameStateTimer = 0;
                    laneAnimationTimer = 2 * REFRESH_RATE;
                }
                break;
            case 1:
                if (++gameStateTimer >= REFRESH_RATE / 4.0f) {
                    gameState = 2;
                    gameStateTimer = 0;
                    roundStarted = 1;
                }
                break;
            case 2:
                cameraBlend = (1.0f/(REFRESH_RATE*2.3f)) * gameStateTimer;
                cameraBlend = sind(90.0f * cameraBlend);
                cameraBlend *= cameraBlend;
                for (cameraParameterIndex = 0; cameraParameterIndex < 7; cameraParameterIndex++) {
                    cameraParameters[cameraParameterIndex] = InterpolateCameraParameter(cameraBlend, cameraSetupParameters[1][cameraParameterIndex], cameraSetupParameters[2][cameraParameterIndex], cameraSetupParameters[0][cameraParameterIndex]);
                }
                CRot.x = cameraParameters[0];
                CRot.y = cameraParameters[1];
                CRot.z = cameraParameters[2];
                Center.x = cameraParameters[3];
                Center.y = cameraParameters[4];
                Center.z = cameraParameters[5];
                CZoom = cameraParameters[6];
                if (++gameStateTimer >= (REFRESH_RATE*2.3f)) {
                    CRot.x = cameraSetupParameters[0][0];
                    CRot.y = cameraSetupParameters[0][1];
                    CRot.z = cameraSetupParameters[0][2];
                    Center.x = cameraSetupParameters[0][3];
                    Center.y = cameraSetupParameters[0][4];
                    Center.z = cameraSetupParameters[0][5];
                    CZoom = cameraSetupParameters[1][6];
                    gameState = 3;
                    gameStateTimer = 0;
                }
                break;
            case 3:
                if (++gameStateTimer >= (REFRESH_RATE / 2.0f)) {
                    gameSequenceId = MGSeqCreate(3, 0);
                    gameState = 4;
                    gameStateTimer = 0;
                }
                break;
            case 4:
                if (gameSequenceId != -1) {
                    sequenceStatus = MGSeqStatGet(gameSequenceId);
                    if ((sequenceStatus & 16) && musicSequenceHandle == -1) {
                        musicSequenceHandle = HuAudSeqPlay(70);
                    }
                }
                else {
                    sequenceStatus = 0;
                }
                if (gameSequenceId == -1 || !sequenceStatus) {
                    gameSequenceId = -1;
                    playerControlsEnabled = 1;
                    crossingEffectsEnabled = 1;
                    gameState = 5;
                    gameStateTimer = 30 * REFRESH_RATE;
                    SetScoreboardsVisible(1);
                    countdownSequenceId = MGSeqCreate(1, gameStateTimer / REFRESH_RATE, -1, -1);
                }
                break;
            case 5:
                countdownSeconds = (gameStateTimer + REFRESH_RATE - 1) / REFRESH_RATE;
                if (countdownSeconds < 0) {
                    countdownSeconds = 0;
                }
                MGSeqParamSet(countdownSequenceId, 1, countdownSeconds);
                if (gameStateTimer == 10 * REFRESH_RATE) {
                    crossingEffectSpeed = 3.0f;
                }
                else if (gameStateTimer == 20 * REFRESH_RATE) {
                    crossingEffectSpeed = 2.5f;
                }
                if (--gameStateTimer < -1 || teamScores[0] <= 0 || teamScores[1] <= 0) {
                    MGSeqParamSet(countdownSequenceId, 2, -1);
                    countdownSequenceId = -1;
                    HuAudSeqAllFadeOut(100);
                    gameSequenceId = MGSeqCreate(3, 1);
                    playerControlsEnabled = 0;
                    crossingEffectsEnabled = 0;
                    gameState = 6;
                    gameStateTimer = 0;
                }
                break;
            case 6:
                if (gameSequenceId == -1 || !MGSeqStatGet(gameSequenceId)) {
                    gameSequenceId = -1;
                    winnerIndex = GetCrossingWinner();
                    if (winnerIndex >= 0) {
                        if (ArePlayersReady()) {
                            if (teamScores[0] == teamScores[1]) {
                                winnerIndex = 2;
                            }
                            else if (teamScores[0] > teamScores[1]) {
                                winnerIndex = 0;
                            }
                            else {
                                winnerIndex = 1;
                            }
                            if (winnerIndex < 2) {
                                playerGroup = teamObjects[winnerIndex]->data;
                                for (cameraParameterIndex = 0; cameraParameterIndex < 2; cameraParameterIndex++) {
                                    GWPlayerCoinWinAdd(playerGroup->playerIndices[cameraParameterIndex], 10);
                                    playerState = playerGroup->playerObjects[cameraParameterIndex]->data;
                                    sp8[cameraParameterIndex] = playerState->characterId;
                                    playerState->playerActionState = 8;
                                }
                                playerGroup = teamObjects[~winnerIndex & 1]->data;
                                for (cameraParameterIndex = 0; cameraParameterIndex < 2; cameraParameterIndex++) {
                                    playerState = playerGroup->playerObjects[cameraParameterIndex]->data;
                                    playerState->playerActionState = 9;
                                }
                                gameSequenceId = MGSeqCreate(5, 3, sp8[0], sp8[1], -1, -1);
                                HuAudSStreamPlay(1);
                            }
                            else {
                                for (teamIndex = 0; teamIndex < 2; teamIndex++) {
                                    playerGroup = teamObjects[teamIndex]->data;
                                    for (cameraParameterIndex = 0; cameraParameterIndex < 2; cameraParameterIndex++) {
                                        playerState = playerGroup->playerObjects[cameraParameterIndex]->data;
                                        playerState->playerActionState = 9;
                                    }
                                }
                                gameSequenceId = MGSeqCreate(3, 2);
                                HuAudSStreamPlay(4);
                            }
                            gameState = 7;
                            gameStateTimer = 0;
                        }
                    }
                }
                break;
            case 7:
                if (++gameStateTimer > 3.5f * REFRESH_RATE) {
                    gameSequenceId = -1;
                    gameState = 8;
                    gameStateTimer = 0;
                }
                break;
            case 8:
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                object->objFunc = FinishMinigame;
                break;
            default:
                break;
        }
    }
    gameplayFrame++;
    CheckMinigameExit(object);
}

s32 initialTeamLayout = 66051;

s32 characterDataDirectoryIds[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

s32 playerMotionData[8] = {
    DATADIR_MARIOMOT,
    DATADIR_LUIGIMOT,
    DATADIR_PEACHMOT,
    DATADIR_YOSHIMOT,
    DATADIR_DONKEYMOT,
    DATADIR_WARIOMOT,
    DATADIR_WALUIGIMOT,
    DATADIR_DAISYMOT,
};

CharacterMotionResource playerMotionResources[12] = {
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0), DATA_NUM_LISTEND },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 2), DATA_NUM_LISTEND },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 3), DATA_NUM_LISTEND },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 5), DATA_NUM_LISTEND },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 6), DATA_NUM_LISTEND },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 20), DATA_NUM_LISTEND },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 22), DATA_NUM_LISTEND },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 25), DATA_NUM_LISTEND },
    { 0, 0 },
    { 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 23), DATA_NUM_LISTEND },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 114), DATA_NUM_LISTEND },
};

s32 characterModelDataIds[2] = { DATA_MAKE_NUM(DATADIR_M426, 32), DATA_MAKE_NUM(DATADIR_M426, 40) };

void FinishMinigame(OMOBJ *object)
{
    if (!WipeStatGet()) {
        RemoveScoreboards();
        RemoveImpactEffects();
        RemoveParticleEffects();
        RemoveLaneObjects();
        RemoveCrossingEffects();
        DestroyTeams();
        RemoveCameraController();
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void CheckMinigameExit(OMOBJ *object)
{
    s32 exitRequested;

    exitRequested = 0;
    if (WipeStatGet() || object->objFunc == FinishMinigame) {
        return;
    }
    if (omSysExitReq) {
        exitRequested = 1;
    }
    if (exitRequested) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        HuAudSeqAllFadeOut(100);
        object->objFunc = FinishMinigame;
    }
}

void AssignTeamPlayers(void)
{
    s32 playerIndex;
    s32 teamPlayerCount;

    for (playerIndex = 0, teamPlayerCount = 0; playerIndex < 4; playerIndex++) {
        if (!GWPlayerCfg[playerIndex].group) {
            playerOrder[teamPlayerCount] = playerIndex;
            teamPlayerCount++;
        }
    }
    if (teamPlayerCount != 2) {
        memcpy(&playerOrder, &defaultPlayerOrder, 4);
    }
    else {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (GWPlayerCfg[playerIndex].group) {
                playerOrder[teamPlayerCount] = playerIndex;
                teamPlayerCount++;
            }
        }
    }
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        teamObjects[playerIndex] = omAddObjEx(gameObjectManager, 32, 0, 0, -1, InitializeTeam);
        teamObjects[playerIndex]->work[0] = playerIndex;
    }
}

void InitializeTeam(OMOBJ *object)
{
    TeamPlayerGroup *playerGroup;
    s32 memberIndex;

    playerGroup = object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(TeamPlayerGroup), HU_MEMNUM_OVL);
    memset(playerGroup, 0, sizeof(TeamPlayerGroup));
    playerGroup->teamIndex = object->work[0];
    playerGroup->playerIndices[0] = playerOrder[playerGroup->teamIndex * 2];
    playerGroup->playerIndices[1] = playerOrder[playerGroup->teamIndex * 2 + 1];
    for (memberIndex = 0; memberIndex < 6; memberIndex++) {
        playerGroup->playerMotionSlots[memberIndex] = 255;
    }
    for (memberIndex = 0; memberIndex < 2; memberIndex++) {
        playerGroup->playerObjects[memberIndex] = playerObjects[playerGroup->playerIndices[memberIndex]] = omAddObjEx(gameObjectManager, 36, 1, 12, -1, InitializePlayer);
        playerGroup->playerObjects[memberIndex]->work[0] = playerGroup->playerIndices[memberIndex];
        playerGroup->playerObjects[memberIndex]->work[1] = playerGroup->teamIndex;
        playerGroup->playerObjects[memberIndex]->work[2] = memberIndex;
    }
    omSetStatBit(object, 160);
    object->objFunc = UpdateTeam;
}

void UpdateTeam(OMOBJ *object)
{
    TeamPlayerGroup *playerGroup;

    playerGroup = object->data;
    if (gameplayFrame & 1) {
        UpdatePlayer(playerGroup->playerObjects[0]);
        UpdatePlayer(playerGroup->playerObjects[1]);
        return;
    }
    UpdatePlayer(playerGroup->playerObjects[1]);
    UpdatePlayer(playerGroup->playerObjects[0]);
}

void DestroyTeam(OMOBJ *object)
{
    s32 memberIndex;
    TeamPlayerGroup *playerGroup;

    playerGroup = object->data;
    for (memberIndex = 0; memberIndex < 2; memberIndex++) {
        ClearObjectCallback(playerGroup->playerObjects[memberIndex]);
    }
    object->data = NULL;
    object->objFunc = NULL;
}

void InitializePlayer(OMOBJ *object)
{
    PlayerState *playerState;
    s32 motionIndex;
    TeamPlayerGroup *playerGroup;
    s32 motionSlot;
    s32 playerCharacter;
    s32 modelId;

    playerState = object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerState), HU_MEMNUM_OVL);
    memset(playerState, 0, sizeof(PlayerState));
    playerState->teamIndex = object->work[1];
    playerState->playerIndex = object->work[0];
    playerState->teamSlot = object->work[2];
    playerCharacter = GWPlayerCfg[playerState->playerIndex].character;
    playerState->characterId = playerCharacter;
    playerState->controllerIndex = GWPlayerCfg[playerState->playerIndex].pad_idx;
    playerState->isComputer = GWPlayerCfg[playerState->playerIndex].iscom;
    playerState->difficulty = GWPlayerCfg[playerState->playerIndex].diff;
    playerCharacter = 0;
    motionSlot = playerState->teamSlot + (playerState->teamIndex * 2);
    playerState->aiTargetLane = -1;
    playerState->aiDecisionTimer = 0;
    playerState->aiWaitTimer = frand() % 15;
    playerState->aiState = 0;
    playerState->unreferencedPlayerStateWord1c = 0;
    playerState->unreferencedPlayerStateWord20 = 0;
    playerState->playerActionState = 0;
    playerState->motionIndex = 0;
    playerState->actionTimer = 0;
    playerState->inputLockTimer = 0;
    playerState->visibilityToggle = 0;
    playerState->visibilityTimer = 0;
    playerState->collisionCooldown = 0;
    playerState->spawnPositionX = playerSpawnPositions[motionSlot].x;
    if (playerState->spawnPositionX < 0.0f) {
        playerState->facingAngle = 90.0f;
    }
    else {
        playerState->facingAngle = -90.0f;
    }
    playerState->teamObject = teamObjects[playerState->teamIndex];
    playerGroup = playerState->teamObject->data;
    if (object != playerGroup->playerObjects[0]) {
        playerState->partnerObject = playerGroup->playerObjects[0];
    }
    else {
        playerState->partnerObject = playerGroup->playerObjects[1];
    }
    memset(&playerState->movementHistory, 0, 0x3C);
    playerState->movementHistoryIndex = 0;
    playerState->currentLaneIndex = GetNearestLane(playerSpawnPositions[motionSlot].z);
    playerState->targetLaneIndex = -1;
    playerState->jumpHeight = 0.0f;
    playerGroup->playerMotionSlots[playerState->currentLaneIndex] = playerState->playerIndex;
    playerState->effectSlot = AllocateParticleEffect(&playerSpawnPositions[motionSlot]);
    playerState->effectTimer = 0;
    object->trans.x = playerSpawnPositions[motionSlot].x;
    object->trans.y = playerSpawnPositions[motionSlot].y;
    object->trans.z = playerSpawnPositions[motionSlot].z;
    modelId = CharModelCreate(playerState->characterId, 4);
    object->mdlId[0] = modelId;
    Hu3DModelLayerSet(object->mdlId[0], 2);
    Hu3DModelShadowSet(object->mdlId[0]);
    CharModelStepFxSet(playerState->characterId, 0);
    for (motionIndex = 0; motionIndex < 12; motionIndex++) {
        if (playerMotionResources[motionIndex].jointDataIndex == -1) {
            if ((motionIndex == 11) && (playerState->characterId == 4)) {
                object->mtnId[motionIndex] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 114));
            }
            else {
                object->mtnId[motionIndex] = CharMotionCreate(playerState->characterId, playerMotionResources[motionIndex].motionResourceId);
            }
            if (motionIndex == 5) {
                CharMotionVoiceOnSet(playerState->characterId, object->mtnId[motionIndex], 0);
            }
        }
        else {
            object->mtnId[motionIndex] = Hu3DJointMotion(
                object->mdlId[0], HuDataSelHeapReadNum(playerState->characterId + characterJointMotionIndices[playerMotionResources[motionIndex].jointDataIndex], 268435456, HEAP_MODEL));
        }
    }
    CharMotionDataClose(playerState->characterId);
    CharMotionSet(playerState->characterId, object->mtnId[playerState->motionIndex]);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    HuDataDirClose(characterMotionDirectories[playerCharacter] & 0xFFFF0000);
    object->objFunc = NULL;
}

void UpdatePlayer(OMOBJ *object)
{
    s32 candidateLanes[3];
    void *teamData;
    GXColor particleColor;
    float stickX;
    float stickY;
    s32 unreferencedLocalState;
    s32 unreferencedLocalFlags;
    s32 buttonFlags;
    GXColor unreferencedParticleColor;

    float movementDeltaZ;
    float movementDeltaX;
    float movementSpeed;
    float movementDistance;
    float facingAngle;
    float targetPositionX;
    float targetPositionZ;
    float unreferencedFloatScratch;
    float previousPositionX;
    float previousPositionZ;

    PlayerState *playerState;
    s32 historySampleIndex;
    PlayerState *playerState2;
    s32 candidateLaneIndex;
    s32 motionIndex;
    s32 alternateLaneIndex;
    s32 characterModelId;
    s32 particleGrayLevel;
    s32 particleCount;
    s32 controllerButtonState;

    unreferencedLocalState = 0;
    unreferencedLocalFlags = 0;
    playerState = object->data;
    playerState2 = playerState->partnerObject->data;
    teamData = teamObjects[playerState->teamIndex]->data;
    characterModelId = object->mdlId[0];
    if (omPauseChk()) {
        Hu3DModelTPLvlSet(characterModelId, 1.0f);
        return;
    }
    if (playerState->isComputer) {
        stickX = stickY = 0.0f;
        controllerButtonState = buttonFlags = 0;
        UpdateComputerPlayerInput(object, &stickX, &stickY, &buttonFlags);
    }
    else {
        stickX = HuPadStkX[playerState->controllerIndex];
        stickY = HuPadStkY[playerState->controllerIndex];
        controllerButtonState = HuPadBtn[playerState->controllerIndex];
        buttonFlags = HuPadBtnDown[playerState->controllerIndex];
    }
    movementDeltaX = movementDeltaZ = 0.0f;
    candidateLaneIndex = alternateLaneIndex = -1;
    previousPositionX = object->trans.x;
    previousPositionZ = object->trans.z;
    if (!cameraUpdatesDisabled) {
        switch (playerState->playerActionState) {
            case 0:
            case 1:
                if (playerState->currentLaneIndex == -1) {
                    playerState->currentLaneIndex = GetNearestLane(object->trans.z);
                }
                else {
                    if (!playerControlsEnabled) {
                        stickX = stickY = 0.0f;
                        controllerButtonState = buttonFlags = 0;
                    }
                    if (fabs(stickX) > 8.0) {
                        movementDeltaX = 0.25f * stickX;
                    }
                    if (fabs(stickY) > 8.0) {
                        movementDeltaZ = -(0.25f * stickY);
                    }
                    if (playerState->collisionCooldown) {
                        playerState->collisionCooldown--;
                    }
                    if (playerState->targetLaneIndex != -1) {
                        movementDistance = 16.0f;
                    }
                    else {
                        movementDistance = 0.0f;
                    }
                    playerState->movementHistory[playerState->movementHistoryIndex] = movementDistance;
                    if (++playerState->movementHistoryIndex >= 15) {
                        playerState->movementHistoryIndex = 0;
                    }
                    for (historySampleIndex = 0, movementSpeed = 0.0f; historySampleIndex < 15; historySampleIndex++) {
                        movementSpeed += playerState->movementHistory[historySampleIndex];
                    }
                    movementSpeed *= 0.06666667f;
                    if (playerState->jumpHeight != 0.0f) {
                        motionIndex = 3;
                    }
                    else if (movementSpeed <= 0.0f) {
                        if (gameState == 5) {
                            motionIndex = 8;
                        }
                        else {
                            motionIndex = 0;
                        }
                    }
                    else if (movementSpeed < 10.0f) {
                        motionIndex = 1;
                    }
                    else {
                        motionIndex = 2;
                    }
                    if (motionIndex != playerState->motionIndex) {
                        playerState->motionIndex = motionIndex;
                        if (motionIndex == 3) {
                            CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, HU3D_MOTATTR_NONE);
                        }
                        else {
                            CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                        }
                    }
                    if (playerState->targetLaneIndex == -1) {
                        if (fabs(movementDeltaZ) > 2.0) {
                            if (movementDeltaZ < 0.0f) {
                                candidateLaneIndex = playerState->currentLaneIndex - 1;
                                alternateLaneIndex = playerState->currentLaneIndex - 2;
                            }
                            else {
                                candidateLaneIndex = playerState->currentLaneIndex + 1;
                                alternateLaneIndex = playerState->currentLaneIndex + 2;
                            }
                            if ((candidateLaneIndex >= 0) && (candidateLaneIndex < 6)
                                && (((playerState2->currentLaneIndex != candidateLaneIndex) && (playerState2->targetLaneIndex != candidateLaneIndex))
                                    || ((playerState2->currentLaneIndex == candidateLaneIndex) && (playerState2->targetLaneIndex == alternateLaneIndex) && (alternateLaneIndex >= 0) && (alternateLaneIndex < 6))
                                    || ((playerState2->jumpHeight != 0.0f) && (playerState2->currentLaneIndex == candidateLaneIndex)))) {
                                playerState->targetLaneIndex = candidateLaneIndex;
                            }
                        }
                    }
                    if (playerState->jumpHeight != 0.0f) {
                        object->trans.y = object->trans.y + playerState->jumpHeight;
                        playerState->jumpHeight -= 3.103333333333333;
                        if (object->trans.y <= 0.0f) {
                            playerState->jumpHeight = 0.0f;
                            object->trans.y = 0.0f;
                        }
                    }
                    if (playerState->targetLaneIndex != -1) {
                        targetPositionX = playerState->spawnPositionX;
                        targetPositionZ = GetLanePosition(playerState->targetLaneIndex);
                        movementDeltaX = targetPositionX - object->trans.x;
                        movementDeltaZ = targetPositionZ - object->trans.z;

                        movementDistance = sqrtf((movementDeltaX * movementDeltaX) + (movementDeltaZ * movementDeltaZ));

                        if (movementDistance <= movementSpeed) {
                            playerState->currentLaneIndex = playerState->targetLaneIndex;
                            playerState->targetLaneIndex = -1;
                            if (playerState->collisionCooldown) {
                                buttonFlags |= 256;
                            }
                            playerState->collisionCooldown = 0;
                            object->trans.x = targetPositionX;
                            object->trans.z = targetPositionZ;
                        }
                        else if (movementDistance != 0.0f) {
                            movementDistance = 1.0f / movementDistance;
                            object->trans.x += movementSpeed * (movementDeltaX * movementDistance) * (60.0f / REFRESH_RATE);
                            object->trans.z += movementSpeed * (movementDeltaZ * movementDistance) * (60.0f / REFRESH_RATE);
                        }
                    }
                    movementDeltaX = object->trans.x - previousPositionX;
                    movementDeltaZ = object->trans.z - previousPositionZ;
                    if (fabs(movementDeltaX) > 8.0 || fabs(movementDeltaZ) > 8.0) {
                        facingAngle = atan2d(movementDeltaX, movementDeltaZ);
                    }
                    else {
                        if (fabs(stickX) > 8.0 || fabs(stickY) > 8.0) {
                            facingAngle = atan2d(stickX, -stickY);
                        }
                        else {
                            facingAngle = object->rot.y;
                        }
                    }
                    object->rot.y = ApproachAngle(facingAngle, object->rot.y, 20.0f);
                    if (playerState->targetLaneIndex == -1 && playerState->jumpHeight == 0.0f) {
                        if (buttonFlags & 256) {
                            playerState->actionTimer = 0;
                            playerState->playerActionState = 2;
                            playerState->motionIndex = 9;
                            CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 8.0f, 8.0f, HU3D_MOTATTR_NONE);
                        }
                    }
                    else if (buttonFlags & 256) {
                        playerState->collisionCooldown = 20;
                    }
                }
                break;
            case 2:
                object->rot.y = ApproachAngle(playerState->facingAngle, object->rot.y, 20.0f);
                playerState->actionTimer++;
                if (playerState->actionTimer == 17) {
                    collisionPosition.x = object->trans.x;
                    collisionPosition.y = object->trans.y;
                    collisionPosition.z = object->trans.z;
                    if (TryCrossLane(&collisionPosition, object->rot.y, 1)) {
                        if (!playerState->teamIndex) {
                            HuAudFXPlay(1601);
                        }
                        else {
                            HuAudFXPlay(1602);
                        }
                    }
                }
                else if (playerState->actionTimer >= 20) {
                    playerState->playerActionState = 0;
                    playerState->actionTimer = 0;
                    playerState->motionIndex = 8;
                    CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                }
                break;
            case 3:
                object->rot.y = ApproachAngle(playerState->facingAngle, object->rot.y, 20.0f);
                object->trans.x += 4.0f * playerState->movementDirection.x;
                object->trans.z += 4.0f * playerState->movementDirection.z;
                object->trans.y += playerState->movementDirection.y;
                playerState->movementDirection.y -= 3.103333333333333;
                if ((object->rot.x -= 30.0f) <= 0.0f) {
                    object->rot.y += 360.0f;
                }
                if (fabs(object->trans.x) > 750.0) {
                    if (object->trans.x > 0.0f) {
                        object->trans.x = 750.0f;
                    }
                    else {
                        object->trans.x = -750.0f;
                    }
                }
                if (object->trans.y <= 0.0f) {
                    CharFXPlay(playerState->characterId, 281);
                    playerState->movementDirection.x = playerState->movementDirection.z = 0.0f;
                    playerState->movementDirection.y = 28.0f;
                    object->trans.y = 0.0f;
                    object->rot.x = 0.0f;
                    playerState->playerActionState = 5;
                }
                break;
            case 5:
                object->trans.y += playerState->movementDirection.y;
                playerState->movementDirection.y -= 3.103333333333333;
                if (object->trans.y <= 0.0f) {
                    object->trans.y = 0.0f;
                    playerState->movementDirection.y = 0.0f;
                    playerState->playerActionState = 6;
                    playerState->actionTimer = 0;
                    CharFXPlay(playerState->characterId, 281);
                    playerState->motionIndex = 6;
                    CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                }
                break;
            case 6:
                playerState->actionTimer++;
                if (playerState->actionTimer < 1.0f * REFRESH_RATE) {
                    if (playerControlsEnabled == 0) {
                        playerState->actionTimer = REFRESH_RATE - 1;
                    }
                }
                else if (playerState->actionTimer == 1.0f * REFRESH_RATE) {
                    playerState->motionIndex = 7;
                    CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, HU3D_MOTATTR_NONE);
                }
                else if (playerState->actionTimer >= ((REFRESH_RATE/7)+1)*10.0f) {
                    playerState->playerActionState = 7;
                    playerState->motionIndex = 8;
                    CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                }
                break;
            case 7:
                candidateLaneIndex = GetNearestLane(object->trans.z);
                candidateLanes[0] = candidateLaneIndex;
                candidateLanes[1] = candidateLaneIndex + 1;
                candidateLanes[2] = candidateLaneIndex - 1;
                if (candidateLanes[1] >= 6) {
                    candidateLanes[1] = candidateLaneIndex - 2;
                }
                if (candidateLanes[2] < 0) {
                    candidateLanes[2] = candidateLaneIndex + 2;
                }
                for (historySampleIndex = 0, playerState->targetLaneIndex = -1; historySampleIndex < 3; historySampleIndex++) {
                    if (playerState2->currentLaneIndex != candidateLanes[historySampleIndex] && playerState2->targetLaneIndex != candidateLanes[historySampleIndex] && candidateLanes[historySampleIndex] >= 0 && candidateLanes[historySampleIndex] < 6) {
                        playerState->targetLaneIndex = candidateLanes[historySampleIndex];
                        break;
                    }
                }
                if (playerState->targetLaneIndex != -1) {
                    playerState->playerActionState = 1;
                    playerState->actionTimer = 0;
                }
                break;
            case 8:
            case 9:
                object->rot.y = ApproachAngle(0.0f, object->rot.y, 10.0f);
                if (playerState->playerActionState == 8) {
                    motionIndex = 10;
                }
                else {
                    motionIndex = 11;
                }
                if (motionIndex != playerState->motionIndex) {
                    playerState->motionIndex = motionIndex;
                    CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, HU3D_MOTATTR_NONE);
                }
                break;
        }
        if (playerState->effectTimer) {
            playerState->effectTimer--;
        }
        particleCount = playerState->effectTimer / 2;
        if (particleCount > 16) {
            particleCount = 16;
        }
        for (historySampleIndex = 0; historySampleIndex < particleCount; historySampleIndex++) {
            collisionPosition.y = object->trans.y + ((frand() % 70) + 35);
            collisionPosition.x = object->trans.x + ((frand() % 50) + 25);
            collisionPosition.z = object->trans.z + ((frand() % 50) + 25);
            particlePosition.x = 0.0f;
            particlePosition.z = 0.0f;
            particlePosition.y = 0.0f;

            particleGrayLevel = (frand() & 63) + 128;

            particleColor.r = particleGrayLevel;
            particleColor.g = particleGrayLevel;
            particleColor.b = particleGrayLevel;
            particleColor.a = 64;

            SpawnParticle(playerState->effectSlot, &collisionPosition, NULL, 100.0f, particleColor);
        }
        if (playerState->inputLockTimer) {
            if (++playerState->visibilityTimer > 1) {
                playerState->visibilityTimer = 0;
                playerState->visibilityToggle = playerState->visibilityToggle ^ 1;
            }
        }
        if ((playerState->playerActionState == 0 || playerState->playerActionState == 1 || playerState->playerActionState == 2) && playerState->inputLockTimer) {
            if (playerState->inputLockTimer) {
                playerState->inputLockTimer = playerState->inputLockTimer - 1;
            }
            if (playerState->visibilityToggle) {
                Hu3DModelTPLvlSet(characterModelId, 0.25f);
                return;
            }
            Hu3DModelTPLvlSet(characterModelId, 1.0f);
            return;
        }
        Hu3DModelTPLvlSet(characterModelId, 1.0f);
    }
    (void)controllerButtonState;
}

Vec scoreboardPositions[2] = {
    { -473.71, 432.536, -569.82 },
    { 473.71, 432.536, -569.82 },
};

s32 scoreboardLabelResources[2] = { 6357019, 6357020 };

s16 scoreDigitOffsets[6] = { 11, 31, 0, 0, -33, -9 };

Vec particleQuadVertices[4] = {
    { -0.5f, 0.5f, 0.0f },
    { 0.5f, 0.5f, 0.0f },
    { 0.5f, -0.5f, 0.0f },
    { -0.5f, -0.5f, 0.0f },
};

Vec2f particleTextureOffsets[4] = {
    { 0.0f, 0.0f },
    { 0.25f, 0.0f },
    { 0.25f, 0.25f },
    { 0.0f, 0.25f },
};

s32 billboardSpriteVertices[] = {
    0xC2480000,
    0x42480000,
    0x00000000,
    0x42480000,
    0x42480000,
    0x00000000,
    0x42480000,
    0xC2480000,
    0x00000000,
    0xC2480000,
    0xC2480000,
    0x00000000,
};

s32 billboardSpriteColors[] = {
    0xFFFFA080,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
};

s32 billboardSpriteTextureCoordinates[] = {
    0x00000000,
    0x00000000,
    0x3F800000,
    0x00000000,
    0x3F800000,
    0x3F800000,
    0x00000000,
    0x3F800000,
};

void ClearObjectCallback(OMOBJ *object)
{
    object->data = NULL;
    object->objFunc = NULL;
}

void DestroyTeams(void)
{
    s32 teamIndex;

    for (teamIndex = 0; teamIndex < 2; teamIndex++) {
        DestroyTeam(teamObjects[teamIndex]);
    }
}

void HitPlayersNearPosition(Vec *hitPosition)
{
    Vec effectPosition;
    void *partnerStateData;

    PlayerState *playerState;
    OMOBJ *object;
    TeamPlayerGroup *playerGroup;
    s32 unreferencedEffectIndex;
    s32 soundHandle;
    s32 teamIndex;

    if (hitPosition->x < 0.0f) {
        teamIndex = 0;
    }
    else {
        teamIndex = 1;
    }
    if (teamObjects[teamIndex]) {
        playerGroup = teamObjects[teamIndex]->data;
        if (playerGroup) {
            for (unreferencedEffectIndex = 0; unreferencedEffectIndex < 2; unreferencedEffectIndex++) {
                object = playerGroup->playerObjects[unreferencedEffectIndex];
                playerState = object->data;
                partnerStateData = playerState->partnerObject->data;
                if (playerState && playerState->playerActionState <= 2) {
                    if (fabs(object->trans.z - hitPosition->z) < 140.0 && !playerState->inputLockTimer) {
                        playerState->actionTimer = 0;
                        playerState->playerActionState = 3;
                        playerState->effectTimer = 32;
                        playerState->inputLockTimer = 1.5f * REFRESH_RATE;
                        playerState->currentLaneIndex = playerState->targetLaneIndex = -1;
                        if (!playerGroup->teamIndex) {
                            playerState->movementDirection.x = -1.0f;
                        }
                        else {
                            playerState->movementDirection.x = 1.0f;
                        }
                        if (fabs(playerState->partnerObject->trans.z - object->trans.z) < 100.0) {
                            playerState->currentLaneIndex = GetNearestLane(object->trans.z);
                            if (playerState->currentLaneIndex == 5) {
                                playerState->movementDirection.z = -1.0f;
                            }
                            else if (!playerState->currentLaneIndex) {
                                playerState->movementDirection.z = 1.0f;
                            }
                            else if ((playerState->partnerObject->trans.z - object->trans.z) < 0.0f) {
                                playerState->movementDirection.z = 1.0f;
                            }
                            else {
                                playerState->movementDirection.z = -1.0f;
                            }
                        }
                        else {
                            playerState->movementDirection.z = 0.0f;
                        }
                        playerState->movementDirection.y = 45.5f;
                        playerState->motionIndex = 5;
                        CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->motionIndex], 0.0f, 8.0f, HU3D_MOTATTR_NONE);
                        effectPosition.x = object->trans.x;
                        effectPosition.y = object->trans.y;
                        effectPosition.z = object->trans.z;
                        SetParticleEffectPosition(playerState->effectSlot, &effectPosition);
                        soundHandle = HuAudFXPlay(1606);
                        if (soundHandle != -1) {
                            HuAudFXPanning(soundHandle, playerGroup->teamIndex * 64 + 32);
                        }
                        CharFXPlay(playerState->characterId, 291);
                        if (soundHandle != -1) {
                            HuAudFXPanning(soundHandle, playerGroup->teamIndex * 64 + 32);
                        }
                    }
                }
            }
        }
    }
}

s32 ArePlayersReady(void)
{
    PlayerState *playerState;
    s32 playerIndex;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerState = playerObjects[playerIndex]->data;
        if ((playerState->playerActionState != 0 && playerState->playerActionState != 1) || playerState->targetLaneIndex != -1) {
            break;
        }
    }
    if (playerIndex < 4) {
        return 0;
    }
    return 1;
}

void UpdateComputerPlayerInput(OMOBJ *object, float *stickX, float *stickY, s32 *buttonFlags)
{
    s32 candidateLanes[6];
    s32 opposingTeamLanes[2];
    TeamPlayerGroup *teamGroup;

    float inputMagnitude;

    PlayerState *playerState;
    s32 candidateIndex;
    s32 partnerLaneIndex;
    PlayerState *playerState2;
    s32 candidateCount;
    s32 laneChoiceScore;
    OMOBJ *object;
    TeamPlayerGroup *playerGroup;
    s32 decisionBlocked;
    s32 bestLaneScore;
    s32 candidateTeamIndex;

    candidateCount = 0;
    decisionBlocked = 0;
    playerState = object->data;
    playerState2 = playerState->partnerObject->data;
    teamGroup = teamObjects[playerState->teamIndex]->data;

    playerGroup = teamObjects[playerState->teamIndex ^ 1]->data;
    object = playerGroup->playerObjects[0];
    opposingTeamLanes[0] = GetNearestLane(object->trans.z);
    object = playerGroup->playerObjects[1];
    opposingTeamLanes[1] = GetNearestLane(object->trans.z);
    partnerLaneIndex = playerState2->currentLaneIndex;

    if (partnerLaneIndex == -1) {
        partnerLaneIndex = playerState2->targetLaneIndex;
    }
    if (gameState == 5) {
        if (playerState->playerActionState != 0 && playerState->playerActionState != 1 && playerState->playerActionState != 2) {
            decisionBlocked = 1;
        }
        if (playerState->aiTargetLane != -1 && partnerLaneIndex != -1
            && ((partnerLaneIndex <= playerState->currentLaneIndex && playerState->aiTargetLane <= partnerLaneIndex) || (partnerLaneIndex >= playerState->currentLaneIndex && playerState->aiTargetLane >= partnerLaneIndex))) {
            decisionBlocked = 1;
        }
        if (decisionBlocked) {
            playerState->aiTargetLane = -1;
            playerState->aiWaitTimer = 0;
            playerState->aiDecisionTimer = 0;
            playerState->aiState = 3;
            return;
        }
        switch (playerState->aiState) {
            case 0:
                if (playerState->playerActionState != 0 && playerState->playerActionState != 1 && playerState->playerActionState != 2) {
                    playerState->aiWaitTimer = 0;
                }
                if (--playerState->aiWaitTimer <= 0) {
                    playerState->aiWaitTimer = 0;
                    playerState->aiDecisionTimer = 0;
                    playerState->aiState = 3;
                    return;
                }
                break;
            case 1:
                if (playerState->aiTargetLane != -1) {
                    if (++playerState->aiDecisionTimer < 2.0f * REFRESH_RATE) {
                        if (playerState->currentLaneIndex == playerState->aiTargetLane) {
                            playerState->aiTargetLane = -1;
                            return;
                        }
                        if (playerState->currentLaneIndex < playerState->aiTargetLane) {
                            *stickY = -64.0f;
                            return;
                        }
                        *stickY = 64.0f;
                        return;
                    }
                }
                playerState->aiWaitTimer = 0;
                playerState->aiDecisionTimer = 0;
                playerState->aiState = 3;
                return;
            case 2:
                if (playerState->aiTargetLane != -1) {
                    if (++playerState->aiDecisionTimer < 2.0f * REFRESH_RATE && playerState->aiTargetLane != playerState2->aiTargetLane) {
                        if (playerState->currentLaneIndex == playerState->aiTargetLane) {
                            *buttonFlags |= 256;
                            playerState->aiTargetLane = -1;
                            playerState->aiDecisionTimer = 0;
                            playerState->aiWaitTimer = ((((REFRESH_RATE/7)+2)*5.0f) + (frand() % 15)) - (playerState->difficulty * 10);
                            playerState->aiState = 0;
                            return;
                        }
                        if (playerState->currentLaneIndex < playerState->aiTargetLane) {
                            *stickY = -64.0f;
                            return;
                        }
                        *stickY = 64.0f;
                        return;
                    }
                    playerState->aiTargetLane = -1;
                    playerState->aiDecisionTimer = 0;
                    return;
                }
                playerState->aiWaitTimer = 0;
                playerState->aiDecisionTimer = 0;
                playerState->aiState = 3;
                return;
            case 3:
                playerState->aiTargetLane = -1;
                if (partnerLaneIndex == -1) {
                    for (candidateIndex = 0; candidateIndex < 6; candidateCount++, candidateIndex++) {
                        candidateLanes[candidateCount] = candidateIndex;
                    }
                }
                else {
                    if (playerState->currentLaneIndex < partnerLaneIndex) {
                        for (candidateIndex = 0; candidateIndex < 3; candidateCount++, candidateIndex++) {
                            candidateLanes[candidateCount] = candidateIndex;
                        }
                    }
                    else {
                        for (candidateIndex = 3; candidateIndex < 6; candidateCount++, candidateIndex++) {
                            candidateLanes[candidateCount] = candidateIndex;
                        }
                    }
                    if (playerState->difficulty < playerState2->difficulty) {
                        for (candidateIndex = 0; candidateIndex < 6; candidateIndex++) {
                            if (candidateLanes[candidateIndex] == 3) {
                                candidateLanes[candidateIndex] = -1;
                            }
                        }
                    }
                }
                for (candidateIndex = 0, bestLaneScore = 256; candidateIndex < candidateCount; candidateIndex++) {
                    if (candidateLanes[candidateIndex] >= 0) {
                        candidateTeamIndex = GetLaneSide(candidateLanes[candidateIndex]);
                        if (candidateTeamIndex != playerState->teamIndex) {
                            laneChoiceScore = (frand() % 5) - playerState->difficulty;
                            if (laneChoiceScore < 0) {
                                laneChoiceScore = 0;
                            }
                            laneChoiceScore += (s32)fabs(playerState2->currentLaneIndex - playerState->aiTargetLane);
                            if (opposingTeamLanes[0] == candidateLanes[candidateIndex] || opposingTeamLanes[1] == candidateLanes[candidateIndex]) {
                                laneChoiceScore += playerState->difficulty * 2;
                            }
                            if (bestLaneScore > laneChoiceScore) {
                                playerState->aiTargetLane = candidateLanes[candidateIndex];
                                bestLaneScore = laneChoiceScore;
                            }
                        }
                    }
                }
                if (playerState->aiTargetLane == -1) {
                    if (partnerLaneIndex == -1) {
                        if (playerState->currentLaneIndex < 3) {
                            playerState->aiTargetLane = 1;
                        }
                        else {
                            playerState->aiTargetLane = 4;
                        }
                    }
                    else if (partnerLaneIndex > playerState->currentLaneIndex) {
                        playerState->aiTargetLane = 1;
                    }
                    else {
                        playerState->aiTargetLane = 4;
                    }
                    playerState->aiState = 1;
                }
                else {
                    playerState->aiState = 2;
                }
                playerState->aiWaitTimer = 0;
                playerState->aiDecisionTimer = 0;
                break;
        }
    }
}

void VibrateTeamControllers(s32 teamIndex)
{
    omVibrate(playerOrder[teamIndex * 2], 12, 6, 6);
    omVibrate(playerOrder[teamIndex * 2 + 1], 12, 6, 6);
}

void CreateLaneObjects(void)
{
    laneObjects = omAddObjEx(gameObjectManager, 64, 14, 0, -1, InitializeLaneObjects);
}

void InitializeLaneObjects(OMOBJ *object)
{
    Vec lanePosition;

    LaneObjectState *roundState;
    s32 laneIndex;
    LaneObjectState *roundState2;
    s32 hookIndex;
    HU3DMODEL *model;
    s32 animationDelayFrames;

    roundState = roundState2 = object->data = HuMemDirectMallocNum(HEAP_HEAP, 6 * sizeof(LaneObjectState), HU_MEMNUM_OVL);
    memset(roundState, 0, 6 * sizeof(LaneObjectState));
    laneAnimationTimer = 0;
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 2));
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 3));
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 20));
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 21));
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 11));
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 12));
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 13));
    object->mdlId[7] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 22));
    object->mdlId[8] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 14));
    object->mdlId[9] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 15));
    object->mdlId[10] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 16));
    object->mdlId[11] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 17));
    object->mdlId[12] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 18));
    object->mdlId[13] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 19));
    Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[6], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[8], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[9], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[11], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[13], HU3D_ATTR_DISPOFF);
    for (laneIndex = 0; laneIndex < 14; laneIndex++) {
        Hu3DModelLayerSet(object->mdlId[laneIndex], 2);
    }
    Hu3DModelShadowMapSet(object->mdlId[0]);
    Hu3DModelShadowMapSet(object->mdlId[1]);
    object->work[0] = 0;
    object->work[1] = 0;
    object->work[2] = 0;
    object->work[3] = 0;
    for (laneIndex = 0; laneIndex < 6; laneIndex++, roundState++) {
        roundState->laneIndex = laneIndex;
        roundState->crossingDirection = -1;
        roundState->laneSide = laneIndex & 1;
        roundState->crossingFrame = 0;
        if (laneIndex == 0) {
            roundState->shadowModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 23));
            Hu3DModelShadowMapSet(roundState->shadowModelId);
            roundState->upperLeftModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 5));
            roundState->upperRightModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 4));
            roundState->lowerLeftInnerModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 7));
            roundState->lowerLeftOuterModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 6));
            roundState->lowerRightOuterModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 8));
            roundState->lowerRightInnerModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 9));
        }
        else {
            roundState->shadowModelId = Hu3DModelLink(roundState2->shadowModelId);
            Hu3DModelShadowMapSet(roundState->shadowModelId);
            roundState->upperLeftModelId = Hu3DModelLink(roundState2->upperLeftModelId);
            roundState->upperRightModelId = Hu3DModelLink(roundState2->upperRightModelId);
            roundState->lowerLeftInnerModelId = Hu3DModelLink(roundState2->lowerLeftInnerModelId);
            roundState->lowerLeftOuterModelId = Hu3DModelLink(roundState2->lowerLeftOuterModelId);
            roundState->lowerRightOuterModelId = Hu3DModelLink(roundState2->lowerRightOuterModelId);
            roundState->lowerRightInnerModelId = Hu3DModelLink(roundState2->lowerRightInnerModelId);
        }
        Hu3DModelLayerSet(roundState->shadowModelId, 2);
        Hu3DModelLayerSet(roundState->upperLeftModelId, 4);
        Hu3DModelLayerSet(roundState->upperRightModelId, 4);
        Hu3DModelLayerSet(roundState->lowerLeftInnerModelId, 2);
        Hu3DModelLayerSet(roundState->lowerLeftOuterModelId, 2);
        Hu3DModelLayerSet(roundState->lowerRightOuterModelId, 2);
        Hu3DModelLayerSet(roundState->lowerRightInnerModelId, 2);
        roundState->animationHandle = -1;
        roundState->laneZPosition = -425.0f + (140.0f * laneIndex);
        Hu3DModelPosSet(roundState->shadowModelId, 0.0f, 50.0f, roundState->laneZPosition);
        Hu3DModelPosSet(roundState->upperLeftModelId, 0.0f, 50.0f, roundState->laneZPosition);
        Hu3DModelPosSet(roundState->upperRightModelId, 0.0f, 50.0f, roundState->laneZPosition);
        Hu3DModelPosSet(roundState->lowerLeftInnerModelId, -500.0f, 30.000002f, roundState->laneZPosition);
        Hu3DModelPosSet(roundState->lowerLeftOuterModelId, -500.0f, 50.0f, roundState->laneZPosition);
        Hu3DModelPosSet(roundState->lowerRightOuterModelId, 500.0f, 50.0f, roundState->laneZPosition);
        Hu3DModelPosSet(roundState->lowerRightInnerModelId, 500.0f, 30.000002f, roundState->laneZPosition);
        SetLaneSide(laneIndex, roundState->laneSide);
        lanePosition.x = 0.0f;
        lanePosition.y = 500.0f;
        lanePosition.z = roundState->laneZPosition;
        animationDelayFrames = 0.3f * (roundState->laneIndex * REFRESH_RATE);
        roundState->animationHandle = SpawnCrossingEffect(roundState->laneIndex, animationDelayFrames, &lanePosition);
    }
    particleSpriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M426, 1), HU_MEMNUM_OVL));
    for (laneIndex = 0; laneIndex < 2; laneIndex++) {
        hookIndex = Hu3DHookFuncCreate(DrawBillboardSprite);
        model = &Hu3DData[hookIndex];
        model->hookData = particleSpriteAnimation;
        Hu3DModelPosSetV(hookIndex, &scoreboardPositions[laneIndex]);
        Hu3DModelScaleSet(hookIndex, 4.0f, 4.0f, 4.0f);
        Hu3DModelLayerSet(hookIndex, 4);
    }
    object->objFunc = UpdateLaneObjects;
}

void UpdateLaneObjects(OMOBJ *object)
{
    float crossingProgress;
    float leftPieceHeight;
    float rightPieceHeight;

    LaneObjectState *roundState;
    HU3DLIGHT *light;
    s32 laneIndex;
    s32 crossingModelIndex;
    s32 lightIndex;
    s32 crossingState;
    s32 nextCrossingDirection;

    roundState = object->data;
    for (laneIndex = 0; laneIndex < 6; laneIndex++, roundState++) {
        if (roundState->crossingDirection != -1) {
            roundState->crossingFrame++;
            crossingProgress = 0.5f * roundState->crossingFrame;
            if (!roundState->crossingDirection) {
                rightPieceHeight = 30.000002f + (19.999998f * crossingProgress);
                leftPieceHeight = 50.0f + (-19.999998f * crossingProgress);
                Hu3DModelPosSet(roundState->lowerLeftOuterModelId, -500.0f, leftPieceHeight, roundState->laneZPosition);
                Hu3DModelPosSet(roundState->lowerRightInnerModelId, 500.0f, rightPieceHeight, roundState->laneZPosition);
            }
            else {
                rightPieceHeight = 50.0f + (-19.999998f * crossingProgress);
                leftPieceHeight = 30.000002f + (19.999998f * crossingProgress);
                Hu3DModelPosSet(roundState->lowerLeftInnerModelId, -500.0f, leftPieceHeight, roundState->laneZPosition);
                Hu3DModelPosSet(roundState->lowerRightOuterModelId, 500.0f, rightPieceHeight, roundState->laneZPosition);
            }
            if (roundState->crossingFrame >= 2) {
                if (roundState->crossingDirection == 0) {
                    HuAudFXPlay(1600);
                }
                else {
                    HuAudFXPlay(1599);
                }
                SetLaneSide(laneIndex, roundState->crossingDirection);
                roundState->crossingDirection = -1;
                roundState->crossingFrame = 0;
                Hu3DModelPosSet(roundState->lowerLeftInnerModelId, -500.0f, 30.000002f, roundState->laneZPosition);
                Hu3DModelPosSet(roundState->lowerLeftOuterModelId, -500.0f, 50.0f, roundState->laneZPosition);
                Hu3DModelPosSet(roundState->lowerRightOuterModelId, 500.0f, 50.0f, roundState->laneZPosition);
                Hu3DModelPosSet(roundState->lowerRightInnerModelId, 500.0f, 30.000002f, roundState->laneZPosition);
            }
        }
    }
    Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[6], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[7], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[8], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[9], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[10], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[11], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[12], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[13], HU3D_ATTR_DISPOFF);

    if (laneAnimationTimer) {
        if (++object->work[0] >= 20) {
            object->work[0] = 0;
        }
        if (++object->work[1] >= 15) {
            object->work[1] = 0;
        }
        if (++object->work[2] >= 20) {
            object->work[2] = 0;
        }
        laneAnimationTimer--;
    }
    else {
        if (object->work[0]) {
            if (++object->work[0] >= 20) {
                object->work[0] = 0;
            }
        }
        if (object->work[1]) {
            if (++object->work[1] >= 15) {
                object->work[1] = 0;
            }
        }
        if (object->work[2]) {
            if (++object->work[2] >= 20) {
                object->work[2] = 0;
            }
        }
    }
    if (++object->work[3] >= 20) {
        object->work[3] = 0;
    }
    Hu3DModelAttrReset(object->mdlId[(object->work[0] / 5) + 3], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[(object->work[1] / 5) + 7], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[(object->work[2] / 10) + 10], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[(object->work[3] / 10) + 12], HU3D_ATTR_DISPOFF);

    light = &Hu3DGlobalLight[globalLightId];
    crossingModelIndex = globalLightColor.r + (s32)(255.0f * globalLightFade);
    if (crossingModelIndex > 255) {
        crossingModelIndex = 255;
    }
    lightIndex = globalLightColor.g + (s32)(255.0f * globalLightFade);
    if (lightIndex > 255) {
        lightIndex = 255;
    }
    crossingState = globalLightColor.b + (s32)(255.0f * globalLightFade);
    if (crossingState > 255) {
        crossingState = 255;
    }
    nextCrossingDirection = globalLightColor.a + (s32)(255.0f * globalLightFade);
    if (nextCrossingDirection > 255) {
        nextCrossingDirection = 255;
    }

    light->color.r = crossingModelIndex;
    light->color.g = lightIndex;
    light->color.b = crossingState;
    light->color.a = nextCrossingDirection;

    if (globalLightFade > 0.0f) {
        if ((globalLightFade -= 0.05f) < 0.0f) {
            globalLightFade = 0.0f;
        }
    }
}

void DestroyLaneObjects(OMOBJ *object)
{
    DestroySpriteAnimation(particleSpriteAnimation);
    ClearObjectCallback(object);
}

void RemoveLaneObjects(void)
{
    OMOBJ *object;

    object = laneObjects;
    DestroyLaneObjects(object);
    laneObjects = NULL;
}

void StartLaneCrossing(s32 laneIndex, s32 crossingDirection)
{
    LaneObjectState *roundState;

    if (!laneObjects || laneIndex < 0 || laneIndex >= 6) {
        return;
    }
    roundState = laneObjects->data;
    roundState = &roundState[laneIndex];
    if (roundState->crossingDirection == -1) {
        if (roundState->laneSide != crossingDirection) {
            roundState->crossingDirection = crossingDirection;
        }
        roundState->crossingFrame = 0;
        return;
    }
    if (roundState->crossingDirection != crossingDirection) {
        roundState->crossingDirection = crossingDirection;
        roundState->crossingFrame = 0;
    }
}

void SetLaneSide(s32 laneIndex, s32 laneSide)
{
    LaneObjectState *roundState;

    if (!laneObjects || laneIndex < 0 || laneIndex >= 6) {
        return;
    }
    roundState = laneObjects->data;
    roundState = &roundState[laneIndex];
    roundState->laneSide = laneSide;
    switch (laneSide) {
        case 0:
            Hu3DModelAttrReset(roundState->upperLeftModelId, HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(roundState->lowerLeftInnerModelId, HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(roundState->lowerRightOuterModelId, HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(roundState->upperRightModelId, HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(roundState->lowerLeftOuterModelId, HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(roundState->lowerRightInnerModelId, HU3D_ATTR_DISPOFF);
            break;
        case 1:
            Hu3DModelAttrSet(roundState->upperLeftModelId, HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(roundState->lowerLeftInnerModelId, HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(roundState->lowerRightOuterModelId, HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(roundState->upperRightModelId, HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(roundState->lowerLeftOuterModelId, HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(roundState->lowerRightInnerModelId, HU3D_ATTR_DISPOFF);
            break;
        default:
            break;
    }
}

s32 GetLaneSide(s32 laneIndex)
{
    LaneObjectState *roundState;

    if (!laneObjects || laneIndex < 0 || laneIndex >= 6) {
        return 0;
    }
    roundState = laneObjects->data;
    roundState = &roundState[laneIndex];
    return roundState->laneSide;
}

s32 GetNearestLane(float worldZ)
{
    float nearestLaneDistance;

    LaneObjectState *roundState;
    s32 laneIndex;
    s32 nearestLaneIndex;

    if (!laneObjects) {
        return -1;
    }
    roundState = laneObjects->data;
    if (!laneObjects->data) {
        return -1;
    }
    nearestLaneDistance = fabs(roundState->laneZPosition - worldZ);
    nearestLaneIndex = 0;
    roundState++;
    for (laneIndex = 1; laneIndex < 6; laneIndex++, roundState++) {
        if (nearestLaneDistance > fabs(roundState->laneZPosition - worldZ)) {
            nearestLaneDistance = fabs(roundState->laneZPosition - worldZ);
            nearestLaneIndex = laneIndex;
        }
    }
    return nearestLaneIndex;
}

float GetLanePosition(s32 laneIndex)
{
    LaneObjectState *roundState;

    if (!laneObjects || laneIndex < 0 || laneIndex >= 6) {
        return 0.0f;
    }
    roundState = laneObjects->data;
    if (!laneObjects->data) {
        return 0.0f;
    }
    return roundState[laneIndex].laneZPosition;
}

s32 TryCrossLane(Vec *worldPosition, float facingAngle, s32 startCrossing)
{
    float laneDeltaX;
    float laneDeltaZ;
    float boardEdgeX;
    float ApproachAngle;
    float distanceToLane;
    float lanePositionZ;

    LaneObjectState *roundState;
    s32 laneIndex;

    if (!laneObjects) {
        return 0;
    }
    roundState = laneObjects->data;
    if (worldPosition->x < 0.0f) {
        boardEdgeX = -500.0f;
    }
    else {
        boardEdgeX = 500.0f;
    }
    for (laneIndex = 0; laneIndex < 6; laneIndex++, roundState++) {
        lanePositionZ = roundState->laneZPosition;
        laneDeltaX = boardEdgeX - worldPosition->x;
        laneDeltaZ = lanePositionZ - worldPosition->z;

        distanceToLane = sqrtf((laneDeltaX * laneDeltaX) + (laneDeltaZ * laneDeltaZ));
        ApproachAngle = atan2d(laneDeltaX, laneDeltaZ);
        if (distanceToLane < 150.0f && (fabs(GetAngleDifference(ApproachAngle, facingAngle)) < 30.0f)) {
            break;
        }
    }
    if (laneIndex >= 6) {
        return 0;
    }
    if (worldPosition->x < 0.0f) {
        if (!roundState->laneSide) {
            return 0;
        }
    }
    else if (roundState->laneSide == 1) {
        return 0;
    }
    if (startCrossing) {
        StartLaneCrossing(laneIndex, roundState->laneSide ^ 1);
    }
    return 1;
}

void CreateCrossingEffects(void)
{
    crossingEffectObject = omAddObjEx(gameObjectManager, 64, 0, 0, -1, InitializeCrossingEffects);
}

void InitializeCrossingEffects(OMOBJ *object)
{
    CrossingEffectState *effectState;
    CrossingEffectState *effectState2;
    s32 effectIndex;

    effectState = effectState2 = object->data = HuMemDirectMallocNum(HEAP_HEAP, 6 * sizeof(CrossingEffectState), HU_MEMNUM_OVL);
    memset(effectState, 0, 6 * sizeof(CrossingEffectState));
    for (effectIndex = 0; effectIndex < 6; effectIndex++, effectState++) {
        if (effectIndex == 0) {
            effectState->modelId = Hu3DModelCreateFile(3735579);
        }
        else {
            effectState->modelId = Hu3DModelLink(effectState2->modelId);
        }
        Hu3DModelShadowSet(effectState->modelId);
        Hu3DModelLayerSet(effectState->modelId, 2);
        if (effectIndex == 0) {
            effectState->idleMotionId = Hu3DJointMotionFile(effectState->modelId, DATA_MAKE_NUM(DATADIR_M426, 28));
            effectState->actionMotionId = Hu3DJointMotionFile(effectState->modelId, DATA_MAKE_NUM(DATADIR_M426, 29));
        }
        else {
            effectState->idleMotionId = effectState2->idleMotionId;
            effectState->actionMotionId = effectState2->actionMotionId;
        }
        effectState->displayLayer = effectState->previousDisplayLayer = 2;
        effectState->isHidden = 0;
        effectState->effectSlot = effectIndex;
        effectState->playerIndex = -1;
        effectState->effectState = 5;
        effectState->stateTimer = 0;
        effectState->stateFrame = 0;
        effectState->movementSpeed = 0.0f;
        effectState->positionX = effectState->positionY = effectState->positionZ = 0.0f;
        effectState->facingAngle = 0.0f;
        effectState->sideOffset = 0.0f;
        Hu3DModelAttrSet(effectState->modelId, HU3D_ATTR_DISPOFF);
        Hu3DModelScaleSet(effectState->modelId, 0.8f, 0.8f, 0.8f);
    }
    object->objFunc = UpdateCrossingEffects;
}

void UpdateCrossingEffects(OMOBJ *object)
{
    Vec effectPosition;

    float targetFacingAngle;

    CrossingEffectState *effectState;
    s32 effectIndex;
    s32 playerTeam;

    effectState = object->data;
    for (effectIndex = 0; effectIndex < 6; effectIndex++, effectState++) {
        playerTeam = GetLaneSide(effectState->playerIndex);
        switch (effectState->effectState) {
            case 0:
                if (roundStarted) {
                    if (--effectState->stateTimer <= 0) {
                        if (!effectState->isHidden) {
                            Hu3DModelAttrReset(effectState->modelId, HU3D_ATTR_DISPOFF);
                        }
                        Hu3DMotionSet(effectState->modelId, effectState->idleMotionId);
                        Hu3DModelAttrSet(effectState->modelId, HU3D_MOTATTR_LOOP);
                        HuAudFXPlay(1607);
                        effectState->stateTimer = 0;
                        effectState->effectState = 1;
                    }
                }
                break;
            case 1:
                if ((effectState->positionY -= ((60.0f/REFRESH_RATE)*50)) <= 50.0f) {
                    if (gameState <= 4) {
                        VibrateTeamControllers(0);
                        VibrateTeamControllers(1);
                    }
                    HuAudFXPlay(1598);
                    effectState->positionY = 50.0f;
                    effectState->stateFrame = 0;
                    effectState->effectState = 2;
                }
                break;
            case 2:
                if (crossingEffectsEnabled) {
                    effectState->stateFrame = 0;
                    effectState->effectState = 3;
                    Hu3DMotionShiftSet(effectState->modelId, effectState->actionMotionId, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                }
                else {
                    effectState->facingAngle = ApproachAngle(0.0f, effectState->facingAngle, 10.0f);
                }
                break;
            case 3:
                if (!playerTeam) {
                    targetFacingAngle = 90.0f;
                }
                else {
                    targetFacingAngle = 270.0f;
                }
                if (fabs(GetAngleDifference(targetFacingAngle, effectState->facingAngle)) < 5.0) {
                    effectState->facingAngle = targetFacingAngle;
                    if ((effectState->movementSpeed += 0.1f) > crossingEffectSpeed * (60.0f / REFRESH_RATE)) {
                        effectState->movementSpeed = crossingEffectSpeed * (60.0f / REFRESH_RATE);
                    }
                }
                else {
                    effectState->facingAngle = ApproachAngle(targetFacingAngle, effectState->facingAngle, 10.0f);
                    if ((effectState->movementSpeed -= 0.1f) < 0.0f) {
                        effectState->movementSpeed = 0.0f;
                    }
                }
                if (effectState->movementSpeed > 0.0f) {
                    effectState->positionX = effectState->positionX + (effectState->movementSpeed * sind(effectState->facingAngle));
                }
                Hu3DMotionSpeedSet(effectState->modelId, 1.4f * (0.33333334f * effectState->movementSpeed));
                if (crossingEffectsEnabled == 0) {
                    Hu3DMotionShiftSet(effectState->modelId, effectState->idleMotionId, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                    effectState->stateTimer = 0;
                    effectState->stateFrame = 0;
                    effectState->effectState = 2;
                }
                if (fabs(effectState->positionX) >= 400.0) {
                    effectState->movementSpeed = 0.0f;
                    if (effectState->positionX > 0.0f) {
                        effectState->positionX = 399.0f;
                    }
                    else {
                        effectState->positionX = -399.0f;
                    }
                    effectState->displayLayer = 0;
                    effectPosition.x = effectState->positionX;
                    effectPosition.y = effectState->positionY;
                    effectPosition.z = effectState->positionZ;
                    SpawnImpactEffect(&effectPosition);
                    if (effectState->positionX < 0.0f) {
                        teamScores[0]--;
                        VibrateTeamControllers(0);
                        HuAudFXPlay(1603);
                    }
                    else {
                        teamScores[1]--;
                        VibrateTeamControllers(1);
                        HuAudFXPlay(1604);
                    }
                    if (teamScores[0] < 0) {
                        teamScores[0] = 0;
                    }
                    if (teamScores[1] < 0) {
                        teamScores[1] = 0;
                    }
                    laneAnimationTimer = REFRESH_RATE;
                    globalLightFade = 1.0f;
                    effectState->stateFrame = 0;
                    effectState->effectState = 4;
                }
                break;
            case 4:
                if (++effectState->stateFrame >= 20) {
                    effectPosition.x = effectState->positionX;
                    effectPosition.y = effectState->positionY;
                    effectPosition.z = effectState->positionZ;

                    HitPlayersNearPosition(&effectPosition);
                    if (effectState->positionX < 0.0f) {
                        StartLaneCrossing(effectState->playerIndex, 0);
                    }
                    else {
                        StartLaneCrossing(effectState->playerIndex, 1);
                    }
                    effectState->effectState = 5;

                    if (playerTeam == 0) {
                        effectState->sideOffset += 30.000002f;
                    }
                    else {
                        effectState->sideOffset -= 30.000002f;
                    }
                    if (fabs(effectState->sideOffset) > 150.0) {
                        if (!playerTeam) {
                            effectState->sideOffset = 150.0f;
                        }
                        else {
                            effectState->sideOffset = -150.0f;
                        }
                    }
                    effectPosition.x = effectState->sideOffset;
                    effectPosition.y = 500.0f;
                    effectPosition.z = effectState->positionZ;
                    SpawnCrossingEffect(effectState->playerIndex, 0, &effectPosition);
                }
                break;
            case 5:
                break;
            default:
                break;
        }
        if (effectState->displayLayer != effectState->previousDisplayLayer) {
            if (effectState->displayLayer < 0 || effectState->displayLayer > 7) {
                effectState->displayLayer = 2;
            }
            Hu3DModelLayerSet(effectState->modelId, effectState->displayLayer);
            effectState->previousDisplayLayer = effectState->displayLayer;
        }
        if (effectState->effectState != 5 && effectState->effectState) {
            if (!effectState->isHidden) {
                Hu3DModelPosSet(effectState->modelId, effectState->positionX, effectState->positionY, effectState->positionZ);
                Hu3DModelRotSet(effectState->modelId, 0.0f, effectState->facingAngle, 0.0f);
            }
            else {
                Hu3DModelAttrSet(effectState->modelId, HU3D_ATTR_DISPOFF);
            }
        }
    }
}

void DestroyEffectObject(OMOBJ *object)
{
    ClearObjectCallback(object);
}

void RemoveCrossingEffects(void)
{
    DestroyEffectObject(crossingEffectObject);
    crossingEffectObject = NULL;
}

// unused
void SetCrossingEffectsHidden(s32 isHidden)
{
    CrossingEffectState *effectState;
    s32 previousHiddenState;
    s32 effectIndex;

    if (crossingEffectObject) {
        effectState = crossingEffectObject->data;
        for (effectIndex = 0; effectIndex < 6; effectIndex++, effectState++) {
            previousHiddenState = effectState->isHidden;
            effectState->isHidden = isHidden;
            if (previousHiddenState != isHidden && !isHidden) {
                Hu3DModelAttrReset(effectState->modelId, HU3D_ATTR_DISPOFF);
                Hu3DModelPosSet(effectState->modelId, effectState->positionX, effectState->positionY, effectState->positionZ);
                Hu3DModelRotSet(effectState->modelId, 0.0f, effectState->facingAngle, 0.0f);
            }
        }
    }
}

s32 SpawnCrossingEffect(s32 playerIndex, s32 delayFrames, Vec *spawnPosition)
{
    CrossingEffectState *effectState;
    s32 availableEffectIndex;

    if (!crossingEffectObject) {
        return -1;
    }
    effectState = crossingEffectObject->data;
    for (availableEffectIndex = 0; availableEffectIndex < 6; availableEffectIndex++, effectState++) {
        if (effectState->effectState == 5) {
            break;
        }
    }
    if (availableEffectIndex >= 6) {
        return -1;
    }
    effectState->playerIndex = playerIndex;
    effectState->effectState = 0;
    effectState->stateTimer = delayFrames;
    effectState->movementSpeed = 0.0f;
    effectState->positionX = spawnPosition->x;
    effectState->positionY = spawnPosition->y;
    effectState->positionZ = spawnPosition->z;
    effectState->facingAngle = 0.0f;
    effectState->displayLayer = 2;
    return availableEffectIndex;
}

s32 GetCrossingWinner(void)
{
    CrossingEffectState *effectState;
    s32 leftEffectCount;
    s32 rightEffectCount;
    s32 effectIndex;

    if (!crossingEffectObject) {
        return -1;
    }
    effectState = crossingEffectObject->data;
    for (effectIndex = 0, leftEffectCount = rightEffectCount = 0; effectIndex < 6; effectIndex++, effectState++) {
        if (effectState->effectState != 2 && effectState->effectState != 3) {
            break;
        }
        if (effectState->positionX < 0.0f) {
            leftEffectCount++;
        }
        else if (effectState->positionX > 0.0f) {
            rightEffectCount++;
        }
        else if (frand() & 1) {
            leftEffectCount++;
        }
        else {
            rightEffectCount++;
        }
    }
    if (effectIndex < 6) {
        return -1;
    }
    if (leftEffectCount > rightEffectCount) {
        return 1;
    }
    if (leftEffectCount < rightEffectCount) {
        return 0;
    }
    return 2;
}

void CreateParticleEffects(void)
{
    particleEffectObject = omAddObjEx(gameObjectManager, 0, 0, 0, -1, InitializeParticleEffects);
}

void InitializeParticleEffects(OMOBJ *object)
{
    ParticleEffectState *animationState;
    s32 effectIndex;
    ParticleEffectState *animationState2;
    ParticleModelData *modelHookData;
    HU3DMODEL *model;

    animationState2 = animationState = object->data = HuMemDirectMallocNum(HEAP_HEAP, 4 * sizeof(ParticleEffectState), HU_MEMNUM_OVL);
    memset(animationState, 0, 4 * sizeof(ParticleEffectState));
    for (effectIndex = 0; effectIndex < 4; effectIndex++, animationState++) {
        if (effectIndex == 0) {
            animationState->spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M426, 0), HU_MEMNUM_OVL));
            animationState->modelId = CreateParticleModel(animationState->spriteAnimation, 512);
        }
        else {
            animationState->spriteAnimation = animationState2->spriteAnimation;
            animationState->modelId = CloneParticleModel(animationState2->modelId);
        }
        Hu3DModelLayerSet(animationState->modelId, 5);
        model = &Hu3DData[animationState->modelId];
        modelHookData = model->hookData;
        modelHookData->blendMode = 0;
        Hu3DModelAttrSet(animationState->modelId, HU3D_ATTR_DISPOFF);
        animationState->effectSlot = effectIndex;
        animationState->isActive = 0;
        animationState->remainingFrames = 0;
        animationState->position.x = animationState->position.y = animationState->position.z = 0.0f;
        animationState->scale = 1.0f;
    }
    object->objFunc = UpdateParticleEffects;
}

void UpdateParticleEffects(OMOBJ *object)
{
    ParticleVertex *vertexData;
    ParticleEffectState *animationState;
    ParticleModelData *modelHookData;
    s32 effectIndex;
    s32 targetPlayerIndex;
    HU3DMODEL *model;
    s32 targetTeamIndex;

    animationState = object->data;
    if (animationState) {
        for (effectIndex = 0; effectIndex < 4; effectIndex++, animationState++) {
            if (!animationState->isActive) {
                continue;
            }
            if (animationState->remainingFrames <= 0) {
                Hu3DModelAttrSet(animationState->modelId, HU3D_ATTR_DISPOFF);
            }
            else {
                Hu3DModelAttrReset(animationState->modelId, HU3D_ATTR_DISPOFF);
                model = &Hu3DData[animationState->modelId];
                modelHookData = model->hookData;
                vertexData = modelHookData->particles;
                for (targetPlayerIndex = targetTeamIndex = 0; targetPlayerIndex < modelHookData->particleCount; targetPlayerIndex++, vertexData++) {
                    if (vertexData->alpha <= 0.0f) {
                        continue;
                    }
                    vertexData->position.x += vertexData->velocity.x;
                    vertexData->position.y += vertexData->velocity.y;
                    vertexData->position.z += vertexData->velocity.z;
                    targetTeamIndex++;
                    vertexData->animationFrame++;
                    vertexData->textureFrame = vertexData->animationFrame;
                    if (vertexData->textureFrame >= 16) {
                        vertexData->textureFrame = 0;
                        vertexData->alpha = 0.0f;
                        animationState->remainingFrames--;
                    }
                }
                Hu3DModelPosSet(animationState->modelId, animationState->position.x, animationState->position.y, animationState->position.z);
            }
        }
    }
}

void DestroyParticleEffects(OMOBJ *object)
{
    ParticleEffectState *animationState;

    if (object && object->data) {
        animationState = object->data;
        DestroySpriteAnimation(animationState->spriteAnimation);
        DestroyEffectObject(object);
    }
}

void RemoveParticleEffects(void)
{
    DestroyParticleEffects(particleEffectObject);
}

s32 AllocateParticleEffect(Vec *position)
{
    ParticleEffectState *animationState;
    s32 effectIndex;

    if (!particleEffectObject) {
        return -1;
    }
    animationState = particleEffectObject->data;
    if (!animationState) {
        return;
    }
    for (effectIndex = 0; effectIndex < 4; effectIndex++, animationState++) {
        if (!animationState->isActive) {
            break;
        }
    }
    if (effectIndex >= 4) {
        return -1;
    }
    animationState->isActive = 1;
    animationState->remainingFrames = 0;
    SetParticleEffectPosition(animationState->effectSlot, effectIndex);
    animationState->scale = 1.0f;
    return animationState->effectSlot;
}

void SetParticleEffectPosition(s32 effectIndex, Vec *position)
{
    ParticleEffectState *animationState;

    if (particleEffectObject) {
        animationState = particleEffectObject->data;
        if (!animationState) {
            return;
        }
        animationState = &animationState[effectIndex];
        if (animationState->isActive) {
            animationState->position.x = position->x;
            animationState->position.y = position->y;
            animationState->position.z = position->z;
            Hu3DModelPosSet(animationState->modelId, animationState->position.x, animationState->position.y, animationState->position.z);
        }
    }
}

s32 SpawnParticle(s32 effectIndex, Vec *position, Vec *velocity, float alpha, GXColor color)
{
    ParticleVertex *vertexData;
    ParticleEffectState *animationState;
    ParticleModelData *modelHookData;
    s32 effectIndex;
    HU3DMODEL *model;

    if (!particleEffectObject) {
        return -1;
    }

    animationState = particleEffectObject->data;
    if (!animationState) {
        return -1;
    }

    animationState = &animationState[effectIndex];
    if (!animationState->isActive) {
        return -1;
    }
    model = &Hu3DData[animationState->modelId];
    modelHookData = model->hookData;
    vertexData = modelHookData->particles;

    for (effectIndex = 0; effectIndex < modelHookData->particleCount; effectIndex++, vertexData++) {
        if (vertexData->alpha <= 0.0f) {
            break;
        }
    }
    if (effectIndex >= modelHookData->particleCount) {
        return -1;
    }
    vertexData->animationFrame = vertexData->reservedFlag = 0;

    if (position) {
        vertexData->position.x = position->x - animationState->position.x;
        vertexData->position.y = position->y - animationState->position.y;
        vertexData->position.z = position->z - animationState->position.z;
    }
    else {
        vertexData->position.x = animationState->position.x;
        vertexData->position.y = animationState->position.y;
        vertexData->position.z = animationState->position.z;
    }

    if (velocity) {
        vertexData->velocity.x = velocity->x;
        vertexData->velocity.y = velocity->y;
        vertexData->velocity.z = velocity->z;
    }
    else {
        vertexData->velocity.x = vertexData->velocity.y = vertexData->velocity.z = 0.0f;
    }

    vertexData->alpha = alpha;
    vertexData->color.r = color.r;
    vertexData->color.g = color.g;
    vertexData->color.b = color.b;
    vertexData->color.a = color.a;
    vertexData->rotation = 0.0f;
    vertexData->textureFrame = 0;
    vertexData->animationFrame = 0;
    animationState->remainingFrames++;

    return effectIndex;
}

void CreateImpactEffects(void)
{
    impactEffectObject = omAddObjEx(gameObjectManager, 0, 0, 0, -1, InitializeImpactEffects);
}

void InitializeImpactEffects(OMOBJ *object)
{
    HSFDATA *spawnPosition;

    ImpactEffectState *motionState;
    s32 effectIndex;
    HU3DMODEL *model;

    motionState = object->data = HuMemDirectMallocNum(HEAP_HEAP, 16 * sizeof(ImpactEffectState), HU_MEMNUM_OVL);
    memset(motionState, 0, 16 * sizeof(ImpactEffectState));
    for (effectIndex = 0; effectIndex < 16; effectIndex++, motionState++) {
        motionState->modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M426, 30));
        model = &Hu3DData[motionState->modelId];
        spawnPosition = model->hsf;
        Hu3DModelLayerSet(motionState->modelId, 5);
        Hu3DModelAttrSet(motionState->modelId, HU3D_ATTR_DISPOFF);
        motionState->isActive = 0;
        motionState->elapsedFrames = 0;
        motionState->positionX = motionState->positionY = motionState->positionZ = 0.0f;
        motionState->scale = 1.0f;
    }
    object->objFunc = UpdateImpactEffects;
}

void UpdateImpactEffects(OMOBJ *object)
{
    ImpactEffectState *motionState;
    s32 effectIndex;

    motionState = object->data;
    if (!motionState) {
        return;
    }
    for (effectIndex = 0; effectIndex < 16; effectIndex++, motionState++) {
        if (motionState->isActive) {
            Hu3DModelPosSet(motionState->modelId, motionState->positionX, motionState->positionY, motionState->positionZ);
            Hu3DModelRotSet(motionState->modelId, CRot.x, CRot.y, CRot.z);
            if (++motionState->elapsedFrames >= 2.0f * REFRESH_RATE) {
                Hu3DModelAttrSet(motionState->modelId, HU3D_ATTR_DISPOFF);
                motionState->isActive = 0;
                motionState->elapsedFrames = 0;
            }
        }
    }
}

void DestroyImpactEffects(OMOBJ *object)
{
    if (object && object->data) {
        DestroyEffectObject(object);
    }
}

void RemoveImpactEffects(void)
{
    DestroyImpactEffects(impactEffectObject);
}

s32 SpawnImpactEffect(Vec *position)
{
    ImpactEffectState *motionState;
    s32 effectIndex;

    if (!impactEffectObject) {
        return;
    }
    motionState = impactEffectObject->data;
    if (!motionState) {
        return;
    }
    for (effectIndex = 0; effectIndex < 16; effectIndex++, motionState++) {
        if (!motionState->isActive) {
            break;
        }
    }
    if (effectIndex >= 16) {
        return -1;
    }
    motionState->isActive = 1;
    motionState->elapsedFrames = 0;
    motionState->positionX = position->x;
    motionState->positionY = position->y;
    motionState->positionZ = position->z;
    Hu3DMotionTimeSet(motionState->modelId, 0.0f);
    Hu3DMotionSpeedSet(motionState->modelId, 0.5f);
    Hu3DModelAttrSet(motionState->modelId, HU3D_MOTATTR_NONE);
    Hu3DModelPosSet(motionState->modelId, motionState->positionX, motionState->positionY, motionState->positionZ);
    Hu3DModelScaleSet(motionState->modelId, 1.0f, 1.0f, 1.0f);
    Hu3DModelAttrReset(motionState->modelId, HU3D_ATTR_DISPOFF);
    return effectIndex;
}

void CreateScoreboards(void)
{
    scoreboardObject = omAddObjEx(gameObjectManager, 254, 0, 0, -1, InitializeScoreboards);
}

void InitializeScoreboards(OMOBJ *object)
{
    ScoreboardState *motionTable;
    s32 digitIndex;
    s32 digitIndex;

    motionTable = object->data = HuMemDirectMallocNum(HEAP_HEAP, 2 * sizeof(ScoreboardState), HU_MEMNUM_OVL);
    memset(motionTable, 0, 2 * sizeof(ScoreboardState));
    for (digitIndex = 0; digitIndex < 2; digitIndex++, motionTable++) {
        if (digitIndex == 0) {
            motionTable->scorePositionX = 100;
        }
        else {
            motionTable->scorePositionX = 490;
        }
        motionTable->scorePositionY = 65;
        motionTable->displayedScore = 10;
        motionTable->scaleAnimationFrames = 0;
        motionTable->digitSpacingX = 21;
        motionTable->digitSpacingY = 0;
        motionTable->isVisible = 1;
        for (digitIndex = 0; digitIndex < 6; digitIndex++) {
            motionTable->digitBasePositions[digitIndex][0] = scoreDigitOffsets[digitIndex];
            motionTable->digitBasePositions[digitIndex][1] = 0;
            switch (digitIndex) {
                case 0:
                case 1:
                    motionTable->digitSprites[digitIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 51), 64, 0);
                    motionTable->digitScaleOffsets[digitIndex][0] = motionTable->digitBasePositions[digitIndex][0] - motionTable->digitSpacingX;
                    motionTable->digitScaleOffsets[digitIndex][1] = motionTable->digitBasePositions[digitIndex][1] - motionTable->digitSpacingY;
                    break;
                case 2:
                    motionTable->digitSprites[digitIndex] = espEntry(scoreboardLabelResources[digitIndex], 255, 0);
                    break;
                case 3:
                    motionTable->digitSprites[digitIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 29), 256, 0);
                    espTPLvlSet(motionTable->digitSprites[digitIndex], 0.6f);
                    espColorSet(motionTable->digitSprites[digitIndex], 0, 0, 0);
                    break;
                case 4:
                    motionTable->digitSprites[digitIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 9), 128, 0);
                    break;
                case 5:
                    motionTable->digitSprites[digitIndex] = espEntry(DATA_MAKE_NUM(DATADIR_M426, 31), 128, 0);
                    espZRotSet(motionTable->digitSprites[digitIndex], 45.0f);
                    break;
                default:
                    break;
            }
            espDispOn(motionTable->digitSprites[digitIndex]);
            espDrawNoSet(motionTable->digitSprites[digitIndex], 0);
            espAttrSet(motionTable->digitSprites[digitIndex], HUSPR_ATTR_NOANIM);
            espPosSet(motionTable->digitSprites[digitIndex], motionTable->scorePositionX + motionTable->digitBasePositions[digitIndex][0], motionTable->scorePositionY + motionTable->digitBasePositions[digitIndex][1]);
        }
    }
    SetScoreboardsVisible(0);
    object->objFunc = UpdateScoreboards;
}

void UpdateScoreboards(OMOBJ *object)
{
    s32 displayedDigits[3];

    float digitScale;
    float digitPositionX;
    float digitPositionY;

    ScoreboardState *motionTable;
    s32 digitIndex;
    s32 scoreboardIndex;
    s32 scoreValue;

    motionTable = object->data;
    for (scoreboardIndex = 0; scoreboardIndex < 2; scoreboardIndex++, motionTable++) {
        if (!motionTable->isVisible) {
            continue;
        }
        if (motionTable->displayedScore != teamScores[scoreboardIndex]) {
            motionTable->displayedScore = teamScores[scoreboardIndex];
            motionTable->scaleAnimationFrames = 20;
        }
        if (motionTable->scaleAnimationFrames) {
            digitScale = 1.0 + (0.8f * sind((180.0f * (motionTable->scaleAnimationFrames / 20.0f))));
            motionTable->scaleAnimationFrames--;
        }
        else {
            digitScale = 1.0f;
        }
        scoreValue = teamScores[scoreboardIndex];
        if ((displayedDigits[0] = scoreValue / 10) > 9) {
            displayedDigits[0] = 9;
        }
        scoreValue -= displayedDigits[0] * 10;
        displayedDigits[1] = scoreValue;
        if (scoreValue > 9) {
            displayedDigits[1] = 9;
        }
        for (digitIndex = 0; digitIndex < 2; digitIndex++) {
            digitPositionX = (motionTable->scorePositionX + motionTable->digitSpacingX) + (motionTable->digitScaleOffsets[digitIndex][0] * digitScale);
            digitPositionY = (motionTable->scorePositionY + motionTable->digitSpacingY) + (motionTable->digitScaleOffsets[digitIndex][1] * digitScale);
            if (displayedDigits[digitIndex] < 0) {
                displayedDigits[digitIndex] = 0;
            }
            espPosSet(motionTable->digitSprites[digitIndex], digitPositionX, digitPositionY);
            espScaleSet(motionTable->digitSprites[digitIndex], digitScale, digitScale);
            espBankSet(motionTable->digitSprites[digitIndex], displayedDigits[digitIndex]);
        }
        for (digitIndex = 2; digitIndex < 6; digitIndex++) {
            digitPositionX = motionTable->scorePositionX + motionTable->digitBasePositions[digitIndex][0];
            digitPositionY = motionTable->scorePositionY + motionTable->digitBasePositions[digitIndex][1];
            espPosSet(motionTable->digitSprites[digitIndex], digitPositionX, digitPositionY);
            espScaleSet(motionTable->digitSprites[digitIndex], 1.0f, 1.0f);
        }
    }
}

void DestroyScoreboards(OMOBJ *object)
{
    ScoreboardState *motionTable;
    s32 spriteIndex;
    s32 scoreboardIndex;

    if (object && object->data) {
        motionTable = object->data;
        for (scoreboardIndex = 0; scoreboardIndex < 2; scoreboardIndex++, motionTable++) {
            for (spriteIndex = 0; spriteIndex < 6; spriteIndex++) {
                espKill(motionTable->digitSprites[spriteIndex]);
            }
        }
        DestroyEffectObject(object);
    }
}

void RemoveScoreboards(void)
{
    DestroyScoreboards(scoreboardObject);
}

void SetScoreboardsVisible(s32 isVisible)
{
    ScoreboardState *motionTable;
    s32 digitIndex;
    s32 digitIndex;

    motionTable = scoreboardObject->data;
    for (digitIndex = 0; digitIndex < 2; digitIndex++, motionTable++) {
        motionTable->isVisible = isVisible;
        for (digitIndex = 0; digitIndex < 6; digitIndex++) {
            if (isVisible) {
                espDispOn(motionTable->digitSprites[digitIndex]);
            }
            else {
                espDispOff(motionTable->digitSprites[digitIndex]);
            }
        }
    }
}

void InitializeCamera(void)
{
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 45.0f, 100.0f, 50000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    omAddObjEx(gameObjectManager, 32730, 0, 0, -1, omOutView);
    CRot.x = cameraSetupParameters[1][0];
    CRot.y = cameraSetupParameters[1][1];
    CRot.z = cameraSetupParameters[1][2];
    Center.x = cameraSetupParameters[1][3];
    Center.y = cameraSetupParameters[1][4];
    Center.z = cameraSetupParameters[1][5];
    CZoom = cameraSetupParameters[1][6];
    cameraControllerObject = omAddObjEx(gameObjectManager, 254, 0, 0, -1, InitializeCameraController);
}

void InitializeCameraController(OMOBJ *object)
{
    cameraUpdatesDisabled = 0;
    object->objFunc = UpdateCameraController;
}

void UpdateCameraController(OMOBJ *object) { }

void ResetJointState(JointState *jointState)
{
    jointState->jointState = 0;
}

void RemoveCameraController(void)
{
    OMOBJ *object;

    object = cameraControllerObject;
    object->objFunc = NULL;
    cameraControllerObject = NULL;
}

float GetAngleDifference(float targetAngle, float currentAngle)
{
    float angleDelta;

    if (targetAngle >= 360.0f) {
        targetAngle -= 360.0f;
    }
    else if (targetAngle < 0.0f) {
        targetAngle += 360.0f;
    }
    if (currentAngle >= 360.0f) {
        currentAngle -= 360.0f;
    }
    else if (currentAngle < 0.0f) {
        currentAngle += 360.0f;
    }
    angleDelta = targetAngle - currentAngle;
    if (angleDelta <= -180.0f) {
        angleDelta += 360.0f;
    }
    else if (angleDelta >= 180.0f) {
        angleDelta -= 360.0f;
    }
    return angleDelta;
}

float ApproachAngle(float targetAngle, float currentAngle, float maximumStep)
{
    float angleDelta;
    float rotationStep;

    if (targetAngle >= 360.0) {
        targetAngle -= 360.0;
    }
    else if (targetAngle < 0.0) {
        targetAngle += 360.0;
    }
    if (currentAngle >= 360.0) {
        currentAngle -= 360.0;
    }
    else if (currentAngle < 0.0) {
        currentAngle += 360.0;
    }
    rotationStep = 360.0 + (targetAngle - currentAngle);
    if (fabs(rotationStep) >= 360.0) {
        rotationStep = fmod(rotationStep, 360.0);
    }
    if (rotationStep < 180.0) {
        if (rotationStep <= maximumStep) {
            angleDelta = rotationStep;
        }
        else {
            angleDelta = maximumStep;
        }
    }
    else if ((360.0 - rotationStep) <= maximumStep) {
        angleDelta = -(360.0 - rotationStep);
    }
    else {
        angleDelta = -maximumStep;
    }
    angleDelta += currentAngle;
    if (angleDelta >= 360.0) {
        angleDelta -= 360.0;
        angleDelta = angleDelta;
    }
    else if (angleDelta < 0.0) {
        angleDelta += 360.0;
        angleDelta = angleDelta;
    }
    return angleDelta;
}

s16 CreateParticleModel(ANIMDATA *animationData, s16 particleCount)
{
    ParticleModelData *modelHookData;
    s16 animationFrame;
    Vec2f *textureCoordinate;
    ParticleVertex *vertexData;
    HU3DMODEL *model;
    Vec *position;
    void *modelIndex;
    s16 vertexIndex;
    void *particleIndex;

    vertexIndex = Hu3DHookFuncCreate(DrawParticleModel);
    model = &Hu3DData[vertexIndex];
    modelHookData = HuMemDirectMallocNum(HEAP_MODEL, sizeof(ParticleModelData), model->mallocNo);
    model->hookData = modelHookData;
    modelHookData->animationData = animationData;
    modelHookData->particleCount = particleCount;
    modelHookData->blendMode = 0;
    modelHookData->drawHook = NULL;
    modelHookData->relatedModel = NULL;
    modelHookData->animationFrame = 0.0f;
    modelHookData->reservedHookByte = 0;
    modelHookData->animationFrameLimit = 0;
    modelHookData->unreferencedInitialFrame = modelHookData->reservedParticleFlag = 0;
    vertexData = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(ParticleVertex), model->mallocNo);
    modelHookData->particles = vertexData;
    for (animationFrame = 0; animationFrame < particleCount; animationFrame++, vertexData++) {
        vertexData->alpha = 0.0f;
        vertexData->rotation = 0.0f;
        vertexData->position.x = ((frand() & 127) - 64) * 20;
        vertexData->position.y = ((frand() & 127) - 64) * 30;
        vertexData->position.z = ((frand() & 127) - 64) * 20;
        vertexData->color.r = vertexData->color.g = vertexData->color.b = vertexData->color.a = 255;
        vertexData->textureFrame = 0;
    }
    position = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(Vec) * 4, model->mallocNo);
    modelHookData->particlePositions = position;
    for (animationFrame = 0; animationFrame < particleCount * 4; animationFrame++, position++) {
        position->x = position->y = position->z = 0.0f;
    }
    textureCoordinate = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(Vec2f) * 4, model->mallocNo);
    modelHookData->particleTextureCoordinates = textureCoordinate;
    for (animationFrame = 0; animationFrame < particleCount; animationFrame++) {
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
    modelIndex = HuMemDirectMallocNum(HEAP_MODEL, 0x4000, model->mallocNo);
    particleIndex = modelIndex;
    DCFlushRange(modelIndex, 0x4000);
    GXBeginDisplayList(particleIndex, 0x4000);
    GXBegin(GX_QUADS, GX_VTXFMT0, particleCount * 4);
    for (animationFrame = 0; animationFrame < particleCount; animationFrame++) {
        GXPosition1x16(animationFrame * 4);
        GXColor1x16(animationFrame);
        GXTexCoord1x16(animationFrame * 4);

        GXPosition1x16(animationFrame * 4 + 1);
        GXColor1x16(animationFrame);
        GXTexCoord1x16(animationFrame * 4 + 1);

        GXPosition1x16(animationFrame * 4 + 2);
        GXColor1x16(animationFrame);
        GXTexCoord1x16(animationFrame * 4 + 2);

        GXPosition1x16(animationFrame * 4 + 3);
        GXColor1x16(animationFrame);
        GXTexCoord1x16(animationFrame * 4 + 3);
    }
    modelHookData->displayListSize = GXEndDisplayList();
    modelHookData->displayListBuffer = HuMemDirectMallocNum(HEAP_MODEL, modelHookData->displayListSize, model->mallocNo);
    memcpy(modelHookData->displayListBuffer, modelIndex, modelHookData->displayListSize);
    DCFlushRange(modelHookData->displayListBuffer, modelHookData->displayListSize);
    HuMemDirectFree(modelIndex);
    return vertexIndex;
}

s16 CloneParticleModel(s16 sourceModelId)
{
    ParticleModelData *modelHookData;
    Vec2f *textureCoordinate;
    ParticleVertex *vertexData;
    s16 modelIndex;
    HU3DMODEL *model;
    ParticleModelData *modelHookData2;
    Vec *position;
    s32 particleCount;
    HU3DMODEL *model2;
    s16 vertexIndex;

    model2 = &Hu3DData[sourceModelId];
    modelHookData2 = model2->hookData;
    vertexIndex = Hu3DHookFuncCreate(model2->hookFunc);
    model = &Hu3DData[vertexIndex];
    modelHookData = HuMemDirectMallocNum(HEAP_MODEL, sizeof(ParticleModelData), model->mallocNo);
    model->hookData = modelHookData;
    modelHookData->animationData = modelHookData2->animationData;
    modelHookData->particleCount = modelHookData2->particleCount;
    particleCount = modelHookData->particleCount;
    modelHookData->blendMode = 0;
    modelHookData->drawHook = NULL;
    modelHookData->relatedModel = NULL;
    modelHookData->animationFrame = 0;
    modelHookData->reservedHookByte = 0;
    modelHookData->animationFrameLimit = 0;
    modelHookData->unreferencedInitialFrame = modelHookData->reservedParticleFlag = 0;
    vertexData = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(ParticleVertex), model->mallocNo);
    modelHookData->particles = vertexData;
    for (modelIndex = 0; modelIndex < particleCount; modelIndex++, vertexData++) {
        vertexData->alpha = 0.0f;
        vertexData->rotation = 0.0f;
        vertexData->position.x = ((frand() & 127) - 64) * 20;
        vertexData->position.y = ((frand() & 127) - 64) * 30;
        vertexData->position.z = ((frand() & 127) - 64) * 20;
        vertexData->color.r = vertexData->color.g = vertexData->color.b = vertexData->color.a = 255;
        vertexData->textureFrame = 0;
    }

    position = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(Vec) * 4, model->mallocNo);
    modelHookData->particlePositions = position;
    for (modelIndex = 0; modelIndex < (4 * particleCount); modelIndex++, position++) {
        position->x = position->y = position->z = 0.0f;
    }
    textureCoordinate = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(Vec2f) * 4, model->mallocNo);
    modelHookData->particleTextureCoordinates = textureCoordinate;
    for (modelIndex = 0; modelIndex < particleCount; modelIndex++) {
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
    modelHookData->displayListSize = modelHookData2->displayListSize;
    modelHookData->displayListBuffer = modelHookData2->displayListBuffer;
    (void)modelHookData2;

    return vertexIndex;
}

void DrawParticleModel(HU3DMODEL *model, Mtx modelMatrix)
{
    Mtx inverseModelMatrix;
    Mtx rotatedModelMatrix;
    Mtx particleRotationMatrix;
    Vec scaledQuadVertices[4];
    Vec rotatedQuadVertices[4];
    Vec particleQuadOffsets[4];
    ROMtx reorderedModelMatrix;

    ParticleModelData *modelHookData;
    Vec *position;
    ParticleVertex *vertexData;
    Vec *position2;
    Vec *position3;
    Vec2f *textureCoordinate;
    s16 textureCoordinateIndex;
    s16 particleIndex;
    s16 textureFormat;
    s16 textureFrameX;
    s16 textureFrameY;
    ParticleModelDrawHook ParticleDrawCallback;

    modelHookData = model->hookData;
    if ((modelHookData->lastDrawFrame != GlobalCounter) || shadowModelDrawF) {
        if ((modelHookData->relatedModel) && (modelHookData->relatedModel != model)) {
            DrawParticleModel(modelHookData->relatedModel, model);
        }
        GXLoadPosMtxImm(modelMatrix, 0);
        GXSetNumTevStages(1);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 60, GX_FALSE, 125);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        if (shadowModelDrawF != 0) {
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ONE, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
            GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
        }
        else {
            textureFormat = modelHookData->animationData->bmp->dataFmt & 15;
            if (textureFormat == 7 || textureFormat == 8) {
                GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
            }
            else {
                GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
            }
            if (model->attr & 2) {
                GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
            }
            else {
                GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
            }
        }
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
        HuSprTexLoad(modelHookData->animationData, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
        GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
        GXSetZCompLoc(GX_FALSE);
        switch (modelHookData->blendMode) {
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
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GXSetArray(GX_VA_POS, modelHookData->particlePositions, 12);
        GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
        GXSetArray(GX_VA_CLR0, &modelHookData->particles->color, 68);
        GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
        GXSetArray(GX_VA_TEX0, modelHookData->particleTextureCoordinates, 8);
        if (!HmfInverseMtxF3X3(modelMatrix, inverseModelMatrix)) {
            MTXIdentity(inverseModelMatrix);
        }
        PSMTXReorder(inverseModelMatrix, reorderedModelMatrix);
        if (modelHookData->drawHook) {
            ParticleDrawCallback = modelHookData->drawHook;
            ParticleDrawCallback(model, modelHookData, modelMatrix);
        }
        vertexData = modelHookData->particles;
        position = modelHookData->particlePositions;
        textureCoordinate = modelHookData->particleTextureCoordinates;
        PSMTXROMultVecArray(reorderedModelMatrix, particleQuadVertices, &particleQuadOffsets[0], 4);
        for (particleIndex = 0; particleIndex < modelHookData->particleCount; particleIndex++, vertexData++) {
            if (!vertexData->alpha) {
                position->x = position->y = position->z = 0.0f;
                position++;
                position->x = position->y = position->z = 0.0f;
                position++;
                position->x = position->y = position->z = 0.0f;
                position++;
                position->x = position->y = position->z = 0.0f;
                position++;
            }
            else {
                if (!vertexData->rotation) {
                    position2 = scaledQuadVertices;
                    position3 = particleQuadOffsets;
                    VECScale(position3++, position2, vertexData->alpha);
                    VECAdd(position2++, &vertexData->position, position++);
                    VECScale(position3++, position2, vertexData->alpha);
                    VECAdd(position2++, &vertexData->position, position++);
                    VECScale(position3++, position2, vertexData->alpha);
                    VECAdd(position2++, &vertexData->position, position++);
                    VECScale(position3++, position2, vertexData->alpha);
                    VECAdd(position2++, &vertexData->position, position++);
                }
                else {
                    VECScale(&particleQuadVertices[0], &scaledQuadVertices[0], vertexData->alpha);
                    VECScale(&particleQuadVertices[1], &scaledQuadVertices[1], vertexData->alpha);
                    VECScale(&particleQuadVertices[2], &scaledQuadVertices[2], vertexData->alpha);
                    VECScale(&particleQuadVertices[3], &scaledQuadVertices[3], vertexData->alpha);
                    MTXRotRad(particleRotationMatrix, 90, vertexData->rotation);
                    MTXConcat(inverseModelMatrix, particleRotationMatrix, &rotatedModelMatrix[0]);
                    MTXMultVecArray(rotatedModelMatrix, &scaledQuadVertices[0], &rotatedQuadVertices[0], 4);
                    VECAdd(&rotatedQuadVertices[0], &vertexData->position, position++);
                    VECAdd(&rotatedQuadVertices[1], &vertexData->position, position++);
                    VECAdd(&rotatedQuadVertices[2], &vertexData->position, position++);
                    VECAdd(&rotatedQuadVertices[3], &vertexData->position, position++);
                }
                textureFrameX = vertexData->textureFrame & 3;
                textureFrameY = (vertexData->textureFrame >> 2) & 3;
                for (textureCoordinateIndex = 0; textureCoordinateIndex < 4; textureCoordinateIndex++, textureCoordinate++) {
                    textureCoordinate->x = (0.25f * textureFrameX) + particleTextureOffsets[textureCoordinateIndex].x;
                    textureCoordinate->y = (0.25f * textureFrameY) + particleTextureOffsets[textureCoordinateIndex].y;
                }
            }
        }
        DCFlushRangeNoSync(modelHookData->particlePositions, modelHookData->particleCount * sizeof(Vec) * 4);
        DCFlushRangeNoSync(modelHookData->particleTextureCoordinates, modelHookData->particleCount * sizeof(Vec2f) * 4);
        PPCSync();
        GXCallDisplayList(modelHookData->displayListBuffer, modelHookData->displayListSize);
        if (!shadowModelDrawF) {
            if ((modelHookData->reservedHookByte & 2) == 0) {
                modelHookData->animationFrame++;
            }
            if (modelHookData->animationFrameLimit && (modelHookData->animationFrameLimit <= modelHookData->animationFrame)) {
                if (modelHookData->reservedHookByte & 1) {
                    modelHookData->animationFrame = 0.0f;
                }
                modelHookData->animationFrame = modelHookData->animationFrameLimit;
            }
            modelHookData->lastDrawFrame = GlobalCounter;
        }
    }
}

void DrawBillboardSprite(HU3DMODEL *model, Mtx modelMatrix)
{
    float animationProgress;
    float spriteScale;
    float spriteRotation;
    float spriteAlpha;
    float spriteDepth;

    HU3DCAMERA *spriteIndex;
    ANIMDATA *animationData;
    s16 particleIndex;

    spriteIndex = Hu3DCamera;
    animationProgress = spriteIndex->pos.x - model->pos.x;
    spriteAlpha = spriteIndex->pos.y - model->pos.y;
    spriteScale = spriteIndex->pos.z - model->pos.z;
    spriteDepth = sqrtf((animationProgress * animationProgress) + (spriteScale * spriteScale));
    model->rot.x = (atan2d(spriteDepth, spriteAlpha)) - 90.0;
    model->rot.y = atan2d(animationProgress, spriteScale);
    animationData = model->hookData;
    GXLoadPosMtxImm(modelMatrix, 0);
    GXSetNumTevStages(1);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 60, GX_FALSE, 125);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    if (shadowModelDrawF) {
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ONE, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
        GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    }
    else {
        particleIndex = animationData->bmp->dataFmt & 15;
        if (particleIndex == 7 || particleIndex == 8) {
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
        }
        else {
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
        }
        GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    }
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    HuSprTexLoad(animationData, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZCompLoc(GX_FALSE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_POS, billboardSpriteVertices, 12);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, billboardSpriteColors, 4);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_TEX0, billboardSpriteTextureCoordinates, 8);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

    GXPosition1x16(0);
    GXColor1x16(0);
    GXTexCoord1x16(0);

    GXPosition1x16(1);
    GXColor1x16(0);
    GXTexCoord1x16(1);

    GXPosition1x16(2);
    GXColor1x16(0);
    GXTexCoord1x16(2);

    GXPosition1x16(3);
    GXColor1x16(0);
    GXTexCoord1x16(3);
}

void DestroySpriteAnimation(ANIMDATA *animationData)
{
    animationData->useNum = 0;
    HuSprAnimKill(animationData);
}

void DrawCameraOverlay(HU3DMODEL *model, Mtx modelMatrix)
{
    HU3DCAMERA *cameraModelIndex;

    Mtx viewMatrix;
    Mtx44 projectionMatrix;
    GXColor cameraState = { 255, 0, 0, 128 };

    cameraModelIndex = Hu3DCamera;
    GXSetScissor(cameraModelIndex->scissorX, cameraModelIndex->scissorY, cameraModelIndex->scissorW, cameraModelIndex->scissorH);
    GXSetViewport(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    C_MTXOrtho(projectionMatrix, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    GXSetProjection(projectionMatrix, GX_ORTHOGRAPHIC);
    GXSetTevColor(GX_TEVREG0, cameraState);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    MTXIdentity(viewMatrix);
    GXLoadPosMtxImm(viewMatrix, 0);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_TRUE);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_U8, 0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3u8(0, 0, 0);
    GXPosition3u8(1, 0, 0);
    GXPosition3u8(1, 1, 0);
    GXPosition3u8(0, 1, 0);
}
#pragma dontInline on
void UpdateModelNode(s32 modelIndex, ModelNode *node, s32 clearMask)
{
    switch (node->nodeType) {
        case 0:
            UpdateContainerModelNode(modelIndex, node, clearMask);
            return;
        case 1:
            UpdateSingleChildModelNode(modelIndex, node, clearMask);
            return;
        case 2:
            UpdateChildModelNodeTypeFive(modelIndex, node, clearMask);
            return;
        case 3:
            UpdateGroupModelNode(modelIndex, node, clearMask);
            return;
        case 4:
            UpdateChildModelNodeTypeFour(modelIndex, node, clearMask);
            return;
        case 5:
            UpdateRecursiveModelNode(modelIndex, node, clearMask);
            return;
        case 6:
            UpdateContainerModelNode(modelIndex, node, clearMask);
            return;
        default:
            UpdateContainerModelNode(modelIndex, node, clearMask);
            return;
    }
}
#pragma dontInline reset

void UpdateGroupModelNode(s32 modelIndex, ModelNode *node, s32 clearMask)
{
    ModelNode *animationNode;
    ModelNode *animationNode2;
    ModelNode *animationNode3;
    u32 firstChildIndex;
    u32 secondChildIndex;
    u32 thirdChildIndex;
    u32 fourthChildIndex;

    node->processedFlags &= ~clearMask;
    for (firstChildIndex = 0; firstChildIndex < node->childCount; firstChildIndex++) {
        animationNode = node->children[firstChildIndex];
        switch (animationNode->nodeType) {
            case 0:
                UpdateContainerModelNode(modelIndex, animationNode, node);
                break;
            case 1:
                UpdateSingleChildModelNode(modelIndex, animationNode, node);
                break;
            case 2:
                UpdateChildModelNodeTypeFive(modelIndex, animationNode, node);
                break;
            case 3:
                animationNode->processedFlags &= ~clearMask;
                for (secondChildIndex = 0; secondChildIndex < animationNode->childCount; secondChildIndex++) {
                    animationNode2 = animationNode->children[secondChildIndex];
                    switch (animationNode2->nodeType) {
                        case 0:
                            UpdateContainerModelNode(modelIndex, animationNode2, node);
                            break;
                        case 1:
                            UpdateSingleChildModelNode(modelIndex, animationNode2, node);
                            break;
                        case 2:
                            UpdateChildModelNodeTypeFive(modelIndex, animationNode2, node);
                            break;
                        case 3:
                            animationNode2->processedFlags &= ~clearMask;
                            for (thirdChildIndex = 0; thirdChildIndex < animationNode2->childCount; thirdChildIndex++) {
                                animationNode3 = animationNode2->children[thirdChildIndex];
                                switch (animationNode3->nodeType) {
                                    case 0:
                                        UpdateContainerModelNode(modelIndex, animationNode3, node);
                                        break;
                                    case 1:
                                        UpdateSingleChildModelNode(modelIndex, animationNode3, node);
                                        break;
                                    case 2:
                                        UpdateChildModelNodeTypeFive(modelIndex, animationNode3, node);
                                        break;
                                    case 3:
                                        animationNode3->processedFlags &= ~clearMask;
                                        for (fourthChildIndex = 0; fourthChildIndex < animationNode3->childCount; fourthChildIndex++) {
                                            UpdateModelNode(modelIndex, animationNode3->children[fourthChildIndex], node);
                                        }
                                        break;
                                    case 4:
                                        UpdateChildModelNodeTypeFour(modelIndex, animationNode3, node);
                                        break;
                                    case 5:
                                        UpdateRecursiveModelNode(modelIndex, animationNode3, node);
                                        break;
                                    case 6:
                                        UpdateContainerModelNode(modelIndex, animationNode3, node);
                                        break;
                                    default:
                                        UpdateContainerModelNode(modelIndex, animationNode3, node);
                                        break;
                                }
                            }
                            break;
                        case 4:
                            UpdateChildModelNodeTypeFour(modelIndex, animationNode2, node);
                            break;
                        case 5:
                            UpdateRecursiveModelNode(modelIndex, animationNode2, node);
                            break;
                        case 6:
                            UpdateContainerModelNode(modelIndex, animationNode2, node);
                            break;
                        default:
                            UpdateContainerModelNode(modelIndex, animationNode2, node);
                            break;
                    }
                }
                break;
            case 4:
                UpdateChildModelNodeTypeFour(modelIndex, animationNode, node);
                break;
            case 5:
                UpdateRecursiveModelNode(modelIndex, animationNode, node);
                break;
            case 6:
                UpdateContainerModelNode(modelIndex, animationNode, node);
                break;
            default:
                UpdateContainerModelNode(modelIndex, animationNode, node);
                break;
        }
    }
}

void UpdateContainerModelNode(s32 modelIndex, ModelNode *node, s32 clearMask)
{
    ModelNode *animationNode;
    u32 childIndex;
    u32 firstDescendantIndex;
    u32 secondDescendantIndex;
    u32 thirdDescendantIndex;
    u32 fallbackDescendantIndex;

    node->processedFlags &= ~clearMask;
    for (childIndex = 0; childIndex < node->childCount; childIndex++) {
        animationNode = node->children[childIndex];
        switch (animationNode->nodeType) {
            case 0:
                animationNode->processedFlags = animationNode->processedFlags & ~clearMask;
                for (firstDescendantIndex = 0; firstDescendantIndex < animationNode->childCount; firstDescendantIndex++) {
                    UpdateModelNode(modelIndex, animationNode->children[firstDescendantIndex], node);
                }
                break;
            case 1:
                UpdateSingleChildModelNode(modelIndex, animationNode, node);
                break;
            case 2:
                UpdateChildModelNodeTypeFive(modelIndex, animationNode, node);
                break;
            case 3:
                animationNode->processedFlags = animationNode->processedFlags & ~clearMask;
                for (secondDescendantIndex = 0; secondDescendantIndex < animationNode->childCount; secondDescendantIndex++) {
                    UpdateModelNode(modelIndex, animationNode->children[secondDescendantIndex], node);
                }
                break;
            case 4:
                UpdateChildModelNodeTypeFour(modelIndex, animationNode, node);
                break;
            case 5:
                UpdateRecursiveModelNode(modelIndex, animationNode, node);
                break;
            case 6:
                animationNode->processedFlags = animationNode->processedFlags & ~clearMask;
                for (thirdDescendantIndex = 0; thirdDescendantIndex < animationNode->childCount; thirdDescendantIndex++) {
                    UpdateModelNode(modelIndex, animationNode->children[thirdDescendantIndex], node);
                }
                break;
            default:
                animationNode->processedFlags = animationNode->processedFlags & ~clearMask;
                for (fallbackDescendantIndex = 0; fallbackDescendantIndex < animationNode->childCount; fallbackDescendantIndex++) {
                    UpdateModelNode(modelIndex, animationNode->children[fallbackDescendantIndex], node);
                }
                break;
        }
    }
}

void UpdateRecursiveModelNode(s32 modelIndex, ModelNode *node, s32 clearMask)
{
    u32 childIndex;

    node->processedFlags = node->processedFlags & ~clearMask;
    for (childIndex = 0; childIndex < node->childCount; childIndex++) {
        UpdateModelNode(modelIndex, node->children[childIndex], clearMask);
    }
}

void UpdateSingleChildModelNode(s32 modelIndex, ModelNode *node, s32 clearMask)
{
    node->processedFlags = node->processedFlags & ~clearMask;
    UpdateModelNode(modelIndex, node->singleChild, clearMask);
}

void UpdateChildModelNodeTypeFour(s32 modelIndex, ModelNode *node, s32 clearMask)
{
    u32 childIndex;

    node->processedFlags &= ~clearMask;
    for (childIndex = 0; childIndex < node->childCount; childIndex++) {
        UpdateModelNode(modelIndex, node->children[childIndex], clearMask);
    }
}

void UpdateChildModelNodeTypeFive(s32 modelIndex, ModelNode *node, s32 clearMask)
{
    u32 childIndex;

    node->processedFlags &= ~clearMask;
    for (childIndex = 0; childIndex < node->childCount; childIndex++) {
        UpdateModelNode(modelIndex, node->children[childIndex], clearMask);
    }
}

float InterpolateCameraParameter(float progress, float startValue, float controlValue, float endValue)
{
    float blendWeight;
    float interpolatedValue;

    blendWeight = 1.0 - progress;
    interpolatedValue = (endValue * (progress * progress)) + ((startValue * (blendWeight * blendWeight)) + (2.0 * (controlValue * (blendWeight * progress))));

    return interpolatedValue;
}

void SwapModelFaceAttributes(s16 modelId)
{
    FaceAttributeSet faceAttributeSwap;

    ModelFaceAttributes *faceAttribute;
    HSFBUFFER *modelBuffer;
    HU3DMODEL *model;
    HSFDATA *modelData;
    s32 faceIndex;
    s32 faceAttributeIndex;

    model = &Hu3DData[modelId];
    modelData = model->hsf;
    modelBuffer = modelData->face;
    for (faceIndex = 0; faceIndex < modelData->faceNum; faceIndex++, modelBuffer++) {
        faceAttribute = modelBuffer->data;
        for (faceAttributeIndex = 0; faceAttributeIndex < modelBuffer->count; faceAttributeIndex++, faceAttribute++) {
            switch (faceAttribute->faceCommandFlags & 7) {
                case 2:
                case 3:
                    faceAttributeSwap = faceAttribute->firstFaceAttributes;
                    faceAttribute->firstFaceAttributes = faceAttribute->secondFaceAttributes;
                    faceAttribute->secondFaceAttributes = faceAttributeSwap;
                    break;
                default:
                    break;
            }
        }
        DCStoreRangeNoSync(modelBuffer->data, modelBuffer->count * sizeof(ModelFaceAttributes));
    }
    HuMemDirectFreeNum(HEAP_MODEL, model->mallocNo);
    MakeDisplayList(modelId, model->mallocNo);
}
