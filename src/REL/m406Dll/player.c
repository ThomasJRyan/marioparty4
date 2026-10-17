#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/gamework_data.h"
#include "game/hsfload.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"

#include "REL/m406Dll.h"
#include <string.h>

#ifndef __MWERKS__
#include "game/frand.h"
#include "game/hsfex.h"
#endif

typedef struct PlayerPathCoordinates {
    /* 0x00 */ Float laneCoordinate;
    /* 0x04 */ Float segmentProgress;
} PlayerPathCoordinates; /* size = 0x08 */

typedef struct PlayerGroundCollision {
    /* 0x00 */ HSFFACE *groundFace;
    /* 0x04 */ HSFFACE *adjacentFace;
    /* 0x08 */ s16 isFinishLineFace;
} PlayerGroundCollision; /* size = 0x0C */

typedef struct M406PlayerWork {
    /* 0x00 */ union {
        struct {
            u8 isFinished : 1;
            u8 isGrounded : 1;
            u8 isHit : 1;
            u8 isStunned : 1;
            u8 hasReachedTarget : 1;
            u8 impactSoundPlayed : 1;
        };
        s16 statusFlags;
    };
    /* 0x02 */ s16 playerIndex;
    /* 0x04 */ s16 controllerPort;
    /* 0x06 */ s8 stickX;
    /* 0x07 */ s8 stickY;
    /* 0x08 */ u16 buttonHold;
    /* 0x0A */ u16 buttonPressed;
    /* 0x0C */ s16 characterId;
    /* 0x0E */ s16 difficulty;
    /* 0x10 */ s16 playerGroup;
    /* 0x12 */ s16 movementState;
    /* 0x14 */ Vec movementAnglesAndSpeed;
    /* 0x20 */ char alignmentPadding[0xC];
    /* 0x2C */ Vec velocity;
    /* 0x38 */ s32 motionId;
    /* 0x3C */ u32 motionAttributes;
    /* 0x40 */ Float motionSpeed;
    /* 0x44 */ PlayerGroundCollision collisionFaceData;
    /* 0x50 */ Float spinAngle;
    /* 0x54 */ Float movementSpeedScale;
    /* 0x58 */ s16 collisionRecoveryTimer;
    /* 0x5A */ s16 stateTimer;
    /* 0x5C */ s16 shakeCount;
    /* 0x5E */ s16 shakeTimer;
    /* 0x60 */ s16 hitRecoveryTimer;
    /* 0x62 */ s16 collisionSoundTimer;
    /* 0x64 */ s16 effectTimer;
    /* 0x68 */ Float modelAnchorX;
    /* 0x6C */ Float modelAnchorY;
    /* 0x70 */ Float modelAnchorZ;
    /* 0x74 */ Float steeringOffset;
    /* 0x78 */ Float steeringTarget;
    /* 0x7C */ s32 audioEmitterIds[2];
} M406PlayerWork; /* size = 0x84 */

void InitializePlayerMovement(OMOBJ *object);
void UpdatePlayerEffects(OMOBJ *object);
void InitializePlayerFinish(OMOBJ *object);
void UpdatePlayerSpin(OMOBJ *object);
void ApplyGroundTangentImpulse(Float scale, Float rotationSpeed, PlayerGroundCollision *source, Vec *destination);
Float GetPlayerPathDistance(Vec *source);
void MovePlayerTowardTarget(OMOBJ *object);
void WaitForPlayersToSeparate();
s32 SeparateOverlappingPlayers();
s32 IsPlayerNearby(OMOBJ *object);
void DestroyPlayerObject(OMOBJ *object);
void ResetCameraPositionHistory(OMOBJ *object);
void RecordCameraPosition(Vec *source);
void UpdateCameraPositionHistory(OMOBJ *object);
void GetPlayerJointTransform(s16 source, char *destination, Mtx effectColor);
Float ApproachAngle(Float scale, Float rotationSpeed, Float secondScale);
s32 ResolvePlayerSurfaceCollision(Vec *source, Vec *destination);
Float FindPlayerGroundSurface(Vec *source, PlayerGroundCollision *destination);
void UpdatePlayerModelTransform(OMOBJ *object);
void SetPlayerMotion(OMOBJ *object, s32 destination, Float scale, u32 effectColor);
void UpdatePlayerMovement(OMOBJ *object);
void UpdatePlayerRunning(OMOBJ *object);
void UpdatePlayerCollision(OMOBJ *object);
void UpdatePlayerGrounding(OMOBJ *object);
void UpdatePlayerOrderAndCollisions(HU3DMODEL *model, Mtx mtx);

Vec leftCourseBoundary[70] = {
    { -495.876f, 50.005f, -0.0f },
    { -500.023f, -2841.226f, -5007.74f },
    { -502.015f, -7170.411f, -12506.11f },
    { -515.622f, -8314.231f, -14487.27f },
    { -515.6f, -8761.908f, -15262.67f },
    { -508.661f, -9195.703f, -16014.02f },
    { -494.762f, -9629.499f, -16765.38f },
    { -473.947f, -10063.294f, -17516.73f },
    { -471.378f, -10495.727f, -18265.73f },
    { -495.719f, -10915.74f, -18993.21f },
    { -552.519f, -11337.781f, -19724.21f },
    { -621.485f, -11611.703f, -20198.66f },
    { -731.018f, -11887.653f, -20676.62f },
    { -950.131f, -12303.607f, -21397.07f },
    { -1209.766f, -12735.795f, -22145.64f },
    { -1469.474f, -13170.012f, -22897.73f },
    { -1627.69f, -13466.252f, -23410.83f },
    { -1757.595f, -13772.641f, -23941.51f },
    { -1838.731f, -14079.026f, -24472.19f },
    { -1871.186f, -14393.528f, -25016.92f },
    { -1834.734f, -14703.97f, -25554.62f },
    { -1749.541f, -15014.414f, -26092.33f },
    { -1623.78f, -15312.685f, -26608.94f },
    { -1384.433f, -15761.101f, -27385.62f },
    { -1124.798f, -16191.259f, -28130.68f },
    { -775.859f, -16769.535f, -29132.28f },
    { -568.923f, -17183.457f, -29849.22f },
    { -398.537f, -17621.73f, -30608.33f },
    { -187.485f, -18055.945f, -31360.41f },
    { 96.491f, -18634.223f, -32362.02f },
    { 254.886f, -19042.057f, -33068.41f },
    { 315.738f, -19315.977f, -33542.85f },
    { 331.965f, -19581.783f, -34003.24f },
    { 291.397f, -19839.473f, -34449.57f },
    { 189.969f, -20091.07f, -34885.35f },
    { -12.871f, -20340.645f, -35317.62f },
    { -325.245f, -20598.332f, -35763.96f },
    { -747.265f, -20870.219f, -36234.88f },
    { -1230.024f, -21152.258f, -36723.38f },
    { -2276.853f, -21730.535f, -37724.99f },
    { -2759.968f, -22028.799f, -38241.6f },
    { -3189.989f, -22343.3f, -38786.33f },
    { -3514.533f, -22669.979f, -39352.15f },
    { -3620.335f, -22840.408f, -39647.35f },
    { -3693.358f, -23018.965f, -39956.61f },
    { -3705.528f, -23193.463f, -40258.85f },
    { -3693.358f, -23367.96f, -40561.09f },
    { -3551.37f, -23698.695f, -41133.94f },
    { -3307.945f, -24019.275f, -41689.2f },
    { -2995.571f, -24323.633f, -42216.36f },
    { -2646.686f, -24621.904f, -42732.99f },
    { -1920.124f, -25200.174f, -43734.58f },
    { -1457.648f, -25612.07f, -44448.0f },
    { -1238.474f, -25871.78f, -44897.83f },
    { -1084.316f, -26137.584f, -45358.22f },
    { -958.555f, -26417.594f, -45843.21f },
    { -861.049f, -26697.596f, -46328.19f },
    { -783.97f, -26979.635f, -46816.69f },
    { -715.004f, -27261.672f, -47305.2f },
    { -617.584f, -27979.957f, -48549.3f },
    { -548.553f, -28700.271f, -49796.93f },
    { -491.712f, -29278.535f, -50798.51f },
    { -475.485f, -29856.814f, -51800.12f },
    { -479.514f, -30433.059f, -52798.2f },
    { -505.531f, -34619.363f, -60049.28f },
    { -513.632f, -34659.062f, -60100.39f },
    { -509.573f, -34721.215f, -60151.54f },
    { -509.589f, -34805.496f, -60191.15f },
    { -509.587f, -34970.055f, -60233.17f },
    { -509.591f, -45195.266f, -61377.84f },
};

Vec rightCourseBoundary[70] = {
    { 497.979f, 49.997f, 0.0f },
    { 501.235f, -2841.251f, -5007.79f },
    { 497.983f, -7170.06f, -12505.5f },
    { 479.876f, -8314.231f, -14487.27f },
    { 479.899f, -8761.908f, -15262.67f },
    { 486.837f, -9195.703f, -16014.02f },
    { 500.736f, -9629.499f, -16765.38f },
    { 521.552f, -10063.294f, -17516.73f },
    { 524.121f, -10495.727f, -18265.73f },
    { 503.13f, -10936.027f, -19028.35f },
    { 446.486f, -11379.887f, -19797.14f },
    { 364.665f, -11685.236f, -20326.02f },
    { 245.082f, -11987.438f, -20849.45f },
    { 18.48f, -12434.451f, -21623.7f },
    { -252.156f, -12875.162f, -22387.03f },
    { -497.617f, -13300.134f, -23123.1f },
    { -655.129f, -13580.305f, -23608.37f },
    { -768.419f, -13847.879f, -24071.83f },
    { -843.945f, -14131.193f, -24562.54f },
    { -875.687f, -14393.528f, -25016.92f },
    { -850.353f, -14653.739f, -25467.62f },
    { -768.533f, -14927.61f, -25941.98f },
    { -642.655f, -15201.48f, -26416.33f },
    { -428.664f, -15620.156f, -27141.5f },
    { -170.569f, -16051.424f, -27888.48f },
    { 188.182f, -16636.941f, -28902.62f },
    { 408.467f, -17083.947f, -29676.86f },
    { 572.186f, -17512.059f, -30418.37f },
    { 786.178f, -17940.18f, -31159.9f },
    { 1075.696f, -18525.695f, -32174.04f },
    { 1245.631f, -18972.703f, -32948.28f },
    { 1315.06f, -19274.898f, -33471.7f },
    { 1327.648f, -19592.842f, -34022.39f },
    { 1283.591f, -19907.637f, -34567.64f },
    { 1157.714f, -20235.021f, -35134.68f },
    { 887.077f, -20562.406f, -35701.73f },
    { 496.857f, -20883.496f, -36257.87f },
    { 31.029f, -21188.842f, -36786.75f },
    { -491.693f, -21487.889f, -37304.71f },
    { -1517.594f, -22057.664f, -38291.59f },
    { -1958.808f, -22331.527f, -38765.94f },
    { -2323.853f, -22595.953f, -39223.93f },
    { -2588.195f, -22850.938f, -39665.58f },
    { -2651.134f, -22964.264f, -39861.87f },
    { -2695.42f, -23074.441f, -40052.7f },
    { -2714.302f, -23187.768f, -40248.99f },
    { -2689.126f, -23316.832f, -40472.53f },
    { -2594.718f, -23552.928f, -40881.46f },
    { -2393.337f, -23814.195f, -41334.0f },
    { -2116.406f, -24081.771f, -41797.45f },
    { -1782.831f, -24371.38f, -42299.07f },
    { -1059.036f, -24941.158f, -43285.95f },
    { -561.404f, -25391.299f, -44065.62f },
    { -290.767f, -25709.24f, -44616.31f },
    { -108.245f, -26024.033f, -45161.55f },
    { 23.926f, -26323.088f, -45679.52f },
    { 130.922f, -26625.29f, -46202.95f },
    { 212.947f, -26918.023f, -46709.98f },
    { 275.886f, -27213.93f, -47222.51f },
    { 376.588f, -27941.104f, -48482.01f },
    { 452.187f, -28658.824f, -49725.14f },
    { 508.832f, -29241.193f, -50733.83f },
    { 521.5f, -29814.105f, -51726.14f },
    { 515.985f, -30433.059f, -52798.2f },
    { 503.81f, -34617.785f, -60046.38f },
    { 495.709f, -34657.49f, -60097.48f },
    { 499.768f, -34719.64f, -60148.64f },
    { 499.752f, -34803.92f, -60188.25f },
    { 499.754f, -34968.48f, -60230.3f },
    { 499.75f, -45193.69f, -61374.92f },
};

Float playerLaneOffsets[4] = { -300.0f, -100.0f, 100.0f, 300.0f };

char *playerModelHookNames[4] = {
    "test11_tex_we-itemhook-l",
    "test11_tex_we-itemhook-r",
    "test11_tex_we-itemhook-fl",
    "test11_tex_we-itemhook-fr",
};

s32 playerMotionAssets[10] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x37),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x55),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x39),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x54),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x41),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x56),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x50),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x16),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x53),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x23),
};

