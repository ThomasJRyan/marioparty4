#include "ext_math.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/frand.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/msm.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include <string.h>
#include "version.h"

#ifndef __MWERKS__
#include "game/audio.h"
#include "game/esprite.h"
#endif

typedef struct ScoreboardRow {
    s16 score;
    s16 digitSpriteIds[6];
    s16 unreferencedSpritePadding[10];
} ScoreboardRow;

typedef struct CameraPreset {
    float cameraZoom;
    float cameraCenterX;
    float cameraCenterY;
    float cameraCenterZ;
    float cameraRotationX;
    float cameraRotationY;
    float cameraRotationZ;
} CameraPreset;

typedef struct PlayerWork {
    struct {
        u8 isControlsLocked : 1;
        u8 hasReachedGoal : 1;
        u8 isEliminated : 1;
    };
    s8 unreferencedStatusByte;
    s16 playerIndex;
    s16 controllerPort;
    s8 stickX;
    s8 stickY;
    u16 buttonsHeld;
    u16 buttonsPressed;
    s16 characterId;
    s16 difficulty;
    s16 playerGroup;
    s16 actionState;
    float movementAngle;
    s32 unreferencedMotionData[2];
    float horizontalVelocityX;
    float verticalVelocity;
    float horizontalVelocityZ;
    s32 motionIndex;
    s32 unreferencedTimingData[2];
    s16 stateTimer;
    s16 unreferencedStatusWord;
    ANIMDATA *characterImageAnimation;
    ANIMDATA *characterTextureAnimation;
    void *graphicsDisplayList;
    s32 graphicsDisplayListSize;
    ANIMDATA *characterMaskAnimation;
    ANIMDATA *imageMaskAnimation;
    u16 matchingPixelCount;
    u16 totalPixelCount;
    u16 completionScore;
    u16 imageScanStartX;
    u16 bestMatchingScore;
    u16 splinePointIndex;
    float splineProgress;
    float splineElapsedTime;
    float splineSegmentDuration;
    u16 splinePointCount;
    float (*splinePoints)[2];
    s32 soundHandle;
    s32 unreferencedWorkData[91];
} PlayerWork; // size 0x1E4

OMOBJ *minigameSequenceObject;
OMOBJ *cameraUpdateObject;
OMOBJ *outOfViewObject;
OMOBJ *scoreboardObject;
s32 minigameState;
s32 winningPlayerIndices[4];
s32 winningCharacterIds[4];
s32 sequenceTimer;
s32 nextPlayerIndex;
s32 imageVariantIndex;
Vec globalLightDirection;
s16 minigameSequenceId;
s32 reservedStateValue;
float cameraTransitionProgress;
s32 allPlayersCompletionSoundPlayed;

float characterOutlinePointsVariant0[][2] = {
    { -0.00026f, 0.51953f },
    { 0.02825f, 0.51563f },
    { 0.04108f, 0.5166f },
    { 0.05996f, 0.5166f },
    { 0.07878f, 0.39063f },
    { 0.08437f, 0.35254f },
    { 0.09284f, 0.2998f },
    { 0.10234f, 0.2998f },
    { 0.11283f, 0.2998f },
    { 0.1319f, 0.2959f },
    { 0.1373f, 0.40918f },
    { 0.15072f, 0.38574f },
    { 0.17747f, 0.38183f },
    { 0.19753f, 0.47168f },
    { 0.20098f, 0.74902f },
    { 0.2235f, 0.73438f },
    { 0.26048f, 0.71094f },
    { 0.27695f, 0.27344f },
    { 0.30872f, 0.29883f },
    { 0.32409f, 0.30566f },
    { 0.3291f, 0.56738f },
    { 0.34388f, 0.58594f },
    { 0.36296f, 0.58789f },
    { 0.3653f, 0.33984f },
    { 0.38112f, 0.32227f },
    { 0.38717f, 0.31543f },
    { 0.40221f, 0.4541f },
    { 0.42044f, 0.44433f },
    { 0.4375f, 0.43555f },
    { 0.46302f, 0.13281f },
    { 0.48229f, 0.13965f },
    { 0.50013f, 0.14844f },
    { 0.51406f, 0.14844f },
    { 0.52493f, 0.29394f },
    { 0.53854f, 0.5166f },
    { 0.53105f, 0.86816f },
    { 0.50755f, 0.87207f },
    { 0.48893f, 0.85254f },
    { 0.47923f, 0.64258f },
    { 0.48828f, 0.35156f },
    { 0.49401f, 0.19238f },
    { 0.50228f, 0.16797f },
    { 0.52454f, 0.1709f },
    { 0.54518f, 0.17188f },
    { 0.55547f, 0.1748f },
    { 0.57611f, 0.16992f },
    { 0.5916f, 0.16797f },
    { 0.61471f, 0.56543f },
    { 0.62467f, 0.76367f },
    { 0.65156f, 0.76465f },
    { 0.65742f, 0.75488f },
    { 0.69544f, 0.75488f },
    { 0.69544f, 0.63477f },
    { 0.69538f, 0.33203f },
    { 0.69544f, 0.22949f },
    { 0.7028f, 0.22558f },
    { 0.73079f, 0.2207f },
    { 0.76628f, 0.22558f },
    { 0.78555f, 0.22754f },
    { 0.79388f, 0.72949f },
    { 0.81113f, 0.73047f },
    { 0.83034f, 0.72949f },
    { 0.83789f, 0.33789f },
    { 0.86953f, 0.35449f },
    { 0.88047f, 0.36035f },
    { 0.88242f, 0.60058f },
    { 0.89479f, 0.63672f },
    { 0.93939f, 0.68555f },
    { 0.9252f, 0.52051f },
    { 0.94284f, 0.51758f },
    { 0.96113f, 0.51367f },
    { 0.99303f, 0.50977f },
    { 1.0f, 0.50683f },
};
float characterOutlinePointsVariant1[][2] = { { -0.00026f, 0.51953f }, { 0.02825f, 0.51563f }, { 0.0541f, 0.78222f }, { 0.07298f, 0.78222f }, { 0.0918f, 0.65625f },
    { 0.0724f, 0.16504f }, { 0.08607f, 0.15918f }, { 0.09928f, 0.15918f }, { 0.11615f, 0.47852f }, { 0.1248f, 0.60156f }, { 0.13379f, 0.73144f },
    { 0.15592f, 0.66602f }, { 0.17044f, 0.66308f }, { 0.18418f, 0.64355f }, { 0.19603f, 0.1748f }, { 0.22279f, 0.1709f }, { 0.26087f, 0.17383f },
    { 0.27507f, 0.16308f }, { 0.30443f, 0.16602f }, { 0.32142f, 0.16992f }, { 0.26738f, 0.70019f }, { 0.27487f, 0.70996f }, { 0.29395f, 0.71191f },
    { 0.35384f, 0.70703f }, { 0.36966f, 0.71289f }, { 0.37565f, 0.71582f }, { 0.37122f, 0.36523f }, { 0.38763f, 0.36621f }, { 0.40117f, 0.36719f },
    { 0.42135f, 0.16406f }, { 0.44062f, 0.1709f }, { 0.45846f, 0.17969f }, { 0.5099f, 0.07813f }, { 0.52077f, 0.22363f }, { 0.53437f, 0.44629f },
    { 0.53262f, 0.90722f }, { 0.50911f, 0.91113f }, { 0.49049f, 0.8916f }, { 0.48496f, 0.76758f }, { 0.48984f, 0.5f }, { 0.49557f, 0.34082f },
    { 0.5127f, 0.40234f }, { 0.53496f, 0.40527f }, { 0.55605f, 0.40625f }, { 0.56322f, 0.20801f }, { 0.58118f, 0.21387f }, { 0.59499f, 0.21875f },
    { 0.60208f, 0.35644f }, { 0.61634f, 0.37305f }, { 0.62865f, 0.37305f }, { 0.63242f, 0.30566f }, { 0.64707f, 0.30957f }, { 0.66439f, 0.31348f },
    { 0.67487f, 0.6084f }, { 0.69551f, 0.6084f }, { 0.69499f, 0.44922f }, { 0.69486f, 0.30078f }, { 0.69492f, 0.19824f }, { 0.70228f, 0.19433f },
    { 0.74681f, 0.18945f }, { 0.78229f, 0.19433f }, { 0.80156f, 0.19629f }, { 0.8194f, 0.76074f }, { 0.83665f, 0.76172f }, { 0.85586f, 0.76074f },
    { 0.85488f, 0.30859f }, { 0.88132f, 0.30469f }, { 0.89225f, 0.31055f }, { 0.89076f, 0.74902f }, { 0.90312f, 0.78516f }, { 0.94772f, 0.83398f },
    { 0.93092f, 0.52832f }, { 0.94857f, 0.52539f }, { 0.96686f, 0.52148f }, { 0.99303f, 0.50977f }, { 1.0f, 0.50683f } };
