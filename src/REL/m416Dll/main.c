#include "game/audio.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "ext_math.h"
#include <string.h>

#include "REL/m416Dll.h"

typedef struct CameraViewParams {
    float zoom;
    Vec pos;
    Vec rot;
} CameraViewParams;

CameraViewParams cameraViews[2] = { { 1676.6, { 0, 172, 123 }, { -18, 0, 0 } }, { 1200, { 0, 0, -600 }, { -24, 0, 0 } } };

GXColor whiteLightColor = { 255, 255, 255, 255 };
GXColor warmLightColor = { 255, 160, 96, 255 };

typedef struct GridCell {
    float gridX;
    float gridZ;
    u8 pathDistance;
    float nearbyOpponentInfluence;
} GridCell;

s32 activePlayerIndex;
s16 gameStateTimer;
OMOBJ *gameStateObject;
OMOBJ *cameraObject;
OMOBJ *outOfViewObject;
OMOBJ *teamPlayers[2][4];
s16 nextPlayerIndex;
s16 teamPlayerCounts[2];
s16 gameFinishTimer;
s16 gameFinishDelay;
s16 winningTeam;
float sceneFadeAlpha;
s16 globalLightIds[2];
s16 shadowEnabled;
ANIMDATA *playerSpriteAnimation;
GridCell gridCells[12][12];
s16 currentMessageSequence;
s16 currentTimerSequence;
s32 teamScoreValues[3];
s32 soundPlayedFlag;

void InitializeGameState(OMOBJ *object);
void UpdateGameState(OMOBJ *object);

void InitializePlayersAndGrid(HUPROCESS *objman);
void InitializeCamera(OMOBJ *object);

void ObjectSetup(void)
{
    HUPROCESS *objman;
    Vec pos;
    Vec up;
    Vec target;
    OSReport("******* M416ObjectSetup *********\n");
    objman = omInitObjMan(50, 8192);
    omGameSysInit(objman);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 30, 20, 25000, 1.2);
    Hu3DCameraViewportSet(1, 0, 0, 640, 480, 0, 1);
    outOfViewObject = omAddObjEx(objman, 32730, 0, 0, -1, omOutView);
    gameStateObject = omAddObjEx(objman, 0, 0, 0, -1, InitializeGameState);
    InitializePlayersAndGrid(objman);
    cameraObject = omAddObjEx(objman, 10, 0, 0, -1, InitializeCamera);
    pos.x = 0;
    pos.y = 3000;
    pos.z = 1;
    up.x = 0;
    up.y = 1;
    up.z = 0;
    target.x = 0;
    target.y = 0;
    target.z = 0;
    Hu3DShadowCreate(30, 20, 25000);
    Hu3DShadowTPLvlSet(0.9f);
    Hu3DShadowPosSet(&pos, &up, &target);
    Hu3DLightAllKill();
    globalLightIds[0] = Hu3DGLightCreate(-700, 1200, 700, 0, -0.5, -1, whiteLightColor.r, whiteLightColor.g, whiteLightColor.b);
    Hu3DGLightStaticSet(globalLightIds[0], 1);
    Hu3DGLightInfinitytSet(globalLightIds[0]);
    globalLightIds[1] = Hu3DGLightCreate(0, 0, 0, 0, 0, 0, 0, 0, 0);
    Hu3DGLightStaticSet(globalLightIds[1], 1);
    Hu3DGLightPointSet(globalLightIds[1], 0, 0, 3);
    shadowEnabled = 0;
    HuAudSndGrpSet(41);
    Hu3DBGColorSet(0, 0, 0);
    InitializeMap(objman);
}

void InitializeGameState(OMOBJ *object)
{
    gameFinishTimer = 30;
    gameFinishDelay = REFRESH_RATE;
    SetGameState(0);
    winningTeam = -1;
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    sceneFadeAlpha = 1;
    SetMapFadeAlpha(1);
    object->objFunc = UpdateGameState;
}

typedef struct PlayerTransitionWork {
    u8 collisionActive : 1;
    float collisionTargetX;
    float collisionRadius;
    float collisionTargetZ;
    s16 collisionTeamIndex;
} PlayerTransitionWork;

typedef struct PlayerWork {
    u8 collisionDetected : 1;
    u8 playerStunned : 1;
    u8 attachedModelActive : 1;
    u8 computerControlled : 1;
    u8 positionUpdateEnabled : 1;
    s16 playerIndex;
    s16 controllerPort;
    s8 stickX;
    s8 stickY;
    u16 buttonsHeld;
    u16 buttonsPressed;
    s16 characterId;
    s16 difficulty;
    s16 teamId;
    s16 teamIndex;
    s16 playerState;
    float movementSpeed;
    Vec velocity;
    s32 stateTimer;
    s32 targetIndex;
    u32 movementFlags;
    u32 motionAttributes;
    float motionSpeed;
    u32 animationFlags;
    s16 animationTimer;
    s16 animationState;
    s16 actionTimer;
    s16 actionState;
    OMOBJFUNC updateCallback;
    PlayerTransitionWork *transitionWork;
    s16 transitionFrame;
    Vec previousPosition;
    s16 previousPositionState;
    Vec targetPosition;
    s16 targetPositionState;
    Vec lastPosition;
    float rotationY;
    float rotationStartY;
} PlayerWork;

s32 audioSequenceId = -1;

void FinishGame(OMOBJ *object);
s32 ArePlayersStationary(void);
void AssignBlueTeamPositions(void);

void UpdateGameState(OMOBJ *object)
{
    PlayerWork *playerWork31;
    s32 index30;
    s16 index29;
    Vec position1C;
    Vec position10;
    s16 scalarValues8[4];

    switch (GetGameState()) {
        case 0:
            if (!WipeStatGet()) {
                HuAudFXPlay(1471);
                SetGameState(1);
                gameStateTimer = REFRESH_RATE / 2;
            }
            break;

        case 1:
            if (--gameStateTimer == 0) {
                SetGameState(2);
                gameStateTimer = REFRESH_RATE * 1.4;
            }
            break;

        case 2:
            if (gameStateTimer) {
                if (--gameStateTimer == 0) {
                    HuAudFXPlay(1470);
                }
            }
            else {
                if (sceneFadeAlpha > 0) {
                    sceneFadeAlpha -= 1.2f * REFRESH_FREQ;
                    if (sceneFadeAlpha < 0) {
                        sceneFadeAlpha = 0;
                    }
                    Hu3DGLightColorSet(
                        globalLightIds[0], whiteLightColor.r * sceneFadeAlpha, whiteLightColor.g * sceneFadeAlpha, whiteLightColor.b * sceneFadeAlpha, 255);
                }
                if (sceneFadeAlpha < 0.1f) {
                    if (soundPlayedFlag == 0) {
                        playerWork31 = teamPlayers[0][0]->data;
                        CharFXPlay(playerWork31->characterId, 296);
                        soundPlayedFlag = 1;
                    }
                }
                if (sceneFadeAlpha <= 0.0f) {
                    SetGameState(3);
                    gameStateTimer = REFRESH_RATE;
                }
            }
            break;

        case 3:
            if (--gameStateTimer == 0) {
                SetGameState(4);
                gameStateTimer = REFRESH_RATE;
                Hu3DGLightColorSet(globalLightIds[1], warmLightColor.r, warmLightColor.g, warmLightColor.b, warmLightColor.a);
                SetMapFadeAlpha(0.0625f);
                Hu3DShadowData.fov = 135;
                MTXLightPerspective(Hu3DShadowData.projMtx, Hu3DShadowData.fov, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
                shadowEnabled = 1;
                HuAudFXEmiterPlay(1464, &teamPlayers[0][0]->trans);
            }
            break;

        case 4:
            if (--gameStateTimer == 0) {
                SetGameState(5);
                currentMessageSequence = MGSeqStartCreate();
            }
            break;

        case 5:
            if (MGSeqStatGet(currentMessageSequence) == 0) {
                currentTimerSequence = MGSeqTimerCreate(30);
                SetGameState(6);
            }
            if (audioSequenceId < 0 && (MGSeqStatGet(currentMessageSequence) & 0x10)) {
                audioSequenceId = HuAudSeqPlay(73);
            }
            break;

        case 6:
            if (--gameFinishDelay == 0) {
                gameFinishTimer--;
                MGSeqParamSet(currentTimerSequence, 1, gameFinishTimer);
                gameFinishDelay = REFRESH_RATE;
            }
            if (gameFinishTimer == 0 || winningTeam >= 0) {
                currentMessageSequence = MGSeqFinishCreate();
                SetGameState(7);
                HuAudSeqAllFadeOut(100);
            }
            break;

        case 7:
            if (currentTimerSequence >= 0) {
                MGSeqParamSet(currentTimerSequence, 2, -1);
                currentTimerSequence = -1;
            }
            if (MGSeqStatGet(currentMessageSequence) == 0) {
                gameStateTimer = REFRESH_RATE;
                SetGameState(8);
            }
            break;

        case 8:
            if (gameStateTimer) {
                if (--gameStateTimer == 0) {
                    Hu3DShadowData.fov = 30;
                    MTXLightPerspective(Hu3DShadowData.projMtx, Hu3DShadowData.fov, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
                    shadowEnabled = 0;
                    position1C.x = 0;
                    position1C.y = 3000;
                    position1C.z = 1;
                    position10.x = 0;
                    position10.y = 0;
                    position10.z = 0;
                    Hu3DShadowPosSet(&position1C, &Hu3DShadowData.camUp, &position10);
                    HuAudFXPlay(1470);
                }
            }
            else {
                sceneFadeAlpha += 1.2f * REFRESH_FREQ;
                if (sceneFadeAlpha >= 1.0f) {
                    sceneFadeAlpha = 1.0f;
                    SetGameState(9);
                    gameStateTimer = REFRESH_RATE;
                    AssignBlueTeamPositions();
                }
                SetMapFadeAlpha(sceneFadeAlpha);
                Hu3DGLightColorSet(
                    globalLightIds[0], whiteLightColor.r * sceneFadeAlpha, whiteLightColor.g * sceneFadeAlpha, whiteLightColor.b * sceneFadeAlpha, 255);
                if (globalLightIds[1] >= 0) {
                    Hu3DGLightColorSet(globalLightIds[1], warmLightColor.r * (1.0f - sceneFadeAlpha), warmLightColor.g * (1.0f - sceneFadeAlpha),
                        warmLightColor.b * (1.0f - sceneFadeAlpha), warmLightColor.a * (1.0f - sceneFadeAlpha));
                }
            }
            break;

        case 9:
            if (ArePlayersStationary()) {
                if (--gameStateTimer == 0) {
                    SetGameState(10);
                    gameStateTimer = REFRESH_RATE * 3.5;
                    for (index30 = 0; index30 < 4; index30++) {
                        scalarValues8[index30] = -1;
                    }
                    index29 = 0;
                    if (winningTeam < 0) {
                        playerWork31 = teamPlayers[0][0]->data;
                        scalarValues8[index29++] = playerWork31->characterId;
                        GWPlayerCoinWinSet(playerWork31->playerIndex, 10);
                    }
                    else {
                        for (index30 = 0; index30 < 3; index30++) {
                            playerWork31 = teamPlayers[1][index30]->data;
                            scalarValues8[index29++] = playerWork31->characterId;
                            GWPlayerCoinWinSet(playerWork31->playerIndex, 10);
                        }
                    }
                    currentMessageSequence = MGSeqWinCreate(scalarValues8[0], scalarValues8[1], scalarValues8[2], scalarValues8[3]);
                    HuAudSStreamPlay(1);
                }
            }
            break;

        case 10:
            if (--gameStateTimer == 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                object->objFunc = FinishGame;
            }
            break;
    }
    if (omSysExitReq && !WipeStatGet()) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = FinishGame;
    }
    SetTrackedModelAmbient(sceneFadeAlpha);
}

void FinishGame(OMOBJ *object)
{
    if (!WipeStatGet()) {
        MGSeqKillAll();
        HuAudFadeOut(1);
        ShutdownMap();
        omOvlReturnEx(1, 1);
        OSReport("******* M416Exit *********\n");
    }
}

s32 GetGameState(void)
{
    return activePlayerIndex;
}

void SetGameState(s32 gameState)
{
    activePlayerIndex = gameState;
}

void CreatePlayer(OMOBJ *object);
void UpdatePlayerGroup(OMOBJ *object);

void InitializePlayersAndGrid(HUPROCESS *objman)
{
    s32 i;
    s32 j;

    nextPlayerIndex = 0;
    teamPlayerCounts[0] = teamPlayerCounts[1] = 0;
    omMakeGroupEx(objman, 0, 4);
    for (i = 0; i < 4; i++) {
        omAddObjEx(objman, 5, 5, 50, 0, CreatePlayer);
    }
    playerSpriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M416, 0x05));
    memset(gridCells, 0, sizeof(gridCells));
    for (i = 0; i < 10; i++) {
        for (j = 0; j < 10; j++) {
            gridCells[i + 1][j + 1].gridX = ((0.5f + (j - 5)) * 1100) / 9;
            gridCells[i + 1][j + 1].gridZ = ((0.5f + (i - 5)) * 1100) / 9;
        }
    }
    omAddObjEx(objman, 6, 0, 0, -1, UpdatePlayerGroup);
}