s32 characterMotionTable[8][4] = {
    { DATA_MAKE_NUM(DATADIR_M406, 0), DATA_MAKE_NUM(DATADIR_M406, 1), DATA_MAKE_NUM(DATADIR_M406, 16), DATA_MAKE_NUM(DATADIR_M406, 17) },
    { DATA_MAKE_NUM(DATADIR_M406, 2), DATA_MAKE_NUM(DATADIR_M406, 3), DATA_MAKE_NUM(DATADIR_M406, 18), DATA_MAKE_NUM(DATADIR_M406, 19) },
    { DATA_MAKE_NUM(DATADIR_M406, 4), DATA_MAKE_NUM(DATADIR_M406, 5), DATA_MAKE_NUM(DATADIR_M406, 20), DATA_MAKE_NUM(DATADIR_M406, 21) },
    { DATA_MAKE_NUM(DATADIR_M406, 6), DATA_MAKE_NUM(DATADIR_M406, 7), DATA_MAKE_NUM(DATADIR_M406, 22), DATA_MAKE_NUM(DATADIR_M406, 23) },
    { DATA_MAKE_NUM(DATADIR_M406, 8), DATA_MAKE_NUM(DATADIR_M406, 9), DATA_MAKE_NUM(DATADIR_M406, 24), DATA_MAKE_NUM(DATADIR_M406, 25) },
    { DATA_MAKE_NUM(DATADIR_M406, 10), DATA_MAKE_NUM(DATADIR_M406, 11), DATA_MAKE_NUM(DATADIR_M406, 26), DATA_MAKE_NUM(DATADIR_M406, 27) },
    { DATA_MAKE_NUM(DATADIR_M406, 12), DATA_MAKE_NUM(DATADIR_M406, 13), DATA_MAKE_NUM(DATADIR_M406, 28), DATA_MAKE_NUM(DATADIR_M406, 29) },
    { DATA_MAKE_NUM(DATADIR_M406, 14), DATA_MAKE_NUM(DATADIR_M406, 15), DATA_MAKE_NUM(DATADIR_M406, 30), DATA_MAKE_NUM(DATADIR_M406, 31) },
};

s32 playerModelAssets[8] = {
    DATA_MAKE_NUM(DATADIR_M406, 69),
    DATA_MAKE_NUM(DATADIR_M406, 70),
    DATA_MAKE_NUM(DATADIR_M406, 71),
    DATA_MAKE_NUM(DATADIR_M406, 72),
    DATA_MAKE_NUM(DATADIR_M406, 73),
    DATA_MAKE_NUM(DATADIR_M406, 74),
    DATA_MAKE_NUM(DATADIR_M406, 75),
    DATA_MAKE_NUM(DATADIR_M406, 76),
};

s16 playerMotionSpeeds[4] = { 0, 40, 20, 90 };

OMOBJ *cameraUpdateObject;
s16 playerCount;
s16 playerState;
s16 playerEffectModelIds[6];
s16 playerAnimationIds[6];
s16 finishLineMaterialIds[6];
s32 playerEffectTimer;
HUPROCESS *playerObjectProcess;
s32 cameraPositionHistoryIndex;
Vec cameraPositionHistory[16];
Float cameraHistorySpacing;
s32 playerObjectIds[2];
Vec cameraTargets[2];
s32 cameraTargetIndex;
s32 cameraTargetCount;

void InitializePlayerSystem(HUPROCESS *source)
{
    HSFMATERIAL *material;
    s32 playerModelIndex;
    s32 materialIndex;
    HSFDATA *characterModelData;
    OMOBJ **playerObjects;
    HU3DMODEL *playerModel;
    s16 hookFunctionId;

    playerEffectTimer = -1;
    playerCount = playerState = 0;
    playerObjectProcess = source;
    omMakeGroupEx(source, 0, 4);
    for (playerModelIndex = 0; playerModelIndex < 4; playerModelIndex++) {
        omAddObjEx(source, 0x40, 0xA, 0x32, 0, UpdatePlayerModelTransform);
    }
    hookFunctionId = Hu3DHookFuncCreate(UpdatePlayerOrderAndCollisions);
    Hu3DModelLayerSet(hookFunctionId, 7);
    cameraUpdateObject = omAddObjEx(source, 0x45, 0, 0, -1, ResetCameraPositionHistory);
    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 2);
    for (playerModelIndex = 0; playerModelIndex < 6; playerModelIndex++) {
        playerEffectModelIds[playerModelIndex] = playerAnimationIds[playerModelIndex] = finishLineMaterialIds[playerModelIndex] = -1;
        playerModel = &Hu3DData[(*playerObjects)->mdlId[playerModelIndex]];
        characterModelData = playerModel->hsf;
        for (materialIndex = 0; materialIndex < characterModelData->materialNum; materialIndex++) {
            material = &characterModelData->material[materialIndex];
            if (material->color[0] == 0xFF && material->color[1] == 0 && material->color[2] == 0) {
                playerEffectModelIds[playerModelIndex] = materialIndex;
            }
            if (material->color[0] == 0 && material->color[1] == 0xFF && material->color[2] == 0) {
                finishLineMaterialIds[playerModelIndex] = materialIndex;
            }
            if (material->color[0] == 0 && material->color[1] == 0 && material->color[2] == 0xFF) {
                playerAnimationIds[playerModelIndex] = materialIndex;
            }
        }
    }
}

void CreatePlayerObjects(void)
{
    s32 playerIndex;
    OMOBJ **playerObjects;

    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        DestroyPlayerObject(playerObjects[playerIndex]);
    }
}

void UpdatePlayerModelTransform(OMOBJ *object)
{
    Mtx hookTransform;

    M406PlayerWork *playerWork;
    s32 motionIndex;
    s32 characterId;

    object->data = playerWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M406PlayerWork), HU_MEMNUM_OVL);
    memset(playerWork, 0, sizeof(M406PlayerWork));
    playerWork->playerIndex = playerCount++;
    playerWork->isFinished = 0;
    playerWork->isGrounded = 1;
    playerWork->controllerPort = GWPlayerCfg[playerWork->playerIndex].padIdx;
    characterId = playerWork->characterId = GWPlayerCfg[playerWork->playerIndex].character;
    playerWork->difficulty = GWPlayerCfg[playerWork->playerIndex].diff;
    playerWork->playerGroup = GWPlayerCfg[playerWork->playerIndex].group;
    object->mdlId[0] = CharModelCreate(characterId, 8);
    for (motionIndex = 0; motionIndex < 4; motionIndex++) {
        object->mdlId[motionIndex + 1] = Hu3DModelCreateFile(characterMotionTable[characterId][motionIndex]);
        Hu3DModelHookSet(object->mdlId[0], playerModelHookNames[motionIndex], object->mdlId[motionIndex + 1]);
    }
    Hu3DModelShadowSet(object->mdlId[0]);
    CharEffectLayerSet(Hu3DData[object->mdlId[0]].layerNo + 1);
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M406, 32));
    Hu3DModelHookSet(object->mdlId[0], "test11_tex_we-itemhook-body", object->mdlId[5]);
    Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
    GetPlayerJointTransform(object->mdlId[0], "test11_tex_we-itemhook-body", hookTransform);
    playerWork->modelAnchorX = hookTransform[0][3];
    playerWork->modelAnchorY = hookTransform[1][3];
    playerWork->modelAnchorZ = hookTransform[2][3];
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M406, 33));
    Hu3DModelAttrSet(object->mdlId[6], HU3D_ATTR_DISPOFF);
    object->mdlId[7] = Hu3DModelCreateFile(playerModelAssets[playerWork->characterId]);
    Hu3DModelAttrSet(object->mdlId[7], HU3D_ATTR_DISPOFF);
    for (motionIndex = 0; motionIndex < 10U; motionIndex++) {
        object->mtnId[motionIndex] = CharMotionCreate(playerWork->characterId, playerMotionAssets[motionIndex]);
    }
    playerWork->motionId = -1;
    playerWork->collisionRecoveryTimer = 0;
    playerWork->movementAnglesAndSpeed.y = 0.0f;
    playerWork->movementAnglesAndSpeed.z = 45.0f;
    SetPlayerMotion(object, 0, 1.0f, HU3D_MOTATTR_LOOP);
    object->trans.x = playerLaneOffsets[playerWork->playerIndex];
    object->trans.z = -500.0f;
    object->trans.y = FindPlayerGroundSurface(&object->trans, &playerWork->collisionFaceData);
    ApplyGroundTangentImpulse(playerWork->movementAnglesAndSpeed.x, 50.0f, &playerWork->collisionFaceData, &playerWork->velocity);
    object->rot.x = 30.0f;
    object->rot.y = 180.0f;
    #if VERSION_NTSC
    object->work[3] = playerMotionSpeeds[playerWork->playerIndex];
    #else
    object->work[3] = (playerMotionSpeeds[playerWork->playerIndex]*REFRESH_RATE)/60;
    #endif
    CharModelDataClose(playerWork->characterId);
    playerWork->audioEmitterIds[0] = playerWork->audioEmitterIds[1] = -1;
    playerWork->steeringTarget = 600.0f * ((frand() & 0x1FF) - 256.0f) / 256.0f;
    object->objFunc = UpdatePlayerMovement;
}

Vec playerSpawnPositions[12] = {
    { -100.0f, 0.0f, -1000.0f },
    { -200.0f, 0.0f, -2000.0f },
    { -100.0f, 0.0f, -3000.0f },
    { -50.0f, 0.0f, -1000.0f },
    { 100.0f, 0.0f, -1500.0f },
    { 0.0f, 0.0f, -3000.0f },
    { 100.0f, 0.0f, -1000.0f },
    { -100.0f, 0.0f, -2000.0f },
    { 200.0f, 0.0f, -3000.0f },
    { 200.0f, 0.0f, -1000.0f },
    { 300.0f, 0.0f, -1500.0f },
    { -100.0f, 0.0f, -3000.0f },
};

s32 finishSequenceTimer = (REFRESH_RATE*16)/5;

PlayerPathCoordinates finishLinePathRanges[4] = {
    { -300.0f, -600.0f },
    { -100.0f, -400.0f },
    { 100.0f, -300.0f },
    { 450.0f, -900.0f },
};

PlayerPathCoordinates playerTargetPathRanges[3] = {
    { -320.0f, -700.0f },
    { 250.0f, -1000.0f },
    { 380.0f, -500.0f },
};

void SetPlayerMotion(OMOBJ *object, s32 destination, Float scale, u32 effectColor)
{
    HU3DMODEL *playerModel;
    M406PlayerWork *playerWork;

    playerWork = object->data;
    playerModel = &Hu3DData[object->mdlId[0]];
    if (destination != playerWork->motionId) {
        CharMotionShiftSet(playerWork->characterId, object->mtnId[destination], 0.0f, 10.0f, effectColor);
        playerWork->motionSpeed = -1.0f;
        playerWork->motionId = destination;
        playerWork->motionAttributes = effectColor;
    }
    if (scale != playerWork->motionSpeed) {
        CharMotionSpeedSet(playerWork->characterId, 1.0f);
    }
}

void UpdatePlayerMovement(OMOBJ *object)
{
    Float unusedMovementValue;

    Float targetDeltaX;
    Float targetDeltaZ;
    Float unusedMotionValue2;
    Float unusedMotionValue3;
    Float headingTarget;
    Float distanceToTarget;

    M406PlayerWork *playerWork;
    Vec *targetPosition;

    playerWork = object->data;
    if (playerWork->hasReachedTarget == 0) {
        if (getSceneState() <= 1 && playerWork->audioEmitterIds[0] < 0) {
            playerWork->audioEmitterIds[0] = HuAudFXEmiterPlay(0x53A, &object->trans);
        }
        if (object->work[3] != 0) {
            object->work[3] -= 1;
            return;
        }
        targetPosition = &playerSpawnPositions[object->work[0] + (playerWork->playerIndex * 3)];
        targetDeltaX = targetPosition->x - object->trans.x;
        targetDeltaZ = targetPosition->z - object->trans.z;
        headingTarget = atan2d(-targetDeltaX, -targetDeltaZ);
        playerWork->movementAnglesAndSpeed.x += 0.1f * (headingTarget - playerWork->movementAnglesAndSpeed.x);
        distanceToTarget = sqrtf((targetDeltaX * targetDeltaX) + (targetDeltaZ * targetDeltaZ));
        if (distanceToTarget < 100.0f) {
            object->work[0]++;
            if (object->work[0] > 2) {
                playerWork->hasReachedTarget = 1;
                playerWork->movementAnglesAndSpeed.x = 0.0f;
                object->trans.x = playerLaneOffsets[playerWork->playerIndex];
                object->trans.z = -3000.0f;
                object->trans.y = FindPlayerGroundSurface(&object->trans, &playerWork->collisionFaceData);
                ApplyGroundTangentImpulse(playerWork->movementAnglesAndSpeed.x, 50.0f, &playerWork->collisionFaceData, &playerWork->velocity);
                object->objFunc = UpdatePlayerRunning;
            }
        }
        UpdatePlayerGrounding(object);
        object->trans.x += playerWork->velocity.x;
        object->trans.y += playerWork->velocity.y;
        object->trans.z += playerWork->velocity.z;
        object->trans.y = FindPlayerGroundSurface(&object->trans, &playerWork->collisionFaceData);
        object->rot.x = 30.0f;
        object->rot.y = 180.0f + playerWork->movementAnglesAndSpeed.x;
        HuAudFXEmiterUpDate(playerWork->audioEmitterIds[0], &object->trans);
    }
}