float characterOutlinePointsVariant2[][2] = { { -0.00026f, 0.51953f }, { 0.02825f, 0.51563f }, { 0.0332f, 0.55176f }, { 0.04759f, 0.4375f }, { 0.05775f, 0.2959f },
    { 0.06413f, 0.19629f }, { 0.08991f, 0.19531f }, { 0.10723f, 0.19531f }, { 0.11543f, 0.49707f }, { 0.12305f, 0.65918f }, { 0.13607f, 0.94336f },
    { 0.14779f, 0.68945f }, { 0.15514f, 0.53222f }, { 0.16204f, 0.4043f }, { 0.18034f, 0.49512f }, { 0.1847f, 0.35449f }, { 0.19095f, 0.15625f },
    { 0.20742f, 0.17285f }, { 0.22279f, 0.1709f }, { 0.24108f, 0.16894f }, { 0.24896f, 0.08887f }, { 0.2569f, 0.36328f }, { 0.26159f, 0.53125f },
    { 0.2679f, 0.69922f }, { 0.28079f, 0.80859f }, { 0.29987f, 0.89941f }, { 0.31224f, 0.7334f }, { 0.32246f, 0.55664f }, { 0.33112f, 0.41016f },
    { 0.36022f, 0.43164f }, { 0.37207f, 0.60058f }, { 0.38906f, 0.8457f }, { 0.40573f, 0.64844f }, { 0.42227f, 0.39941f }, { 0.43411f, 0.23926f },
    { 0.46732f, 0.79199f }, { 0.4875f, 0.82813f }, { 0.51387f, 0.83496f }, { 0.52324f, 0.13574f }, { 0.48893f, 0.12988f }, { 0.46237f, 0.16699f },
    { 0.46589f, 0.63086f }, { 0.48184f, 0.72266f }, { 0.49629f, 0.81836f }, { 0.50579f, 0.82422f }, { 0.52806f, 0.82715f }, { 0.54915f, 0.82813f },
    { 0.54974f, 0.19433f }, { 0.56764f, 0.17773f }, { 0.57936f, 0.16699f }, { 0.59199f, 0.16894f }, { 0.60111f, 0.16992f }, { 0.60124f, 0.4668f },
    { 0.60124f, 0.45996f }, { 0.60124f, 0.63183f }, { 0.61302f, 0.62695f }, { 0.6321f, 0.63477f }, { 0.65273f, 0.63477f }, { 0.65254f, 0.55176f },
    { 0.65254f, 0.49121f }, { 0.6526f, 0.39551f }, { 0.69297f, 0.39844f }, { 0.69642f, 0.62402f }, { 0.71595f, 0.61621f }, { 0.73451f, 0.61035f },
    { 0.74297f, 0.22266f }, { 0.77773f, 0.21484f }, { 0.79701f, 0.20898f }, { 0.79167f, 0.79102f }, { 0.80892f, 0.79785f }, { 0.83607f, 0.80762f },
    { 0.8487f, 0.4707f }, { 0.87995f, 0.47168f }, { 0.89089f, 0.47754f }, { 0.88796f, 0.14453f }, { 0.90833f, 0.13379f }, { 0.95078f, 0.18066f },
    { 0.95039f, 0.38183f }, { 0.95169f, 0.50977f }, { 0.96999f, 0.50488f }, { 0.99303f, 0.50977f }, { 1.0f, 0.50683f } };

CameraPreset cameraPresets[] = { { 11000.0f, 450.0f, 0.0f, -87.5f, 323.2f, 42.7f, 0.0f }, { 6175.0f, 100.0f, 0.0f, -87.5f, 323.2f, 0.0f, 0.0f } };

Vec globalLightPosition = { 700.0f, 2500.0f, -700.0f };
Vec worldOrigin = { 0 };
GXColor globalLightColor = { 255, 255, 255, 255 };
Vec shadowCameraPosition = { 1500.0f, 3000.0f, -200.0f };
Vec shadowUpDirection = { 0.0f, 1.0f, 0.0f };

s32 AreAllPlayersLocked(void);
void InitializeMinigameSequence(OMOBJ *object);
void UpdateMinigameSequence(OMOBJ *object);
void ExitMinigame(OMOBJ *object);
void CreateScoreboard(OMOBJ *object);
void UpdateScoreboard(OMOBJ *object);
void CreatePlayers(HUPROCESS *objectManager);
void DestroyPlayers(void);
void AwardCoinsByScore(void);
void InitializePlayer(OMOBJ *object);
void UpdatePlayerFall(OMOBJ *object);
float IntegrateBezierSpeedAdaptive(float controlPoints[], float parameter);
void BeginPlayerScoring(OMOBJ *object);
void UpdatePlayerControls(OMOBJ *object);
void UpdatePlayerScore(OMOBJ *object);
void UpdatePlayerMask(OMOBJ *object);
void CreateBackgroundModels(HUPROCESS *objectManager);
void DestroyPlayer(OMOBJ *object);
void InitializeBackgroundAssets(void);
void InitializeBackgroundModels(OMOBJ *object);
void UpdateBackgroundModels(OMOBJ *object);
void DrawPlayerTexture(HU3DMODEL *modelData, Mtx transform);
ANIMDATA *CreateBitmapAnimation(u16 width, u16 height, s32 formatIndex);
void WriteBitmapPixel(ANIMDATA *animation, u16 pixelX, u16 pixelY, u16 pixelValue);
u16 ReadBitmapPixel(ANIMDATA *animation, u16 pixelX, u16 pixelY);
s32 CreateBarrierDisplayList(void *displayListBuffer, s32 bufferSize);
void InitializeCamera(OMOBJ *object);
void InterpolateCameraPreset(s32 startPresetIndex, s32 endPresetIndex, float progress);
void UpdateCamera(OMOBJ *object);

void ObjectSetup(void)
{
    HU3DCAMERA *camera;
    HUPROCESS *objectManager;
    s16 globalLightId;

    camera = Hu3DCamera;
    OSReport("******* M404ObjectSetup *********\n");
    objectManager = omInitObjMan(50, 8192);
    omGameSysInit(objectManager);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 10.0f, 20.0f, 25000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    outOfViewObject = omAddObjEx(objectManager, 32730, 0, 0, -1, omOutView);
    globalLightDirection.x = worldOrigin.x - globalLightPosition.x;
    globalLightDirection.y = worldOrigin.y - globalLightPosition.y;
    globalLightDirection.z = worldOrigin.z - globalLightPosition.z;
    VECNormalize(&globalLightDirection, &globalLightDirection);
    globalLightId = Hu3DGLightCreateV(&globalLightPosition, &globalLightDirection, &globalLightColor);
    Hu3DGLightInfinitytSet(globalLightId);
    Hu3DGLightStaticSet(globalLightId, 1);
    Hu3DShadowCreate(45.0f, camera->near, camera->far);
    Hu3DShadowTPLvlSet(0.625f);
    Hu3DShadowPosSet(&shadowCameraPosition, &shadowUpDirection, &worldOrigin);
    HuAudSndGrpSet(29);
    imageVariantIndex = ((s32)(frand() & 0xFF) * 3) >> 8;
    minigameSequenceObject = omAddObjEx(objectManager, 0, 0, 0, -1, InitializeMinigameSequence);
    cameraUpdateObject = omAddObjEx(objectManager, 0xA, 0, 0, -1, InitializeCamera);
    scoreboardObject = omAddObjEx(objectManager, 0xB, 0, 0, -1, CreateScoreboard);
    CreatePlayers(objectManager);
    CreateBackgroundModels(objectManager);
}

s32 backgroundMusicHandle = 0xFFFFFFFFF;

void SetMinigameState(s32 state)
{
    minigameState = state;
}

s32 GetMinigameState(void)
{
    return minigameState;
}

void InitializeMinigameSequence(OMOBJ *object)
{
    SetMinigameState(0);
    sequenceTimer = REFRESH_RATE;
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    object->objFunc = UpdateMinigameSequence;
}