void ResetPlayer(OMOBJ *object);

void ResetPlayers(void)
{
    s32 i;
    OMOBJ **group;
    group = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    if (group == NULL) {
        return;
    }
    for (i = 0; i < 4; i++) {
        ResetPlayer(group[i]);
    }
}

s32 ArePlayersStationary(void)
{
    s32 i;
    OMOBJ **group;
    group = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    for (i = 0; i < 4; i++) {
        PlayerWork *work = group[i]->data;
        if (!work->computerControlled) {
            return 0;
        }
    }
    return 1;
}

void CreateRedTeamPlayer(OMOBJ *object);
void CreateBlueTeamPlayer(OMOBJ *object);
void UpdatePlayer(OMOBJ *object);

void UpdateBlueTeamParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

float teamStartPositions[4][4] = { 0, 0, -400, 0, -340, 0, 200, 155, 0, 0, 400, 180, 340, 0, 200, 205 };

void CreatePlayer(OMOBJ *object)
{
    PlayerWork *work;
    s32 player;
    work = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerWork), HU_MEMNUM_OVL);
    object->data = work;
    memset(work, 0, sizeof(PlayerWork));
    work->playerIndex = nextPlayerIndex++;
    work->controllerPort = GWPlayerCfg[work->playerIndex].pad_idx;
    work->characterId = GWPlayerCfg[work->playerIndex].character;
    work->difficulty = GWPlayerCfg[work->playerIndex].diff;
    work->teamId = GWPlayerCfg[work->playerIndex].group;
    object->mdlId[0] = CharModelCreate(work->characterId, 4);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(object->mdlId[0]);
    work->attachedModelActive = 1;
    work->positionUpdateEnabled = 1;
    work->stateTimer = 0;
    work->targetIndex = -1;
    work->motionAttributes = HU3D_MOTATTR_NONE;
    work->motionSpeed = 50;
    if (!work->teamId) {
        CreateRedTeamPlayer(object);
        teamPlayers[0][teamPlayerCounts[0]] = object;
        player = teamPlayerCounts[0];
        work->teamIndex = teamPlayerCounts[0];
        teamPlayerCounts[0]++;
    }
    else {
        CreateBlueTeamPlayer(object);
        teamPlayers[1][teamPlayerCounts[1]] = object;
        player = teamPlayerCounts[1] + 1;
        work->teamIndex = teamPlayerCounts[1];
        teamPlayerCounts[1]++;
    }
    Hu3DModelHookSet(object->mdlId[0], "test11_tex_we-itemhook-r", object->mdlId[1]);
    Hu3DModelShadowSet(object->mdlId[1]);
    Hu3DModelLayerSet(object->mdlId[0], 3);
    Hu3DModelLayerSet(object->mdlId[1], 3);
    TrackModelMaterials(object->mdlId[0]);
    TrackModelMaterials(object->mdlId[1]);
    Hu3DMotionSet(object->mdlId[0], object->mtnId[0]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    CharMotionNoSet(work->characterId, object->mtnId[1], DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02));
    CharMotionNoSet(work->characterId, object->mtnId[2], DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02));
    CharMotionDataClose(work->characterId);
    object->mdlId[4] = Hu3DParticleCreate(playerSpriteAnimation, 2);
    Hu3DParticleHookSet(object->mdlId[4], UpdateBlueTeamParticle);
    Hu3DParticleBlendModeSet(object->mdlId[4], 1);
    Hu3DModelLayerSet(object->mdlId[4], 6);
    Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF);
    object->trans.x = teamStartPositions[player][0];
    object->trans.y = teamStartPositions[player][1];
    object->trans.z = teamStartPositions[player][2];
    object->rot.y = teamStartPositions[player][3];
    object->rot.x = object->rot.z = 0;
    object->scale.x = object->scale.y = object->scale.z = 1;
    work->rotationStartY = object->rot.y;
    object->objFunc = UpdatePlayer;
}

u32 characterMotionIds[9][2] = {
    DATA_MAKE_NUM(DATADIR_M416, 0x08),
    1,
    DATA_MAKE_NUM(DATADIR_M416, 0x10),
    1,
    DATA_MAKE_NUM(DATADIR_M416, 0x18),
    1,
    DATA_MAKE_NUM(DATADIR_M416, 0x20),
    1,
    0,
    0,
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    0,
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17),
    0,
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18),
    0,
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02),
    0,
};

void UpdateRedTeamPlayer(OMOBJ *object);

void CreateRedTeamPlayer(OMOBJ *object)
{
    PlayerWork *work;
    s32 i;
    work = object->data;
    for (i = 0; i < 9; i++) {
        if (!characterMotionIds[i][1]) {
            object->mtnId[i] = CharMotionCreate(work->characterId, characterMotionIds[i][0]);
        }
        else {
            object->mtnId[i] = CharMotionCreate(work->characterId, work->characterId + characterMotionIds[i][0]);
        }
    }
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M416, 0x03));
    object->mdlId[2] = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M416, 0x06)), 100);
    Hu3DParticleHookSet(object->mdlId[2], UpdatePlayerParticle);
    Hu3DParticleBlendModeSet(object->mdlId[2], 1);
    Hu3DModelLayerSet(object->mdlId[2], 6);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_ZWRITE_OFF);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    work->actionTimer = 5;
    work->updateCallback = UpdateRedTeamPlayer;
}

void UpdateBlueTeamPlayer(OMOBJ *object);
void UpdateRedTeamParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void SetModelMotionFlags(s16 modelId, u32 motionFlags);