void UpdatePlayerRunning(OMOBJ *object)
{
    Mtx modelPivotTranslation;
    Mtx modelRotation;
    Mtx modelTransform;
    Vec projectedPosition;
    Vec collisionImpulse;

    Float motionSpeedScale;
    Float effectModelScale;
    Float desiredHeading;
    double unusedMotionIntermediate;

    M406PlayerWork *playerWork;
    u16 motionId;
    HU3DMODEL *playerModel;
    u16 motionAttributes;
    s32 emitterIndex;
    s16 cameraRelativeStickX;
    s16 cameraRelativeStickY;

    playerWork = object->data;
    motionId = 0xFFFF;
    motionAttributes = 0;
    playerModel = &Hu3DData[object->mdlId[0]];

    if (getSceneState() >= 2) {
        if (playerWork->isFinished != 0) {
            SetPlayerMotion(object, 9, 1.0f, HU3D_MOTATTR_NONE);
            if ((playerModel->attr & 1) == 0) {
                Hu3D3Dto2D(&object->trans, 1, &projectedPosition);
                if (projectedPosition.x < 0.0 || projectedPosition.x >= 576.0f || projectedPosition.y < 0.0 || projectedPosition.y >= 480.0f) {
                    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
                }
            }
            return;
        }
        if (getSceneState() != 4) {
            playerWork->buttonHold = playerWork->buttonPressed = 0;
            playerWork->stickX = playerWork->stickY = 0;
        }
        else if (!GWPlayerCfg[playerWork->playerIndex].iscom) {
            playerWork->stickX = HuPadStkX[playerWork->controllerPort];
            playerWork->stickY = HuPadStkY[playerWork->controllerPort];
            playerWork->buttonHold = HuPadBtn[playerWork->controllerPort];
            playerWork->buttonPressed = HuPadBtnDown[playerWork->controllerPort];
        }
        else {
            playerWork->buttonHold = playerWork->buttonPressed = 0;
            playerWork->stickX = playerWork->stickY = 0;
            MovePlayerTowardTarget(object);
        }
        switch (playerWork->movementState) {
            case 0:
                cameraRelativeStickX = (playerWork->stickX * cosd(CRot.y)) - playerWork->stickY * sind(CRot.y);
                cameraRelativeStickY = (playerWork->stickX * sind(CRot.y)) + playerWork->stickY * cosd(CRot.y);
                if (((cameraRelativeStickX * cameraRelativeStickX) + (cameraRelativeStickY * cameraRelativeStickY)) > 16.0f) {
                    desiredHeading = 180.0 + (atan2d(cameraRelativeStickX, -cameraRelativeStickY));
                    playerWork->movementAnglesAndSpeed.x = ApproachAngle(playerWork->movementAnglesAndSpeed.x, desiredHeading, 0.05f);
                    if (playerWork->movementAnglesAndSpeed.x >= 180.0f) {
                        playerWork->movementAnglesAndSpeed.x -= 360.0f;
                    }
                    if (playerWork->movementAnglesAndSpeed.x < -60.0f) {
                        playerWork->movementAnglesAndSpeed.x = -60.0f;
                    }
                    else if (playerWork->movementAnglesAndSpeed.x > 60.0f) {
                        playerWork->movementAnglesAndSpeed.x = 60.0f;
                    }
                }
                motionId = 0;
                motionSpeedScale = 1.0f;
                motionAttributes = 1;
                if (playerWork->isHit == 0 && playerWork->collisionRecoveryTimer == 0 && playerWork->buttonPressed & 0x100) {
                    playerWork->collisionRecoveryTimer = 0x1E;
                    playerWork->motionId = -1;
                    playerWork->collisionSoundTimer = 0;
                }
                if (playerWork->collisionRecoveryTimer != 0) {
                    motionId = 1;
                    motionSpeedScale = 1.0f;
                    motionAttributes = 0;
                    playerWork->collisionRecoveryTimer--;
                    if (playerWork->collisionRecoveryTimer == 5) {
                        HuAudFXPlay(0x540);
                        ApplyGroundTangentImpulse(playerWork->movementAnglesAndSpeed.x, 10.0f, &playerWork->collisionFaceData, &collisionImpulse);
                        VECAdd(&playerWork->velocity, &collisionImpulse, &playerWork->velocity);
                    }
                }
                if (playerWork->isHit == 0) {
                    if (playerWork->collisionFaceData.isFinishLineFace == 1) {
                        playerWork->isHit = 1;
                        playerWork->hitRecoveryTimer = 0x14;
                        VECScale(&playerWork->velocity, &playerWork->velocity, 1.5f);
                        if (VECMag(&playerWork->velocity) > 50.0f) {
                            VECNormalize(&playerWork->velocity, &playerWork->velocity);
                            VECScale(&playerWork->velocity, &playerWork->velocity, 50.0f);
                        }
                        HuAudFXPlay(0x53E);
                        HuAudFXVolSet(playerWork->audioEmitterIds[0], 0);
                        CharFXPlay(playerWork->characterId, 0x125);
                    }
                    if (getSceneState() == 4) {
                        playerWork->collisionSoundTimer++;
                        if (playerWork->collisionSoundTimer > (REFRESH_RATE*4.0f)) {
                            playerWork->collisionSoundTimer = REFRESH_RATE*4;
                        }
                    }
                }
                else {
                    motionId = 4;
                    motionSpeedScale = 2.0f;
                    motionAttributes = 0;
                    if (playerWork->hitRecoveryTimer == 0 && playerWork->isGrounded != 0) {
                        playerWork->isHit = 0;
                        HuAudFXPlay(0x53D);
                        HuAudFXVolSet(playerWork->audioEmitterIds[0], 127);
                    }
                    if (playerWork->hitRecoveryTimer != 0) {
                        playerWork->hitRecoveryTimer--;
                    }
                }
                if (IsPlayerNearby(object) != 0) {
                    playerWork->isStunned = 1;
                    playerWork->movementState = 1;
                    playerWork->stateTimer = 10;
                    playerWork->movementSpeedScale = 1.0f;
                    playerWork->steeringTarget = 0.0f;
                    playerWork->collisionSoundTimer = 0;
                    ApplyGroundTangentImpulse(playerWork->movementAnglesAndSpeed.x, 50.0f, &playerWork->collisionFaceData, &playerWork->velocity);
                    omVibrate(playerWork->playerIndex, 12, 6, 6);
                    playerWork->audioEmitterIds[1] = HuAudFXPlay(0x53C);
                }
                break;
            case 1:
                motionId = 6;
                motionSpeedScale = 1.0f;
                motionAttributes = 0;
                if (--playerWork->stateTimer == 0) {
                    playerWork->movementState = 2;
                    playerWork->stateTimer = 60;
                }
                break;
            case 2:
                Hu3DModelAttrReset(object->mdlId[5], HU3D_ATTR_DISPOFF);
                if (playerWork->stateTimer < 20) {
                    playerWork->movementSpeedScale = playerWork->stateTimer / 20.0f;
                }
                else {
                    playerWork->movementSpeedScale = 1.0f;
                }
                effectModelScale = (0x3C - playerWork->stateTimer) / 10.0f;
                if (effectModelScale > 1.0f) {
                    effectModelScale = 1.0f;
                }
                Hu3DModelScaleSet(object->mdlId[5], effectModelScale, effectModelScale, effectModelScale);
                playerWork->spinAngle += VECMag(&playerWork->velocity) / 3.125f;
                MTXTrans(modelPivotTranslation, -playerWork->modelAnchorX, -playerWork->modelAnchorY, -playerWork->modelAnchorZ);
                MTXRotRad(modelRotation, 0x78, 0.017453292f * playerWork->spinAngle);
                MTXConcat(modelRotation, modelPivotTranslation, modelTransform);
                MTXTrans(modelPivotTranslation, playerWork->modelAnchorX, playerWork->modelAnchorY, playerWork->modelAnchorZ);
                MTXConcat(modelPivotTranslation, modelTransform, playerModel->mtx);
                motionId = 6;
                motionSpeedScale = 2.0f;
                motionAttributes = 0;
                if (--playerWork->stateTimer == 0) {
                    playerWork->movementState = 3;
                    if (playerWork->audioEmitterIds[1] >= 0) {
                        HuAudFXStop(playerWork->audioEmitterIds[1]);
                        playerWork->audioEmitterIds[1] = -1;
                    }
                }
                break;
            case 3:
                if (playerWork->shakeTimer == 0 && playerWork->buttonPressed & 0x100) {
                    playerWork->shakeTimer = 8;
                }
                if (playerWork->isGrounded) {
                    playerWork->velocity.x = playerWork->velocity.y = playerWork->velocity.z = 0.0f;
                }
                playerWork->spinAngle = ApproachAngle(playerWork->spinAngle, 0.0f, 0.5f);
                MTXTrans(modelPivotTranslation, -playerWork->modelAnchorX, -playerWork->modelAnchorY, -playerWork->modelAnchorZ);
                MTXRotRad(modelRotation, 0x78, 0.017453292f * playerWork->spinAngle);
                MTXConcat(modelRotation, modelPivotTranslation, modelTransform);
                MTXTrans(modelPivotTranslation, playerWork->modelAnchorX, playerWork->modelAnchorY, playerWork->modelAnchorZ);
                MTXConcat(modelPivotTranslation, modelTransform, playerModel->mtx);
                if (playerWork->shakeTimer != 0) {
                    MTXTrans(modelPivotTranslation, 100.0f * (0.2f * (((playerWork->shakeTimer & 1) * 2) - 1)), 0.0f, 0.0f);
                    MTXConcat(modelPivotTranslation, playerModel->mtx, playerModel->mtx);
                    if (--playerWork->shakeTimer == 0) {
                        if (++playerWork->shakeCount >= 4) {
                            playerWork->isStunned = 0;
                            playerWork->movementState = 0;
                            playerWork->shakeCount = 0;
                            Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
                            Hu3DData[object->mdlId[6]].motWork.time = 0.0f;
                            Hu3DModelPosSet(object->mdlId[6], object->trans.x + playerWork->modelAnchorX, object->trans.y + playerWork->modelAnchorY,
                                object->trans.z + playerWork->modelAnchorZ);
                            Hu3DModelAttrReset(object->mdlId[6], HU3D_ATTR_DISPOFF);
                            MTXIdentity(playerModel->mtx);
                            ApplyGroundTangentImpulse(playerWork->movementAnglesAndSpeed.x, 25.0f, &playerWork->collisionFaceData, &playerWork->velocity);
                        }
                    }
                }
                motionId = 7;
                motionSpeedScale = 1.0f;
                motionAttributes = 1;
                break;
        }
        if (Hu3DMotionEndCheck(object->mdlId[6])) {
            Hu3DModelAttrSet(object->mdlId[6], HU3D_ATTR_DISPOFF);
        }
        SetPlayerMotion(object, motionId, motionSpeedScale, motionAttributes);
        UpdatePlayerCollision(object);
        if (object->trans.z + playerWork->velocity.z < -60000.0f) {
            playerEffectTimer = playerWork->playerIndex;
            return;
        }
        projectedPosition.x = object->trans.x;
        projectedPosition.y = object->trans.y;
        projectedPosition.z = object->trans.z;
        if (getPathLateralOffset(projectedPosition) == 1) {
            playerWork->isFinished = 1;
            playerModel = &Hu3DData[object->mdlId[0]];
            MTXIdentity(playerModel->mtx);
            omVibrate(playerWork->playerIndex, 0xC, 4, 2);
            CharFXPlayPos(playerWork->characterId, 0x123, &object->trans);
            for (emitterIndex = 0; emitterIndex < 2; emitterIndex++) {
                if (playerWork->audioEmitterIds[emitterIndex] >= 0) {
                    HuAudFXStop(playerWork->audioEmitterIds[emitterIndex]);
                    playerWork->audioEmitterIds[emitterIndex] = -1;
                }
            }
        }
        object->rot.y = 180.0f + playerWork->movementAnglesAndSpeed.x;
    }
}

void UpdatePlayerCollision(OMOBJ *object)
{
    Mtx playerCollisionTransform;
    Mtx collisionOffsetTransform;
    Vec gravityAcceleration;
    Vec groundNormalAcceleration;
    Vec groundCollisionAcceleration;
    Vec opposingVelocity;
    Vec collisionReflectionVelocity;
    Vec velocityDirection;

    Float collisionVelocityDot;
    Float groundResponseDot;
    Float playerSpeed;
    Float groundResponseScale;
    Float collisionResponseScale;

    M406PlayerWork *playerWork;
    HU3DMODEL *playerModel;

    playerWork = object->data;
    playerModel = &Hu3DData[object->mdlId[0]];
    if (playerWork->isGrounded && playerWork->collisionFaceData.groundFace != NULL) {
        collisionResponseScale = !playerWork->isStunned ? 0.7f : 3.5f;
        gravityAcceleration.x = 0.0f;
        gravityAcceleration.y = -((980.0f/REFRESH_RATE)/REFRESH_RATE);
        gravityAcceleration.z = 0.0f;
        groundResponseScale = ((980.0f/REFRESH_RATE)/REFRESH_RATE) * playerWork->collisionFaceData.groundFace->nbt.y;
        VECScale(&playerWork->collisionFaceData.groundFace->nbt, &groundNormalAcceleration, groundResponseScale);
        VECAdd(&gravityAcceleration, &groundNormalAcceleration, &groundCollisionAcceleration);
        VECAdd(&playerWork->velocity, &groundCollisionAcceleration, &playerWork->velocity);
        opposingVelocity.x = opposingVelocity.y = opposingVelocity.z = 0.0f;
        collisionReflectionVelocity.x = collisionReflectionVelocity.y = collisionReflectionVelocity.z = 0.0f;
        playerSpeed = VECMag(&playerWork->velocity);
        if (playerSpeed > 0.0f && getSceneState() == 4) {
            VECNormalize(&playerWork->velocity, &velocityDirection);
            opposingVelocity.x = -velocityDirection.x;
            opposingVelocity.y = -velocityDirection.y;
            opposingVelocity.z = -velocityDirection.z;
            if (playerWork->collisionSoundTimer < (REFRESH_RATE*2.0f)) {
                VECScale(&opposingVelocity, &opposingVelocity, groundResponseScale * collisionResponseScale);
            }
            else {
                VECScale(&opposingVelocity, &opposingVelocity, (0.15f * (playerWork->collisionSoundTimer / (REFRESH_RATE*4.0f))) + (groundResponseScale * collisionResponseScale));
            }
            if (!playerWork->isStunned) {
                ApplyGroundTangentImpulse(playerWork->movementAnglesAndSpeed.x, 1.0f, &playerWork->collisionFaceData, &gravityAcceleration);
                groundResponseDot = VECDotProduct(&gravityAcceleration, &velocityDirection);
                collisionVelocityDot = 0.02f * playerSpeed;
                collisionReflectionVelocity.x = (-velocityDirection.x * collisionVelocityDot) - (groundResponseDot * (-gravityAcceleration.x * collisionVelocityDot));
                collisionReflectionVelocity.y = (-velocityDirection.y * collisionVelocityDot) - (groundResponseDot * (-gravityAcceleration.y * collisionVelocityDot));
                collisionReflectionVelocity.z = (-velocityDirection.z * collisionVelocityDot) - (groundResponseDot * (-gravityAcceleration.z * collisionVelocityDot));
                if ((groundResponseDot < 0.9f) && (playerSpeed >= 20.0f)) {
                    mtxRot(playerCollisionTransform, playerModel->rot.x, playerModel->rot.y, playerModel->rot.z);
                    mtxTransCat(playerCollisionTransform, playerModel->pos.x, playerModel->pos.y, playerModel->pos.z);
                    VECCrossProduct(&gravityAcceleration, &velocityDirection, &gravityAcceleration);
                    if (gravityAcceleration.y >= 0.0f) {
                        MTXTrans(collisionOffsetTransform, -10.0f, 0.0f, 0.0f);
                        MTXConcat(collisionOffsetTransform, playerCollisionTransform, playerCollisionTransform);
                        SetEffectTransform(playerCollisionTransform, 10, 0);
                    }
                    else {
                        MTXTrans(collisionOffsetTransform, 10.0f, 0.0f, 0.0f);
                        MTXConcat(collisionOffsetTransform, playerCollisionTransform, playerCollisionTransform);
                        SetEffectTransform(playerCollisionTransform, 10, 1);
                    }
                }
            }
        }
        VECAdd(&opposingVelocity, &collisionReflectionVelocity, &groundCollisionAcceleration);
        if (VECMag(&groundCollisionAcceleration) > playerSpeed) {
            playerWork->velocity.x = playerWork->velocity.y = playerWork->velocity.z = 0.0f;
            return;
        }
        VECAdd(&playerWork->velocity, &groundCollisionAcceleration, &playerWork->velocity);
        return;
    }
    playerWork->velocity.y += -((980.0f/REFRESH_RATE)/REFRESH_RATE);
}