void UpdateMinigameSequence(OMOBJ *object)
{
    switch (GetMinigameState()) {
        case 0:
            if (--sequenceTimer) {
                break;
            }
            SetMinigameState(1);
            sequenceTimer = 3 * REFRESH_RATE;
            break;
        case 1:
            if (--sequenceTimer) {
                break;
            }
            SetMinigameState(2);
            minigameSequenceId = MGSeqCreate(3, 0);
            break;
        case 2:
            if (!MGSeqStatGet(minigameSequenceId)) {
                SetMinigameState(3);
            }
            if (backgroundMusicHandle >= 0) {
                break;
            }
            if (!(MGSeqStatGet(minigameSequenceId) & 16)) {
                break;
            }
            backgroundMusicHandle = HuAudSeqPlay(69);
            break;
        case 3:
            if (!AreAllPlayersLocked()) {
                break;
            }
            SetMinigameState(4);
            minigameSequenceId = MGSeqCreate(3, 1);
            HuAudSeqFadeOut(backgroundMusicHandle, 100);
            break;
        case 4:
            if (!MGSeqStatGet(minigameSequenceId)) {
                SetMinigameState(5);
                WipeColorSet(255, 255, 255);
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, REFRESH_RATE / 2);
                sequenceTimer = REFRESH_RATE;
            }
        case 5:
            if (--sequenceTimer) {
                break;
            }
            SetMinigameState(6);
            WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, REFRESH_RATE);
            break;
        case 6:
            if (WipeStatGet()) {
                break;
            }
            SetMinigameState(7);
            sequenceTimer = 3 * REFRESH_RATE;
            break;
        case 7:
            if (--sequenceTimer) {
                break;
            }
            SetMinigameState(9);
            AwardCoinsByScore();
            if (winningPlayerIndices[0] >= 0) {
                HuAudSStreamPlay(1);
            }
            else {
                HuAudSStreamPlay(4);
            }
            sequenceTimer = 3.5 * REFRESH_RATE;
            break;
        case 8:
            break;
        case 9:
            if (--sequenceTimer) {
                break;
            }
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
            object->objFunc = ExitMinigame;
            break;
        default:
            break;
    }
    if (omSysExitReq && !WipeStatGet()) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = ExitMinigame;
    }
}

void ExitMinigame(OMOBJ *object)
{
    if (!WipeStatGet()) {
        DestroyPlayers();
        InitializeBackgroundAssets();
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
        OSReport("******* M404Exit *********\n");
    }
}

u16 scoreDigitSpriteTypes[] = { 0, 1, 2, 3, 3, 3 };

float scoreDigitSpriteOffsets[][2] = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { -46.0f, -2.0f }, { -8.0f, 2.0f }, { 10.0f, 2.0f }, { 28.0f, 2.0f } };

float scoreboardPlayerPositions[][2] = { { 472.0f, 128.0f }, { 472.0f, 208.0f }, { 472.0f, 288.0f }, { 472.0f, 368.0f } };

s32 characterPortraitAssetIds[] = {
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x00),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x01),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x02),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x03),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x04),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x05),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x06),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x07),
};

s32 completionSoundHandle = 0xFFFFFFFF;

u32 characterMotionAssetIds[][2] = {
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00), 0x00000000 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02), 0x00000000 },
    { DATA_MAKE_NUM(DATADIR_M404, 0x22), 0x00000001 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17), 0x00000000 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18), 0x00000000 },
};

s32 characterModelAssetIds[] = {
    DATA_MAKE_NUM(DATADIR_M404, 0x01),
    DATA_MAKE_NUM(DATADIR_M404, 0x02),
    DATA_MAKE_NUM(DATADIR_M404, 0x03),
    DATA_MAKE_NUM(DATADIR_M404, 0x04),
    DATA_MAKE_NUM(DATADIR_M404, 0x05),
    DATA_MAKE_NUM(DATADIR_M404, 0x06),
    DATA_MAKE_NUM(DATADIR_M404, 0x07),
    DATA_MAKE_NUM(DATADIR_M404, 0x08),
};

float cpuSplineRandomizationByDifficulty[] = { 35.0f, 30.000002f, 25.0f, 20.0f };

float (*characterOutlinePointsByVariant[])[2] = { characterOutlinePointsVariant0, characterOutlinePointsVariant1, characterOutlinePointsVariant2 };

u16 characterOutlinePointCounts[] = { 73, 76, 82, 0 };

s32 characterImageAnimationAssets[][2] = {
    { DATA_MAKE_NUM(DATADIR_M404, 0x13), DATA_MAKE_NUM(DATADIR_M404, 0x14) },
    { DATA_MAKE_NUM(DATADIR_M404, 0x15), DATA_MAKE_NUM(DATADIR_M404, 0x16) },
    { DATA_MAKE_NUM(DATADIR_M404, 0x17), DATA_MAKE_NUM(DATADIR_M404, 0x18) },
};

void CreateScoreboard(OMOBJ *object)
{
    ScoreboardRow *scoreboardRow;
    u32 digitIndex;
    s32 playerIndex;
    ScoreboardRow *playerWork;
    OMOBJ **playerObjects;

    scoreboardRow = HuMemDirectMallocNum(HEAP_HEAP, sizeof(ScoreboardRow) * 4, HU_MEMNUM_OVL);
    object->data = scoreboardRow;
    memset(object->data, 0, 136);
    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);

    for (playerIndex = 0; playerIndex < 4; scoreboardRow++, playerIndex++) {
        for (playerWork = playerObjects[playerIndex]->data, digitIndex = 0; digitIndex < 6; digitIndex++) {
            switch (scoreDigitSpriteTypes[digitIndex]) {
                case 0:
                    scoreboardRow->digitSpriteIds[digitIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x14), 1, 0);
                    espPriSet(scoreboardRow->digitSpriteIds[digitIndex], 256);
                    break;
                case 1:
                    scoreboardRow->digitSpriteIds[digitIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x15), 0, 0);
                    espPriSet(scoreboardRow->digitSpriteIds[digitIndex], 257);
                    espColorSet(scoreboardRow->digitSpriteIds[digitIndex], 0, 0, 0);
                    espTPLvlSet(scoreboardRow->digitSpriteIds[digitIndex], 0.5f);
                    break;
                case 2:
#if defined(NON_MATCHING)
                    // TODO 1 or 0?
                    scoreboardRow->digitSpriteIds[digitIndex] = espEntry(characterPortraitAssetIds[playerWork->digitSpriteIds[5]], 0, 0);
#else
                    scoreboardRow->digitSpriteIds[digitIndex] = espEntry(characterPortraitAssetIds[playerWork->digitSpriteIds[5]]);
#endif
                    espPriSet(scoreboardRow->digitSpriteIds[digitIndex], 255);
                    break;
                case 3:
                    scoreboardRow->digitSpriteIds[digitIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x33), 1, 0);
                    espPriSet(scoreboardRow->digitSpriteIds[digitIndex], 255);
                    break;
                default:
                    break;
            }
            espDispOff(scoreboardRow->digitSpriteIds[digitIndex]);
            espDrawNoSet(scoreboardRow->digitSpriteIds[digitIndex], 0);
            espAttrSet(scoreboardRow->digitSpriteIds[digitIndex], HUSPR_ATTR_NOANIM);
            espPosSet(scoreboardRow->digitSpriteIds[digitIndex], scoreboardPlayerPositions[playerIndex][0] + scoreDigitSpriteOffsets[digitIndex][0],
                scoreboardPlayerPositions[playerIndex][1] + scoreDigitSpriteOffsets[digitIndex][1]);
        }
    }

    HuDataDirClose(DATADIR_MGCONST);
    object->objFunc = UpdateScoreboard;
}

void UpdateScoreboard(OMOBJ *object)
{
    s32 playerIndex;
    ScoreboardRow *scoreboardRows;
    s32 completedPlayerCount;
    PlayerWork *playerWork;
    OMOBJ **playerObjects;

    scoreboardRows = object->data;
    switch (GetMinigameState()) {
        case 6:
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                for (completedPlayerCount = 0; completedPlayerCount < 6; completedPlayerCount++) {
                    espDispOn(scoreboardRows[playerIndex].digitSpriteIds[completedPlayerCount]);
                }
            }
            break;
        case 7:
            playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
            for (completedPlayerCount = 0, playerIndex = 0; playerIndex < 4; playerIndex++) {
                playerWork = playerObjects[playerIndex]->data;
                scoreboardRows[playerIndex].score = playerWork->completionScore;
                if (playerWork->hasReachedGoal) {
                    completedPlayerCount++;
                }
            }
            if ((completedPlayerCount >= 4) && (allPlayersCompletionSoundPlayed == 0)) {
                HuAudFXPlay(1325);
                allPlayersCompletionSoundPlayed = 1;
            }
            if ((allPlayersCompletionSoundPlayed == 0) && (completionSoundHandle < 0)) {
                completionSoundHandle = HuAudFXPlay(1324);
            }
            break;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        espBankSet(scoreboardRows[playerIndex].digitSpriteIds[3], scoreboardRows[playerIndex].score / 100);
        espBankSet(scoreboardRows[playerIndex].digitSpriteIds[4], (scoreboardRows[playerIndex].score / 10) % 10);
        espBankSet(scoreboardRows[playerIndex].digitSpriteIds[5], scoreboardRows[playerIndex].score % 10);
    }
}