s32 characterIdleMotionIds[9] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x30), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x31), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x32),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x2D), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x2E), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02) };

void CreateBlueTeamPlayer(OMOBJ *object)
{
    PlayerWork *work;
    s32 i;
    work = object->data;
    for (i = 0; i < 9; i++) {
        object->mtnId[i] = CharMotionCreate(work->characterId, characterIdleMotionIds[i]);
    }
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M416, 0x04));
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M416, 0x02));
    Hu3DModelLayerSet(object->mdlId[2], 3);
    SetModelMotionFlags(object->mdlId[2], 16);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    Hu3DMotionSpeedSet(object->mdlId[2], 0);
    Hu3DAnimCreate(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M416, 0x07), HU_MEMNUM_OVL), object->mdlId[2], "sb00");
    object->mdlId[3] = Hu3DParticleCreate(playerSpriteAnimation, 50);
    Hu3DParticleHookSet(object->mdlId[3], UpdateRedTeamParticle);
    Hu3DModelLayerSet(object->mdlId[3], 3);
    Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
    work->transitionWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerTransitionWork), HU_MEMNUM_OVL);
    memset(work->transitionWork, 0, sizeof(PlayerTransitionWork));

    work->updateCallback = UpdateBlueTeamPlayer;
}

float ApproachAngle(float currentAngle, float targetAngle, float blendAmount)
{
    float scalarValue31;
    float scalarValue30;
    scalarValue31 = fmod(targetAngle - currentAngle, 360);
    if (scalarValue31 < 0.0f) {
        scalarValue31 += 360.0f;
    }
    if (scalarValue31 > 180.0f) {
        scalarValue31 -= 360.0f;
    }
    scalarValue30 = fmod(currentAngle + (blendAmount * scalarValue31), 360);
    if (scalarValue30 < 0.0f) {
        scalarValue30 += 360.0f;
    }
    return scalarValue30;
}

void UpdateComputerPlayer(OMOBJ *object);
s32 FindClearPath(OMOBJ *movingObject, Vec *targetPosition, Vec *adjustedPosition);

void UpdatePlayer(OMOBJ *object)
{
    float scalarValue31;
    float scalarValue30;
    float scalarValue28;
    float scalarValue27;
    float scalarValue23;
    PlayerWork *playerWork31;
    s32 index29;
    u32 index28;
    s32 index27;
    Vec position40;
    Vec position34;
    playerWork31 = object->data;
    index29 = playerWork31->stateTimer;
    index28 = playerWork31->movementFlags;
    switch (playerWork31->playerState) {
        case 0:
            if (GetGameState() >= 6) {
                if (GetGameState() == 7) {
                    playerWork31->playerState = 1;
                    playerWork31->animationTimer = REFRESH_RATE;
                    playerWork31->buttonsHeld = playerWork31->buttonsPressed = 0;
                    playerWork31->stickX = playerWork31->stickY = 0;
                }
                else {
                    if (!GWPlayerCfg[playerWork31->playerIndex].iscom) {
                        playerWork31->stickX = HuPadStkX[playerWork31->controllerPort];
                        playerWork31->stickY = HuPadStkY[playerWork31->controllerPort];
                        playerWork31->buttonsHeld = HuPadBtn[playerWork31->controllerPort];
                        playerWork31->buttonsPressed = HuPadBtnDown[playerWork31->controllerPort];
                    }
                    else {
                        playerWork31->buttonsHeld = playerWork31->buttonsPressed = 0;
                        playerWork31->stickX = playerWork31->stickY = 0;
                        UpdateComputerPlayer(object);
                    }
                    scalarValue31 = playerWork31->stickX;
                    scalarValue30 = -(float)playerWork31->stickY;
                    if (playerWork31->playerStunned) {
                        scalarValue31 = scalarValue30 = 0;
                    }
                    scalarValue28 = sqrtf((scalarValue31 * scalarValue31) + (scalarValue30 * scalarValue30));
                    if (scalarValue28 > 0) {
                        scalarValue31 /= scalarValue28;
                        scalarValue30 /= scalarValue28;
                        if (scalarValue28 > 72) {
                            scalarValue28 = 72;
                        }
                        playerWork31->movementSpeed = ((600.0f / REFRESH_RATE_F) * scalarValue28) / 72;
                        playerWork31->velocity.x = scalarValue31 * playerWork31->movementSpeed;
                        playerWork31->velocity.z = scalarValue30 * playerWork31->movementSpeed;
                        object->rot.y = ApproachAngle(object->rot.y, atan2d(playerWork31->velocity.x, playerWork31->velocity.z), 0.4f);
                        if (playerWork31->movementSpeed < (450.0f / REFRESH_RATE_F)) {
                            index29 = 1;
                        }
                        else {
                            index29 = 2;
                        }
                        index28 = HU3D_MOTATTR_LOOP;
                    }
                    else {
                        playerWork31->velocity.x = playerWork31->velocity.z = 0;
                        index29 = 0;
                        index28 = HU3D_MOTATTR_LOOP;
                    }
                    object->trans.x += playerWork31->velocity.x;
                    object->trans.z += playerWork31->velocity.z;
                }
            }
            break;

        case 1:
            if (GetGameState() == 9) {
                if (--playerWork31->animationTimer == 0) {
                    playerWork31->playerState = 2;
                    playerWork31->animationTimer = REFRESH_RATE * 5;
                    playerWork31->targetIndex = -1;
                }
            }
            if (!playerWork31->attachedModelActive) {
                index29 = 5;
            }
            else {
                index29 = 0;
            }
            index28 = HU3D_MOTATTR_LOOP;
            break;

        case 2:
            if (GetGameState() == 10) {
                playerWork31->playerState = 3;
            }
            else {
                VECSubtract(&playerWork31->targetPosition, &object->trans, &position34);
                scalarValue28 = VECMag(&position34);
                if (scalarValue28 < 20 || playerWork31->animationTimer == 0) {
                    index29 = 5;
                    index28 = HU3D_MOTATTR_LOOP;
                    object->rot.y = ApproachAngle(object->rot.y, 0, 0.2f);
                    playerWork31->computerControlled = 1;
                }
                else {
                    if (!playerWork31->computerControlled) {
                        index27 = FindClearPath(object, &playerWork31->targetPosition, &position40);
                    }
                    else {
                        position40.x = playerWork31->targetPosition.x;
                        position40.y = playerWork31->targetPosition.y;
                        position40.z = playerWork31->targetPosition.z;
                        index27 = 0;
                    }
                    VECSubtract(&position40, &object->trans, &position34);
                    object->rot.y = ApproachAngle(object->rot.y, atan2d(position34.x, position34.z), 0.2f);
                    if (!index27) {
                        if (scalarValue28 < 100.0f) {
                            scalarValue23 = (450.0f / REFRESH_RATE_F) * (scalarValue28 / 100.0f);
                        }
                        else {
                            scalarValue23 = (450.0f / REFRESH_RATE_F);
                        }
                    }
                    else {
                        scalarValue23 = (225.0f / REFRESH_RATE_F);
                    }
                    playerWork31->velocity.x = scalarValue23 * sind(object->rot.y);
                    playerWork31->velocity.z = scalarValue23 * cosd(object->rot.y);
                    object->trans.x += playerWork31->velocity.x;
                    object->trans.z += playerWork31->velocity.z;
                    index29 = 8;
                    index28 = HU3D_MOTATTR_LOOP;
                }
                if (playerWork31->animationTimer) {
                    playerWork31->animationTimer--;
                }
            }
            break;

        case 3:
            if (GetGameState() < 10) {
                index29 = 0;
                index28 = HU3D_MOTATTR_LOOP;
            }
            else {
                if ((!playerWork31->teamId && winningTeam < 0) || (playerWork31->teamId && winningTeam >= 0)) {
                    index29 = 6;
                }
                else {
                    index29 = 7;
                }
                index28 = HU3D_MOTATTR_NONE;
                object->rot.y = ApproachAngle(object->rot.y, 0, 0.2f);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
            }
            break;
    }
    if (playerWork31->targetIndex >= 0) {
        index29 = playerWork31->targetIndex;
        index28 = playerWork31->motionAttributes;
    }
    if ((index29 >= 0 && index29 != playerWork31->stateTimer) || index28 != playerWork31->movementFlags) {
        playerWork31->stateTimer = index29;
        playerWork31->movementFlags = index28;
        CharMotionShiftSet(playerWork31->characterId, object->mtnId[playerWork31->stateTimer], 0, 8, index28);
    }
}

void SeparateOverlappingPlayers(void);

void UpdatePlayerPosition(OMOBJ *object);

void UpdatePlayerGroup(OMOBJ *object)
{
    PlayerWork *playerWork31;
    s32 index30;
    OMOBJ *playerObject29;
    OMOBJ **playerObjects;
    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    SeparateOverlappingPlayers();
    for (index30 = 0; index30 < 4; index30++) {
        playerObject29 = playerObjects[index30];
        playerWork31 = playerObject29->data;
        if (playerWork31->updateCallback != NULL) {
            playerWork31->updateCallback(playerObject29);
        }
        if (playerWork31->positionUpdateEnabled) {
            UpdatePlayerPosition(playerObject29);
        }
    }
}

float animationBlendTimes[6] = { 0, 0.3, 0.7, 1.0, 1.3, 1.6 };