void UpdatePlayerGrounding(OMOBJ *object)
{
    Mtx playerCollisionTransform;
    Mtx collisionOffsetTransform;
    Vec groundPlaneVelocity;
    Vec groundTangent;
    Vec velocityDirection;
    Vec collisionAxis;

    Float alignmentFactor;
    Float groundNormalVelocity;
    Float groundAlignment;
    Float groundSpeed;

    M406PlayerWork *playerWork;
    HU3DMODEL *playerModel;

    playerWork = object->data;
    playerModel = &Hu3DData[object->mdlId[0]];
    playerWork->velocity.y += -1.0f;
    if (playerWork->isGrounded && playerWork->collisionFaceData.groundFace != NULL) {
        groundPlaneVelocity.x = playerWork->velocity.x;
        groundPlaneVelocity.y = playerWork->velocity.y;
        groundPlaneVelocity.z = playerWork->velocity.z;

        groundNormalVelocity = -((playerWork->collisionFaceData.groundFace->nbt.z * groundPlaneVelocity.z) + ((playerWork->collisionFaceData.groundFace->nbt.x * groundPlaneVelocity.x) + (playerWork->collisionFaceData.groundFace->nbt.y * groundPlaneVelocity.y)));

        groundPlaneVelocity.x += groundNormalVelocity * playerWork->collisionFaceData.groundFace->nbt.x;
        groundPlaneVelocity.y += groundNormalVelocity * playerWork->collisionFaceData.groundFace->nbt.y;
        groundPlaneVelocity.z += groundNormalVelocity * playerWork->collisionFaceData.groundFace->nbt.z;
        if (!playerWork->isStunned) {
            ApplyGroundTangentImpulse(playerWork->movementAnglesAndSpeed.x, 1.0f, &playerWork->collisionFaceData, &groundTangent);
            VECNormalize(&groundPlaneVelocity, &velocityDirection);
            groundAlignment = VECDotProduct(&groundTangent, &velocityDirection);
            alignmentFactor = groundAlignment * groundAlignment;
            alignmentFactor *= alignmentFactor;
            if (alignmentFactor < 0.999f && VECMag(&playerWork->velocity) >= 10.0f) {
                mtxRot(playerCollisionTransform, playerModel->rot.x, playerModel->rot.y, playerModel->rot.z);
                mtxTransCat(playerCollisionTransform, playerModel->pos.x, playerModel->pos.y, playerModel->pos.z);
                VECCrossProduct(&groundTangent, &velocityDirection, &collisionAxis);
                if (collisionAxis.y >= 0.0f) {
                    MTXTrans(collisionOffsetTransform, -10.0f, 0.0f, 0.0f);
                    MTXConcat(collisionOffsetTransform, playerCollisionTransform, playerCollisionTransform);
                    SetEffectTransform(playerCollisionTransform, 10, 0);
                }
                else {
                    MTXTrans(collisionOffsetTransform, 10.0f, 0.0f, 0.0f);
                    MTXConcat(collisionOffsetTransform, playerCollisionTransform, playerCollisionTransform);
                    SetEffectTransform(playerCollisionTransform, 10, 1);
                }
            }
            groundSpeed = VECMag(&groundPlaneVelocity);
            VECScale(&groundTangent, &groundTangent, groundSpeed);
            playerWork->velocity.x = groundPlaneVelocity.x + (0.9f * (groundTangent.x - groundPlaneVelocity.x));
            playerWork->velocity.y = groundPlaneVelocity.y + (0.9f * (groundTangent.y - groundPlaneVelocity.y));
            playerWork->velocity.z = groundPlaneVelocity.z + (0.9f * (groundTangent.z - groundPlaneVelocity.z));
        }
        else {
            playerWork->velocity.x = groundPlaneVelocity.x;
            playerWork->velocity.y = groundPlaneVelocity.y;
            playerWork->velocity.z = groundPlaneVelocity.z;
            VECScale(&playerWork->velocity, &playerWork->velocity, playerWork->movementSpeedScale);
        }
        VECScale(&playerWork->velocity, &playerWork->velocity, 0.98f);
    }
}

void UpdatePlayerOrderAndCollisions(HU3DMODEL *model, Mtx mtx)
{
    OMOBJ *playersByDepth[4];
    Vec surfaceNormalOrPlayerDelta;
    Vec normalizedVelocity;
    Vec collisionTestPosition;

    Float remainingMovementDistance;
    Float playerDepthGap;
    Float spacingDistance;
    Float collisionImpulseScale;
    Float targetSpeedLimit;
    Float collisionNormalDot;
    Float groundHeight;

    M406PlayerWork *playerWork;
    OMOBJ *playerObject;
    s32 playerIndex;
    OMOBJ **playerObjects;
    s32 activePlayerCount;
    s32 otherPlayerIndex;
    s32 playersAheadCount;
    OMOBJ *currentPlayerObject;
    OMOBJ *otherPlayerObject;
    M406PlayerWork *currentPlayerWork;
    M406PlayerWork *otherPlayerWork;

    if (omPauseChk() == 0) {
        playerObjects = omGetGroupMemberListEx(playerObjectProcess, 0);
        switch (cameraTargetCount) {
            case 0:
                if (--finishSequenceTimer == 0) {
                    BeginCameraTransition();
                    cameraTargetCount++;
                    playerObjectIds[0] = playerObjectIds[1] = 0;
                }
                if ((finishSequenceTimer % 6) == 0) {
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        playerWork = playerObjects[playerIndex]->data;
                        omVibrate(playerWork->playerIndex, 6, 2, 4);
                    }
                    return;
                }
                break;
            case 1:
                if (playerEffectTimer >= 0) {
                    SetSelectedPlayer(playerEffectTimer);
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        playerObject = playerObjects[playerIndex];
                        playerWork = playerObject->data;
                        if (playerIndex == playerEffectTimer) {
                            playerObject->objFunc = InitializePlayerMovement;
                        }
                        else {
                            playerObject->objFunc = InitializePlayerFinish;
                        }
                        Hu3DModelAttrSet(*playerObject->mdlId, HU3D_ATTR_DISPOFF);
                        for (otherPlayerIndex = 0; otherPlayerIndex < 2; otherPlayerIndex++) {
                            if (playerWork->audioEmitterIds[otherPlayerIndex] >= 0) {
                                HuAudFXStop(playerWork->audioEmitterIds[otherPlayerIndex]);
                                playerWork->audioEmitterIds[otherPlayerIndex] = -1;
                            }
                        }
                    }
                    cameraTargetCount++;
                    return;
                }
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    playerWork = playerObjects[playerIndex]->data;
                    if (!playerWork->isFinished) {
                        break;
                    }
                }
                if (playerIndex >= 4) {
                    SetSelectedPlayer(0x63);
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        playerObjects[playerIndex]->objFunc = InitializePlayerFinish;
                    }
                    cameraTargetCount++;
                    return;
                }
                WaitForPlayersToSeparate();
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    playerObject = playerObjects[playerIndex];
                    playerWork = playerObjects[playerIndex]->data;
                    if (!playerWork->isFinished) {
                        playerWork->movementAnglesAndSpeed.y = VECMag(&playerWork->velocity);
                        if (!playerWork->isStunned && !playerWork->isHit && (playerWork->movementAnglesAndSpeed.y > playerWork->movementAnglesAndSpeed.z)) {
                            playerWork->movementAnglesAndSpeed.y = playerWork->movementAnglesAndSpeed.z;
                            VECNormalize(&playerWork->velocity, &playerWork->velocity);
                            VECScale(&playerWork->velocity, &playerWork->velocity, playerWork->movementAnglesAndSpeed.y);
                        }
                        remainingMovementDistance = playerWork->movementAnglesAndSpeed.y;
                        VECNormalize(&playerWork->velocity, &normalizedVelocity);

                        while (remainingMovementDistance > 0.0f) {
                            if (remainingMovementDistance < 50.0f) {
                                playerObject->trans.x += normalizedVelocity.x * remainingMovementDistance;
                                playerObject->trans.y += normalizedVelocity.y * remainingMovementDistance;
                                playerObject->trans.z += normalizedVelocity.z * remainingMovementDistance;
                            }
                            else {
                                playerObject->trans.x += 50.0f * normalizedVelocity.x;
                                playerObject->trans.y += 50.0f * normalizedVelocity.y;
                                playerObject->trans.z += 50.0f * normalizedVelocity.z;
                            }
                            collisionTestPosition.x = playerObject->trans.x;
                            collisionTestPosition.y = 10.0f + playerObject->trans.y;
                            collisionTestPosition.z = playerObject->trans.z;
                            if (ResolvePlayerSurfaceCollision(&collisionTestPosition, &surfaceNormalOrPlayerDelta)) {
                                playerObject->trans.x = collisionTestPosition.x;
                                playerObject->trans.z = collisionTestPosition.z;
                                if (playerWork->isHit == 0) {
                                    VECNormalize(&playerWork->velocity, &normalizedVelocity);
                                    collisionNormalDot = VECDotProduct(&normalizedVelocity, &surfaceNormalOrPlayerDelta);
                                    if (collisionNormalDot < -0.2f && playerWork->movementAnglesAndSpeed.y >= 10.0f) {
                                        omVibrate(playerWork->playerIndex, 12, 6, 6);
                                    }
                                    if (collisionNormalDot < 0.0f) {
                                        collisionImpulseScale
                                            = 0.8f * (((-playerWork->velocity.x * surfaceNormalOrPlayerDelta.x) - (playerWork->velocity.y * surfaceNormalOrPlayerDelta.y)) - (playerWork->velocity.z * surfaceNormalOrPlayerDelta.z));
                                        playerWork->velocity.x += (2.0f * surfaceNormalOrPlayerDelta.x * collisionImpulseScale);
                                        playerWork->velocity.y += (2.0f * surfaceNormalOrPlayerDelta.y * collisionImpulseScale);
                                        playerWork->velocity.z += (2.0f * surfaceNormalOrPlayerDelta.z * collisionImpulseScale);
                                    }
                                }
                            }
                            remainingMovementDistance -= 50.0f;
                        }
                        groundHeight = FindPlayerGroundSurface(&playerObject->trans, &playerWork->collisionFaceData);
                        if (playerWork->collisionFaceData.groundFace != NULL && playerObject->trans.y < (0.01f + groundHeight)) {
                            playerObject->trans.y = groundHeight;
                            playerWork->isGrounded = 1;
                        }
                        else {
                            playerWork->isGrounded = 0;
                        }
                        RecordCameraPosition(&playerObject->trans);
                        if (playerWork->audioEmitterIds[0] >= 0) {
                            HuAudFXEmiterUpDate(playerWork->audioEmitterIds[0], &playerObject->trans);
                        }
                    }
                }
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    playersByDepth[playerIndex] = NULL;
                }
                for (activePlayerCount = 0, playerIndex = 0; playerIndex < 4; playerIndex++) {
                    currentPlayerObject = playerObjects[playerIndex];
                    currentPlayerWork = currentPlayerObject->data;
                    if (!currentPlayerWork->isFinished) {
                        for (playersAheadCount = 0, otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                            otherPlayerObject = playerObjects[otherPlayerIndex];
                            otherPlayerWork = otherPlayerObject->data;
                            if (currentPlayerObject != otherPlayerObject && (!otherPlayerWork->isFinished && currentPlayerObject->trans.z > otherPlayerObject->trans.z)) {
                                playersAheadCount++;
                            }
                        }
                        while (playersByDepth[playersAheadCount] != NULL) {
                            playersAheadCount++;
                        }
                        playersByDepth[playersAheadCount] = currentPlayerObject;
                        activePlayerCount++;
                    }
                }
                if (activePlayerCount != 0) {
                    spacingDistance = GetPlayerPathDistance(&playersByDepth[0]->trans) - 1500.0f;
                    if (spacingDistance >= 0.0f) {
                        SetMapPathSpeed((1800.0001f/REFRESH_RATE) + (0.005f * spacingDistance));
                    }
                    else {
                        SetMapPathSpeed((1800.0001f/REFRESH_RATE));
                    }
                    cameraTargets[0].x = playersByDepth[0]->trans.x;
                    cameraTargets[0].y = playersByDepth[0]->trans.y;
                    cameraTargets[0].z = playersByDepth[0]->trans.z - 1000.0f;
                    RecordCameraPosition(&cameraTargets[0]);
                    if (playerObjectIds[1] == 0) {
                        cameraTargets[1].x = playersByDepth[activePlayerCount - 1]->trans.x;
                        cameraTargets[1].y = playersByDepth[activePlayerCount - 1]->trans.y;
                        cameraTargets[1].z = playersByDepth[activePlayerCount - 1]->trans.z;
                        playerObjectIds[1] = 1;
                    }
                    surfaceNormalOrPlayerDelta.x = playersByDepth[activePlayerCount - 1]->trans.x - cameraTargets[1].x;
                    surfaceNormalOrPlayerDelta.y = playersByDepth[activePlayerCount - 1]->trans.y - cameraTargets[1].y;
                    surfaceNormalOrPlayerDelta.z = playersByDepth[activePlayerCount - 1]->trans.z - cameraTargets[1].z;
                    spacingDistance = VECMag(&surfaceNormalOrPlayerDelta);
                    VECNormalize(&surfaceNormalOrPlayerDelta, &surfaceNormalOrPlayerDelta);
                    if (spacingDistance < 100.0f) {
                        cameraTargets[1].x = playersByDepth[activePlayerCount - 1]->trans.x;
                        cameraTargets[1].y = playersByDepth[activePlayerCount - 1]->trans.y;
                        cameraTargets[1].z = playersByDepth[activePlayerCount - 1]->trans.z;
                    }
                    else {
                        cameraTargets[1].x += (100.0f * surfaceNormalOrPlayerDelta.x);
                        cameraTargets[1].y += (100.0f * surfaceNormalOrPlayerDelta.y);
                        cameraTargets[1].z += (100.0f * surfaceNormalOrPlayerDelta.z);
                    }
                    RecordCameraPosition(&cameraTargets[1]);
                    if (getSceneState() == 2 || getSceneState() == 3) {
                        return;
                    }
                    for (playerIndex = 0; playerIndex < activePlayerCount; playerIndex++) {
                        playerObject = playersByDepth[playerIndex];
                        playerWork = playerObject->data;
                        if (!playerWork->isFinished) {
                            playerDepthGap = playersByDepth[activePlayerCount - 1]->trans.z - playerObject->trans.z;
                            if (playerDepthGap > 3000.0f) {
                                if (playerDepthGap > 5000.0f) {
                                    playerDepthGap = 5000.0f;
                                }
                                targetSpeedLimit = 45.0f - (9.0f * (playerDepthGap / 5000.0f));
                            }
                            else {
                                targetSpeedLimit = 45.0f;
                            }
                            playerWork->movementAnglesAndSpeed.z += (0.2f * (targetSpeedLimit - playerWork->movementAnglesAndSpeed.z));
                        }
                    }
                    return;
                }
                break;
            case 2:
                if (getSceneState() == 8 || getSceneState() == 10) {
                    cameraTargetCount++;
                    finishSequenceTimer = (REFRESH_RATE*7)/2;
                    return;
                }
                break;
            case 3:
                if (--finishSequenceTimer == 0) {
                    CompleteSceneTransition();
                }
                break;
        }
    }
}