void DestroyScoreboard(void)
{
    HuMemDirectFree(scoreboardObject->data);
}

void CreatePlayers(HUPROCESS *process)
{
    s32 playerIndex;

    winningPlayerIndices[0] = winningPlayerIndices[1] = winningPlayerIndices[2] = winningPlayerIndices[3] = -1;
    winningCharacterIds[0] = winningCharacterIds[1] = winningCharacterIds[2] = winningCharacterIds[3] = -1;
    nextPlayerIndex = 0;
    omMakeGroupEx(process, 0, 4);

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        omAddObjEx(process, 5, 10, 50, 0, InitializePlayer);
    }
}

void DestroyPlayers(void)
{
    s32 playerIndex;
    OMOBJ **playerObjects;

    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        DestroyPlayer(playerObjects[playerIndex]);
    }
}

s32 AreAllPlayersLocked(void)
{
    s32 playerIndex;
    OMOBJ **playerObjects;
    PlayerWork *playerWork;

    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerWork = playerObjects[playerIndex]->data;
        if (!playerWork->isControlsLocked) {
            break;
        }
    }
    if (playerIndex < 4) {
        return 0;
    }
    return 1;
}

u16 CountVisibleCharacterPixels(OMOBJ *object)
{
    PlayerWork *playerWork;
    s32 pixelY;
    s32 pixelX;
    s32 visiblePixelCount;

    playerWork = object->data;

    for (visiblePixelCount = 0, pixelY = 0; pixelY < playerWork->characterImageAnimation->bmp->sizeY; pixelY++) {
        for (pixelX = 0; pixelX < playerWork->characterImageAnimation->bmp->sizeX; pixelX++) {
            if (!ReadBitmapPixel(playerWork->characterImageAnimation, pixelX, pixelY)) {
                visiblePixelCount++;
            }
        }
    }
    return visiblePixelCount;
}

s32 MeasureImageMatch(OMOBJ *object, float imageScanFraction)
{
    PlayerWork *playerWork;
    s32 pixelY;
    s32 pixelX;
    u16 maskPixelX;
    u16 maskPixelY;
    s32 imageScanLimit;
    u16 maskPixelX;
    u16 maskPixelY;

    playerWork = object->data;
    imageScanLimit = playerWork->characterImageAnimation->bmp->sizeX * imageScanFraction;

    for (pixelY = 0; pixelY < playerWork->characterImageAnimation->bmp->sizeY; pixelY++) {
        for (pixelX = playerWork->imageScanStartX; pixelX < imageScanLimit; pixelX++) {
            if (!ReadBitmapPixel(playerWork->characterImageAnimation, pixelX, pixelY)) {
                for (maskPixelY = 0; maskPixelY < (playerWork->characterMaskAnimation->bmp->sizeY / playerWork->characterImageAnimation->bmp->sizeY); maskPixelY++) {
                    for (maskPixelX = 0; maskPixelX < (playerWork->characterMaskAnimation->bmp->sizeX / playerWork->characterImageAnimation->bmp->sizeX); maskPixelX++) {
                        maskPixelX = ((s32)maskPixelX) + playerWork->characterMaskAnimation->bmp->sizeX * pixelX / playerWork->characterImageAnimation->bmp->sizeX;
                        maskPixelY = ((s32)maskPixelY) + playerWork->characterMaskAnimation->bmp->sizeY * pixelY / playerWork->characterImageAnimation->bmp->sizeY;
                        if (ReadBitmapPixel(playerWork->characterMaskAnimation, maskPixelX, maskPixelY)) {
                            playerWork->matchingPixelCount++;
                            break;
                        }
                    }
                    if (maskPixelX < playerWork->characterMaskAnimation->bmp->sizeX / playerWork->characterImageAnimation->bmp->sizeX) {
                        break;
                    }
                }
            }
        }
    }
    playerWork->imageScanStartX = imageScanLimit;
    return (playerWork->matchingPixelCount * 100) / playerWork->totalPixelCount;
}

void AwardCoinsByScore(void)
{
    s32 playerRankings[4];
    u16 coinAwards[3] = { 28, 18, 29 };
    s32 playerIndex;
    s32 otherPlayerIndex;
    PlayerWork *playerWork;
    OMOBJ **playerObjects;
    s32 lowerRankCount;
    s32 highestScore;
    PlayerWork *otherPlayerWork;
    s16 coinAward;

    highestScore = 0;
    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerWork = playerObjects[playerIndex]->data;
        for (lowerRankCount = 0, otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
            if (playerIndex != otherPlayerIndex) {
                otherPlayerWork = playerObjects[otherPlayerIndex]->data;
                if (playerWork->completionScore < otherPlayerWork->completionScore) {
                    lowerRankCount++;
                }
            }
        }
        playerRankings[playerIndex] = lowerRankCount;
        if (playerWork->completionScore > highestScore) {
            highestScore = playerWork->completionScore;
        }
    }
    if (highestScore > 30) {
        for (otherPlayerIndex = 0, playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (!playerRankings[playerIndex]) {
                winningPlayerIndices[otherPlayerIndex] = playerIndex;
                winningCharacterIds[otherPlayerIndex] = GWPlayerCfg[winningPlayerIndices[otherPlayerIndex]].character;
                otherPlayerIndex++;
            }
            coinAward = playerRankings[playerIndex];
            GWPlayerCoinWinSet(playerIndex, coinAward);
        }
        return;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        GWPlayerCoinWinSet(playerIndex, 3);
    }
}