float teamLightSettings[6][2] = { 100.0f, 0.75f, 110.0f, 0.8f, 120.000008f, 0.85f, 130.0f, 0.9f, 140.0f, 0.95f, 150.0f, 0.99f };

void UpdateRedTeamPlayer(OMOBJ *object)
{
    Mtx transformMatrix50;
    Mtx transformMatrix20;
    Vec position14;
    Vec position8;
    PlayerWork *playerWork31;
    HU3DMODEL *model29;
    playerWork31 = object->data;
    if (playerWork31->actionTimer > 0 && playerWork31->collisionActive) {
        if (--playerWork31->actionTimer == 0) {
            winningTeam = 1;
            Hu3DGLightKill(globalLightIds[1]);
            globalLightIds[1] = -1;
            Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
            HuAudFXEmiterPlay(1469, &object->trans);
            CharFXPlay(playerWork31->characterId, 291);
        }
        else {
            HuAudFXEmiterPlay(1466, &object->trans);
            CharFXPlay(playerWork31->characterId, 296);
        }
        playerWork31->collisionActive = 0;
        playerWork31->playerStunned = 1;
        playerWork31->targetIndex = 3;
        playerWork31->motionAttributes = HU3D_MOTATTR_NONE;
        playerWork31->actionState = REFRESH_RATE;
        playerWork31->previousPositionState = REFRESH_RATE / 2;
        omVibrate(playerWork31->playerIndex, 12, 6, 6);
    }
    if (playerWork31->actionState > 0) {
        if (playerWork31->previousPositionState) {
            if (--playerWork31->previousPositionState == 0) {
                playerWork31->targetIndex = -1;
                playerWork31->playerStunned = 0;
            }
        }
        else {
            if ((playerWork31->actionState & 0x2) == 0) {
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
            }
            else {
                Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
            }
            playerWork31->actionState--;
        }
    }
    if (playerWork31->attachedModelActive) {
        Hu3DModelObjMtxGet(object->mdlId[0], "test11_tex_we-itemhook-r", transformMatrix50);
        MTXTrans(transformMatrix20, 0, 52, 0);
        MTXConcat(transformMatrix50, transformMatrix20, transformMatrix50);
        Hu3DModelPosSet(object->mdlId[2], transformMatrix50[0][3], transformMatrix50[1][3], transformMatrix50[2][3]);
        Hu3DModelScaleSet(object->mdlId[2], animationBlendTimes[playerWork31->actionTimer], animationBlendTimes[playerWork31->actionTimer], animationBlendTimes[playerWork31->actionTimer]);
    }
    if (GetGameState() >= 4 && GetGameState() <= 8 && globalLightIds[1] >= 0) {
        Hu3DGLightPointSet(
            globalLightIds[1], (15 * sind(object->work[0])) + teamLightSettings[playerWork31->actionTimer][0], teamLightSettings[playerWork31->actionTimer][1], 3);
        model29 = &Hu3DData[object->mdlId[2]];
        position8 = model29->pos;
        position14 = position8;
        position14.y = 800;
        position8.y = 0;
        position14.z += 1;
        Hu3DGLightPosSet(globalLightIds[1], model29->pos.x, 140, model29->pos.z, 0, 1, 0);
        if (shadowEnabled) {
            Hu3DShadowPosSet(&position14, &Hu3DShadowData.camUp, &position8);
        }
        object->work[0] += 10.0f;
        Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
    }
    else {
        Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    }
}

void UpdatePlayerCollision(OMOBJ *object);

void UpdateBlueTeamPlayer(OMOBJ *object)
{
    PlayerWork *playerWork30;
    PlayerTransitionWork *transitionWork29;
    HU3DPARTICLE *particle28;
    HU3DMODEL *model27;
    Mtx transformMatrix3C;
    Mtx transformMatrixC;
    playerWork30 = object->data;
    transitionWork29 = playerWork30->transitionWork;
    switch (playerWork30->transitionFrame) {
        case 0:
            if (playerWork30->buttonsPressed & PAD_BUTTON_A) {
                playerWork30->playerStunned = 1;
                playerWork30->transitionFrame = 1;
                playerWork30->previousPositionState = REFRESH_RATE / 4;
                playerWork30->targetIndex = 3;
                playerWork30->motionAttributes = HU3D_MOTATTR_NONE;
                HuAudFXEmiterPlay(1465, &object->trans);
            }
            break;

        case 1:
            if (--playerWork30->previousPositionState == 0) {
                Hu3DModelObjMtxGet(object->mdlId[0], "test11_tex_we-itemhook-r", transformMatrix3C);
                MTXTrans(transformMatrixC, 0, 2, 50);
                MTXConcat(transformMatrix3C, transformMatrixC, transformMatrix3C);
                Hu3DModelPosSet(object->mdlId[2], transformMatrix3C[0][3], transformMatrix3C[1][3], transformMatrix3C[2][3]);
                Hu3DModelScaleSet(object->mdlId[2], 0.5f, 0.5f, 0.5f);
                Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
                Hu3DMotionTimeSet(object->mdlId[2], 0);
                Hu3DMotionSpeedSet(object->mdlId[2], 2.0f);
                Hu3DModelRotSet(object->mdlId[2], 0, 180.0f + object->rot.y, 0);
                Hu3DData[object->mdlId[2]].motShapeWork.time = 0;
                Hu3DData[object->mdlId[2]].motShapeWork.stackValueEED = 2;
                Hu3DModelAttrReset(object->mdlId[3], HU3D_ATTR_DISPOFF);
                Hu3DModelPosSet(object->mdlId[3], transformMatrix3C[0][3], transformMatrix3C[1][3], transformMatrix3C[2][3]);
                model27 = &Hu3DData[object->mdlId[3]];
                particle28 = model27->hookData;
                particle28->dataCnt = 0;
                particle28->pos.x = transformMatrix3C[0][3];
                particle28->pos.y = transformMatrix3C[1][3];
                particle28->pos.z = transformMatrix3C[2][3];
                particle28->unk_10.x = object->rot.y;
#if VERSION_PAL
                particle28->prevCount = 30 - (sqrtf((2.0f * particle28->pos.y) / 1.2f) / 2.0f);
#else
                particle28->prevCount = 30 - (sqrtf((2.0f * particle28->pos.y) / 1.0000001f) / 2.0f);
#endif
                transitionWork29->collisionActive = 1;
                playerWork30->transitionFrame = 2;
                playerWork30->previousPositionState = REFRESH_RATE / 3;
            }
            break;

        case 2:
            if (--playerWork30->previousPositionState == 0) {
                playerWork30->targetIndex = 4;
                playerWork30->motionAttributes = HU3D_MOTATTR_NONE;
                transitionWork29->collisionActive = 0;
                playerWork30->transitionFrame = 3;
                playerWork30->previousPositionState = REFRESH_RATE / 2;
            }
            Hu3DModelObjMtxGet(object->mdlId[0], "test11_tex_we-itemhook-r", transformMatrix3C);
            MTXTrans(transformMatrixC, 0, 2, 50);
            MTXConcat(transformMatrix3C, transformMatrixC, transformMatrix3C);
            Hu3DModelPosSet(object->mdlId[2], transformMatrix3C[0][3], transformMatrix3C[1][3], transformMatrix3C[2][3]);
            Hu3DModelPosSet(object->mdlId[3], transformMatrix3C[0][3], transformMatrix3C[1][3], transformMatrix3C[2][3]);
            UpdatePlayerCollision(object);
            break;

        case 3:
            if (--playerWork30->previousPositionState == 0) {
                Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
                Hu3DData[object->mdlId[2]].motIdShape = -1;
                playerWork30->playerStunned = 0;
                playerWork30->targetIndex = -1;
                playerWork30->transitionFrame = 0;
            }
            break;
    }
    if (playerWork30->transitionFrame && GetGameState() > 6 && winningTeam < 0) {
        transitionWork29->collisionActive = 0;
        Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
    }
    if (transitionWork29->collisionActive) {
        transitionWork29->collisionTargetX = object->trans.x + (((float)transitionWork29->collisionTeamIndex * (200 * sind(object->rot.y))) / 4);
        transitionWork29->collisionRadius = 75;
        transitionWork29->collisionTargetZ = object->trans.z + (((float)transitionWork29->collisionTeamIndex * (200 * cosd(object->rot.y))) / 4);
        transitionWork29->collisionTargetX = object->trans.x + (200.0 * sind(object->rot.y));
        transitionWork29->collisionRadius = 120.00001f;
        transitionWork29->collisionTargetZ = object->trans.z + (200.0 * cosd(object->rot.y));
        if (transitionWork29->collisionTeamIndex < 4) {
            transitionWork29->collisionTeamIndex++;
        }
    }
}