void InitializePlayerMovement(OMOBJ *object)
{
    M406PlayerWork *playerWork = object->data;

    playerWork->movementState = 0;
    playerWork->statusFlags = 0;
    playerWork->movementSpeedScale = 1.0f;
    playerWork->movementAnglesAndSpeed.x = 0.0f;
    playerWork->effectTimer = REFRESH_RATE;
    object->trans.z = -60000.0f;
    object->rot.x = object->rot.y = object->rot.z = 0.0f;
    playerWork->velocity.x = playerWork->velocity.y = 0.0f;
    playerWork->velocity.z = (-1200.0f/REFRESH_RATE);
    Hu3DModelShadowReset(object->mdlId[0]);
    SpawnPathParticles(&object->trans);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    object->objFunc = UpdatePlayerEffects;
}

void UpdatePlayerEffects(OMOBJ *object)
{
    Mtx playerEffectTransform;
    Mtx effectOffsetTransform;

    Float effectAlpha;

    M406PlayerWork *playerWork;
    HU3DMODEL *playerModel;
    s32 modelPartIndex;

    playerWork = object->data;
    playerModel = &Hu3DData[object->mdlId[0]];
    switch (playerWork->movementState) {
        case 0:
            SetPlayerMotion(object, 0, 1.0f, HU3D_MOTATTR_NONE);
            if (--playerWork->effectTimer == 0) {
                AdvanceCameraTransition();
                playerWork->movementState++;
            }
            if (playerWork->effectTimer == (REFRESH_RATE*0.9f)) {
                HuAudFXPlay(0x541);
            }
            mtxRot(playerEffectTransform, playerModel->rot.x, playerModel->rot.y, playerModel->rot.z);
            mtxTransCat(playerEffectTransform, playerModel->pos.x, playerModel->pos.y, playerModel->pos.z);
            if (playerWork->effectTimer < 0x1E) {
                effectAlpha = 0.0f;
            }
            else {
                effectAlpha = (255.0f * (playerWork->effectTimer - 0x1E)) / 30.0f;
            }
            SetModelEmissionTransform(playerEffectTransform, frand() & 0xF, effectAlpha);
            break;
        case 1:
            if (getSceneState() == 8) {
                playerWork->movementState++;
                playerWork->movementAnglesAndSpeed.x = 180.0f;
                playerWork->effectTimer = (REFRESH_RATE*3)/2;
                #if VERSION_NTSC
                playerWork->velocity.x = 100.0 * (0.1f * sind(180.0f + playerWork->movementAnglesAndSpeed.x));
                playerWork->velocity.y = 0.0f;
                playerWork->velocity.z = 100.0 * (0.1f * cosd(180.0f + playerWork->movementAnglesAndSpeed.x));
                #else
                playerWork->velocity.x = 100.0 * (6.0f * sind(180.0f + playerWork->movementAnglesAndSpeed.x))*(1.0f/REFRESH_RATE);
                playerWork->velocity.y = 0.0f;
                playerWork->velocity.z = 100.0 * (6.0f * cosd(180.0f + playerWork->movementAnglesAndSpeed.x))*(1.0f/REFRESH_RATE);
                #endif
                object->trans.x = 300.0f;
                object->trans.z = -1000.0f;
                object->trans.y = 1000.0f;
                CharModelKill(playerWork->characterId);
                object->mdlId[0] = CharModelCreate(playerWork->characterId, 2);
                for (modelPartIndex = 0; modelPartIndex < 4; modelPartIndex++) {
                    Hu3DModelHookSet(object->mdlId[0], playerModelHookNames[modelPartIndex], object->mdlId[modelPartIndex + 1]);
                }
                for (modelPartIndex = 0; modelPartIndex < 10U; modelPartIndex++) {
                    object->mtnId[modelPartIndex] = CharMotionCreate(playerWork->characterId, playerMotionAssets[modelPartIndex]);
                }
                playerWork->motionId = -1;
                SetPlayerMotion(object, 0, 1.0f, HU3D_MOTATTR_NONE);
                Hu3DModelShadowSet(object->mdlId[0]);
                return;
            }
            break;
        case 2:
            playerWork->velocity.y += (-7200.0f/REFRESH_RATE)*(1.0f/REFRESH_RATE);
            if (object->trans.y < 0.0f) {
                object->trans.y = 0.0f;
                playerWork->movementState++;
                playerWork->velocity.y = 0.0f;
                SetPlayerMotion(object, 2, 1.0f, HU3D_MOTATTR_NONE);
                omVibrate(playerWork->playerIndex, 0xC, 6, 6);
                playerWork->audioEmitterIds[0] = HuAudFXPlay(0x53B);
            }
            break;
        case 3:
            playerWork->movementAnglesAndSpeed.x -= (30.0f*(1.0f/REFRESH_RATE));
            playerWork->movementSpeedScale = 1.0f;
            #if VERSION_NTSC
            if (playerWork->effectTimer < 0x14) {
                playerWork->movementSpeedScale = playerWork->effectTimer / 20.0f;
            }
            if ((playerWork->effectTimer < 0x50) && (playerWork->effectTimer > 0x1E)) {
            #else
            if (playerWork->effectTimer < (REFRESH_RATE/3.0f)) {
                playerWork->movementSpeedScale = playerWork->effectTimer / (REFRESH_RATE/3.0f);
            }
            if ((playerWork->effectTimer < ((REFRESH_RATE*4.0f)/3.0f)) && (playerWork->effectTimer > (REFRESH_RATE*0.5f))) {
            #endif
                mtxRot(playerEffectTransform, playerModel->rot.x, playerModel->rot.y, playerModel->rot.z);
                mtxTransCat(playerEffectTransform, playerModel->pos.x, playerModel->pos.y, playerModel->pos.z);
                MTXTrans(effectOffsetTransform, 10.0f, 0.0f, 0.0f);
                MTXConcat(effectOffsetTransform, playerEffectTransform, playerEffectTransform);
                SetEffectTransform(playerEffectTransform, 10, 0);
            }
            #if VERSION_NTSC
            playerWork->velocity.x = playerWork->movementSpeedScale * (100.0 * (0.1f * sind(180.0f + playerWork->movementAnglesAndSpeed.x)));
            playerWork->velocity.z = playerWork->movementSpeedScale * (100.0 * (0.1f * cosd(180.0f + playerWork->movementAnglesAndSpeed.x)));
            #else
            playerWork->velocity.x = ((1.0f/REFRESH_RATE)*(100.0 * (6.0f * sind(180.0f + playerWork->movementAnglesAndSpeed.x)))*playerWork->movementSpeedScale);
            playerWork->velocity.z = ((1.0f/REFRESH_RATE)*(100.0 * (6.0f * cosd(180.0f + playerWork->movementAnglesAndSpeed.x)))*playerWork->movementSpeedScale);
            #endif
            if (playerWork->effectTimer == (REFRESH_RATE*0.8f)) {
                SetPlayerMotion(object, 8, 1.0f, HU3D_MOTATTR_NONE);
            }
            if (--playerWork->effectTimer == 0) {
                playerWork->movementState = -1;
                playerWork->velocity.x = playerWork->velocity.z = 0.0f;
            }
            break;
        default:
            break;
    }
    object->trans.x += playerWork->velocity.x;
    object->trans.y += playerWork->velocity.y;
    object->trans.z += playerWork->velocity.z;
    object->rot.y = 180.0f + playerWork->movementAnglesAndSpeed.x;
}

void InitializePlayerFinish(OMOBJ *object)
{
    HU3DMODEL *playerModel;

    M406PlayerWork *playerWork;
    void *unusedCharacterModelData;
    s32 modelPartIndex;

    playerWork = object->data;
    if (playerEffectTimer < 0) {
        object->trans.x = finishLinePathRanges[playerState].laneCoordinate;
        object->trans.z = finishLinePathRanges[playerState].segmentProgress;
        playerWork->effectTimer = (REFRESH_RATE*0.5f) + (frand() & 0x1F);
    }
    else {
        object->trans.x = playerTargetPathRanges[playerState].laneCoordinate;
        object->trans.z = playerTargetPathRanges[playerState].segmentProgress;
        playerWork->effectTimer = (REFRESH_RATE*1.5f) + (frand() & 0x1F);
    }
    for (modelPartIndex = 0; modelPartIndex < 8; modelPartIndex++) {
        Hu3DModelAttrSet(object->mdlId[modelPartIndex], HU3D_ATTR_DISPOFF);
        Hu3DModelPosSet(object->mdlId[modelPartIndex], 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(object->mdlId[modelPartIndex], 0.0f, 0.0f, 0.0f);
        Hu3DModelScaleSet(object->mdlId[modelPartIndex], 1.0f, 1.0f, 1.0f);
    }
    Hu3DModelHookReset(object->mdlId[0]);
    SetPlayerMotion(object, 0, 1.0f, HU3D_MOTATTR_NONE);
    playerWork->statusFlags = 0;
    playerWork->movementState = 0;
    playerWork->isStunned = 1;
    playerWork->spinAngle = 360.0f * frandf();
    playerWork->spinAngle = 160.0f + ((frand() & 0x3F) - 0x20);
    playerWork->movementSpeedScale = 30.0f;
    playerWork->stateTimer = 3;
    object->trans.y = 1000.0f;
    object->rot.x = object->rot.y = 0.0f;
    object->rot.z = (frand() & 0x3F) - 0x20;
    playerWork->velocity.x = playerWork->velocity.y = playerWork->velocity.z = 0.0f;
    playerState++;
    object->objFunc = UpdatePlayerSpin;
}

void UpdatePlayerSpin(OMOBJ *object)
{
    Mtx playerModelTransform;
    Mtx spinTransform;
    Mtx accessoryOffsetTransform;
    Mtx accessoryTransform;
    HU3DMODEL *playerModel;
    M406PlayerWork *playerWork;

    playerWork = object->data;
    playerModel = &Hu3DData[object->mdlId[0]];
    switch (playerWork->movementState) {
        case 0:
            if (getSceneState() == 8 || getSceneState() == 10) {
                playerWork->movementState++;
            }
            return;
        case 1:
            if (playerWork->effectTimer != 0) {
                if (--playerWork->effectTimer == 0) {
                    Hu3DModelAttrReset(object->mdlId[5], HU3D_ATTR_DISPOFF);
                    Hu3DModelAttrReset(object->mdlId[7], HU3D_ATTR_DISPOFF);
                }
                else {
                    return;
                }
            }
            playerWork->spinAngle += playerWork->movementSpeedScale;
            playerWork->movementSpeedScale -= 0.5f;
            if (playerWork->movementSpeedScale < 0.0f) {
                playerWork->movementSpeedScale = 0.0f;
            }
            if (playerWork->stateTimer != 0) {
                playerWork->velocity.y += (-7200.0f/REFRESH_RATE)*(1.0f/REFRESH_RATE);
                object->trans.y += playerWork->velocity.y;
                if (object->trans.y < 0.0f) {
                    object->trans.y = 0.0f;
                    playerWork->velocity.y = 0.5f * -playerWork->velocity.y;
                    playerWork->stateTimer--;
                    if (!playerWork->impactSoundPlayed) {
                        omVibrate(playerWork->playerIndex, 12, 4, 2);
                        playerWork->impactSoundPlayed = 1;
                        HuAudFXPlay(0x53F);
                    }
                }
            }
            break;
        default:
            break;
    }
    mtxRot(playerModelTransform, object->rot.x, object->rot.y, object->trans.z);
    mtxScaleCat(playerModelTransform, object->scale.x, object->scale.y, object->scale.z);
    MTXTrans(playerModelTransform, object->trans.x, object->trans.y, object->trans.z);
    MTXRotRad(spinTransform, 0x78, 0.017453292f * playerWork->spinAngle);
    MTXConcat(playerModelTransform, spinTransform, playerModelTransform);
    MTXCopy(playerModelTransform, accessoryTransform);
    mtxScaleCat(accessoryTransform, 2.0f, 2.0f, 2.0f);
    MTXCopy(accessoryTransform, Hu3DData[object->mdlId[5]].mtx);
    MTXTrans(accessoryOffsetTransform, 0.0f, 110.0f, 0.0f);
    MTXConcat(playerModelTransform, accessoryOffsetTransform, accessoryTransform);
    MTXCopy(accessoryTransform, Hu3DData[object->mdlId[7]].mtx);
}

void ApplyGroundTangentImpulse(Float scale, Float rotationSpeed, PlayerGroundCollision *source, Vec *destination)
{
    Vec pushDirection;
    Float unusedMovementValue;

    Float normalAlignment;
    Vec *groundSurfaceNormal;

    if (source->groundFace == NULL) {
        destination->x = destination->y = destination->z = 0.0f;
        return;
    }
    groundSurfaceNormal = &source->groundFace->nbt;
    pushDirection.x = groundSurfaceNormal->y * sind((180.0f + scale));
    pushDirection.z = groundSurfaceNormal->y * cosd((180.0f + scale));
    normalAlignment = 1.0f - (groundSurfaceNormal->y * groundSurfaceNormal->y);
    pushDirection.y = -sqrtf(normalAlignment);
    VECScale(&pushDirection, destination, rotationSpeed);
}
s32 FindPathSegment(Vec *source, PlayerPathCoordinates *destination)
{
    Vec intersectionCoordinates;

    Float boundaryDeltaX;
    Float boundaryDeltaZ;
    Float intersectionDeterminant;
    Float boundaryOffsetX;
    Float boundaryOffsetZ;
    Float xAxisCoefficient;
    Float xBoundaryCoefficient;
    Float zAxisCoefficient;
    Float zBoundaryCoefficient;
    Float lineXIntersectionScale;
    Float lineZIntersectionScale;
    Float boundaryXIntersectionScale;
    Float boundaryZIntersectionScale;
    Float normalizedXCoordinate;

    s32 segmentIndex;
    s32 nextSegmentIndex;

    for (segmentIndex = 0; segmentIndex < pathSegmentCount - 1; segmentIndex++) {
        nextSegmentIndex = segmentIndex + 1;
        boundaryDeltaX = leftCourseBoundary[nextSegmentIndex].x - rightCourseBoundary[segmentIndex].x;
        boundaryDeltaZ = leftCourseBoundary[nextSegmentIndex].z - rightCourseBoundary[segmentIndex].z;
        if (((boundaryDeltaX * (source->z - rightCourseBoundary[segmentIndex].z)) - (boundaryDeltaZ * (source->x - rightCourseBoundary[segmentIndex].x))) < 0.0f) {
            boundaryDeltaX = leftCourseBoundary[nextSegmentIndex].x - leftCourseBoundary[segmentIndex].x;
            boundaryOffsetX = rightCourseBoundary[segmentIndex].x - leftCourseBoundary[segmentIndex].x;
            boundaryDeltaZ = leftCourseBoundary[nextSegmentIndex].z - leftCourseBoundary[segmentIndex].z;
            boundaryOffsetZ = rightCourseBoundary[segmentIndex].z - leftCourseBoundary[segmentIndex].z;
            xAxisCoefficient = 0.0f;
            xBoundaryCoefficient = 1.0f;
            zAxisCoefficient = 1.0f;
            zBoundaryCoefficient = 0.0f;
            intersectionDeterminant = (boundaryDeltaX * boundaryOffsetZ) - (boundaryOffsetX * boundaryDeltaZ);
            lineXIntersectionScale = ((xAxisCoefficient * boundaryOffsetZ) - (xBoundaryCoefficient * boundaryDeltaZ)) / intersectionDeterminant;
            lineZIntersectionScale = ((xBoundaryCoefficient * boundaryDeltaX) - (xAxisCoefficient * boundaryOffsetX)) / intersectionDeterminant;
            boundaryXIntersectionScale = ((zAxisCoefficient * boundaryOffsetZ) - (zBoundaryCoefficient * boundaryDeltaZ)) / intersectionDeterminant;
            boundaryZIntersectionScale = ((zBoundaryCoefficient * boundaryDeltaX) - (zAxisCoefficient * boundaryOffsetX)) / intersectionDeterminant;
            intersectionCoordinates.z = -(lineXIntersectionScale * leftCourseBoundary[segmentIndex].x) - (lineZIntersectionScale * leftCourseBoundary[segmentIndex].z);
            intersectionCoordinates.y = -(boundaryXIntersectionScale * leftCourseBoundary[segmentIndex].x) - (boundaryZIntersectionScale * leftCourseBoundary[segmentIndex].z);
        }
        else {
            boundaryDeltaX = rightCourseBoundary[nextSegmentIndex].x - leftCourseBoundary[nextSegmentIndex].x;
            boundaryOffsetX = rightCourseBoundary[segmentIndex].x - leftCourseBoundary[nextSegmentIndex].x;
            boundaryDeltaZ = rightCourseBoundary[nextSegmentIndex].z - leftCourseBoundary[nextSegmentIndex].z;
            boundaryOffsetZ = rightCourseBoundary[segmentIndex].z - leftCourseBoundary[nextSegmentIndex].z;
            xAxisCoefficient = 1.0f;
            xBoundaryCoefficient = 1.0f;
            zAxisCoefficient = 0.0f;
            zBoundaryCoefficient = -1.0f;
            intersectionDeterminant = (boundaryDeltaX * boundaryOffsetZ) - (boundaryOffsetX * boundaryDeltaZ);
            lineXIntersectionScale = ((xAxisCoefficient * boundaryOffsetZ) - (xBoundaryCoefficient * boundaryDeltaZ)) / intersectionDeterminant;
            lineZIntersectionScale = ((xBoundaryCoefficient * boundaryDeltaX) - (xAxisCoefficient * boundaryOffsetX)) / intersectionDeterminant;
            boundaryXIntersectionScale = ((zAxisCoefficient * boundaryOffsetZ) - (zBoundaryCoefficient * boundaryDeltaZ)) / intersectionDeterminant;
            boundaryZIntersectionScale = ((zBoundaryCoefficient * boundaryDeltaX) - (zAxisCoefficient * boundaryOffsetX)) / intersectionDeterminant;
            intersectionCoordinates.z = -(lineXIntersectionScale * leftCourseBoundary[nextSegmentIndex].x) - (lineZIntersectionScale * leftCourseBoundary[nextSegmentIndex].z);
            intersectionCoordinates.y = (1.0f - (boundaryXIntersectionScale * leftCourseBoundary[nextSegmentIndex].x)) - (boundaryZIntersectionScale * leftCourseBoundary[nextSegmentIndex].z);
        }
        normalizedXCoordinate = intersectionCoordinates.z + ((lineXIntersectionScale * source->x) + (lineZIntersectionScale * source->z));
        intersectionCoordinates.x = intersectionCoordinates.y + ((boundaryXIntersectionScale * source->x) + (boundaryZIntersectionScale * source->z));
        if ((normalizedXCoordinate >= 0.0f) && (normalizedXCoordinate < 1.0f) && (intersectionCoordinates.x >= 0.0f) && (intersectionCoordinates.x < 1.0f)) {
            destination->laneCoordinate = normalizedXCoordinate;
            destination->segmentProgress = intersectionCoordinates.x;
            return segmentIndex;
        }
    }
    return -1;
}

void GetPathPosition(Vec *source, Float scale, Vec *destination)
{
    PlayerPathCoordinates pathPosition;

    Float segmentDistance;
    Float distanceAlongSegment;
    Float remainingSegmentLength;
    Float segmentOffset;

    MapPathSegment *pathSegment;
    s32 segmentIndex;
    s32 nextSegmentIndex;

    segmentIndex = -1;
    segmentIndex = FindPathSegment(source, &pathPosition);
    if (segmentIndex >= 0) {
        pathSegment = &pathSegments[segmentIndex];
        distanceAlongSegment = 0.0f;
        segmentDistance = pathSegment->segmentLength * pathPosition.segmentProgress;
        remainingSegmentLength = pathSegment->segmentLength - segmentDistance;

        for (nextSegmentIndex = segmentIndex; nextSegmentIndex < pathSegmentCount - 1; nextSegmentIndex++) {
            if (distanceAlongSegment + remainingSegmentLength >= 3000.0f) {
                segmentOffset = (3000.0f + segmentDistance) - distanceAlongSegment;
                destination->x = (pathSegment->segmentDirection.x + (pathSegment->segmentMidpoint.x * segmentOffset)) - (pathSegment->segmentMidpoint.z * scale);
                destination->y = pathSegment->segmentDirection.y + (pathSegment->segmentMidpoint.y * segmentOffset);
                destination->z = pathSegment->segmentDirection.z + (pathSegment->segmentMidpoint.z * segmentOffset) + (pathSegment->segmentMidpoint.x * scale);
                return;
            }
            distanceAlongSegment += remainingSegmentLength;
            segmentDistance = 0.0f;
            pathSegment++;
            remainingSegmentLength = pathSegment->segmentLength;
        }
    }
    destination->x = destination->y = 0.0f;
    destination->z = -61000.0f;
}

Float GetPlayerPathDistance(Vec *source)
{
    Vec cameraPathPosition;
    PlayerPathCoordinates cameraPathCoordinates;
    PlayerPathCoordinates playerPathCoordinates;

    MapPathSegment *cameraPathSegment;
    MapPathSegment *playerPathSegment;
    s32 playerSegmentIndex;
    s32 cameraSegmentIndex;

    GetMapPathOrigin(&cameraPathPosition);
    cameraSegmentIndex = FindPathSegment(&cameraPathPosition, &cameraPathCoordinates);
    playerSegmentIndex = FindPathSegment(source, &playerPathCoordinates);
    if (cameraSegmentIndex < 0 || playerSegmentIndex < 0) {
        return -1.0f;
    }
    cameraPathSegment = &pathSegments[cameraSegmentIndex];
    playerPathSegment = &pathSegments[playerSegmentIndex];
    return (playerPathSegment->segmentStartDistance + (playerPathSegment->segmentLength * playerPathCoordinates.segmentProgress)) - (cameraPathSegment->segmentStartDistance + (cameraPathSegment->segmentLength * cameraPathCoordinates.segmentProgress));
}

void MovePlayerTowardTarget(OMOBJ *object)
{
    Vec targetPathPosition;
    Vec directionToTarget;
    PlayerPathCoordinates currentPathCoordinates;

    Float distanceAlongSegment;
    Float segmentDistance;
    Float remainingSegmentLength;
    Float segmentOffset;
    Float steeringOffset;

    M406PlayerWork *playerState;
    MapPathSegment *pathSegment;
    s32 segmentIndex;
    s32 nextSegmentIndex;

    playerState = object->data;
    if (!playerState->isStunned) {
        if (playerState->movementAnglesAndSpeed.y < 30.000002f) {
            playerState->buttonPressed = 0x100;
        }
        else if ((frand() & 0x1FF) < (playerState->difficulty + 6)) {
            playerState->buttonPressed = 0x100;
        }
        if ((frand() & 0xFF) == 0) {
            playerState->steeringTarget = (400.0f + (100.0f * (4 - GWPlayerCfg[playerState->playerIndex].diff))) * ((frand() & 0x1FF) - 256.0f) / 256;
        }
        if (fabs(playerState->steeringOffset - playerState->steeringTarget) > 10.0) {
            playerState->steeringOffset = playerState->steeringOffset + (0.05f * (playerState->steeringTarget - playerState->steeringOffset));
        }
        steeringOffset = playerState->steeringOffset;
        segmentIndex = -1;
        segmentIndex = FindPathSegment(&object->trans, &currentPathCoordinates);
        if (segmentIndex >= 0) {
            pathSegment = &pathSegments[segmentIndex];
            distanceAlongSegment = 0.0f;
            segmentOffset = pathSegment->segmentLength * currentPathCoordinates.segmentProgress;
            remainingSegmentLength = pathSegment->segmentLength - segmentOffset;
            for (nextSegmentIndex = segmentIndex; nextSegmentIndex < pathSegmentCount - 1; nextSegmentIndex++) {
                if (distanceAlongSegment + remainingSegmentLength >= 3000.0f) {
                    segmentDistance = (3000.0f + segmentOffset) - distanceAlongSegment;
                    targetPathPosition.x = (pathSegment->segmentDirection.x + (pathSegment->segmentMidpoint.x * segmentDistance)) - (pathSegment->segmentMidpoint.z * steeringOffset);
                    targetPathPosition.y = pathSegment->segmentDirection.y + (pathSegment->segmentMidpoint.y * segmentDistance);
                    targetPathPosition.z = pathSegment->segmentDirection.z + (pathSegment->segmentMidpoint.z * segmentDistance) + (pathSegment->segmentMidpoint.x * steeringOffset);
                    goto block12270; // TODO
                }
                else {
                    distanceAlongSegment += remainingSegmentLength;
                    segmentOffset = 0.0f;
                    pathSegment++;
                    remainingSegmentLength = pathSegment->segmentLength;
                }
            }
        }
        targetPathPosition.x = targetPathPosition.y = 0.0f;
        targetPathPosition.z = -61000.0f;

    block12270:
        VECSubtract(&targetPathPosition, &object->trans, &directionToTarget);
        if (VECMag(&directionToTarget) > 0.0f) {
            VECNormalize(&directionToTarget, &directionToTarget);
            playerState->stickX = 72.0 * ((directionToTarget.x * cosd(CRot.y)) - directionToTarget.z * sind(CRot.y));
            playerState->stickY = 72.0 * -((directionToTarget.x * sind(CRot.y)) + directionToTarget.z * cosd(CRot.y));
        }
    }
    else if ((frand() & 3) == 0) {
        playerState->buttonPressed = 256;
    }
}

void WaitForPlayersToSeparate(void)
{
    OMOBJ **playerObjectGroup;
    s32 maxWaitFrames;

    maxWaitFrames = 0;
    playerObjectGroup = omGetGroupMemberListEx(playerObjectProcess, 0);

    while (SeparateOverlappingPlayers() != 0 && maxWaitFrames < 10) {
        maxWaitFrames++;
    }
}

s32 SeparateOverlappingPlayers(void)
{
    Vec playerSeparationOffsets[4];
    Vec playerSeparationVector;

    Float playerDistance;
    Float separationPushStrength;

    s32 playerIndex;
    s32 otherPlayerIndex;
    OMOBJ **playerObjects;
    OMOBJ *playerObject;
    s32 overlappingPlayerPairs;
    OMOBJ *currentPlayerObject;
    OMOBJ *otherPlayerObject;
    M406PlayerWork *playerWork;
    M406PlayerWork *currentPlayerWork;
    M406PlayerWork *otherPlayerWork;

    overlappingPlayerPairs = 0;
    playerObjects = omGetGroupMemberListEx(playerObjectProcess, 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSeparationOffsets[playerIndex].x = playerSeparationOffsets[playerIndex].y = playerSeparationOffsets[playerIndex].z = 0.0f;
    }
    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
        currentPlayerObject = playerObjects[playerIndex];
        currentPlayerWork = currentPlayerObject->data;
        if (!currentPlayerWork->isFinished) {
            for (otherPlayerIndex = playerIndex + 1; otherPlayerIndex < 4; otherPlayerIndex++) {
                otherPlayerObject = playerObjects[otherPlayerIndex];
                otherPlayerWork = otherPlayerObject->data;
                if (!otherPlayerWork->isFinished) {
                    VECSubtract(&currentPlayerObject->trans, &otherPlayerObject->trans, &playerSeparationVector);
                    playerDistance = VECMag(&playerSeparationVector);
                    if (playerDistance < 100.0f) {
                        VECNormalize(&playerSeparationVector, &playerSeparationVector);
                        separationPushStrength = 0.01f + (0.5f * (100.0f - playerDistance));
                        VECScale(&playerSeparationVector, &playerSeparationVector, separationPushStrength);
                        VECAdd(&playerSeparationOffsets[playerIndex], &playerSeparationVector, &playerSeparationOffsets[playerIndex]);
                        VECSubtract(&playerSeparationOffsets[otherPlayerIndex], &playerSeparationVector, &playerSeparationOffsets[otherPlayerIndex]);
                        overlappingPlayerPairs++;
                    }
                }
            }
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerObject = playerObjects[playerIndex];
        playerWork = playerObject->data;
        if (!playerWork->isFinished) {
            VECAdd(&playerObject->trans, &playerSeparationOffsets[playerIndex], &playerObject->trans);
        }
    }
    return overlappingPlayerPairs;
}

s32 IsPlayerNearby(OMOBJ *object)
{
    Vec modelDistanceVector;

    Float modelDistance;

    s32 modelIndex;
    OMOBJ **playerObjects;
    HU3DMODEL *playerModel;

    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 3);
    for (modelIndex = 0; modelIndex < playerObjects[0]->mdlcnt; modelIndex++) {
        playerModel = &Hu3DData[(*playerObjects)->mdlId[modelIndex]];
        VECSubtract(&playerModel->pos, &object->trans, &modelDistanceVector);
        modelDistance = VECMag(&modelDistanceVector);
        if (modelDistance < 100.0f) {
            return 1;
        }
    }
    return 0;
}

void DestroyPlayerObject(OMOBJ *object)
{
    if (object->data != NULL) {
        HuMemDirectFree(object->data);
        object->data = NULL;
    }
}

void ResetCameraPositionHistory(OMOBJ *object)
{
    cameraPositionHistoryIndex = 0;
    cameraHistorySpacing = -1.0f;
    object->objFunc = UpdateCameraPositionHistory;
}

void RecordCameraPosition(Vec *source)
{
    cameraPositionHistory[cameraPositionHistoryIndex].x = source->x;
    cameraPositionHistory[cameraPositionHistoryIndex].y = source->y;
    cameraPositionHistory[cameraPositionHistoryIndex].z = source->z;
    cameraPositionHistoryIndex++;
}

void UpdateCameraPositionHistory(OMOBJ *object)
{
    Vec cameraSpacePositions[16];
    Vec cameraProjectionPositions[16];
    Mtx cameraViewMatrix;
    Vec minimumFocusBounds;
    Vec maximumFocusBounds;
    Vec cameraPosition;
    Vec cameraDirection;

    Float cameraAdjustmentRate;
    Float focusSmoothingRate;
    Float requiredCameraDistance;
    Float targetCameraZoom;

    s32 focusPointIndex;
    HU3DCAMERA *cameraData = Hu3DCamera;

    if (cameraPositionHistoryIndex != 0) {
        if (getSceneState() == 4) {
            focusSmoothingRate = 0.4f;
        }
        else {
            focusSmoothingRate = 0.05f;
        }
        cameraAdjustmentRate = focusSmoothingRate;
        for (focusPointIndex = 0; focusPointIndex < cameraPositionHistoryIndex; focusPointIndex++) {
            if (focusPointIndex == 0) {
                minimumFocusBounds = maximumFocusBounds = cameraPositionHistory[focusPointIndex];
            }
            else {
                if (cameraPositionHistory[focusPointIndex].x < minimumFocusBounds.x) {
                    minimumFocusBounds.x = cameraPositionHistory[focusPointIndex].x;
                }
                else if (cameraPositionHistory[focusPointIndex].x > maximumFocusBounds.x) {
                    maximumFocusBounds.x = cameraPositionHistory[focusPointIndex].x;
                }
                if (cameraPositionHistory[focusPointIndex].y < minimumFocusBounds.y) {
                    minimumFocusBounds.y = cameraPositionHistory[focusPointIndex].y;
                }
                else if (cameraPositionHistory[focusPointIndex].y > maximumFocusBounds.y) {
                    maximumFocusBounds.y = cameraPositionHistory[focusPointIndex].y;
                }
                if (cameraPositionHistory[focusPointIndex].z < minimumFocusBounds.z) {
                    minimumFocusBounds.z = cameraPositionHistory[focusPointIndex].z;
                }
                else if (cameraPositionHistory[focusPointIndex].z > maximumFocusBounds.z) {
                    maximumFocusBounds.z = cameraPositionHistory[focusPointIndex].z;
                }
            }
        }
        cameraAdjustmentRate = 0.4f;
        Center.x += cameraAdjustmentRate * ((0.5f * (minimumFocusBounds.x + maximumFocusBounds.x)) - Center.x);
        Center.y += cameraAdjustmentRate * ((0.5f * (minimumFocusBounds.y + maximumFocusBounds.y)) - Center.y);
        Center.z += cameraAdjustmentRate * ((0.5f * (minimumFocusBounds.z + maximumFocusBounds.z)) - Center.z);

        cameraData->target.x = Center.x;
        cameraData->target.y = Center.y;
        cameraData->target.z = Center.z;
        C_MTXLookAt(cameraViewMatrix, &cameraData->pos, &cameraData->up, &cameraData->target);
        for (focusPointIndex = 0; focusPointIndex < cameraPositionHistoryIndex; focusPointIndex++) {
            MTXMultVec(cameraViewMatrix, &cameraPositionHistory[focusPointIndex], &cameraSpacePositions[focusPointIndex]);
            cameraProjectionPositions[focusPointIndex].x = cameraSpacePositions[focusPointIndex].x / (cameraSpacePositions[focusPointIndex].z * (sind((cameraData->fov / 2)) / cosd((cameraData->fov / 2))));
            cameraProjectionPositions[focusPointIndex].y = cameraSpacePositions[focusPointIndex].y / (cameraSpacePositions[focusPointIndex].z * (sind((cameraData->fov / 2)) / cosd((cameraData->fov / 2))));
        }
        maximumFocusBounds.x = 0.0f;
        for (focusPointIndex = 0; focusPointIndex < cameraPositionHistoryIndex; focusPointIndex++) {
            if (fabs(cameraProjectionPositions[focusPointIndex].x) > fabs(cameraProjectionPositions[focusPointIndex].y)) {
                if (fabs(cameraProjectionPositions[focusPointIndex].x) > maximumFocusBounds.x) {
                    maximumFocusBounds.x = fabs(cameraProjectionPositions[focusPointIndex].x);
                    maximumFocusBounds.y = fabs(cameraSpacePositions[focusPointIndex].x);
                    maximumFocusBounds.z = cameraSpacePositions[focusPointIndex].z;
                }
            }
            else {
                if (fabs(cameraProjectionPositions[focusPointIndex].y) > maximumFocusBounds.x) {
                    maximumFocusBounds.x = fabs(cameraProjectionPositions[focusPointIndex].y);
                    maximumFocusBounds.y = fabs(cameraSpacePositions[focusPointIndex].y);
                    maximumFocusBounds.z = cameraSpacePositions[focusPointIndex].z;
                }
            }
        }
        requiredCameraDistance = -maximumFocusBounds.y / (sind((cameraData->fov / 2)) / cosd((cameraData->fov / 2)));
        targetCameraZoom = CZoom - (requiredCameraDistance - maximumFocusBounds.z);
        cameraAdjustmentRate = 0.05f;
        if (cameraHistorySpacing >= 0.0f) {
            cameraHistorySpacing += cameraAdjustmentRate * (targetCameraZoom - cameraHistorySpacing);
        }
        else {
            cameraHistorySpacing = CZoom - 500.0f;
        }
        CZoom = 500.0f + cameraHistorySpacing;
        if (CZoom < 1500.0f) {
            CZoom = 1500.0f;
        }
        cameraPositionHistoryIndex = 0;
        if (getSceneState() >= 3) {
            cameraPosition.x = Center.x + (CZoom * (sind(CRot.y) * cosd(CRot.x)));
            cameraPosition.y = Center.y + (CZoom * -sind(CRot.x));
            cameraPosition.z = Center.z + (CZoom * (cosd(CRot.y) * cosd(CRot.x)));
            VECSubtract(&Center, &cameraPosition, &cameraDirection);
            VECNormalize(&cameraDirection, &cameraDirection);
            HuAudFXListnerUpdate(&cameraPosition, &cameraDirection);
        }
    }
}

void InitializePlayerModelData(void) { }

s32 FindModelJointTransform(HSFOBJECT *source, Mtx destination, char *effectColor, Mtx channelValue)
{
    char objectNameBuffer[256];
    Mtx objectTransform;
    Mtx objectRotation;
    u32 childIndex;

    HSFTRANSFORM *playerWork = &source->mesh.curr;

    mtxRot(objectRotation, playerWork->rot.x, playerWork->rot.y, playerWork->rot.z);
    MTXScale(objectTransform, playerWork->scale.x, playerWork->scale.y, playerWork->scale.z);
    MTXConcat(objectRotation, objectTransform, objectTransform);
    mtxTransCat(objectTransform, playerWork->pos.x, playerWork->pos.y, playerWork->pos.z);
    MTXConcat(destination, objectTransform, objectTransform);
    strcpy(objectNameBuffer, MakeObjectName(effectColor));
    if (strcmp(objectNameBuffer, source->name) == 0) {
        MTXCopy(objectTransform, channelValue);
        return 1;
    }
    for (childIndex = 0; childIndex < source->mesh.childrenCount; childIndex++) {
        s32 childMatched = FindModelJointTransform(source->mesh.children[childIndex], objectTransform, effectColor, channelValue);
        if (childMatched) {
            return 1;
        }
    }
    return 0;
}

void GetPlayerJointTransform(s16 source, char *destination, Mtx effectColor)
{
    Mtx identityTransform;

    HU3DMODEL *playerModel;
    HSFDATA *playerModelData;

    playerModel = &Hu3DData[source];
    playerModelData = playerModel->hsf;
    MTXIdentity(identityTransform);
    FindModelJointTransform(playerModelData->root, identityTransform, destination, effectColor);
}

Float ApproachAngle(Float scale, Float rotationSpeed, Float secondScale)
{
    Float wrappedAngleDifference;
    Float rawAngleDifference;
    Float interpolatedAngle;

    rawAngleDifference = fmod(rotationSpeed - scale, 360.0);
    wrappedAngleDifference = rawAngleDifference;
    if (wrappedAngleDifference < 0.0f) {
        wrappedAngleDifference += 360.0f;
    }
    if (wrappedAngleDifference > 180.0f) {
        wrappedAngleDifference -= 360.0f;
    }
    interpolatedAngle = fmod(scale + (secondScale * wrappedAngleDifference), 360.0);
    wrappedAngleDifference = interpolatedAngle;
    if (wrappedAngleDifference < 0.0f) {
        wrappedAngleDifference += 360.0f;
    }
    return wrappedAngleDifference;
}

s32 ResolvePlayerSurfaceCollision(Vec *source, Vec *destination)
{
    Vec *playerEffectTransform[3];
    Vec triangleEdge;
    Vec pointToEdge;
    Vec triangleCrossProduct;
    Vec nearestCollisionPoint;
    Vec candidateCollisionPoint;
    HSFBUFFER *modelNormalBuffer;

    Float candidateDistance;
    Float edgeIntersectionFraction;
    Float nearestCollisionDistance;
    Float planeIntersectionDistance;

    HSFFACE *collisionFace;
    s32 triangleEdgeIndex;
    HSFFACE *nearestCollisionFace;
    HSFOBJECT *modelRoot;
    HSFBUFFER *vertexBuffer;
    HSFBUFFER *faceBuffer;
    s32 faceIndex;
    s32 nextVertexIndex;
    s32 modelIndex;
    OMOBJ **playerObjects;
    HU3DMODEL *playerModel;

    playerObjects = omGetGroupMemberListEx(playerObjectProcess, 2);
    modelIndex = source->z / -10000.0f;
    if (modelIndex >= 6) {
#ifdef NON_MATCHING
        // TODO NON_MATCHING is this right?
        return 1;
#else
        return;
#endif
    }
    playerModel = &Hu3DData[(*playerObjects)->mdlId[modelIndex]];
    nearestCollisionDistance = 100000.0f;
    modelRoot = playerModel->hsf->root;
    if (modelRoot->type != 2) {
#ifdef NON_MATCHING
        return 1;
#else
        return;
#endif
    }
    faceBuffer = modelRoot->mesh.face;
    vertexBuffer = modelRoot->mesh.vertex;
    modelNormalBuffer = modelRoot->mesh.normal;
    collisionFace = faceBuffer->data;
    for (faceIndex = 0; faceIndex < faceBuffer->count; faceIndex++, collisionFace++) {
        if ((collisionFace->type == 2) && ((collisionFace->mat & 0xFFF) == playerEffectModelIds[modelIndex])) {
            playerEffectTransform[0] = &((Vec *)vertexBuffer->data)[collisionFace->indices[0][0]];
            playerEffectTransform[1] = &((Vec *)vertexBuffer->data)[collisionFace->indices[1][0]];
            playerEffectTransform[2] = &((Vec *)vertexBuffer->data)[collisionFace->indices[2][0]];

            planeIntersectionDistance = (collisionFace->nbt.z * playerEffectTransform[0]->z) + ((collisionFace->nbt.x * playerEffectTransform[0]->x) + (collisionFace->nbt.y * playerEffectTransform[0]->y));
            triangleCrossProduct.x = -collisionFace->nbt.x;
            triangleCrossProduct.y = 0.0f;
            triangleCrossProduct.z = -collisionFace->nbt.z;
            VECNormalize(&triangleCrossProduct, &triangleCrossProduct);
            candidateDistance = (planeIntersectionDistance + (((-collisionFace->nbt.x * source->x) - (collisionFace->nbt.y * source->y)) - (collisionFace->nbt.z * source->z)))
                / ((collisionFace->nbt.x * triangleCrossProduct.x) + (collisionFace->nbt.z * triangleCrossProduct.z));
            if (candidateDistance <= 0.0) {
                continue;
            }
            candidateCollisionPoint.x = source->x + (triangleCrossProduct.x * candidateDistance);
            candidateCollisionPoint.y = source->y;
            candidateCollisionPoint.z = source->z + (triangleCrossProduct.z * candidateDistance);
            VECSubtract(playerEffectTransform[1], playerEffectTransform[0], &triangleEdge);
            VECSubtract(&candidateCollisionPoint, playerEffectTransform[1], &pointToEdge);
            VECCrossProduct(&triangleEdge, &pointToEdge, &triangleCrossProduct);
            if (!(VECDotProduct(&collisionFace->nbt, &triangleCrossProduct) < 0.0f)) {
                VECSubtract(playerEffectTransform[2], playerEffectTransform[1], &triangleEdge);
                VECSubtract(&candidateCollisionPoint, playerEffectTransform[2], &pointToEdge);
                VECCrossProduct(&triangleEdge, &pointToEdge, &triangleCrossProduct);
                if (!(VECDotProduct(&collisionFace->nbt, &triangleCrossProduct) < 0.0f)) {
                    VECSubtract(playerEffectTransform[0], playerEffectTransform[2], &triangleEdge);
                    VECSubtract(&candidateCollisionPoint, playerEffectTransform[0], &pointToEdge);
                    VECCrossProduct(&triangleEdge, &pointToEdge, &triangleCrossProduct);
                    if (!(VECDotProduct(&collisionFace->nbt, &triangleCrossProduct) < 0.0f) && (candidateDistance < nearestCollisionDistance)) {
                        nearestCollisionDistance = candidateDistance;
                        nearestCollisionPoint = candidateCollisionPoint;
                        nearestCollisionFace = collisionFace;
                    }
                }
            }
            for (triangleEdgeIndex = 0; triangleEdgeIndex < 3; triangleEdgeIndex++) {
                nextVertexIndex = triangleEdgeIndex + 1;
                if (nextVertexIndex > 2) {
                    nextVertexIndex = 0;
                }
                VECSubtract(playerEffectTransform[nextVertexIndex], playerEffectTransform[triangleEdgeIndex], &triangleEdge);
                if (triangleEdge.y != 0.0f) {
                    edgeIntersectionFraction = (source->y - playerEffectTransform[triangleEdgeIndex]->y) / triangleEdge.y;
                    if ((edgeIntersectionFraction >= 0.0f) && (edgeIntersectionFraction < 1.0f)) {
                        candidateCollisionPoint.x = playerEffectTransform[triangleEdgeIndex]->x + (edgeIntersectionFraction * triangleEdge.x);
                        candidateCollisionPoint.z = playerEffectTransform[triangleEdgeIndex]->z + (edgeIntersectionFraction * triangleEdge.z);
                        VECSubtract(&candidateCollisionPoint, source, &triangleEdge);
                        triangleEdge.y = 0.0f;
                        candidateDistance = VECMag(&triangleEdge);
                        if (candidateDistance < nearestCollisionDistance) {
                            nearestCollisionDistance = candidateDistance;
                            nearestCollisionPoint = candidateCollisionPoint;
                            nearestCollisionFace = collisionFace;
                        }
                    }
                }
                candidateDistance = (triangleEdge.x * triangleEdge.x) + (triangleEdge.z * triangleEdge.z);
                if (candidateDistance != 0.0f) {
                    edgeIntersectionFraction = (((source->z * triangleEdge.z) + ((source->x * triangleEdge.x) - (triangleEdge.x * playerEffectTransform[triangleEdgeIndex]->x))) - (triangleEdge.z * playerEffectTransform[triangleEdgeIndex]->z)) / candidateDistance;
                    if ((edgeIntersectionFraction >= 0.0f) && (edgeIntersectionFraction < 1.0f)) {
                        pointToEdge.y = playerEffectTransform[triangleEdgeIndex]->y + (edgeIntersectionFraction * triangleEdge.y);
                        if ((pointToEdge.y >= source->y) && (pointToEdge.y < (150.0f + source->y))) {
                            pointToEdge.x = playerEffectTransform[triangleEdgeIndex]->x + (edgeIntersectionFraction * triangleEdge.x);
                            pointToEdge.z = playerEffectTransform[triangleEdgeIndex]->z + (edgeIntersectionFraction * triangleEdge.z);
                            VECSubtract(&pointToEdge, source, &triangleEdge);
                            triangleEdge.y = 0.0f;
                            candidateDistance = VECMag(&triangleEdge);
                            if (candidateDistance < nearestCollisionDistance) {
                                nearestCollisionDistance = candidateDistance;
                                nearestCollisionPoint = pointToEdge;
                                nearestCollisionFace = collisionFace;
                            }
                        }
                    }
                }
                if ((playerEffectTransform[triangleEdgeIndex]->y >= source->y) && (playerEffectTransform[triangleEdgeIndex]->y < (150.0f + source->y))) {
                    VECSubtract(source, playerEffectTransform[triangleEdgeIndex], &triangleEdge);
                    triangleEdge.y = 0.0f;
                    candidateDistance = VECMag(&triangleEdge);
                    if (candidateDistance < nearestCollisionDistance) {
                        nearestCollisionDistance = candidateDistance;
                        nearestCollisionPoint.x = playerEffectTransform[triangleEdgeIndex]->x;
                        nearestCollisionPoint.z = playerEffectTransform[triangleEdgeIndex]->z;
                        nearestCollisionFace = collisionFace;
                    }
                }
            }
        }
    }
    if (nearestCollisionDistance < 100.0f) {
        VECSubtract(source, &nearestCollisionPoint, &triangleEdge);
        triangleEdge.y = 0.0f;
        VECNormalize(&triangleEdge, &triangleEdge);
        source->x = nearestCollisionPoint.x + (100.001f * triangleEdge.x);
        source->z = nearestCollisionPoint.z + (100.001f * triangleEdge.z);
        destination->x = nearestCollisionFace->nbt.x;
        destination->y = nearestCollisionFace->nbt.y;
        destination->z = nearestCollisionFace->nbt.z;
        return 1;
    }
    return 0;
}

Float FindPlayerGroundSurface(Vec *source, PlayerGroundCollision *destination)
{
    Vec *faceVertices[3];
    Vec triangleEdge;
    Vec pointToEdge;
    Vec edgeCrossProduct;
    HSFBUFFER *modelNormalBuffer;
    Float floorHeight; // ! - uninitialized

    Float effectAlpha;
    Float planeDistance;

    HSFFACE *playerWork;
    HSFOBJECT *modelRoot;
    HSFBUFFER *vertexBuffer;
    s32 model;
    HSFBUFFER *faceBuffer;
    s32 modelPartIndex;
    OMOBJ **playerObjects;
    HU3DMODEL *playerModel;
    s32 modelIndex;

#ifdef NON_MATCHING
    floorHeight = -5.0f; // TODO NON_MATCHING is this right?
#endif

    destination->groundFace = NULL;
    playerObjects = omGetGroupMemberListEx(playerObjectProcess, 2);
    model = source->z / -10000.0f;
    if (model >= 6) {
        return floorHeight;
    }
    playerModel = &Hu3DData[playerObjects[0]->mdlId[model]];
    modelRoot = playerModel->hsf->root;
    if (modelRoot->type != 2) {
        return -100000.0f;
    }
    faceBuffer = modelRoot->mesh.face;
    vertexBuffer = modelRoot->mesh.vertex;
    modelNormalBuffer = modelRoot->mesh.normal;
    for (playerWork = faceBuffer->data, modelPartIndex = 0; modelPartIndex < faceBuffer->count; modelPartIndex++, playerWork++) {
        if (playerWork->type == 2 && (((playerWork->mat & 0xFFF) == playerAnimationIds[model]) || ((playerWork->mat & 0xFFF) == finishLineMaterialIds[model]))) {
            faceVertices[0] = &((Vec *)vertexBuffer->data)[playerWork->indices[0][0]];
            faceVertices[1] = &((Vec *)vertexBuffer->data)[playerWork->indices[1][0]];
            faceVertices[2] = &((Vec *)vertexBuffer->data)[playerWork->indices[2][0]];
            VECSubtract(faceVertices[1], faceVertices[0], &triangleEdge);
            VECSubtract(source, faceVertices[1], &pointToEdge);
            VECCrossProduct(&triangleEdge, &pointToEdge, &edgeCrossProduct);
            if (edgeCrossProduct.y < 0.0f) {
                continue;
            }
            VECSubtract(faceVertices[2], faceVertices[1], &triangleEdge);
            VECSubtract(source, faceVertices[2], &pointToEdge);
            VECCrossProduct(&triangleEdge, &pointToEdge, &edgeCrossProduct);
            if (edgeCrossProduct.y < 0.0f) {
                continue;
            }
            VECSubtract(faceVertices[0], faceVertices[2], &triangleEdge);
            VECSubtract(source, faceVertices[0], &pointToEdge);
            VECCrossProduct(&triangleEdge, &pointToEdge, &edgeCrossProduct);
            if (edgeCrossProduct.y < 0.0f) {
                continue;
            }
            planeDistance = (playerWork->nbt.z * faceVertices[0]->z) + ((playerWork->nbt.x * faceVertices[0]->x) + (playerWork->nbt.y * faceVertices[0]->y));
            effectAlpha = ((planeDistance - (playerWork->nbt.x * source->x)) - (playerWork->nbt.z * source->z)) / playerWork->nbt.y;
            destination->groundFace = playerWork;
            destination->isFinishLineFace = (playerWork->mat & 0xFFF) == finishLineMaterialIds[model] ? 1 : 0;
            return effectAlpha;
        }
    }
    return -100000.0f;
}