void InitializePlayer(OMOBJ *object)
{
    PlayerWork *playerWork;
    s32 motionIndex;
    float(*splinePointTable)[2];
    s32 characterId;
    HU3DMODEL *modelData;

    playerWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerWork), HU_MEMNUM_OVL);
    object->data = playerWork;
    memset(playerWork, 0, 484);
    playerWork->playerIndex = nextPlayerIndex++;
    playerWork->controllerPort = GWPlayerCfg[playerWork->playerIndex].padIdx;

    playerWork->characterId = characterId = GWPlayerCfg[playerWork->playerIndex].character;
    playerWork->difficulty = GWPlayerCfg[playerWork->playerIndex].diff;
    playerWork->playerGroup = GWPlayerCfg[playerWork->playerIndex].group;
    object->mdlId[0] = CharModelCreate(characterId, 2);
    Hu3DModelShadowSet(object->mdlId[0]);
    object->mdlId[1] = Hu3DModelCreateFile(characterModelAssetIds[characterId]);
    Hu3DModelHookSet(object->mdlId[0], "test11_tex_we-itemhook-r", object->mdlId[1]);
    playerWork->motionIndex = 0;

    for (motionIndex = 0; motionIndex < (u32)5; motionIndex++) {
        if (!characterMotionAssetIds[motionIndex][1]) {
            object->mtnId[motionIndex] = CharMotionCreate(playerWork->characterId, characterMotionAssetIds[motionIndex][0]);
        }
        else {
            object->mtnId[motionIndex] = CharMotionCreate(playerWork->characterId, playerWork->characterId + characterMotionAssetIds[motionIndex][0]);
        }
    }
    Hu3DMotionSet(object->mdlId[0], object->mtnId[0]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_OVL_LOOP | HU3D_MOTATTR_OVL_PAUSE);
    CharMotionVoiceOnSet(characterId, object->mtnId[1], 0);
    object->trans.x = -90.0f;
    object->trans.y = 0.0f;
    object->trans.z = -450.0f + (300.0f * playerWork->playerIndex);
    object->rot.y = 90.0f;
    object->mdlId[2] = Hu3DHookFuncCreate(DrawPlayerTexture);
    Hu3DModelPosSet(object->mdlId[2], 0.0f, object->trans.y, object->trans.z);
    modelData = &Hu3DData[object->mdlId[2]];
    modelData->hookData = object;
    playerWork->characterMaskAnimation = CreateBitmapAnimation(960, 64, 1);
    playerWork->graphicsDisplayListSize = 4096;
    playerWork->graphicsDisplayList = HuMemDirectMallocNum(HEAP_HEAP, playerWork->graphicsDisplayListSize, HU_MEMNUM_OVL);
    DCInvalidateRange(playerWork->graphicsDisplayList, playerWork->graphicsDisplayListSize);
    playerWork->graphicsDisplayListSize = CreateBarrierDisplayList(playerWork->graphicsDisplayList, playerWork->graphicsDisplayListSize);
    playerWork->characterImageAnimation = HuSprAnimReadFile(characterImageAnimationAssets[imageVariantIndex][0]);
    DCFlushRangeNoSync(playerWork->characterImageAnimation->bmp->data, (playerWork->characterImageAnimation->bmp->pixSize * (playerWork->characterImageAnimation->bmp->sizeX * playerWork->characterImageAnimation->bmp->sizeY)) / 8);
    playerWork->matchingPixelCount = 0;
    playerWork->totalPixelCount = CountVisibleCharacterPixels(object);
    playerWork->characterTextureAnimation = HuSprAnimReadFile(characterImageAnimationAssets[imageVariantIndex][1]);
    playerWork->imageMaskAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M404, 0x19));
    DCFlushRangeNoSync(playerWork->imageMaskAnimation->bmp->data, (playerWork->imageMaskAnimation->bmp->pixSize * (playerWork->imageMaskAnimation->bmp->sizeX * playerWork->imageMaskAnimation->bmp->sizeY)) / 8);
    Hu3DMotionOverlaySet(object->mdlId[0], object->mtnId[2]);
    object->trans.y = 1500.0f;
    playerWork->stateTimer = playerWork->playerIndex * REFRESH_RATE / 2;
    CharModelDataClose(playerWork->characterId);
    if (GWPlayerCfg[playerWork->playerIndex].iscom == 1) {
        splinePointTable = characterOutlinePointsByVariant[imageVariantIndex];
        playerWork->splinePointCount = characterOutlinePointCounts[imageVariantIndex];
        playerWork->splinePoints = HuMemDirectMallocNum(HEAP_HEAP, (playerWork->splinePointCount + 3) * 8, HU_MEMNUM_OVL);

        for (motionIndex = 0; motionIndex < playerWork->splinePointCount; motionIndex++, splinePointTable++) {
            playerWork->splinePoints[motionIndex][0] = 3000.0f * splinePointTable[0][0];
            playerWork->splinePoints[motionIndex][1] = object->trans.z - (200.0f * (splinePointTable[0][1] - 0.5f));
            if (motionIndex && motionIndex < (playerWork->splinePointCount - 1)) {
                playerWork->splinePoints[motionIndex][0] += cpuSplineRandomizationByDifficulty[playerWork->difficulty] * (s32)(frand() % 512 - 256) / 256;
                playerWork->splinePoints[motionIndex][1] += cpuSplineRandomizationByDifficulty[playerWork->difficulty] * (s32)(frand() % 512 - 256) / 256;
            }
        }

        while (motionIndex < (playerWork->splinePointCount + 3)) {
            playerWork->splinePoints[motionIndex][0] = playerWork->splinePoints[playerWork->splinePointCount - 1][0];
            playerWork->splinePoints[motionIndex][1] = playerWork->splinePoints[playerWork->splinePointCount - 1][1];
            motionIndex++;
        }
        playerWork->splineElapsedTime = 0.0f;
        playerWork->splineSegmentDuration = IntegrateBezierSpeedAdaptive(playerWork->splinePoints[playerWork->splinePointIndex], 1.0f);
    }
    playerWork->movementAngle = 90.0f;
    playerWork->soundHandle = -1;
    object->objFunc = UpdatePlayerFall;
}

void UpdatePlayerFall(OMOBJ *object)
{
    PlayerWork *playerWork;

    playerWork = object->data;
    if (GetMinigameState() >= 1) {
        if (playerWork->isEliminated) {
            if (playerWork->stateTimer != 0) {
                if (--playerWork->stateTimer == 0) {
                    HuAudFXPlay(1327);
                }
            }
            if (object->trans.y > 0.0f) {
                playerWork->verticalVelocity = playerWork->verticalVelocity - (VERSION_NTSC ? 1.0000001f : 1.2f);
                object->trans.y += playerWork->verticalVelocity;
                if (object->trans.y < 0.0f) {
                    object->trans.y = 0.0f;
                    omVibrate(playerWork->playerIndex, 12, 6, 6);
                    HuAudFXPlay(1326);
                }
            }
            if (GetMinigameState() != 2) {
                return;
            }
            playerWork->verticalVelocity = 0.0f;
            object->objFunc = UpdatePlayerControls;
            return;
        }
        if (playerWork->stateTimer == 0) {
            playerWork->isEliminated = 1;
            playerWork->stateTimer = REFRESH_RATE / 2;
            return;
        }
        playerWork->stateTimer--;
    }
}

float EvaluateBezierCurve(float progress, float startValue, float firstControlValue, float secondControlValue, float endValue)
{
    float remainingProgress;
    float squaredProgress;
    float cubedProgress;

    remainingProgress = 1.0f - progress;
    squaredProgress = remainingProgress * remainingProgress;
    cubedProgress = remainingProgress * remainingProgress * remainingProgress;
    return (progress * progress * progress * endValue) + (3.0f * remainingProgress * progress * progress * secondControlValue + (cubedProgress * startValue + 3.0f * squaredProgress * progress * firstControlValue));
}

float EvaluateBezierDerivative(float progress, float startValue, float firstControlValue, float secondControlValue, float endValue)
{
    float quadraticCoefficient;
    float linearCoefficient;
    float constantCoefficient;

    quadraticCoefficient = 3.0f * (-startValue + endValue + (3.0f * (firstControlValue - secondControlValue)));
    linearCoefficient = 2.0f * ((3.0f * (startValue + secondControlValue)) - (6.0f * firstControlValue));
    constantCoefficient = 3.0f * (-startValue + firstControlValue);
    return quadraticCoefficient * progress * progress + linearCoefficient * progress + constantCoefficient;
}

float MeasureBezierSpeed(float controlPoints[], float parameter)
{
    float horizontalDerivative;
    float verticalDerivative;

    horizontalDerivative = EvaluateBezierDerivative(parameter, controlPoints[0], controlPoints[2], controlPoints[4], controlPoints[6]);
    verticalDerivative = EvaluateBezierDerivative(parameter, controlPoints[1], controlPoints[3], controlPoints[7], controlPoints[7]);

    return sqrtf(horizontalDerivative * horizontalDerivative + verticalDerivative * verticalDerivative);
}

float IntegrateBezierSpeed(float controlPoints[], float parameter)
{
    float stepSize[1];
    float sampleParameter;
    float startParameter;
    float integralSum;
    float intervalCount;
    float intervalWidth;
    s32 sampleIndex;

    startParameter = 0.0f;
    intervalWidth = 16.0f;
    intervalCount = (parameter - startParameter) / intervalWidth;
    stepSize[0] = intervalCount + intervalCount;

    integralSum = MeasureBezierSpeed(controlPoints, startParameter) + MeasureBezierSpeed(controlPoints, parameter);

    for (sampleIndex = 1, sampleParameter = startParameter + intervalCount; sampleIndex < intervalWidth; sampleIndex += 2, sampleParameter += stepSize[0]) {
        integralSum += 4.0f * MeasureBezierSpeed(controlPoints, sampleParameter);
    }

    for (sampleIndex = 2, sampleParameter = startParameter + stepSize[0]; sampleIndex < intervalWidth; sampleIndex += 2, sampleParameter += stepSize[0]) {
        integralSum += 2.0f * MeasureBezierSpeed(controlPoints, sampleParameter);
    }
    return (intervalCount / 3.0f) * integralSum;
}

float IntegrateBezierSpeedAdaptive(float controlPoints[], float parameter)
{
    float integralEstimate;
    float startParameter;
    float intervalWidth;
    float trapezoidEstimate;
    float midpointSum;

    s32 sampleIndex;
    s32 subdivisionCount;
    s32 maxSubdivisions;

    maxSubdivisions = 32;
    startParameter = 0.0f;
    integralEstimate = 0.0f;

    intervalWidth = parameter - startParameter;
    trapezoidEstimate = intervalWidth * (MeasureBezierSpeed(controlPoints, startParameter) + MeasureBezierSpeed(controlPoints, parameter)) / 2;

    for (subdivisionCount = 1; subdivisionCount <= maxSubdivisions; subdivisionCount *= 2) {
        for (midpointSum = 0.0f, sampleIndex = 1; sampleIndex <= subdivisionCount; sampleIndex++) {
            midpointSum += MeasureBezierSpeed(controlPoints, startParameter + intervalWidth * (sampleIndex - 0.5f));
        }
        midpointSum *= intervalWidth;
        integralEstimate = (trapezoidEstimate + (2.0f * midpointSum)) / 3.0f;
        intervalWidth /= 2;
        trapezoidEstimate = (trapezoidEstimate + midpointSum) / 2;
    }
    return integralEstimate;
}