void UpdatePlayerPosition(OMOBJ *object)
{
    PlayerWork *playerWork30;
    HU3DMODEL *model29;
    playerWork30 = object->data;
    if (playerWork30->attachedModelActive) {
        if (GetGameState() == 9) {
            playerWork30->attachedModelActive = 0;
            playerWork30->updateCallback = 0;
            playerWork30->previousPosition.x = 6 * sind(object->rot.y);
            playerWork30->previousPosition.y = 20;
            playerWork30->previousPosition.z = 6 * cosd(object->rot.y);
            playerWork30->previousPositionState = REFRESH_RATE / 2;
            model29 = &Hu3DData[object->mdlId[1]];
            model29->rot.y = object->rot.y;
            Hu3DModelObjPosGet(object->mdlId[0], "test11_tex_we-itemhook-r", &model29->pos);
            Hu3DModelHookReset(object->mdlId[0]);
            Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_HOOK);
            Hu3DModelShadowReset(object->mdlId[1]);
            Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
        }
    }
    else {
        model29 = &Hu3DData[object->mdlId[1]];
        model29->pos.x += playerWork30->previousPosition.x;
        model29->pos.y += playerWork30->previousPosition.y;
        model29->pos.z += playerWork30->previousPosition.z;
#if VERSION_PAL
        playerWork30->previousPosition.y += -1.2f;
#else
        playerWork30->previousPosition.y += -1.0000001f;
#endif
        model29->rot.x += 30.0f;
        if (--playerWork30->previousPositionState == 0) {
            Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
            Hu3DModelPosSet(object->mdlId[4], model29->pos.x, model29->pos.y, model29->pos.z);
            Hu3DModelAttrReset(object->mdlId[4], HU3D_ATTR_DISPOFF);
            if (!playerWork30->teamId) {
                HuAudFXPlay(1468);
            }
            playerWork30->positionUpdateEnabled = 0;
        }
    }
}

void UpdateRedTeamAi(OMOBJ *object);
void UpdateBlueTeamAi(OMOBJ *object);

void UpdateComputerPlayer(OMOBJ *object)
{
    PlayerWork *playerWork30;
    playerWork30 = object->data;
    if (!playerWork30->teamId) {
        UpdateRedTeamAi(object);
    }
    else {
        UpdateBlueTeamAi(object);
    }
}

typedef struct GridPosition {
    s16 gridX;
    s16 gridRow;
} GridPosition;

GridPosition gridNeighborOffsets[8] = { -1, -1, 0, -1, 1, -1, 1, 0, 1, 1, 0, 1, -1, 1, -1, 0 };

void UpdateRedTeamAi(OMOBJ *object)
{
    float scalarValue31;
    float scalarValue30;
    float scalarValue26;
    float scalarValue25;
    float scalarValue22;

    s32 index31;
    s32 index30;
    PlayerWork *playerWork29;
    s16 index27;
    s32 index26;
    s16 index25;
    s16 index24;
    u8 index23;

    GridPosition localData80[100];
    GridPosition localData58[10];
    GridPosition localData30[10];
    GridPosition localData20[4];

    playerWork29 = object->data;
    scalarValue25 = 1;
    localData20[3].gridX = 1 + (5 + ((9 * object->trans.x) / 1100));
    localData20[3].gridRow = 1 + (5 + ((9 * object->trans.z) / 1100));
    for (index31 = 1; index31 < 11; index31++) {
        for (index30 = 1; index30 < 11; index30++) {
            gridCells[index31][index30].pathDistance = 0;
            scalarValue22 = VECMagPoint(gridCells[index31][index30].gridX, gridCells[index31][index30].gridZ, 0);
            if (scalarValue22 <= 550) {
                gridCells[index31][index30].nearbyOpponentInfluence = 0;
                for (index26 = 0; index26 < teamPlayerCounts[1]; index26++) {
                    scalarValue31 = gridCells[index31][index30].gridX - teamPlayers[1][index26]->trans.x;
                    scalarValue30 = gridCells[index31][index30].gridZ - teamPlayers[1][index26]->trans.z;
                    scalarValue22 = VECMagPoint(scalarValue31, scalarValue30, 0);
                    if (scalarValue22 <= 1100) {
                        scalarValue26 = cosd(90 * (scalarValue22 / 1100));
                        scalarValue26 = scalarValue26 * scalarValue26;
                        if (scalarValue26 > gridCells[index31][index30].characterId) {
                            gridCells[index31][index30].characterId = scalarValue26;
                        }
                    }
                }
            }
            else {
                gridCells[index31][index30].nearbyOpponentInfluence = -1;
            }
        }
    }
    gridCells[localData20[3].gridRow][localData20[3].gridX].pathDistance = 1;
    localData80[0] = localData20[3];
    index27 = 0;
    index25 = 1;
    do {
        for (index31 = 0; index31 < 8u; index31++) {
            localData20[2].gridX = localData80[index27].gridX + gridNeighborOffsets[index31].gridX;
            localData20[2].gridRow = localData80[index27].gridRow + gridNeighborOffsets[index31].gridRow;
            if (localData20[2].gridX < 1 || localData20[2].gridX >= 11 || localData20[2].gridRow < 1 || localData20[2].gridRow >= 11) {
                continue;
            }
            if (gridCells[localData20[2].gridRow][localData20[2].gridX].pathDistance) {
                continue;
            }
            if (gridCells[localData20[2].gridRow][localData20[2].gridX].characterId >= 0) {
                gridCells[localData20[2].gridRow][localData20[2].gridX].pathDistance = gridCells[localData80[index27].gridRow][localData80[index27].gridX].pathDistance + 1;
                localData80[index25].gridX = localData20[2].gridX;
                localData80[index25].gridRow = localData20[2].gridRow;
                index25++;
            }
        }
        index27++;
    } while (index27 != index25);
    scalarValue25 = 1.0f;
    localData20[0].gridX = -1;
    for (index31 = 1; index31 < 11; index31++) {
        for (index30 = 1; index30 < 11; index30++) {
            if (gridCells[index31][index30].pathDistance == 0 || gridCells[index31][index30].pathDistance > 5) {
                continue;
            }
            if (gridCells[index31][index30].characterId >= 0.0f && gridCells[index31][index30].characterId < scalarValue25) {
                scalarValue25 = gridCells[index31][index30].characterId;
                localData20[0].gridX = index30;
                localData20[0].gridRow = index31;
            }
        }
    }
    index23 = gridCells[localData20[0].gridRow][localData20[0].gridX].pathDistance;
    if (index23 == 1) {
        if (gridCells[localData20[3].gridRow][localData20[3].gridX].characterId > 0.8f) {
            scalarValue25 = 1.0f;
            for (index31 = 0; index31 < 8u; index31++) {
                localData20[2].gridX = localData20[3].gridX + gridNeighborOffsets[index31].gridX;
                localData20[2].gridRow = localData20[3].gridRow + gridNeighborOffsets[index31].gridRow;
                if (gridCells[localData20[2].gridRow][localData20[2].gridX].characterId >= 0.0f && gridCells[localData20[2].gridRow][localData20[2].gridX].characterId < scalarValue25) {
                    scalarValue25 = gridCells[localData20[2].gridRow][localData20[2].gridX].characterId;
                    localData20[1] = localData20[2];
                }
            }
            localData30[1].gridX = localData20[1].gridX;
            localData30[1].gridRow = localData20[1].gridRow;
        }
        else {
            return;
        }
    }
    else {
        for (index31 = 1; index31 < 11; index31++) {
            for (index30 = 1; index30 < 11; index30++) {
                if (gridCells[index31][index30].pathDistance >= index23) {
                    gridCells[index31][index30].pathDistance = 0;
                }
            }
        }
        gridCells[localData20[0].gridRow][localData20[0].gridX].pathDistance = index23;
        localData80[0] = localData20[3];
        index24 = 1;
        scalarValue25 = 1.0f;
        do {
            localData20[1] = localData80[--index24];
            localData58[gridCells[localData20[1].gridRow][localData20[1].gridX].pathDistance - 1] = localData20[1];
            if (localData20[1].gridX == localData20[0].gridX && localData20[1].gridRow == localData20[0].gridRow) {
                scalarValue26 = 0;
                for (index31 = 1; index31 < index23; index31++) {
                    if (gridCells[localData58[index31].gridRow][localData58[index31].gridX].characterId > scalarValue26) {
                        scalarValue26 = gridCells[localData58[index31].gridRow][localData58[index31].gridX].characterId;
                    }
                }
                if (scalarValue26 < scalarValue25) {
                    scalarValue25 = scalarValue26;
                    memcpy(localData30, localData58, index23 * sizeof(GridPosition));
                }
                continue;
            }
            for (index31 = 0; index31 < 8u; index31++) {
                localData20[2].gridX = localData20[1].gridX + gridNeighborOffsets[index31].gridX;
                localData20[2].gridRow = localData20[1].gridRow + gridNeighborOffsets[index31].gridRow;
                if (gridCells[localData20[2].gridRow][localData20[2].gridX].pathDistance - 1 == gridCells[localData20[1].gridRow][localData20[1].gridX].pathDistance) {
                    localData80[index24++] = localData20[2];
                }
            }
        } while (index24);
    }
    if (localData20[0].gridX >= 0) {
        scalarValue31 = gridCells[localData30[1].gridRow][localData30[1].gridX].gridX - object->trans.x;
        scalarValue30 = gridCells[localData30[1].gridRow][localData30[1].gridX].gridZ - object->trans.z;
        scalarValue22 = VECMagPoint(scalarValue31, scalarValue30, 0);
        if (scalarValue22 >= 50.0f) {
            playerWork29->rotationStartY = ApproachAngle(playerWork29->rotationStartY, atan2d(scalarValue31, scalarValue30), 0.1f);
            playerWork29->stickX = (69.0f + playerWork29->difficulty) * sind(playerWork29->rotationStartY);
            playerWork29->stickY = (69.0f + playerWork29->difficulty) * -cosd(playerWork29->rotationStartY);
        }
    }
}

void UpdateBlueTeamAi(OMOBJ *object)
{
    float scalarValue31;
    float scalarValue28;
    float scalarValue27;
    Vec position2C;
    Vec position20;
    Vec position14;
    Vec position8;

    PlayerWork *playerWork31;
    OMOBJ *playerObject30;
    PlayerWork *playerWork28;
    playerObject30 = teamPlayers[0][0];
    playerWork31 = object->data;
    playerWork28 = playerObject30->data;
    if (playerWork31->playerStunned) {
        return;
    }
    switch (playerWork31->targetPositionState) {
        case 0:
            scalarValue28 = (frand16() * 360) >> 16;
            scalarValue31 = (frand16() * 550) >> 16;
            playerWork31->lastPosition.x = scalarValue31 * sind(scalarValue28);
            playerWork31->lastPosition.y = 0;
            playerWork31->lastPosition.z = scalarValue31 * cosd(scalarValue28);
            playerWork31->targetPositionState = 1;
            playerWork31->rotationY = 0;

        case 1:
            position20.x = playerObject30->trans.x + ((REFRESH_RATE_F * playerWork28->velocity.x) / 4.0f);
            position20.z = playerObject30->trans.z + ((REFRESH_RATE_F * playerWork28->velocity.z) / 4.0f);
            position2C.x = playerWork31->lastPosition.x + (playerWork31->rotationY * (position20.x - playerWork31->lastPosition.x));
            position2C.y = 0;
            position2C.z = playerWork31->lastPosition.z + (playerWork31->rotationY * (position20.z - playerWork31->lastPosition.z));
            VECSubtract(&position2C, &object->trans, &position14);
            scalarValue31 = VECMag(&position14);
            if (scalarValue31 > 0.0f) {
                if (playerWork31->rotationY < 1.0f) {
                    if (scalarValue31 < 10.0f) {
                        playerWork31->rotationY += (10.0f / scalarValue31);
                    }
                    else {
                        playerWork31->rotationY += (2.0f / scalarValue31);
                    }
                    if (playerWork31->rotationY > 1.0f) {
                        playerWork31->rotationY = 1.0f;
                    }
                }
                else {
                    if (((s32)frand() & 0x1F) == 0) {
                        playerWork31->targetPositionState = 0;
                    }
                }
            }
            scalarValue27 = 60.0f + (2.0f * playerWork31->difficulty);
            playerWork31->rotationStartY = ApproachAngle(playerWork31->rotationStartY, atan2d(position14.x, position14.z), 0.1f);
            playerWork31->stickX = scalarValue27 * sind(playerWork31->rotationStartY);
            playerWork31->stickY = scalarValue27 * -cosd(playerWork31->rotationStartY);
            VECSubtract(&playerObject30->trans, &object->trans, &position14);
            if (VECMag(&position14) < 200.0f) {
                VECNormalize(&position14, &position14);
                position8.x = sind(object->rot.y);
                position8.y = 0;
                position8.z = cosd(object->rot.y);
                if (VECDotProduct(&position14, &position8) >= 0.8f) {
                    playerWork31->buttonsPressed |= PAD_BUTTON_A;
                    playerWork31->targetPositionState = 0;
                }
            }
            if (((s32)frand() & 0x3FF) < 4 - playerWork31->difficulty) {
                playerWork31->buttonsPressed |= PAD_BUTTON_A;
                playerWork31->targetPositionState = 0;
            }
            break;
    }
}

s32 ResolvePlayerOverlaps(void);

void SeparateOverlappingPlayers(void)
{
    s32 i = 0;
    OMOBJ **playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    while (ResolvePlayerOverlaps() && i < 50) {
        i++;
    }
}

s32 ResolvePlayerOverlaps(void)
{
    Vec position14[4];
    Vec position8;
    float scalarValue31;
    float scalarValue30;
    s32 index31;
    OMOBJ *playerObject30;
    s32 index29;
    OMOBJ **playerObjects;
    s32 value27;
    OMOBJ *playerObject26;
    OMOBJ *playerObject25;
    value27 = 0;
    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    for (index31 = 0; index31 < 4; index31++) {
        position14[index31].x = position14[index31].y = position14[index31].z = 0;
    }
    for (index31 = 0; index31 < 3; index31++) {
        playerObject26 = playerObjects[index31];
        for (index29 = index31 + 1; index29 < 4; index29++) {
            playerObject25 = playerObjects[index29];
            VECSubtract(&playerObject26->trans, &playerObject25->trans, &position8);
            scalarValue31 = VECMag(&position8);
            if (scalarValue31 < 100.0) {
                VECNormalize(&position8, &position8);
                scalarValue30 = 0.0001f + (0.5f * (100.0 - scalarValue31));
                VECScale(&position8, &position8, scalarValue30);
                VECAdd(&position14[index31], &position8, &position14[index31]);
                VECSubtract(&position14[index29], &position8, &position14[index29]);
                value27++;
            }
        }
    }
    for (index31 = 0; index31 < 4; index31++) {
        playerObject30 = playerObjects[index31];
        VECAdd(&playerObject30->trans, &position14[index31], &playerObject30->trans);
        position8.x = playerObject30->trans.x;
        position8.y = 0;
        position8.z = playerObject30->trans.z;
        scalarValue31 = VECMag(&position8);
        if (scalarValue31 > 500.0) {
            VECNormalize(&position8, &position8);
            playerObject30->trans.x = 500.0 * position8.x;
            playerObject30->trans.z = 500.0 * position8.z;
        }
    }
    return value27;
}

Vec redTeamTargetPosition = { 0, 0, 200 };
Vec blueTeamTargetPositions[3] = { { -300, 0, -200 }, { 0, 0, -200 }, { 300, 0, -200 } };
s32 blueTeamFormationOrders[6][3] = { { 0, 1, 2 }, { 0, 2, 1 }, { 1, 0, 2 }, { 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 } };

void AssignBlueTeamPositions(void)
{
    float scalarValues14[3];
    Vec position8;
    float scalarValue31;
    float scalarValue30;
    float scalarValue29;
    float scalarValue28;

    s32 index31;
    s32 index30;
    PlayerWork *playerWork29;
    s32 index28;
    playerWork29 = teamPlayers[0][0]->data;
    playerWork29->targetPosition = redTeamTargetPosition;
    scalarValue29 = -1;
    for (index30 = 0; index30 < 6; index30++) {
        scalarValue31 = 0;
        for (index31 = 0; index31 < 3; index31++) {
            VECSubtract(&blueTeamTargetPositions[index31], &teamPlayers[1][blueTeamFormationOrders[index30][index31]]->trans, &position8);
            scalarValues14[index31] = VECMag(&position8);
            scalarValue31 += scalarValues14[index31];
        }
        scalarValue28 = scalarValue31 / 3;
        scalarValue30 = 0;
        for (index31 = 0; index31 < 3; index31++) {
            scalarValue30 += fabs(scalarValues14[index31] - scalarValue28);
        }
        if (scalarValue29 < 0 || scalarValue30 + scalarValue31 < scalarValue29) {
            scalarValue29 = scalarValue30 + scalarValue31;
            index28 = index30;
        }
    }
    for (index30 = 0; index30 < 3; index30++) {
        playerWork29 = teamPlayers[1][blueTeamFormationOrders[index28][index30]]->data;
        playerWork29->targetPosition = blueTeamTargetPositions[index30];
    }
}

typedef struct PathSearchNode {
    s32 playerSlot;
    float distanceAlongPath;
    float signedLateralDistance;
    Vec projectedPosition;
} PathSearchNode;