float FindBezierParameterAtDistance(float controlPoints[], float parameter, float targetDistance)
{
    volatile float previousParameter;
    volatile float derivativeTolerance;
    float distanceError;
    float curveSpeed;
    s32 iteration;

    derivativeTolerance = 0.00001f;
    iteration = 0;
    do {
        curveSpeed = IntegrateBezierSpeedAdaptive(controlPoints, parameter) - targetDistance;
        if (fabs(distanceError = MeasureBezierSpeed(controlPoints, parameter)) < derivativeTolerance) {
            distanceError = 1.0f;
        }
        previousParameter = parameter;
        parameter -= curveSpeed / distanceError;
        iteration++;
    } while (parameter != previousParameter && iteration < 16);
    return parameter;
}

float InterpolateAngle(float startAngle, float endAngle, float progress) // same as in REL/m402Dll
{
    float interpolatedAngle;
    float wrappedAngleDelta;

    wrappedAngleDelta = fmod(endAngle - startAngle, 360.0);
    if (wrappedAngleDelta < 0.0f) {
        wrappedAngleDelta += 360.0f;
    }
    if (wrappedAngleDelta > 180.0f) {
        wrappedAngleDelta -= 360.0f;
    }
    interpolatedAngle = fmod(startAngle + (progress * wrappedAngleDelta), 360.0);
    if (interpolatedAngle < 0.0f) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

void UpdateCpuSplineInput(OMOBJ *object)
{
    float curveX;
    float curveZ;

    PlayerWork *playerWork;
    float *curvePoint;

    playerWork = object->data;
    if (playerWork->splinePointIndex < (playerWork->splinePointCount - 1)) {
        if ((180.0f / REFRESH_RATE + playerWork->splineElapsedTime) >= playerWork->splineSegmentDuration) {
            playerWork->splineElapsedTime = 180.0f / REFRESH_RATE - (playerWork->splineSegmentDuration - playerWork->splineElapsedTime);
            playerWork->splinePointIndex += 3;
            playerWork->splineSegmentDuration = IntegrateBezierSpeedAdaptive(playerWork->splinePoints[playerWork->splinePointIndex], 1.0f);
        }
        else {
            playerWork->splineElapsedTime += 180.0f / REFRESH_RATE;
        }
        playerWork->splineProgress = FindBezierParameterAtDistance(playerWork->splinePoints[playerWork->splinePointIndex], playerWork->splineProgress, playerWork->splineElapsedTime);
        curvePoint = playerWork->splinePoints[playerWork->splinePointIndex];
        curveX = EvaluateBezierCurve(playerWork->splineProgress, curvePoint[0], curvePoint[2], curvePoint[4], curvePoint[6]) - 90.0f;
        curveZ = EvaluateBezierCurve(playerWork->splineProgress, curvePoint[1], curvePoint[3], curvePoint[5], curvePoint[7]);
        playerWork->movementAngle = InterpolateAngle(playerWork->movementAngle, atan2d(curveX - object->trans.x, -(curveZ - object->trans.z)), 0.1f);
        playerWork->stickX = (u8)(20.0 * sind(playerWork->movementAngle));
        playerWork->stickY = (u8)(20.0 * cosd(playerWork->movementAngle));
    }
    else {
        playerWork->stickX = 20;
        playerWork->stickY = 0;
    }
}

void SetSoundVolume(int seId, s16 vol)
{
    MSM_SEPARAM param;

    param.flag = MSM_SEPARAM_VOL;
    param.vol = vol;
    msmSeSetParam(seId, &param);
}

void UpdatePlayerControls(OMOBJ *object)
{
    Vec previousPosition;
    float movementAngle;

    PlayerWork *playerWork;
    HU3DMODEL *barrierModel;
    s32 nextMotionIndex;
    s16 soundPan;

    playerWork = object->data;
    nextMotionIndex = -1;
    if (GetMinigameState() == 6) {
        object->objFunc = BeginPlayerScoring;
    }
    if (GetMinigameState() == 3 && !playerWork->isControlsLocked) {
        switch (playerWork->actionState) {
            case 0:
                if (GWPlayerCfg[playerWork->playerIndex].iscom == 0) {
                    playerWork->stickX = HuPadStkX[playerWork->controllerPort];
                    playerWork->stickY = HuPadStkY[playerWork->controllerPort];
                    playerWork->buttonsHeld = HuPadBtn[playerWork->controllerPort];
                    playerWork->buttonsPressed = HuPadBtnDown[playerWork->controllerPort];
                }
                else {
                    playerWork->buttonsHeld = playerWork->buttonsPressed = 0;
                    playerWork->stickX = playerWork->stickY = 0;
                    UpdateCpuSplineInput(object);
                }
                if (((playerWork->stickX * playerWork->stickX) + (playerWork->stickY * playerWork->stickY)) > 16.0f) {
                    movementAngle = atan2d(playerWork->stickX, -playerWork->stickY);
                    playerWork->horizontalVelocityX = (VERSION_NTSC ? 3.0 : 3.5999999046325684) * sind(movementAngle);
                    playerWork->horizontalVelocityZ = (VERSION_NTSC ? 3.0 : 3.5999999046325684) * cosd(movementAngle);
                    nextMotionIndex = 1;
                    if (playerWork->horizontalVelocityX > 0.0f) {
                        CharMotionSpeedSet(playerWork->characterId, 0.5f);
                    }
                    else {
                        CharMotionSpeedSet(playerWork->characterId, -0.5f);
                    }
                    Hu3DMotionOverlaySpeedSet(object->mdlId[0], Hu3DData[object->mdlId[0]].motWork.speed);
                    if (playerWork->motionIndex == 0) {
                        Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_OVL_PAUSE);
                        Hu3DMotionOverlayTimeSet(object->mdlId[0], 0.0f);
                    }
                }
                else {
                    playerWork->horizontalVelocityX = playerWork->horizontalVelocityZ = 0.0f;
                    nextMotionIndex = 0;
                    CharMotionSpeedSet(playerWork->characterId, 1.0f);
                    Hu3DMotionOverlayTimeSet(object->mdlId[0], 0.0f);
                    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_OVL_PAUSE);
                }
                previousPosition.x = object->trans.x;
                previousPosition.z = object->trans.z;
                object->trans.x += playerWork->horizontalVelocityX;
                object->trans.z += playerWork->horizontalVelocityZ;
                if (object->trans.x < -90.0f) {
                    object->trans.x = -90.0f;
                }
                if (object->trans.x < (Center.x - 500.0f)) {
                    object->trans.x = Center.x - 500.0f;
                }
                else if (object->trans.x > (550.0f + Center.x)) {
                    object->trans.x = 550.0f + Center.x;
                }
                if ((object->trans.x != previousPosition.x) || (object->trans.z != previousPosition.z)) {
                    if (playerWork->soundHandle < 0) {
                        playerWork->soundHandle = HuAudFXPlay(0x52B);
                    }
                }
                else if (playerWork->soundHandle >= 0) {
                    HuAudFXStop(playerWork->soundHandle);
                    playerWork->soundHandle = -1;
                }
                barrierModel = &Hu3DData[object->mdlId[2]];
                if (object->trans.z < (barrierModel->pos.z - 100.0f)) {
                    object->trans.z = barrierModel->pos.z - 100.0f;
                }
                else if (object->trans.z > ((100.0f + barrierModel->pos.z) - 1.0f)) {
                    object->trans.z = (100.0f + barrierModel->pos.z) - 1.0f;
                }
                if ((90.0f + object->trans.x) > 3000.0f) {
                    object->trans.x = 2910.0f;
                    playerWork->actionState++;
                    playerWork->motionIndex = -1;
                    nextMotionIndex = 1;
                    CharMotionSpeedSet(playerWork->characterId, 1.0f);
                    Hu3DMotionOverlayReset(object->mdlId[0]);
                    Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
                    if (playerWork->soundHandle >= 0) {
                        HuAudFXStop(playerWork->soundHandle);
                        playerWork->soundHandle = -1;
                    }
                }
                UpdatePlayerMask(object);
                if (playerWork->soundHandle >= 0) {
                    soundPan = 64.0f + ((64.0f * (object->trans.x - Center.x)) / 500.0f);
                    if (soundPan < 0) {
                        soundPan = 0;
                    }
                    else if (soundPan > 127) {
                        soundPan = 127;
                    }
                    HuAudFXPanning(playerWork->soundHandle, soundPan);
                }
                break;
            case 1:
                nextMotionIndex = 1;
                object->trans.x += VERSION_NTSC ? 4.0000005f : 4.8f;
                if (object->trans.x >= 3200.0f) {
                    playerWork->actionState++;
                    playerWork->stateTimer = 0x3C;
                }
                break;
            case 2:
                nextMotionIndex = 0;
                object->rot.y += 0.1f * -object->rot.y;
                if (--playerWork->stateTimer == 0) {
                    playerWork->isControlsLocked = 1;
                }
                break;
        }
        if (nextMotionIndex >= 0 && nextMotionIndex != playerWork->motionIndex) {
            playerWork->motionIndex = nextMotionIndex;
            CharMotionShiftSet(playerWork->characterId, object->mtnId[playerWork->motionIndex], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
        }
    }
}

void BeginPlayerScoring(OMOBJ *object)
{
    PlayerWork *playerWork;

    playerWork = object->data;
    object->trans.x = 3200.0f;
    object->trans.z = -450.0f + (300.0f * playerWork->playerIndex);
    object->rot.y = 0.0f;
    playerWork->motionIndex = -1;
    CharMotionShiftSet(playerWork->characterId, *object->mtnId, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    playerWork->imageScanStartX = 0;
    playerWork->bestMatchingScore = MeasureImageMatch(object, 1.0f);
    playerWork->imageScanStartX = 0;
    playerWork->matchingPixelCount = 0;
    playerWork->soundHandle = -1;
    object->objFunc = UpdatePlayerScore;
}

void UpdatePlayerScore(OMOBJ *object)
{
    PlayerWork *playerWork;
    s32 playerRank;
    u16 completionScore;

    playerWork = object->data;
    if (GetMinigameState() == 7) {
        completionScore = MeasureImageMatch(object, (Center.x - cameraPresets[1].cameraCenterX) / (3000.0f - cameraPresets[1].cameraCenterX));
        if (!playerWork->hasReachedGoal && completionScore == playerWork->bestMatchingScore) {
            playerWork->hasReachedGoal = 1;
        }
        playerWork->completionScore = completionScore;
    }
    if (GetMinigameState() == 9 && playerWork->motionIndex < 0) {
        for (playerRank = 0; playerRank < 4; playerRank++) {
            if (playerWork->playerIndex == winningPlayerIndices[playerRank]) {
                break;
            }
        }
        if (playerRank < 4) {
            playerWork->motionIndex = 3;
        }
        else {
            playerWork->motionIndex = 4;
        }
        CharMotionShiftSet(playerWork->characterId, object->mtnId[playerWork->motionIndex], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    }
}

void UpdatePlayerMask(OMOBJ *object)
{
    float horizontalPositionRatio;
    PlayerWork *playerWork;
    float depthPositionRatio;
    HU3DMODEL *maskModel;
    s32 pixelY;
    s32 pixelX;
    u16 maskX;
    u16 maskY;

    playerWork = object->data;
    maskModel = &Hu3DData[object->mdlId[2]];
    horizontalPositionRatio = ((90.0f + object->trans.x) - maskModel->pos.x) / 3000.0f;
    depthPositionRatio = (object->trans.z - (maskModel->pos.z - 100.0f)) / 200.0f;

    for (pixelY = 0; pixelY < playerWork->imageMaskAnimation->bmp->sizeY; pixelY++) {
        for (pixelX = 0; pixelX < playerWork->imageMaskAnimation->bmp->sizeX; pixelX++) {
            maskX = (pixelX + (horizontalPositionRatio * playerWork->characterMaskAnimation->bmp->sizeX)) - playerWork->imageMaskAnimation->bmp->sizeX / 2;
            maskY = (pixelY + (depthPositionRatio * playerWork->characterMaskAnimation->bmp->sizeY)) - playerWork->imageMaskAnimation->bmp->sizeY / 2;
            if (maskX < playerWork->characterMaskAnimation->bmp->sizeX && maskY < playerWork->characterMaskAnimation->bmp->sizeY) {
                WriteBitmapPixel(playerWork->characterMaskAnimation, maskX, maskY, ReadBitmapPixel(playerWork->imageMaskAnimation, pixelX, pixelY));
            }
        }
    }
    DCFlushRangeNoSync(playerWork->characterMaskAnimation->bmp->data, playerWork->characterMaskAnimation->bmp->dataSize);
}

void DestroyPlayer(OMOBJ *object)
{
    PlayerWork *playerWork;

    playerWork = object->data;
    if (object->data != NULL) {
        HuMemDirectFree(playerWork->graphicsDisplayList);
        HuMemDirectFree(object->data);
        object->data = NULL;
    }
}

void CreateBackgroundModels(HUPROCESS *objectManager)
{
    omMakeGroupEx(objectManager, 1, 1);
    omAddObjEx(objectManager, 10, 7, 0, 1, InitializeBackgroundModels);
}

void InitializeBackgroundAssets(void) { }

s32 backgroundCharacterModelAssets[] = { DATA_MAKE_NUM(DATADIR_M404, 0x0B), DATA_MAKE_NUM(DATADIR_M404, 0x0C), DATA_MAKE_NUM(DATADIR_M404, 0x0D),
    DATA_MAKE_NUM(DATADIR_M404, 0x0E), DATA_MAKE_NUM(DATADIR_M404, 0x0F), DATA_MAKE_NUM(DATADIR_M404, 0x10), DATA_MAKE_NUM(DATADIR_M404, 0x11),
    DATA_MAKE_NUM(DATADIR_M404, 0x12) };

s32 backgroundCharacterAnimationAssets[] = { DATA_MAKE_NUM(DATADIR_M404, 0x1A), DATA_MAKE_NUM(DATADIR_M404, 0x1B), DATA_MAKE_NUM(DATADIR_M404, 0x1C),
    DATA_MAKE_NUM(DATADIR_M404, 0x1D), DATA_MAKE_NUM(DATADIR_M404, 0x1E), DATA_MAKE_NUM(DATADIR_M404, 0x1F), DATA_MAKE_NUM(DATADIR_M404, 0x20),
    DATA_MAKE_NUM(DATADIR_M404, 0x21) };

char *backgroundCharacterAnimationNames[] = { "kari00", "kari01", "kari02", "kari03" };

float backgroundCharacterDepthPositions[] = { -450, -150, 150, 450 };

s32 bitmapFormatByType[][2] = { { 8, 4 }, { 7, 8 }, { 6, 8 }, { 5, 16 }, { 2, 16 }, { 2, 16 } };

s32 bitmapTileDimensions[] = { 8, 8, 8, 4, 4, 4 };
s32 bitmapReadTileDimensions[] = { 8, 8, 8, 4, 4, 4 };

void InitializeBackgroundModels(OMOBJ *object)
{
    s32 characterIndex;

    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M404, 0x09));
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M404, 0x0A));
    Hu3DModelAttrSet(object->mdlId[1], HU3D_MOTATTR_LOOP);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M404, 0x00));
    Hu3DModelShadowMapSet(object->mdlId[2]);

    for (characterIndex = 0; characterIndex < 4; characterIndex++) {
        object->mdlId[characterIndex + 3] = Hu3DModelCreateFile(backgroundCharacterModelAssets[GWPlayerCfg[characterIndex].character]);
        Hu3DModelPosSet(object->mdlId[characterIndex + 3], 0.0f, 0.0f, backgroundCharacterDepthPositions[characterIndex]);
        Hu3DAnimCreate(HuSprAnimReadFile(backgroundCharacterAnimationAssets[GWPlayerCfg[characterIndex].character]), object->mdlId[2], backgroundCharacterAnimationNames[characterIndex]);
    }
    object->objFunc = UpdateBackgroundModels;
}

void UpdateBackgroundModels(OMOBJ *object) { }

void GetCharacterColor(s32 characterId, GXColor *color)
{
    GXColor playerColors[8] = { { 227, 67, 67, 255 }, { 68, 67, 227, 255 }, { 241, 158, 220, 255 }, { 67, 228, 68, 255 }, { 138, 60, 180, 255 },
        { 146, 85, 55, 255 }, { 227, 200, 20, 255 }, { 40, 40, 40, 255 } };

    *color = playerColors[characterId];
}

void DrawPlayerTexture(HU3DMODEL *modelData, Mtx transform)
{
    GXTexObj textureObject;
    PlayerWork *playerWork;
    OMOBJ *playerObject;
    s32 characterId;

    playerObject = modelData->hookData;
    playerWork = playerObject->data;
    {
        GXColor characterColor = { 255, 0, 0, 255 };
        GXColor textureColor = { 255, 255, 255, 192 };
        GXLoadPosMtxImm(transform, 0);
        GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 60, GX_FALSE, 125);
        HuSprTexLoad(playerWork->characterTextureAnimation, 0, 0, GX_CLAMP, GX_CLAMP, GX_LINEAR);
        GXSetNumTevStages(1);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_DIVIDE_2, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_TEXA, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

        characterId = playerWork->characterId;
        GetCharacterColor(characterId, &characterColor);

        GXSetChanMatColor(GX_COLOR0A0, characterColor);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
        GXSetBlendMode(GX_DECAL, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
        GXSetCullMode(GX_CULL_BACK);
        GXCallDisplayList(playerWork->graphicsDisplayList, playerWork->graphicsDisplayListSize);
        GXInitTexObj(&textureObject, playerWork->characterMaskAnimation->bmp->data, playerWork->characterMaskAnimation->bmp->sizeX, playerWork->characterMaskAnimation->bmp->sizeY, 1, GX_CLAMP, GX_CLAMP, GX_FALSE);
        GXLoadTexObj(&textureObject, GX_TEXMAP0);
        GXSetNumTevStages(1);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_A0, GX_CA_TEXA, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

        GXSetTevColor(GX_TEVREG0, textureColor);
        GXCallDisplayList(playerWork->graphicsDisplayList, playerWork->graphicsDisplayListSize);
    }
}