s32 FindClearPath(OMOBJ *movingObject, Vec *targetPosition, Vec *adjustedPosition)
{
    float scalarValue31;
    float scalarValue30;
    float scalarValue29;
    float scalarValue28;

    s32 index30;
    s32 index29;
    OMOBJ *playerObject28;
    s32 index27;

    OMOBJ **playerObjects;
    s32 value23;

    PathSearchNode localDataB4[4];
    Mtx transformMatrix84;
    Vec position78;
    Vec position6C;
    Vec position60;
    Vec position54;
    Vec position48;
    Vec position3C;
    Vec position30;
    Vec position24;
    s32 scalarValues14[4];
    float scalarValuesC[2];
    PlayerWork *movingPlayerWork;
    movingPlayerWork = movingObject->data;
    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    value23 = 0;
    position54.x = targetPosition->x;
    position54.y = targetPosition->y;
    position54.z = targetPosition->z;
    VECSubtract(&position54, &movingObject->trans, &position48);
    scalarValue30 = VECMag(&position48);
    for (index30 = 0; index30 < 4; index30++) {
        scalarValues14[index30] = 0;
    }
    while (1) {
        for (index30 = 0, index29 = 0; index30 < 4; index30++) {
            playerObject28 = playerObjects[index30];
            if (movingObject == playerObject28 || scalarValues14[index30]) {
                continue;
            }
            scalarValue31 = ((playerObject28->trans.x * position48.x) - (movingObject->trans.x * position48.x) + (playerObject28->trans.y * position48.y) - (movingObject->trans.y * position48.y)
                           + (playerObject28->trans.z * position48.z) - (movingObject->trans.z * position48.z))
                / VECMag2Point(&position48);
            if (scalarValue31 < 0.0f || scalarValue31 >= 1.0f) {
                continue;
            }
            VECSubtract(&playerObject28->trans, &movingObject->trans, &position3C);
            VECCrossProduct(&position48, &position3C, &position24);
            position60.x = (scalarValue31 * position48.x) + movingObject->trans.x;
            position60.y = (scalarValue31 * position48.y) + movingObject->trans.y;
            position60.z = (scalarValue31 * position48.z) + movingObject->trans.z;
            localDataB4[index29].projectedPosition = position60;
            VECSubtract(&playerObject28->trans, &position60, &position3C);
            if (position24.y < 0.0f) {
                localDataB4[index29].signedLateralDistance = VECMag(&position3C);
            }
            else {
                localDataB4[index29].signedLateralDistance = -VECMag(&position3C);
            }
            VECSubtract(&position60, &movingObject->trans, &position3C);
            localDataB4[index29].distanceAlongPath = VECMag(&position3C);
            localDataB4[index29].playerSlot = index30;
            index29++;
        }
        if (index29 == 0) {
            break;
        }
        for (index30 = 0; index30 < index29 - 1; index30++) {
            for (index27 = index30 + 1; index27 < index29; index27++) {
                if (localDataB4[index30].distanceAlongPath > localDataB4[index27].distanceAlongPath) {
                    localDataB4[3] = localDataB4[index30];
                    localDataB4[index30] = localDataB4[index27];
                    localDataB4[index27] = localDataB4[3];
                }
            }
        }
        for (index30 = 0; index30 < index29; index30++) {
            if (fabs(localDataB4[index30].signedLateralDistance) < 100.0 && localDataB4[index30].distanceAlongPath > 50.0) {
                scalarValues14[localDataB4[index30].playerSlot] = 1;
                VECSubtract(&playerObjects[localDataB4[index30].playerSlot]->trans, &movingObject->trans, &position3C);
                scalarValue28 = VECMag(&position3C);
                if (0.0f != scalarValue28) {
                    VECNormalize(&position3C, &position3C);
                    scalarValue31 = 100.0 / scalarValue28;
                    if (scalarValue31 < 0.0f) {
                        scalarValue31 = 0.0f;
                    }
                    else if (scalarValue31 > 1.0f) {
                        scalarValue31 = 1.0f;
                    }
                    scalarValue29 = asinf(scalarValue31);
                    MTXRotRad(transformMatrix84, 'Y', -scalarValue29);
                    MTXMultVec(transformMatrix84, &position3C, &position30);
                    position6C.x = movingObject->trans.x + (position30.x * scalarValue30);
                    position6C.y = movingObject->trans.y + (position30.y * scalarValue30);
                    position6C.z = movingObject->trans.z + (position30.z * scalarValue30);
                    VECSubtract(&position54, &position6C, &position30);
                    scalarValuesC[0] = VECMag(&position30);
                    MTXRotRad(transformMatrix84, 'Y', scalarValue29);
                    MTXMultVec(transformMatrix84, &position3C, &position30);
                    position78.x = movingObject->trans.x + (position30.x * scalarValue30);
                    position78.y = movingObject->trans.y + (position30.y * scalarValue30);
                    position78.z = movingObject->trans.z + (position30.z * scalarValue30);
                    VECSubtract(&position54, &position78, &position30);
                    scalarValuesC[1] = VECMag(&position30);
                    if (scalarValuesC[0] < scalarValuesC[1]) {
                        position54 = position6C;
                    }
                    else {
                        position54 = position78;
                    }
                    value23 = 1;
                    break;
                }
            }
        }
        if (index30 >= index29) {
            break;
        }
        VECSubtract(&position54, &movingObject->trans, &position48);
    }
    adjustedPosition->x = position54.x;
    adjustedPosition->y = position54.y;
    adjustedPosition->z = position54.z;
    return value23;
}

float DistanceBetweenPoints(float startX, float startZ, float segmentStartX, float segmentStartZ, float segmentEndX, float segmentEndZ, float collisionRadius);

void UpdatePlayerCollision(OMOBJ *object)
{
    float scalarValue31;
    OMOBJ *playerObject31;
    PlayerTransitionWork *transitionWork30;
    PlayerWork *playerWork28;
    PlayerWork *playerWork27;
    playerObject31 = teamPlayers[0][0];
    playerWork27 = object->data;
    playerWork28 = playerObject31->data;
    transitionWork30 = playerWork27->transitionWork;
    if (playerWork28->actionState > 0 || !transitionWork30->collisionActive) {
        return;
    }
    scalarValue31 = DistanceBetweenPoints(object->trans.x, object->trans.z, transitionWork30->collisionTargetX, transitionWork30->collisionTargetZ, playerObject31->trans.x, playerObject31->trans.z, 50.0f);
    if (scalarValue31 >= 0.0f && scalarValue31 < 1.0f) {
        playerWork28->collisionActive = 1;
    }
}

void ResetPlayer(OMOBJ *object)
{
    PlayerWork *playerWork31;
    playerWork31 = object->data;
    CharModelKill(playerWork31->characterId);
    if (playerWork31->transitionWork != NULL) {
        HuMemDirectFree(playerWork31->transitionWork);
    }
    HuMemDirectFree(playerWork31);
    playerWork31 = NULL;
}

float DistanceBetweenPoints(float startX, float startZ, float segmentStartX, float segmentStartZ, float segmentEndX, float segmentEndZ, float collisionRadius)
{
    float scalarValuesC;
    float scalarValue30;
    float scalarValue29;
    float scalarValue28;
    float scalarValue27;
    float scalarValue26;
    float scalarValue25;
    scalarValue26 = (startZ * startZ) + (((segmentStartZ * segmentStartZ) + ((startX * startX) + ((segmentStartX * segmentStartX) - (2.0f * startX * segmentStartX)))) - (2.0f * startZ * segmentStartZ));
    scalarValue27 = 2.0f
        * (((startZ * segmentEndZ) + (((startZ * segmentStartZ) + (((startX * segmentEndX) + ((startX * segmentStartX) - (segmentStartX * segmentEndX))) - (startX * startX))) - (segmentStartZ * segmentEndZ))) - (startZ * startZ));
    scalarValue25 = ((segmentEndZ * segmentEndZ) + (((startZ * startZ) + ((segmentEndX * segmentEndX) + ((startX * startX) - (2.0f * startX * segmentEndX)))) - (2.0f * startZ * segmentEndZ))) - (collisionRadius * collisionRadius);
    if (0.0f == scalarValue26) {
        if (0.0f != scalarValue27) {
            return -scalarValue25 / scalarValue27;
        }
        else {
            return -1;
        }
    }
    else {
        scalarValue30 = (scalarValue27 * scalarValue27) - (4.0f * scalarValue26 * scalarValue25);
        if (scalarValue30 < 0.0) {
            return -1;
        }
        scalarValue30 = sqrtf(scalarValue30);
        scalarValue29 = (-scalarValue27 + scalarValue30) / (2.0f * scalarValue26);
        scalarValue28 = (-scalarValue27 - scalarValue30) / (2.0f * scalarValue26);
        if (scalarValue29 < scalarValue28) {
            if (scalarValue29 >= 0.0f && scalarValue29 < 1.0f) {
                return scalarValue29;
            }
            else {
                if (scalarValue28 >= 0.0f && scalarValue28 < 1.0f) {
                    return scalarValue28;
                }
            }
        }
        else {
            if (scalarValue28 >= 0.0f && scalarValue28 < 1.0f) {
                return scalarValue28;
            }
            else {
                if (scalarValue29 >= 0.0f && scalarValue29 < 1.0f) {
                    return scalarValue29;
                }
            }
        }
        return -1;
    }
}

void UpdatePlayerParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float scalarValue31;
    float scalarValue30;
    float scalarValue29;
    float scalarValue28;
    HU3DPARTICLEDATA *particleData31;
    s32 index29;

    if (!particle->dataCnt) {
        particle->dataCnt = 1;
        particleData31 = particle->data;
        for (index29 = 0; index29 < particle->maxCnt; particleData31++, index29++) {
            particleData31->vel.x = (4.0f * ((1.0f / 256.0f) * frand8())) + 8;
            particleData31->pos.x = (4.0f * (((1.0f / 256.0f) * ((s32)frand() & 0x1FF)) - 1));
            particleData31->pos.y = (64.0f * ((1.0f / 256.0f) * frand8()));
            particleData31->pos.z = (4.0f * (((1.0f / 256.0f) * ((s32)frand() & 0x1FF)) - 1));
            particleData31->zRot = 360.0f * ((1.0f / 256.0f) * frand8());
            particleData31->time = frand8();
            particleData31->parManId = (frand() & 0x3) + 4;
        }
        scalarValue29 = scalarValue28 = 0;
    }
    else {
        scalarValue29 = (particle->pos.x - model->pos.x);
        scalarValue28 = (particle->pos.z - model->pos.z);
    }
    particleData31 = particle->data;
    for (index29 = 0; index29 < particle->maxCnt; particleData31++, index29++) {
        scalarValue30 = particleData31->time / 255.0f;
        scalarValue31 = scalarValue30 * scalarValue30;
        particleData31->pos.x += scalarValue29 * scalarValue31;
        particleData31->pos.y = 60.0f * scalarValue30;
        particleData31->pos.z += scalarValue28 * scalarValue31;
        particleData31->color.a = 255.0f * (1.0f - scalarValue30);
        scalarValue31 = 0.5 * (1 + sind(270.0f * scalarValue30));
        particleData31->scale = scalarValue31 * (particleData31->vel.x * scalarValue31);
        particleData31->time += particleData31->parManId;
        if (particleData31->time > 255) {
            particleData31->pos.x = (4.0f * (((1.0f / 256.0f) * ((s32)frand() & 0x1FF)) - 1));
            particleData31->pos.y = 0;
            particleData31->pos.z = (4.0f * (((1.0f / 256.0f) * ((s32)frand() & 0x1FF)) - 1));
            particleData31->time = 0;
        }
    }
    DCStoreRangeNoSync(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
    particle->pos.x = model->pos.x;
    particle->pos.z = model->pos.z;
}

void UpdateRedTeamParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float scalarValue31;
    float scalarValue30;
    float scalarValue29;
    float scalarValue28;
    HU3DPARTICLEDATA *particleData31;
    s32 index28;
    s32 index27;
    GXColor localData8 = { 0, 0, 0, 255 };
    if (!particle->dataCnt) {
        particle->dataCnt = 1;
        particleData31 = particle->data;
        for (index28 = 0; index28 < particle->maxCnt; particleData31++, index28++) {
            particleData31->scale = 15;
            particleData31->pos.x = particleData31->pos.y = particleData31->pos.z = 0;
            scalarValue31 = particle->unk_10.x + (10.0f * (((1.0f / 256.0f) * ((s32)frand() & 0x1FF)) - 1.0f));
            scalarValue30 = 100.0f * (0.1f * ((1.0f / 256.0f) * ((s32)frand() & 0xFF))) + 10;
            particleData31->vel.x = scalarValue30 * sind(scalarValue31);
            particleData31->vel.y = 100.0f * (0.02f * (((1.0f / 256.0f) * ((s32)frand() & 0x1FF)) - 1.0f));
            particleData31->vel.z = scalarValue30 * cosd(scalarValue31);
            particleData31->color.r = particleData31->color.g = particleData31->color.b = 255;
            particleData31->color.a = 0;
            particleData31->time = ((float)particle->prevCount) * ((float)index28 / (float)particle->maxCnt);
        }
#ifdef NON_MATCHING
        scalarValue29 = 0.0f;
        scalarValue28 = 0.0f;
#endif
    }
    else {
        scalarValue29 = (particle->pos.x - model->pos.x);
        scalarValue28 = (particle->pos.z - model->pos.z);
    }
    index27 = 0;
    particleData31 = particle->data;
    for (index28 = 0; index28 < particle->maxCnt; particleData31++, index28++) {
        if (particleData31->time >= 0) {
            if (particleData31->time == 0) {
                particleData31->pos.x += particleData31->vel.x + scalarValue29;
                particleData31->pos.y += particleData31->vel.y;
                particleData31->pos.z += particleData31->vel.z + scalarValue28;
#if VERSION_PAL
                particleData31->vel.y += -1.2f;
#else
                particleData31->vel.y += -1.0000001f;
#endif
                if (particleData31->color.a) {
                    if (--particleData31->color.a == 0) {
                        particleData31->time = -1;
                    }
                }
                if (particleData31->pos.y < 0.0f) {
                    particleData31->pos.y = 0;
                    particleData31->time = -1;
                    particleData31->color.a = 0;
                }
            }
            else {
                if (--particleData31->time == 0) {
                    particleData31->color.a = 255;
                }
            }
        }
        else {
            index27++;
        }
    }
    if (index27 == particle->maxCnt) {
        model->attr |= 0x1;
    }
    DCStoreRangeNoSync(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
    particle->pos.x = model->pos.x;
    particle->pos.z = model->pos.z;
}

void UpdateBlueTeamParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData31;
    s32 index29;
    s16 index28;
    s32 index27; // ! - uninitialized

#ifdef NON_MATCHING
    index27 = 0;
#endif

    if (!particle->dataCnt) {
        particle->dataCnt = 1;
        particleData31 = particle->data;
        for (index29 = 0; index29 < particle->maxCnt; particleData31++, index29++) {
            particleData31->scale = 50;
            particleData31->pos.x = 0;
            particleData31->pos.y = 0;
            particleData31->pos.z = 0;
            particleData31->vel.x = 100.0f * (0.05f * (((1.0f / 256.0f) * ((s32)frand() & 0x1FF)) - 1.0f));
            particleData31->vel.y = 100.0f * (0.05f * (((1.0f / 256.0f) * ((s32)frand() & 0x1FF)) - 1.0f));
            particleData31->vel.z = 100.0f * (0.05f * (((1.0f / 256.0f) * ((s32)frand() & 0x1FF)) - 1.0f));
            particleData31->color.a = 128;
            particleData31->time = 0;
        }
    }
    particleData31 = particle->data;
    for (index29 = 0; index29 < particle->maxCnt; index29++, particleData31++) {
        if (particleData31->time < 0) {
            index27++;
            continue;
        }
        particleData31->pos.x += particleData31->vel.x;
        particleData31->pos.y += particleData31->vel.y;
        particleData31->pos.z += particleData31->vel.z;
        particleData31->vel.x *= 0.92f;
        particleData31->vel.y *= 0.92f;
        particleData31->vel.z *= 0.92f;
        index28 = particleData31->color.a;
        index28 -= 4;
        if (index28 < 0) {
            particleData31->time = -1;
            index28 = 0;
        }
        particleData31->color.a = index28;
    }
    if (index27 == particle->maxCnt) {
        model->attr |= 0x1;
    }
    DCStoreRangeNoSync(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void SetModelMotionFlags(s16 modelId, u32 motionFlags)
{
    HSFDATA *modelData31;
    HSFMATERIAL *material30;
    s32 index29;
    HU3DMODEL *model28;
    model28 = &Hu3DData[modelId];
    modelData31 = model28->hsf;
    material30 = modelData31->material;
    for (index29 = 0; index29 < modelData31->materialNum; index29++, material30++) {
        material30->flags |= motionFlags;
    }
}

void InterpolateCameraView(s32 startViewIndex, s32 endViewIndex, float blendAmount);

void UpdateCameraTransition(OMOBJ *object);

void InitializeCamera(OMOBJ *object)
{
    Vec position14;
    Vec position8;

    InterpolateCameraView(0, 0, 0);
    position14.x = (((sind(CRot.y) * cosd(CRot.x)) * CZoom) + Center.x);
    position14.y = (-sind(CRot.x) * CZoom) + Center.y;
    position14.z = ((cosd(CRot.y) * cosd(CRot.x)) * CZoom) + Center.z;
    VECSubtract(&Center, &position14, &position8);
    VECNormalize(&position8, &position8);
    HuAudFXListnerSetEX(&position14, &position8, 10000, 34000.0f / REFRESH_RATE_F, 0, 300, 300);
    object->objFunc = UpdateCameraTransition;
}

void InterpolateCameraView(s32 startViewIndex, s32 endViewIndex, float blendAmount)
{
    CZoom = cameraViews[startViewIndex].zoom + (blendAmount * (cameraViews[endViewIndex].zoom - cameraViews[startViewIndex].zoom));
    Center.x = cameraViews[startViewIndex].pos.x + (blendAmount * (cameraViews[endViewIndex].pos.x - cameraViews[startViewIndex].pos.x));
    Center.y = cameraViews[startViewIndex].pos.y + (blendAmount * (cameraViews[endViewIndex].pos.y - cameraViews[startViewIndex].pos.y));
    Center.z = cameraViews[startViewIndex].pos.z + (blendAmount * (cameraViews[endViewIndex].pos.z - cameraViews[startViewIndex].pos.z));
    CRot.x = cameraViews[startViewIndex].rot.x + (blendAmount * (cameraViews[endViewIndex].rot.x - cameraViews[startViewIndex].rot.x));
    CRot.y = cameraViews[startViewIndex].rot.y + (blendAmount * (cameraViews[endViewIndex].rot.y - cameraViews[startViewIndex].rot.y));
    CRot.z = cameraViews[startViewIndex].rot.z + (blendAmount * (cameraViews[endViewIndex].rot.z - cameraViews[startViewIndex].rot.z));
}

float cameraTransitionProgress[2] = { 0, 0 };

void UpdateCameraTransition(OMOBJ *object)
{
    Vec position14;
    Vec position8;
    switch (GetGameState()) {
        case 2:
            InterpolateCameraView(0, 1, sind(90.0f * cameraTransitionProgress[0]));
            if (cameraTransitionProgress[0] < 1.0f) {
                cameraTransitionProgress[0] += 5.0f / (REFRESH_RATE_F * 4);
                if (cameraTransitionProgress[0] > 1.0f) {
                    cameraTransitionProgress[0] = 1.0f;
                }
            }
            break;

        case 5:
            InterpolateCameraView(1, 0, sind(90.0f * cameraTransitionProgress[1]));
            if (cameraTransitionProgress[1] < 1.0f) {
#if VERSION_PAL
                cameraTransitionProgress[1] += 0.03333333f;
#else
                cameraTransitionProgress[1] += 10.0f / 360.0f;
#endif
                if (cameraTransitionProgress[1] > 1.0f) {
                    cameraTransitionProgress[1] = 1.0f;
                }
            }
            break;
    }
    position14.x = (((sind(CRot.y) * cosd(CRot.x)) * CZoom) + Center.x);
    position14.y = (-sind(CRot.x) * CZoom) + Center.y;
    position14.z = ((cosd(CRot.y) * cosd(CRot.x)) * CZoom) + Center.z;
    VECSubtract(&Center, &position14, &position8);
    VECNormalize(&position8, &position8);
    HuAudFXListnerUpdate(&position14, &position8);
}