ANIMDATA *CreateBitmapAnimation(u16 width, u16 height, s32 formatIndex)
{
    ANIMBMP *bmp;
    ANIMDATA *anim;

    anim = HuMemDirectMallocNum(HEAP_HEAP, sizeof(ANIMBMP), HU_MEMNUM_OVL);
    bmp = HuMemDirectMallocNum(HEAP_HEAP, sizeof(ANIMDATA), HU_MEMNUM_OVL);
    anim->bmp = bmp;
    bmp->sizeX = width;
    bmp->sizeY = height;
    bmp->dataFmt = bitmapFormatByType[formatIndex][0];
    bmp->pixSize = bitmapFormatByType[formatIndex][1];
    bmp->dataSize = (bmp->pixSize * (bmp->sizeX * bmp->sizeY)) / 8;
    bmp->data = HuMemDirectMallocNum(HEAP_HEAP, bmp->dataSize, HU_MEMNUM_OVL);
    memset(bmp->data, 0, bmp->dataSize);
    DCFlushRangeNoSync(bmp->data, bmp->dataSize);
    return anim;
}

void WriteBitmapPixel(ANIMDATA *animation, u16 pixelX, u16 pixelY, u16 pixelValue)
{
    u8 *bitmap;
    u16 row;
    u16 column;
    s32 pixelAddressValue;
    s32 rowOffset;
    s32 pixelAddress;
    u8 pixelMask;

    switch (animation->bmp->pixSize) {
        case 4:
            row = bitmapTileDimensions[0]; // 8
            column = bitmapTileDimensions[1]; // 8
            break;
        case 8:
            row = bitmapTileDimensions[2]; // 8
            column = bitmapTileDimensions[3]; // 4
            break;
        case 16:
            row = bitmapTileDimensions[4]; // 4
            column = bitmapTileDimensions[5]; // 4
            break;
    }

    bitmap = (void *)(((pixelX / row) * 32) + (((pixelX % 8) * animation->bmp->pixSize) / 8)
        + (((animation->bmp->sizeX / row) * ((pixelY / column) << 5)) + ((u32)animation->bmp->data) + ((animation->bmp->pixSize * ((pixelY % column) * 8)) / 8)));

    switch (animation->bmp->pixSize) {
        case 4:
            if ((pixelX & 1) == 0) {
                *bitmap &= 0xF;
                *bitmap |= pixelValue << 4;
                break;
            }
            *bitmap &= 0xF0;
            *bitmap |= pixelValue;
            break;
        case 8:
            *bitmap |= pixelValue;
            break;
        case 16:
            *((u16 *)bitmap) = pixelValue;
            break;
    }
}

u16 ReadBitmapPixel(ANIMDATA *animation, u16 pixelX, u16 pixelY)
{
    u16 bitmap;
    u16 row;
    u8 *column;
    s32 pixelValue;
    u32 rowOffset;
    u32 pixelAddress;
    u8 pixelMask;
    u8 pixelBit;

    switch (animation->bmp->pixSize) {
        case 4:
            bitmap = bitmapReadTileDimensions[0];
            row = bitmapReadTileDimensions[1];
            break;
        case 8:
            bitmap = bitmapReadTileDimensions[2];
            row = bitmapReadTileDimensions[3];
            break;
        case 16:
            bitmap = bitmapReadTileDimensions[4];
            row = bitmapReadTileDimensions[5];
            break;
    }
    column = (void *)(((pixelX / bitmap) * 32) + (((pixelX % 8) * animation->bmp->pixSize) / 8)
        + (((animation->bmp->sizeX / bitmap) * ((pixelY / row) << 5)) + ((u32)animation->bmp->data) + ((animation->bmp->pixSize * ((pixelY % row) * 8)) / 8)));

    (void)bitmap;
    (void)bitmap;
    (void)bitmap;
    (void)row;
    (void)row;
    (void)row;

    switch (animation->bmp->pixSize) {
        case 4:
            if ((pixelX & 1) == 0) {
                pixelValue = *column >> 4;
            }
            else {
                pixelValue = *column & 0xF;
            }
            return pixelValue;
        case 8:
            return *column;
        case 16:
            return *((u16 *)column);
    }
}

s32 CreateBarrierDisplayList(void *displayListBuffer, s32 bufferSize)
{
    s32 displayListSize;

    GXBeginDisplayList(displayListBuffer, bufferSize);
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);

    GXPosition3f32(0.0f, 1.5f, 100.0f);
    GXTexCoord2f32(0.0f, 1.0f);
    GXPosition3f32(0.0f, 1.5f, -100.0f);
    GXTexCoord2f32(0.0f, 0.0f);
    GXPosition3f32(3000.0f, 1.5f, 100.0f);
    GXTexCoord2f32(1.0f, 1.0f);
    GXPosition3f32(3000.0f, 1.5f, -100.0f);
    GXTexCoord2f32(1.0f, 0.0f);

    displayListSize = GXEndDisplayList();
    DCFlushRangeNoSync(displayListBuffer, displayListSize);
    return displayListSize;
}

void InitializeCamera(OMOBJ *object)
{
    InterpolateCameraPreset(0, 0, 0.0f);
    object->objFunc = UpdateCamera;
}

void InterpolateCameraPreset(s32 startPresetIndex, s32 endPresetIndex, float progress)
{
    CZoom = cameraPresets[startPresetIndex].cameraZoom + progress * (cameraPresets[endPresetIndex].cameraZoom - cameraPresets[startPresetIndex].cameraZoom);
    Center.x = cameraPresets[startPresetIndex].cameraCenterX + progress * (cameraPresets[endPresetIndex].cameraCenterX - cameraPresets[startPresetIndex].cameraCenterX);
    Center.y = cameraPresets[startPresetIndex].cameraCenterY + progress * (cameraPresets[endPresetIndex].cameraCenterY - cameraPresets[startPresetIndex].cameraCenterY);
    Center.z = cameraPresets[startPresetIndex].cameraCenterZ + progress * (cameraPresets[endPresetIndex].cameraCenterZ - cameraPresets[startPresetIndex].cameraCenterZ);
    CRot.x = cameraPresets[startPresetIndex].cameraRotationX + progress * (cameraPresets[endPresetIndex].cameraRotationX - cameraPresets[startPresetIndex].cameraRotationX);
    CRot.y = cameraPresets[startPresetIndex].cameraRotationY + progress * (cameraPresets[endPresetIndex].cameraRotationY - cameraPresets[startPresetIndex].cameraRotationY);
    CRot.z = cameraPresets[startPresetIndex].cameraRotationZ + progress * (cameraPresets[endPresetIndex].cameraRotationZ - cameraPresets[startPresetIndex].cameraRotationZ);
}

void UpdateCamera(OMOBJ *object)
{
    HU3DCAMERA *objectIndex;

    objectIndex = Hu3DCamera;
    switch (GetMinigameState()) {
        case 1:
            InterpolateCameraPreset(0, 1, 1.0 - cosd(90.0f * cameraTransitionProgress));
            if (cameraTransitionProgress < 1.0f) {
                cameraTransitionProgress += VERSION_NTSC ? 0.007999999f : 0.0095999995f;
                if (cameraTransitionProgress > 1.0f) {
                    cameraTransitionProgress = 1.0f;
                }
            }
            break;
        case 3:
            Center.x += VERSION_NTSC ? 2.0000002f : 2.4f;
            Hu3DShadowData.camPos.x = 1000.0f + objectIndex->pos.x;
            Hu3DShadowData.camTarget.x = Hu3DShadowData.camPos.x - 1500.0f;
            break;
        case 6:
            InterpolateCameraPreset(1, 1, 0.0f);
            break;
        case 7:
            if (Center.x < 3000.0f) {
                Center.x += VERSION_NTSC ? 25.0f : 30.0f;
            }
        default:
            break;
    }
}
