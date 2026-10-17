#include "REL/executor.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/esprite.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include <string.h>

#include "ext_math.h"
#include "version.h"

s32 rand8(void);

typedef struct {
    /* 0x00 */ s32 playerSlot;
    /* 0x04 */ s32 playerConfigIndex;
    /* 0x08 */ s32 characterId;
    /* 0x0C */ s32 controllerPort;
    /* 0x10 */ s32 difficultyLevel;
    /* 0x14 */ char reservedBytes[4];
    /* 0x18 */ s32 targetColumn;
    /* 0x1C */ s32 targetRow;
    /* 0x20 */ s32 currentMotionIndex;
    /* 0x24 */ s32 motionOverrideIndex;
    /* 0x28 */ s32 jumpPhase;
    /* 0x2C */ s32 jumpState;
    /* 0x30 */ s32 landingState;
    /* 0x34 */ s32 behaviorState;
    /* 0x38 */ s32 stateFrames;
    /* 0x3C */ float movementSpeed;
    /* 0x40 */ float velocityX;
    /* 0x44 */ float verticalVelocity;
    /* 0x48 */ float velocityZ;
} PlayerRuntimeData; // Size 0x4C

typedef struct {
    /* 0x00 */ s32 column;
    /* 0x04 */ s32 row;
    /* 0x08 */ s32 fallCountdownOrScoreDigitIndex;
    /* 0x0C */ s32 ownerPlayerIndex;
    /* 0x10 */ float fallVelocity;
    /* 0x14 */ char reservedBytes[4];
    /* 0x18 */ s32 animationState;
    /* 0x1C */ s32 animationFrames;
    /* 0x20 */ s32 scoreChange;
} BoardPieceData; // Size 0x24

typedef struct {
    /* 0x00 */ s32 activeEffectCount;
    /* 0x04 */ float effectAges[3];
} ScoreEffectState; // Size 0x10

void CameraInputUpdate(void);
void CaptureBuffersInitialize(void);
void CaptureBuffersFree(void);
void ModelCaptureRenderHook(HU3DMODEL *model, Mtx matrix);
void PlayerBehaviorUpdate(OMOBJ *object);
void MinigameUpdate(OMOBJ *object);
void IntroMessageProcess(void);
void GameStateObjectUpdate(OMOBJ *object);
void ParticleEffectHook(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void GameObjectInitialize(OMOBJ *object);
void PlayerObjectInitialize(OMOBJ *object);
void GameStateInitialize(OMOBJ *object);
void ObjectSetup(void);
void PlayerObjectCleanup(OMOBJ *object);

BoardPieceData boardPieceStates[20];
ScoreEffectState scoreEffectStates[13];
OMOBJ *gameStateObject;
OMOBJ *viewExitObject;
OMOBJ *playerObjects[2];
OMOBJ *boardObject;
s32 gamePhaseFrames;
s32 gamePhase;
s32 gamePhaseSubstate;
s32 boardTransitionState;
s32 boardTransitionFrames;
HUPROCESS *objectManager;
s32 directionalLightId;
s32 winnerPlayerIndex;
s32 captureBufferSize;
s32 thumbnailBufferSize;
GXTexObj captureTexture;
char capturePadding[4];
float cameraBlendFactor;
s32 cameraBlendDirection;
char cameraPadding[4];
void *thumbnailBuffer;
void *captureBuffer;
s32 dustMotionIdB;
s32 dustMotionIdA;
s32 roundElapsedFrames;
s32 gameplayElapsedFrames;
s32 activePlayerIndex;
s32 startMessageComplete;
char statePadding[8];
s32 totalUpdateFrames;
s32 roundCountdownFrames;

s32 portraitResourceDirectories[] = { DATA_MAKE_NUM(DATADIR_MGCONST, 23), DATA_MAKE_NUM(DATADIR_MGCONST, 22), DATA_MAKE_NUM(DATADIR_MGCONST, 51),
    DATA_MAKE_NUM(DATADIR_MGCONST, 51), DATADIR_MGCONST, DATA_MAKE_NUM(DATADIR_MGCONST, 23), DATA_MAKE_NUM(DATADIR_MGCONST, 22),
    DATA_MAKE_NUM(DATADIR_MGCONST, 51), DATA_MAKE_NUM(DATADIR_MGCONST, 51), DATADIR_MGCONST };

s32 portraitSpriteIds[10] = { 0 };

s32 characterMotionIds[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0), DATA_MAKE_NUM(DATADIR_MARIOMOT, 2), DATA_MAKE_NUM(DATADIR_MARIOMOT, 3),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 5), DATA_MAKE_NUM(DATADIR_MARIOMOT, 6), DATA_MAKE_NUM(DATADIR_MARIOMOT, 8), DATA_MAKE_NUM(DATADIR_MARIOMOT, 10),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 9), DATA_MAKE_NUM(DATADIR_MARIOMOT, 26), DATA_MAKE_NUM(DATADIR_MARIOMOT, 23),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 24) };

s32 boardMotionDataIds[] = { DATA_MAKE_NUM(DATADIR_M448, 19), DATA_MAKE_NUM(DATADIR_M448, 20), DATA_MAKE_NUM(DATADIR_M448, 21),
    DATA_MAKE_NUM(DATADIR_M448, 24), DATA_MAKE_NUM(DATADIR_M448, 25), DATA_MAKE_NUM(DATADIR_M448, 26), DATA_MAKE_NUM(DATADIR_M448, 27),
    DATA_MAKE_NUM(DATADIR_M448, 28), DATA_MAKE_NUM(DATADIR_M448, 19), DATA_MAKE_NUM(DATADIR_M448, 22), DATA_MAKE_NUM(DATADIR_M448, 23) };

u8 characterMotionOneShotFlags[11] = { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 };
u8 characterMotionAltOneShotFlags[11] = { 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1 };
s32 characterMotionBlendFrames[11] = { 8, 8, 8, 0, 0, 0, 8, 8, 2, 8, 8 };
s32 characterMotionStartFrames[11] = { 0, 0, 0, 3, 5, 0, 0, 0, 0, 0, 0 };
s32 alternateMotionBlendFrames[11] = { 8, 8, 8, 0, 2, 0, 0, 0, 0, 8, 8 };
s32 unusedAnimationData[11] = { 0 }; // unused
s32 playerScoreDigitCounts[2] = { 0 };
s32 characterFacingFlags[] = { 1, 1, 0, 0, 0, 1, 0, 0 };
s16 messageSequenceId = -1;
s16 countdownSequenceId = -1;
s16 timeoutSequenceId = -1;
s32 backgroundMusicSequenceId = -1;
s32 playerScores[2] = { 0 };
s32 playerLastScoreChanges[2] = { 0 };
s32 playerScoreAnimationFrames[2] = { 0 };

float playerStartPositions[][4] = { { -90.0f, 2.0f, 525.0f, 180.0f }, { 90.0f, 2.0f, 525.0f, 180.0f } };

float particleSpawnPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f };

void CameraInputUpdate(void)
{
    Center.x += 10.0 * (((HuPadBtn[0] & 2) != 0) - ((HuPadBtn[0] & 1) != 0));
    Center.z -= 10.0 * (((HuPadBtn[0] & 8) != 0) - ((HuPadBtn[0] & 4) != 0));
    Center.y += 30.0 * ((HuPadTrigL[0] - HuPadTrigR[0]) / 128.0);
    CZoom += 25.0 * (((HuPadBtn[0] & 0x400) != 0) - ((HuPadBtn[0] & 0x800) != 0));
    CRot.y += HuPadSubStkX[0] / 16.0;
    CRot.x += HuPadSubStkY[0] / -16.0;
}

void CaptureBuffersInitialize(void)
{
    captureBufferSize = GXGetTexBufferSize(640, 480, GX_TF_RGB565, GX_FALSE, 0);
    thumbnailBufferSize = GXGetTexBufferSize(160, 120, GX_TF_RGB565, GX_FALSE, 0);
    captureBuffer = HuMemDirectMallocNum(HEAP_HEAP, captureBufferSize, HU_MEMNUM_OVL);
    memset(captureBuffer, 0, captureBufferSize);
    thumbnailBuffer = HuMemDirectMallocNum(HEAP_HEAP, thumbnailBufferSize, HU_MEMNUM_OVL);
    memset(thumbnailBuffer, 0, thumbnailBufferSize);
}

void CaptureBuffersFree(void)
{
    if (captureBuffer) {
        HuMemDirectFree(captureBuffer);
    }
    captureBuffer = NULL;
    if (thumbnailBuffer) {
        HuMemDirectFree(thumbnailBuffer);
    }
    thumbnailBuffer = NULL;
}

void ModelCaptureRenderHook(HU3DMODEL *model, Mtx matrix)
{
    Mtx identityMatrix;
#ifdef NON_MATCHING
    Mtx44 projectionMatrix;
#else
    Mtx projectionMatrix;
#endif
    float textureCoordinateX;
    float vertexPositionX;
    float textureCoordinateY;
    float vertexPositionY;
    s32 captureBytes;
    void *captureAddress;
    s32 vertexAlpha;
    s32 sourceBufferBytes;
    void *sourceBufferAddress;
    s32 i;
    s32 j;

    captureBytes = captureBufferSize;
    captureAddress = captureBuffer;
    if (!captureBuffer) {
        return;
    }
    GXSetScissor(0, 0, 640, 480);
    GXSetViewport(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    C_MTXOrtho(projectionMatrix, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    GXSetProjection(projectionMatrix, GX_ORTHOGRAPHIC);
    MTXIdentity(identityMatrix);
    GXLoadPosMtxImm(identityMatrix, GX_PNMTX0);
    GXSetNumTevStages(1);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_RASC, GX_CC_TEXC, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    for (i = 0; i < 0; i++) {
        s32 captureSourceTypes[5] = { 0, 1, 1, 0, 1 };
        float captureDownsampleFactors[5] = { 1.0f, 2.0f, 2.5f, 0.0f, 0.0f };
        float captureTextureScales[5] = { 1.0f, 4.0f, 5.0f, 1.0f, 5.0f };
        float overlayWidths[5] = { 2.0f, 2.5f, 0.0f, 1.0f, 1.0f };
        float overlayOffsets[5] = { 0.5f, -0.5f, 0.0f, 0.0f, 0.0f };

        sourceBufferBytes = (captureSourceTypes[i] == 0) ? captureBufferSize : thumbnailBufferSize;
        sourceBufferAddress = (captureSourceTypes[i] == 0) ? captureBuffer : thumbnailBuffer;
        if (captureDownsampleFactors[i] > 0.0f) {
            GXSetTexCopySrc(0, 0, 640.0f / captureDownsampleFactors[i], 480.0f / captureDownsampleFactors[i]);
            GXSetTexCopyDst(
                (captureSourceTypes[i] == 0 ? 640 : 320) / captureDownsampleFactors[i], (captureSourceTypes[i] == 0 ? 480 : 240) / captureDownsampleFactors[i], GX_TF_RGB565, captureSourceTypes[i] == 0 ? GX_FALSE : GX_TRUE);
            GXSetCopyClear(BGColor, 0xFFFFFF);
            GXCopyTex(sourceBufferAddress, GX_FALSE);
            DCFlushRange(sourceBufferAddress, sourceBufferBytes);
            GXPixModeSync();
        }
        if (overlayWidths[i] > 0.0f) {
            GXInitTexObj(&captureTexture, sourceBufferAddress, 640.0f / captureTextureScales[i], 480.0f / captureTextureScales[i], GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
            GXLoadTexObj(&captureTexture, GX_TEXMAP0);
            for (j = 0; j < (i != 4 ? 1 : 4); j++) {
                float quadXOffsets[4] = { 16.5f, -8.5f, -16.5f, 8.5f };
                float quadYOffsets[4] = { -8.5f, -16.5f, 8.5f, 16.5f };

                textureCoordinateX = 0.0015625 * (0.5 * quadXOffsets[j]);
                vertexPositionX = 0.0f;
                textureCoordinateY = 0.0015625 * (0.6666666666666666 * quadYOffsets[j]);
                vertexPositionY = 0.0f;
                vertexAlpha = 16 - j * 5;
                vertexAlpha = (i != 4) ? 0xFF : 0xB1;
                if (i != 4) {
                    textureCoordinateX = 0.0f;
                    textureCoordinateY = 0.0f;
                }
                if (i == 3) {
                    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_RASC, GX_CC_TEXC, GX_CC_ZERO);
                    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ZERO, GX_LO_NOOP);
                }
                else if (i == 4) {
                    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_SUBHALF, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
                    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
                    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                    GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ONE, GX_LO_NOOP);
                }
                else {
                    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_RASC, GX_CC_TEXC, GX_CC_ZERO);
                    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ZERO, GX_LO_NOOP);
                }
                textureCoordinateX = 0.0015625 * overlayOffsets[i];
                textureCoordinateY = 0.0015625 * overlayOffsets[i];
                textureCoordinateX = 0.0f;
                textureCoordinateY = 0.0f;
                vertexPositionX += 0.5 * (1.0f / (640.0f / captureTextureScales[i]));
                vertexPositionY += 0.5 * (1.0f / (640.0f / captureTextureScales[i]));
                textureCoordinateX = textureCoordinateY = vertexPositionX = vertexPositionY = 0.0f;
                GXBegin(GX_QUADS, GX_VTXFMT0, 4);
                GXPosition3f32(textureCoordinateX + 1.25 * (0.0f / overlayWidths[i]), textureCoordinateY + 1.25 * (0.0f / overlayWidths[i]), 0.0f);
                GXColor4u8(0xFF, 0xFF, 0xFF, vertexAlpha);
                GXPosition2f32(vertexPositionX + 0.0, vertexPositionY + 0.0);
                GXPosition3f32(textureCoordinateX + 1.25 * (1.0f / overlayWidths[i]), textureCoordinateY + 1.25 * (0.0f / overlayWidths[i]), 0.0f);
                GXColor4u8(0xFF, 0xFF, 0xFF, vertexAlpha);
                GXPosition2f32(vertexPositionX + 1.25, vertexPositionY + 0.0);
                GXPosition3f32(textureCoordinateX + 1.25 * (1.0f / overlayWidths[i]), textureCoordinateY + 1.25 * (1.0f / overlayWidths[i]), 0.0f);
                GXColor4u8(0xFF, 0xFF, 0xFF, vertexAlpha);
                GXPosition2f32(vertexPositionX + 1.25, vertexPositionY + 1.25);
                GXPosition3f32(textureCoordinateX + 1.25 * (0.0f / overlayWidths[i]), textureCoordinateY + 1.25 * (1.0f / overlayWidths[i]), 0.0f);
                GXColor4u8(0xFF, 0xFF, 0xFF, vertexAlpha);
                GXPosition2f32(vertexPositionX + 0.0, vertexPositionY + 1.25);
                GXEnd();
            }
        }
    }
}

const Vec particleOrigin = { 0.0f, 0.0f, 0.0f };

void PlayerBehaviorUpdate(OMOBJ *object)
{
    Mtx inputRotationMatrix;
    Vec rotatedInputVector;
    s32 availablePieceCount;
    PlayerRuntimeData *playerData;
    PlayerRuntimeData *opponentData;
    PlayerRuntimeData *opponentDataAtRoundEnd;
    PlayerRuntimeData *opponentDataForAnimation;
    BoardPieceData *candidatePieceData;
    BoardPieceData *targetPieceData;
    float movementSpeed;
    float inputWorldX;
    float inputWorldZ;
    float jumpButtonHeld;
    float actionButtonPressed;
    float targetDistance;
    float opponentFacingAngle;
    float opponentPositionX;
    float opponentPositionZ;
    float opponentFacingTarget;
    float playerGoalOffsetX;
    float playerGoalOffsetZ;
    float targetFacingAngle;
    s32 portraitIndex;
    s32 candidatePieceIndex;
    float candidatePieceDistance;
    s32 targetPieceIndex;
    float targetPieceDistance;
    float playerSeparationDistance;
    float scoreAnimationProgress;
    float scoreDigitScale;
    float scoreJitterX;
    float scoreJitterY;
    float candidatePieceOffsetX;
    float candidatePieceOffsetZ;
    float targetPieceOffsetX;
    float targetPieceOffsetZ;
    float targetCellOffsetX;
    float targetCellOffsetZ;
    float opponentSeparationX;
    float movementVelocityX;
    float movementVelocityZ;
    float opponentSeparationZ;
    s16 requestedMotionIndex;
    s32 scoreDigitIndex;
    s32 pieceScanIndex;
    s32 initialSequenceMotionIndex;
    s32 secondSequenceMotionIndex;
    s32 appliedMotionIndex;
    s32 otherPlayerIndex;

    playerData = object->data;
    opponentData = playerObjects[1 - object->work[0]]->data;
    movementSpeed = 0.0f;
    inputWorldX = 0.0f;
    inputWorldZ = 0.0f;
    jumpButtonHeld = 0.0f;
    actionButtonPressed = 0.0f;
    requestedMotionIndex = -1;
    if (playerData->currentMotionIndex != 3) {
        playerData->velocityX = playerData->velocityZ = 0.0f;
    }
    playerData->stateFrames++;
    switch (playerData->behaviorState) {
        case 0:
            if (gamePhase == 1001 && gamePhaseFrames == 45 && playerData->characterId != -1) {
                initialSequenceMotionIndex = 8;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[initialSequenceMotionIndex], characterMotionStartFrames[initialSequenceMotionIndex], 45.0f,
                    (characterMotionOneShotFlags[initialSequenceMotionIndex] != 0) ? HU3D_MOTATTR_NONE : HU3D_MOTATTR_LOOP);
                playerData->currentMotionIndex = initialSequenceMotionIndex;
            }
            if (gamePhase == 1001 && gamePhaseFrames == 210 && playerData->characterId != -1) {
                secondSequenceMotionIndex = 0;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[secondSequenceMotionIndex], characterMotionStartFrames[secondSequenceMotionIndex], 30.0f,
                    (characterMotionOneShotFlags[secondSequenceMotionIndex] != 0) ? HU3D_MOTATTR_NONE : HU3D_MOTATTR_LOOP);
                playerData->currentMotionIndex = secondSequenceMotionIndex;
            }
            if (gamePhase == 1003 && playerData->characterId != -1) {
                object->rot.y = 0.9 * (object->rot.y - (playerData->playerSlot == 0 ? 90 : -90)) + (playerData->playerSlot == 0 ? 90 : -90);
            }
            if (gamePhase == 1005) {
                playerData->behaviorState = (activePlayerIndex == playerData->playerSlot) ? 1 : 3;
            }
            break;
        case 1:
            if (playerData->stateFrames == 1 && playerData->characterId != -1) {
                Hu3DMotionNoMotReset(object->mdlId[0], "ske_head", (characterFacingFlags[playerData->characterId] != 0) ? 0x100 : 0x80);
            }
            inputWorldZ = (roundElapsedFrames < 90) ? 0 : 0x48;
            if (object->trans.z < 250.0f) {
                playerData->behaviorState = 2;
                roundCountdownFrames = REFRESH_RATE * 10;
                if (countdownSequenceId != -1) {
                    MGSeqKill(countdownSequenceId);
                }
                countdownSequenceId = MGSeqCreate(1, roundCountdownFrames / REFRESH_RATE, -1, -1);
                for (portraitIndex = 0; portraitIndex < 10; portraitIndex++) {
                    espDispOn(portraitSpriteIds[portraitIndex]);
                }
                if (playerData->controllerPort == -1) {
                    availablePieceCount = 0;
                    playerData->targetColumn = playerData->targetRow = -1;
                    for (pieceScanIndex = 0; pieceScanIndex < 20; pieceScanIndex++) {
                        availablePieceCount += (boardPieceStates[pieceScanIndex].animationState != 9);
                    }
                    if (availablePieceCount != 0) {
                        availablePieceCount = rand8() % availablePieceCount;
                        for (pieceScanIndex = 0; pieceScanIndex < 20; pieceScanIndex++) {
                            if (boardPieceStates[pieceScanIndex].animationState != 9) {
                                if (availablePieceCount <= 0) {
                                    playerData->targetColumn = boardPieceStates[pieceScanIndex].column;
                                    playerData->targetRow = boardPieceStates[pieceScanIndex].row;
                                    break;
                                }
                                availablePieceCount--;
                            }
                        }
                    }
                }
            }
            break;
        case 6:
            inputWorldX = 4.0f * (0.5f + playerStartPositions[playerData->playerSlot][0] - object->trans.x);
            inputWorldZ = 4.0f * -(0.5f + playerStartPositions[playerData->playerSlot][2] - object->trans.z);
            if (gamePhase == 1005) {
                inputWorldX = 0.0f;
            }
            if (object->trans.z >= playerStartPositions[playerData->playerSlot][2]) {
                playerData->behaviorState = (gamePhase < 1005) ? 0 : ((winnerPlayerIndex == -1) ? 3 : 7);
                playerData->stateFrames = 0;
                if (playerData->behaviorState == 0) {
                    object->rot.y = 180.0f;
                }
            }
            break;
        case 2:
            if (activePlayerIndex != playerData->playerSlot) {
                OSReport("m448 player turn error!\n");
            }
            if (roundCountdownFrames > 0 && gameplayElapsedFrames < REFRESH_RATE * 300) {
                if (playerData->controllerPort != -1) {
                    inputWorldX = HuPadStkX[playerData->controllerPort];
                    inputWorldZ = HuPadStkY[playerData->controllerPort];
                    jumpButtonHeld = (HuPadBtn[playerData->controllerPort] & 0x100) > 0;
                    actionButtonPressed = (HuPadBtnDown[playerData->controllerPort] & (playerData->currentMotionIndex == 3 ? 320 : 256)) > 0;
                    rotatedInputVector = particleOrigin;
                    rotatedInputVector.x = inputWorldX;
                    rotatedInputVector.z = -inputWorldZ;
                    MTXRotRad(inputRotationMatrix, 'y', MTXDegToRad(CRot.y));
                    MTXMultVec(inputRotationMatrix, &rotatedInputVector, &rotatedInputVector);
                    inputWorldX = rotatedInputVector.x;
                    inputWorldZ = -rotatedInputVector.z;
                }
                else {
                    if (playerData->currentMotionIndex == 3) {
                        if (playerData->verticalVelocity < 0.0f && playerData->characterId != -1) {
                            actionButtonPressed = jumpButtonHeld = 1.0f;
                        }
                        else {
                            jumpButtonHeld = 1.0f;
                        }
                    }
                    if (playerData->targetColumn != -1 && playerData->targetRow != -1) {
                        targetCellOffsetX = 100.0 + 200.0 * (playerData->targetColumn - 2.5) - object->trans.x;
                        targetCellOffsetZ = 100.0 + 200.0 * (playerData->targetRow - 2.0) - object->trans.z;
                        targetDistance = sqrtf(targetCellOffsetX * targetCellOffsetX + targetCellOffsetZ * targetCellOffsetZ);
                        if (playerData->characterId != -1 && playerData->currentMotionIndex != 3) {
                            if (targetDistance < 60.0f) {
                                actionButtonPressed = jumpButtonHeld = 1.0f;
                            }
                            else {
                                inputWorldX = targetCellOffsetX;
                                inputWorldZ = -targetCellOffsetZ;
                            }
                        }
                        else if (targetDistance < 277.5) {
                            if (playerData->currentMotionIndex != 3) {
                                actionButtonPressed = jumpButtonHeld = 1.0f;
                            }
                            inputWorldX = (targetCellOffsetX / 37.0) / 0.25;
                            inputWorldZ = (-targetCellOffsetZ / 37.0) / 0.25;
                        }
                        else {
                            inputWorldX = targetCellOffsetX;
                            inputWorldZ = -targetCellOffsetZ;
                        }
                    }
                }
            }
            object->trans.x = (object->trans.x < -450.0f) ? -450.0f : (object->trans.x > 450.0f) ? 450.0f : object->trans.x;
            object->trans.z = (object->trans.z < -350.0f) ? -350.0f : (object->trans.z > 350.0f) ? 350.0f : object->trans.z;
            if ((roundCountdownFrames <= 0 || gameplayElapsedFrames >= REFRESH_RATE * 300) && playerData->currentMotionIndex == 0) {
                playerData->behaviorState = 5;
                playerData->stateFrames = 0;
                roundElapsedFrames = 0;
                if (gameplayElapsedFrames >= REFRESH_RATE * 300) {
                    opponentDataAtRoundEnd = playerObjects[1 - playerData->playerSlot]->data;
                    winnerPlayerIndex = (playerData->characterId == -1) ? playerData->playerSlot : (opponentDataAtRoundEnd->characterId == -1) ? opponentDataAtRoundEnd->playerSlot : 4;
                    opponentDataAtRoundEnd->behaviorState = 7;
                    opponentDataAtRoundEnd->stateFrames = 0;
                }
            }
            break;
        case 3:
            opponentFacingTarget = (object->rot.y > 0.0f) ? -180 : 180;
            opponentPositionX = playerObjects[1 - playerData->playerSlot]->trans.x;
            opponentPositionZ = playerObjects[1 - playerData->playerSlot]->trans.z;
            opponentFacingAngle = atan2d(opponentPositionX - object->trans.x, opponentPositionZ - object->trans.z);
            object->rot.y = 0.3 * (object->rot.y + opponentFacingTarget) - opponentFacingTarget;
            if (playerData->characterId != -1) {
                Hu3DMotionForceSet(object->mdlId[0], "ske_head", (characterFacingFlags[playerData->characterId] != 0) ? 0x100 : 0x80,
                    (characterFacingFlags[playerData->characterId] != 0) ? 180.0f + -opponentFacingAngle : 180.0f + opponentFacingAngle);
            }
            break;
        case 5:
            if (object->trans.z >= 400.0f) {
                opponentDataForAnimation = playerObjects[1 - playerData->playerSlot]->data;
                playerData->behaviorState = 6;
                playerData->stateFrames = 0;
                inputWorldZ = -40.0f;
                if (winnerPlayerIndex == -1) {
                    opponentDataForAnimation->behaviorState = 1;
                    opponentDataForAnimation->stateFrames = 0;
                    if (opponentDataForAnimation->characterId != -1) {
                        Hu3DMotionForceSet(
                            playerObjects[1 - playerData->playerSlot]->mdlId[0], "ske_head", (characterFacingFlags[opponentDataForAnimation->characterId] != 0) ? 0x100 : 0x80, 180.0f);
                        Hu3DMotionNoMotReset(
                            playerObjects[1 - playerData->playerSlot]->mdlId[0], "ske_head", (characterFacingFlags[opponentDataForAnimation->characterId] != 0) ? 0x100 : 0x80);
                    }
                }
                activePlayerIndex = 1 - activePlayerIndex;
            }
            else {
                playerGoalOffsetX = playerStartPositions[playerData->playerSlot][0] - object->trans.x;
                playerGoalOffsetZ = 400.0f - object->trans.z;
                targetFacingAngle = atan2d(playerStartPositions[playerData->playerSlot][0] - object->trans.x, 400.0f - object->trans.z);
                inputWorldX = 10.0 * sind(targetFacingAngle) / 0.25;
                inputWorldZ = 10.0 * -cosd(targetFacingAngle) / 0.25;
                if (fabs(inputWorldX) > fabs(4.0f * (playerStartPositions[playerData->playerSlot][0] - object->trans.x))) {
                    inputWorldX = 4.0f * (playerStartPositions[playerData->playerSlot][0] - object->trans.x);
                }
            }
            break;
        case 7:
            if (playerData->stateFrames == 1 && playerData->characterId != -1) {
                Hu3DMotionForceSet(object->mdlId[0], "ske_head", (characterFacingFlags[playerData->characterId] != 0) ? 0x100 : 0x80, 180.0f);
                Hu3DMotionNoMotReset(object->mdlId[0], "ske_head", (characterFacingFlags[playerData->characterId] != 0) ? 0x100 : 0x80);
            }
            object->rot.y *= 0.3;
            if (gamePhase == 1007 && playerData->currentMotionIndex == 0) {
                playerData->motionOverrideIndex = (winnerPlayerIndex == playerData->playerSlot) ? 9 : 10;
            }
            break;
    }
    switch (playerData->currentMotionIndex) {
        case 4:
        case 7:
        case 8:
            if ((playerData->characterId != -1) ? CharMotionEndCheck(playerData->characterId) : Hu3DMotionEndCheck(object->mdlId[0])) {
                requestedMotionIndex = 0;
            }
            /* fallthrough */
        case 0:
        case 1:
        case 2:
            movementVelocityX = 0.25 * inputWorldX;
            movementVelocityZ = 0.25 * -inputWorldZ;
            movementSpeed = sqrtf(movementVelocityX * movementVelocityX + movementVelocityZ * movementVelocityZ);
            if (movementSpeed >= 0.5f) {
                if (movementSpeed < 10.0f) {
                    playerData->velocityX = movementVelocityX;
                    playerData->velocityZ += movementVelocityZ;
                }
                else {
                    movementVelocityX = movementVelocityX / movementSpeed;
                    movementVelocityZ = movementVelocityZ / movementSpeed;
                    movementSpeed = 10.0f;
                    if (playerData->currentMotionIndex != 3) {
                        playerData->velocityX = movementVelocityX * movementSpeed;
                        playerData->velocityZ = movementVelocityZ * movementSpeed;
                    }
                    else {
                        playerData->velocityX = 0.875 * playerData->velocityX + movementVelocityX * movementSpeed * 0.125;
                        playerData->velocityZ = 0.875 * playerData->velocityZ + movementVelocityZ * movementSpeed * 0.125;
                    }
                }
                playerData->movementSpeed = movementSpeed;
                object->rot.y = atan2d(playerData->velocityX, playerData->velocityZ);
                if (playerData->movementSpeed >= 8.0f) {
                    requestedMotionIndex = 2;
                }
                else {
                    requestedMotionIndex = 1;
                }
            }
            else if (playerData->currentMotionIndex != 7 && playerData->currentMotionIndex != 4 && playerData->currentMotionIndex != 8) {
                requestedMotionIndex = 0;
            }
            playerData->verticalVelocity = 0.0f;
            if (actionButtonPressed) {
                playerData->jumpState = 0;
                playerData->jumpPhase = 1;
                playerData->verticalVelocity = 26.766666f;
                requestedMotionIndex = 3;
            }
            break;
        case 3:
            if (playerData->characterId != -1) {
                movementVelocityX = 0.25 * inputWorldX;
                movementVelocityZ = 0.25 * -inputWorldZ;
                movementSpeed = sqrtf(movementVelocityX * movementVelocityX + movementVelocityZ * movementVelocityZ);
                if (movementSpeed >= 0.5f) {
                    if (movementSpeed < 10.0f) {
                        playerData->velocityX = movementVelocityX;
                        playerData->velocityZ = movementVelocityZ;
                    }
                    else {
                        movementVelocityX = movementVelocityX / movementSpeed;
                        movementVelocityZ = movementVelocityZ / movementSpeed;
                        movementSpeed = 10.0f;
                        if (playerData->currentMotionIndex != 3) {
                            playerData->velocityX = movementVelocityX * movementSpeed;
                            playerData->velocityZ = movementVelocityZ * movementSpeed;
                        }
                        else {
                            playerData->velocityX = 0.875 * playerData->velocityX + movementVelocityX * movementSpeed * 0.125;
                            playerData->velocityZ = 0.875 * playerData->velocityZ + movementVelocityZ * movementSpeed * 0.125;
                        }
                    }
                    playerData->movementSpeed = movementSpeed;
                    object->rot.y = atan2d(playerData->velocityX, playerData->velocityZ);
                }
                else {
                    // Required to match.
                    0.875 * playerData->velocityX;
                    0.875 * playerData->velocityZ;
                }
            }
            if (playerData->jumpPhase != 0) {
                if (jumpButtonHeld) {
                    if (++playerData->jumpPhase > 4) {
                        playerData->jumpPhase = 0;
                    }
                    else {
                        playerData->verticalVelocity += 4.866667f;
                    }
                }
                else {
                    playerData->jumpPhase = 0;
                }
            }
            playerData->verticalVelocity += -2.4333334f;
            if (playerData->jumpState != 0) {
                playerData->verticalVelocity = 0.0f;
                playerData->jumpState = 1;
                requestedMotionIndex = 4;
                if (playerData->characterId != -1) {
                    break;
                }
                playerData->verticalVelocity = 9.733334f;
                playerData->verticalVelocity = 0.0f;
                playerData->jumpState = 0;
                requestedMotionIndex = 6;
                if (playerData->behaviorState != 2) {
                    break;
                }
                for (candidatePieceIndex = 0; candidatePieceIndex < 20; candidatePieceIndex++) {
                    candidatePieceData = &boardPieceStates[candidatePieceIndex];
                    candidatePieceOffsetX = 100.0 + 200.0 * (candidatePieceData->column - 2.5) - object->trans.x;
                    candidatePieceOffsetZ = 100.0 + 200.0 * (candidatePieceData->row - 2.0) - object->trans.z;
                    candidatePieceDistance = sqrtf(candidatePieceOffsetX * candidatePieceOffsetX + candidatePieceOffsetZ * candidatePieceOffsetZ);
                    if (candidatePieceData->ownerPlayerIndex == -1 && candidatePieceDistance <= 100.0) {
                        playerData->behaviorState = 4;
                        playerData->stateFrames = 0;
                        candidatePieceData->animationState = 8;
                        candidatePieceData->animationFrames = 0;
                        candidatePieceData->ownerPlayerIndex = playerData->playerSlot;
                        MGSeqParamSet(countdownSequenceId, 2, -1);
                        countdownSequenceId = -1;
                        roundElapsedFrames = 0;
                    }
                }
            }
            else if (actionButtonPressed) {
                playerData->verticalVelocity = 0.0f;
                playerData->landingState = 0;
                requestedMotionIndex = 5;
            }
            break;
        case 5:
            if ((Hu3DData[object->mdlId[0]].motIdShift == -1 && playerData->characterId != -1) ? CharMotionEndCheck(playerData->characterId)
                                                                                 : Hu3DMotionEndCheck(object->mdlId[0])) {
                playerData->verticalVelocity += -2.4333334f;
            }
            else {
                playerData->verticalVelocity = 0.0f;
            }
            if (playerData->verticalVelocity < 0.0f && playerData->jumpState != 0) {
                if (playerData->landingState != 0) {
                    playerData->verticalVelocity = 0.0f;
                    playerData->landingState = 0;
                    requestedMotionIndex = 7;
                }
                else {
                    playerData->verticalVelocity = 9.733334f;
                    playerData->jumpState = 0;
                    requestedMotionIndex = 6;
                    if (playerData->controllerPort != -1) {
                        omVibrate(playerData->playerConfigIndex, 12, 6, 6);
                    }
                    if (playerData->behaviorState == 2) {
                        for (targetPieceIndex = 0; targetPieceIndex < 20; targetPieceIndex++) {
                            targetPieceData = &boardPieceStates[targetPieceIndex];
                            targetPieceOffsetX = 100.0 + 200.0 * (targetPieceData->column - 2.5) - object->trans.x;
                            targetPieceOffsetZ = 100.0 + 200.0 * (targetPieceData->row - 2.0) - object->trans.z;
                            targetPieceDistance = sqrtf(targetPieceOffsetX * targetPieceOffsetX + targetPieceOffsetZ * targetPieceOffsetZ);
                            if (targetPieceData->ownerPlayerIndex == -1 && targetPieceDistance <= 100.0) {
                                playerData->behaviorState = 4;
                                playerData->stateFrames = 0;
                                targetPieceData->animationState = 8;
                                targetPieceData->animationFrames = 0;
                                targetPieceData->ownerPlayerIndex = playerData->playerSlot;
                                MGSeqParamSet(countdownSequenceId, 2, -1);
                                countdownSequenceId = -1;
                                roundElapsedFrames = 0;
                            }
                        }
                    }
                }
            }
            break;
        case 6:
            playerData->verticalVelocity += -2.4333334f;
            if (playerData->jumpState != 0) {
                if ((playerData->characterId != -1) ? CharMotionEndCheck(playerData->characterId) : Hu3DMotionEndCheck(object->mdlId[0])) {
                    requestedMotionIndex = 7;
                }
            }
            break;
        case 9:
        case 10:
            break;
    }
    object->trans.x += playerData->velocityX;
    if (playerData->jumpState == 0) {
        object->trans.y += playerData->verticalVelocity;
    }
    object->trans.z += playerData->velocityZ;
    for (otherPlayerIndex = 1; otherPlayerIndex < 2; otherPlayerIndex++) {
        opponentSeparationX = playerObjects[otherPlayerIndex]->trans.x - object->trans.x;
        opponentSeparationZ = playerObjects[otherPlayerIndex]->trans.z - object->trans.z;
        if (otherPlayerIndex != playerData->playerSlot) {
            if (opponentSeparationX == 0.0f && opponentSeparationZ == 0.0f) {
                opponentSeparationZ = -1.0f;
            }
            playerSeparationDistance = sqrtf(opponentSeparationX * opponentSeparationX + opponentSeparationZ * opponentSeparationZ);
            if (playerSeparationDistance < 100.0f) {
                opponentSeparationX *= 100.0f / playerSeparationDistance;
                opponentSeparationZ *= 100.0f / playerSeparationDistance;
                object->trans.x = playerObjects[otherPlayerIndex]->trans.x - opponentSeparationX;
                object->trans.z = playerObjects[otherPlayerIndex]->trans.z - opponentSeparationZ;
            }
        }
    }
    if (object->trans.y < 0.0f) {
        object->trans.y = (playerData->behaviorState == 7) ? 2.5f : 0.0f;
        playerData->jumpState = 1;
    }
    if (playerData->motionOverrideIndex >= 0) {
        requestedMotionIndex = playerData->motionOverrideIndex;
    }
    if (requestedMotionIndex >= 0 && requestedMotionIndex != playerData->currentMotionIndex) {
        appliedMotionIndex = (playerData->characterId == -1 && (requestedMotionIndex == 9 || requestedMotionIndex == 10)) ? 0 : requestedMotionIndex;
        if (playerData->characterId != -1) {
            Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[appliedMotionIndex], characterMotionStartFrames[appliedMotionIndex], characterMotionBlendFrames[appliedMotionIndex],
                (characterMotionOneShotFlags[appliedMotionIndex] != 0) ? HU3D_MOTATTR_NONE : HU3D_MOTATTR_LOOP);
        }
        else {
            Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[requestedMotionIndex], characterMotionStartFrames[requestedMotionIndex], alternateMotionBlendFrames[requestedMotionIndex],
                (characterMotionAltOneShotFlags[requestedMotionIndex] != 0) ? HU3D_MOTATTR_NONE : HU3D_MOTATTR_LOOP);
            if (requestedMotionIndex == 3) {
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[5], 15.0f, 33.0f, HU3D_MOTATTR_PAUSE);
            }
        }
        playerData->currentMotionIndex = requestedMotionIndex;
    }
    espBankSet(portraitSpriteIds[playerData->playerSlot * 5 + 2], playerScores[playerData->playerSlot] % 10);
    espBankSet(portraitSpriteIds[playerData->playerSlot * 5 + 3], (playerScores[playerData->playerSlot] / 10) % 10);
    playerScoreAnimationFrames[playerData->playerSlot] += (playerScoreAnimationFrames[playerData->playerSlot] < 256) ? 1 : 0;
    if (playerScoreAnimationFrames[playerData->playerSlot] <= 15) {
        scoreAnimationProgress = 1.0 - playerScoreAnimationFrames[playerData->playerSlot] / 15.0;
        scoreDigitScale = 1.0f;
        scoreJitterX = 0.0f;
        scoreJitterY = 0.0f;
        for (scoreDigitIndex = 0; scoreDigitIndex < 2; scoreDigitIndex++) {
            if (playerLastScoreChanges[playerData->playerSlot] > 0) {
                float scoreDigitScaleByDelta[21] = { 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 7.0f, 0.0f, 0.0f, 0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 15.0f };

                scoreAnimationProgress = (playerScoreAnimationFrames[playerData->playerSlot] < 10) ? 1.0 - playerScoreAnimationFrames[playerData->playerSlot] / 10.0 : 0.0;
                scoreDigitScale = 1.0 + (scoreAnimationProgress * 0.5 + scoreAnimationProgress * scoreAnimationProgress * 0.5) * scoreDigitScaleByDelta[playerLastScoreChanges[playerData->playerSlot]] / 10.0;
                espScaleSet(portraitSpriteIds[(scoreDigitIndex == 0 ? 2 : 3) + playerData->playerSlot * 5], scoreDigitScale, scoreDigitScale);
            }
            else if (playerLastScoreChanges[playerData->playerSlot] < 0) {
                s32 scoreDigitOffsets[4] = { 100, 485, 16, -4 };

                scoreJitterX = scoreAnimationProgress * playerLastScoreChanges[playerData->playerSlot] * (rand8() / 255.0 - 0.5);
                scoreJitterY = scoreAnimationProgress * playerLastScoreChanges[playerData->playerSlot] * (rand8() / 255.0 - 0.5);
                espPosSet(
                    portraitSpriteIds[(scoreDigitIndex == 0 ? 2 : 3) + playerData->playerSlot * 5], (&scoreDigitOffsets[scoreDigitIndex])[2] + scoreDigitOffsets[playerData->playerSlot] + scoreJitterX, 72.0f + scoreJitterY);
            }
        }
    }
}

s32 cameraSceneMode = -1;
float cameraStartZoom = 2775.0f;
float cameraStartCenterY = 314.0f;
float cameraStartCenterZ = -100.0f;
float cameraStartRotationX = -38.75f;

void MinigameUpdate(OMOBJ *object)
{
    PlayerRuntimeData *playerDataBySlot[2];
    float nearestPieceDistance;
    float pieceDistance;
    float scoreDigitFrameCount;
    float pieceWorldX;
    float pieceWorldZ;
    float pieceRotationY;
    float scoreDigitTravelDistance;
    float scoreDigitPathLength;
    float remainingPathFraction;
    float pieceStartX;
    float pieceStartZ;
    float scoreTargetX;
    float scoreTargetZ;
    float remainingTravelFraction;
    float pieceScreenX;
    float pieceScreenZ;
    float screenTargetX;
    float screenTargetZ;
    float scoreAnimationProgress;
    float remainingAnimationFraction;
    HU3DMODEL *frontBoardModel;
    HU3DMODEL *backBoardModel;
    HU3DMODEL *backPieceModel;
    HU3DMODEL *frontPieceModel;
    BoardPieceData *pieceData;
    float returnAnimationProgress;
    float pieceDistanceSquared;
    float scoreDigitProgress;
    float scoreWorldX;
    float scoreWorldZ;
    float scoreTargetOffsetZ;
    float scoreTargetOffsetX;
    float cameraBlendProgress;
    float pieceRotationX;
    float pieceWorldY;
    s32 randomBoardPosition;
    s32 randomIndex;
    s32 pieceCount;
    s32 hasUnselectedPiece;
    s32 nearestPieceIndex;
    s32 i;

    for (i = 0; i < 2; i++) {
        playerDataBySlot[i] = playerObjects[i]->data;
    }
    totalUpdateFrames++;
    roundElapsedFrames++;
    if (gamePhase == 1005) {
        gameplayElapsedFrames++;
        if (gameplayElapsedFrames == 270 * REFRESH_RATE) {
            timeoutSequenceId = MGSeqCreate(1, ((REFRESH_RATE * 300 + REFRESH_RATE) - gameplayElapsedFrames) / REFRESH_RATE, -1, -1);
            MGSeqPosSet(timeoutSequenceId, 288.0f, 400.0f);
        }
        if (timeoutSequenceId != -1) {
            MGSeqParamSet(timeoutSequenceId, 1, ((REFRESH_RATE * 300 + REFRESH_RATE) - gameplayElapsedFrames) / REFRESH_RATE);
        }
    }
    particleSpawnPosition[0] = particleSpawnPosition[1] = particleSpawnPosition[2] = 0.0f;
    if (gamePhase == 1004 && gamePhaseFrames == 0) {
        gamePhaseFrames++;
        boardTransitionState = 1;
        boardTransitionFrames = 0;
        for (i = 0; i < 20; i++) {
            boardPieceStates[i].animationState = 2;
            boardPieceStates[i].animationFrames = 0;
            boardPieceStates[i].fallCountdownOrScoreDigitIndex = rand8() % 16;
            boardPieceStates[i].fallVelocity = -40.0f;
        }
    }
    switch (boardTransitionState) {
        case 1:
            boardTransitionFrames++;
            if (boardTransitionFrames == 1) {
                playerDataBySlot[0]->behaviorState = playerDataBySlot[1]->behaviorState = 6;
                playerDataBySlot[0]->stateFrames = playerDataBySlot[1]->stateFrames = 0;
            }
            pieceCount = 0;
            for (i = 0; i < 20; i++) {
                pieceCount += (boardPieceStates[i].fallVelocity == 0.0f);
            }
            if (pieceCount >= 20) {
                boardTransitionState = 2;
                boardTransitionFrames = 0;
            }
            break;
        case 2:
            boardTransitionFrames++;
            if (boardTransitionFrames >= 30) {
                boardTransitionState = 0;
                boardTransitionFrames = 0;
                for (i = 0; i < 20; i++) {
                    boardPieceStates[i].animationState = 3;
                    boardPieceStates[i].animationFrames = 0;
                }
                if (scoreEffectStates[8].activeEffectCount < 3) {
                    scoreEffectStates[8].effectAges[scoreEffectStates[8].activeEffectCount] = 0.0f;
                    scoreEffectStates[8].activeEffectCount++;
                }
            }
            break;
        case 3:
            boardTransitionFrames++;
            if (boardTransitionFrames == 1) {
                s32 randomizedPieceIndex = 0;
                s32 usedRandomPositions[20] = { 0 };

                for (randomizedPieceIndex = 0; randomizedPieceIndex < 20; randomizedPieceIndex++) {
                    frontBoardModel = &Hu3DData[object->mdlId[randomizedPieceIndex + 7]];
                    backBoardModel = &Hu3DData[object->mdlId[randomizedPieceIndex + 27]];
                    do {
                        do {
                            randomIndex = rand8();
                        } while (randomIndex >= 240);
                    } while (usedRandomPositions[randomIndex % 20] != 0);
                    usedRandomPositions[randomIndex % 20] = 1;
                    boardPieceStates[randomizedPieceIndex].column = (randomIndex % 20) % 5;
                    boardPieceStates[randomizedPieceIndex].row = ((randomIndex % 20) / 5) % 4;
                    frontBoardModel->pos.x = backBoardModel->pos.x = 100.0 + 200.0 * (boardPieceStates[randomizedPieceIndex].column - 2.5);
                    frontBoardModel->pos.z = backBoardModel->pos.z = 100.0 + 200.0 * (boardPieceStates[randomizedPieceIndex].row - 2.0);
                }
            }
            if (boardTransitionFrames >= 10) {
                boardTransitionState = 4;
                boardTransitionFrames = 0;
                playerScoreDigitCounts[0] = 1;
            }
            break;
        case 4:
            if (boardTransitionFrames == 0 && scoreEffectStates[0].activeEffectCount < 3) {
                scoreEffectStates[0].effectAges[scoreEffectStates[0].activeEffectCount] = 0.0f;
                scoreEffectStates[0].activeEffectCount++;
            }
            boardTransitionFrames++;
            if (boardTransitionFrames % 4 == 1) {
                nearestPieceDistance = 10000.0f;
                nearestPieceIndex = -1;
                hasUnselectedPiece = 0;
                for (i = 0; i < 20; i++) {
                    hasUnselectedPiece = (boardPieceStates[i].animationState == 6 || !(boardPieceStates[i].column == 0 && boardPieceStates[i].row == 0)) ? 1 : hasUnselectedPiece;
                    if (boardPieceStates[i].animationState != 6 && (boardPieceStates[i].column != 0 || boardPieceStates[i].row != 0)) {
                        pieceDistanceSquared = boardPieceStates[i].column * boardPieceStates[i].column + boardPieceStates[i].row * boardPieceStates[i].row;
                        pieceDistance = sqrtf(pieceDistanceSquared) + (rand8() / 256.0) * 0.01;
                        if (nearestPieceDistance > pieceDistance) {
                            nearestPieceIndex = i;
                            nearestPieceDistance = pieceDistance;
                        }
                    }
                }
                if (nearestPieceIndex != -1) {
                    boardPieceStates[nearestPieceIndex].animationState = 6;
                    boardPieceStates[nearestPieceIndex].animationFrames = 0;
                    playerScoreDigitCounts[0]++;
                    boardPieceStates[nearestPieceIndex].fallCountdownOrScoreDigitIndex = playerScoreDigitCounts[0] - 1;
                }
                if (hasUnselectedPiece == 0) {
                    boardTransitionState = 5;
                    boardTransitionFrames = 0;
                }
            }
            break;
        case 5:
            boardTransitionFrames++;
            if (boardTransitionFrames == 5 && scoreEffectStates[1].activeEffectCount < 3) {
                scoreEffectStates[1].effectAges[scoreEffectStates[1].activeEffectCount] = 0.0f;
                scoreEffectStates[1].activeEffectCount++;
            }
            if (boardTransitionFrames == 5) {
                s32 usedReturnPositions[20] = { 0 };

                for (i = 0; i < 20; i++) {
                    boardPieceStates[i].animationState = 7;
                    boardPieceStates[i].animationFrames = (19 - boardPieceStates[i].fallCountdownOrScoreDigitIndex) * 3;
                    do {
                        randomBoardPosition = rand8() % 20;
                    } while (usedReturnPositions[randomBoardPosition] != 0);
                    usedReturnPositions[randomBoardPosition] = 1;
                    boardPieceStates[i].column = randomBoardPosition % 5;
                    boardPieceStates[i].row = randomBoardPosition / 5;
                }
            }
            if (boardTransitionFrames == 115 && backgroundMusicSequenceId != -1) {
                HuAudSeqFadeOut(backgroundMusicSequenceId, 1000);
                backgroundMusicSequenceId = -1;
            }
            if (boardTransitionFrames == 145) {
                gamePhaseFrames++;
            }
            break;
    }
    for (i = 0; i < 20; i++) {
        frontPieceModel = &Hu3DData[object->mdlId[i + 7]];
        backPieceModel = &Hu3DData[object->mdlId[i + 27]];
        pieceWorldX = frontPieceModel->pos.x;
        pieceWorldY = frontPieceModel->pos.y;
        pieceWorldZ = frontPieceModel->pos.z;
        pieceRotationX = frontPieceModel->rot.x;
        pieceRotationY = frontPieceModel->rot.y;
        pieceData = &boardPieceStates[i];
        switch (boardPieceStates[i].animationState) {
            case 2:
                pieceData->animationFrames++;
                pieceData->fallCountdownOrScoreDigitIndex -= (pieceData->fallCountdownOrScoreDigitIndex > 0);
                pieceRotationX = 180.0f;
                if (pieceData->fallCountdownOrScoreDigitIndex == 0) {
                    if (pieceWorldY > 0.0f && pieceWorldY + pieceData->fallVelocity <= 0.0f && scoreEffectStates[7].activeEffectCount < 3) {
                        scoreEffectStates[7].effectAges[scoreEffectStates[7].activeEffectCount] = 0.0f;
                        scoreEffectStates[7].activeEffectCount++;
                    }
                    pieceWorldY += pieceData->fallVelocity;
                }
                if (pieceWorldY < 150.0 * sind(ABS(pieceRotationX))) {
                    pieceData->fallVelocity = 0.0f;
                    pieceWorldY = 150.0 * sind(ABS(pieceRotationX));
                }
                break;
            case 3:
                pieceData->animationFrames++;
                pieceWorldY = 2.0 * (pieceData->animationFrames / 35.0) - 1.0;
                pieceWorldY = 1.0f - pieceWorldY * pieceWorldY;
                pieceWorldY *= 200.0f;
                pieceRotationX = 180.0 + 360.0 * (pieceData->animationFrames / 35.0) * 0.5;
                if (pieceData->animationFrames >= 35) {
                    pieceWorldY = 0.0f;
                    pieceData->animationState = 4;
                    pieceData->animationFrames = 0;
                }
                break;
            case 4:
                pieceData->animationFrames++;
                pieceRotationX = 0.0f;
                if (boardPieceStates->animationState == 4) {
                    pieceData->animationFrames = boardPieceStates->animationFrames;
                }
                if (pieceData->animationFrames > 120) {
                    pieceData->animationState++;
                    pieceData->animationFrames = 0;
                }
                break;
            case 5:
                if (i == 0 && pieceData->animationFrames == 0 && scoreEffectStates[9].activeEffectCount < 3) {
                    scoreEffectStates[9].effectAges[scoreEffectStates[9].activeEffectCount] = 0.0f;
                    scoreEffectStates[9].activeEffectCount++;
                }
                pieceData->animationFrames++;
                pieceWorldY = 2.0 * (pieceData->animationFrames / 45.0) - 1.0;
                pieceWorldY = 1.0f - pieceWorldY * pieceWorldY;
                pieceWorldY *= 200.0f;
                pieceRotationX = 360.0 * (pieceData->animationFrames / 45.0) * 1.5;
                if (pieceData->animationFrames >= 45) {
                    pieceWorldY = 0.0f;
                    boardTransitionState = 3;
                    boardTransitionFrames = 0;
                    pieceData->animationState = 0;
                    pieceData->animationFrames = 0;
                }
                break;
            case 6:
                pieceData->animationFrames++;
                scoreDigitTravelDistance = 20.0f;
                scoreWorldX = 100.0 + 200.0 * (pieceData->column - 2.5);
                scoreWorldZ = 100.0 + 200.0 * (pieceData->row - 2.0);
                scoreTargetOffsetZ = -400.0f;
                scoreTargetOffsetX = -300.0f;
                scoreDigitPathLength = sqrtf((scoreWorldX - scoreTargetOffsetZ) * (scoreWorldX - scoreTargetOffsetZ) + (scoreWorldZ - scoreTargetOffsetX) * (scoreWorldZ - scoreTargetOffsetX));
                scoreDigitFrameCount = scoreDigitPathLength / scoreDigitTravelDistance;
                scoreDigitProgress = pieceData->animationFrames * scoreDigitTravelDistance;
                scoreDigitProgress = (scoreDigitProgress >= scoreDigitPathLength) ? 1.0f : (scoreDigitProgress / scoreDigitPathLength);
                scoreDigitProgress = 0.75 * scoreDigitProgress + 0.25 * (1.0f - (1.0f - scoreDigitProgress) * (1.0f - scoreDigitProgress));
                remainingPathFraction = 1.0f - scoreDigitProgress;
                pieceWorldX = scoreWorldX * remainingPathFraction + scoreTargetOffsetZ * scoreDigitProgress;
                pieceWorldZ = scoreWorldZ * remainingPathFraction + scoreTargetOffsetX * scoreDigitProgress;
                pieceWorldY = (pieceData->fallCountdownOrScoreDigitIndex * 10 < pieceData->animationFrames * scoreDigitTravelDistance) ? pieceData->fallCountdownOrScoreDigitIndex * 10 : pieceData->animationFrames * scoreDigitTravelDistance;
                scoreDigitFrameCount = 4.0f * (pieceData->animationFrames / scoreDigitFrameCount) - 2.0f;
                scoreDigitFrameCount = 4.0f - scoreDigitFrameCount * scoreDigitFrameCount;
                pieceWorldY = scoreDigitFrameCount / 4.0 * scoreDigitProgress * 0.5 * (pieceData->fallCountdownOrScoreDigitIndex * 10 + 100) + scoreDigitProgress * pieceData->fallCountdownOrScoreDigitIndex * 10.0f;
                if (scoreDigitProgress >= 1.0f) {
                    pieceData->animationState = 0;
                    pieceData->animationFrames = 0;
                    pieceData->column = 0;
                    pieceData->row = 0;
                    pieceWorldY = pieceData->fallCountdownOrScoreDigitIndex * 10;
                }
                break;
            case 7:
                pieceData->animationFrames++;
                if (pieceData->animationFrames == 60 && i != 0) {
                    playerScoreDigitCounts[0]--;
                }
                if (pieceData->animationFrames >= 60) {
                    pieceStartX = 100.0 + 200.0 * (pieceData->column - 2.5);
                    pieceStartZ = 100.0 + 200.0 * (pieceData->row - 2.0);
                    scoreTargetX = -400.0f;
                    scoreTargetZ = -300.0f;
                    returnAnimationProgress = (pieceData->animationFrames - 60) / 50.0;
                    // Cast required to match.
                    returnAnimationProgress = (double)(1.0f - (1.0f - returnAnimationProgress) * (1.0f - returnAnimationProgress));
                    remainingTravelFraction = 1.0f - returnAnimationProgress;
                    pieceWorldX = scoreTargetX * remainingTravelFraction + pieceStartX * returnAnimationProgress;
                    pieceWorldZ = scoreTargetZ * remainingTravelFraction + pieceStartZ * returnAnimationProgress;
                    pieceWorldY = pieceData->fallCountdownOrScoreDigitIndex * 10 * remainingTravelFraction;
                    if (returnAnimationProgress >= 1.0f) {
                        pieceData->animationState = 0;
                        pieceData->animationFrames = 0;
                        pieceData->fallCountdownOrScoreDigitIndex = 0;
                        playerScoreDigitCounts[0] = 0;
                    }
                }
                break;
            case 8:
                if (pieceData->animationFrames == 0) {
                    if (scoreEffectStates[(pieceData->scoreChange > 0) ? 3 : 4].activeEffectCount < 3) {
                        scoreEffectStates[(pieceData->scoreChange > 0) ? 3 : 4].effectAges[scoreEffectStates[(pieceData->scoreChange > 0) ? 3 : 4].activeEffectCount] = 0.0f;
                        scoreEffectStates[(pieceData->scoreChange > 0) ? 3 : 4].activeEffectCount++;
                    }
                }
                pieceData->animationFrames++;
                pieceWorldY = 2.0 * (pieceData->animationFrames / 75.0) - 1.0;
                pieceWorldY = 1.0f - pieceWorldY * pieceWorldY;
                pieceWorldY *= 400.0f;
                pieceRotationX = 360.0 * (pieceData->animationFrames / 75.0) * 3.0;
                if (pieceData->animationFrames == 1) {
                    Hu3DParticleCntSet(boardObject->mdlId[5], 0);
                }
                pieceScreenX = 100.0 + 200.0 * (pieceData->column - 2.5);
                pieceScreenZ = 100.0 + 200.0 * (pieceData->row - 2.0);
                screenTargetX = (pieceData->ownerPlayerIndex == 0) ? -640.0 : 640.0;
                screenTargetZ = (pieceData->ownerPlayerIndex == 0) ? 0.0 : 0.0;
                scoreAnimationProgress = pieceData->animationFrames / 75.0;
                remainingAnimationFraction = 1.0f - scoreAnimationProgress;
                pieceWorldY += scoreAnimationProgress * (playerScoreDigitCounts[pieceData->ownerPlayerIndex] * 10 + 33);
                pieceWorldX = pieceScreenX * remainingAnimationFraction + screenTargetX * scoreAnimationProgress;
                pieceWorldZ = pieceScreenZ * remainingAnimationFraction + screenTargetZ * scoreAnimationProgress;
                particleSpawnPosition[0] = pieceWorldX;
                particleSpawnPosition[1] = pieceWorldY;
                particleSpawnPosition[2] = pieceWorldZ;
                if (pieceData->animationFrames >= 75) {
                    pieceWorldY = playerScoreDigitCounts[pieceData->ownerPlayerIndex] * 10 + 33;
                    playerScoreDigitCounts[pieceData->ownerPlayerIndex]++;
                    playerScores[pieceData->ownerPlayerIndex] += pieceData->scoreChange;
                    playerLastScoreChanges[pieceData->ownerPlayerIndex] = pieceData->scoreChange;
                    playerScoreAnimationFrames[pieceData->ownerPlayerIndex] = 0;
                    playerScores[pieceData->ownerPlayerIndex] = (playerScores[pieceData->ownerPlayerIndex] < 0) ? 0 : playerScores[pieceData->ownerPlayerIndex];
                    playerDataBySlot[pieceData->ownerPlayerIndex]->behaviorState = 5;
                    playerDataBySlot[pieceData->ownerPlayerIndex]->stateFrames = 0;
                    pieceData->animationState = 9;
                    pieceData->animationFrames = 0;
                    if (scoreEffectStates[(pieceData->scoreChange) > 0 ? 5 : 6].activeEffectCount < 3) {
                        scoreEffectStates[(pieceData->scoreChange) > 0 ? 5 : 6].effectAges[scoreEffectStates[(pieceData->scoreChange) > 0 ? 5 : 6].activeEffectCount] = 0.0f;
                        scoreEffectStates[(pieceData->scoreChange) > 0 ? 5 : 6].activeEffectCount++;
                    }
                    if (playerScores[pieceData->ownerPlayerIndex] >= 50 || playerScoreDigitCounts[0] + playerScoreDigitCounts[1] >= 20) {
                        winnerPlayerIndex = (playerScores[pieceData->ownerPlayerIndex] >= 50) ? pieceData->ownerPlayerIndex
                            : (playerScores[0] > playerScores[1])          ? 0
                            : (playerScores[0] < playerScores[1])          ? 1
                                                                               : 4;
                        playerDataBySlot[1 - pieceData->ownerPlayerIndex]->behaviorState = 7;
                        playerDataBySlot[1 - pieceData->ownerPlayerIndex]->stateFrames = 0;
                    }
                }
                break;
        }
        frontPieceModel->pos.x = backPieceModel->pos.x = pieceWorldX;
        frontPieceModel->pos.y = pieceWorldY;
        frontPieceModel->pos.z = backPieceModel->pos.z = pieceWorldZ;
        frontPieceModel->rot.x = backPieceModel->rot.x = pieceRotationX;
        frontPieceModel->rot.y = pieceRotationY;
        if (frontPieceModel->pos.y <= 0.0f || pieceData->animationState == 9) {
            Hu3DModelAttrSet(object->mdlId[i + 7], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(object->mdlId[i + 27], HU3D_ATTR_DISPOFF);
            backPieceModel->pos.y = (pieceData->animationState != 9) ? 0.0f : frontPieceModel->pos.y;
        }
        else {
            Hu3DModelAttrReset(object->mdlId[i + 7], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(object->mdlId[i + 27], HU3D_ATTR_DISPOFF);
        }
    }
    {
        s32 previousCameraSceneMode = cameraSceneMode;
        float cameraScenePresets[][4] = { { 2500.0f, 0.0f, -100.0f, -40.0f }, { 2775.0f, 314.0f, -100.0f, -38.75f }, { 875.0f, 116.0f, -100.0f, -15.0f },
            { 1625.0f, -416.0f, -340.0f, -30.25f }, { 875.0f, 116.0f, -100.0f, -30.0f } };

        if (cameraSceneMode == -1) {
            cameraSceneMode = 1;
            cameraBlendDirection = 0;
            cameraBlendFactor = 0.0f;
        }
        if (gamePhase == 1003 && gamePhaseFrames == 1) {
            cameraSceneMode = 2;
            cameraBlendDirection = 0;
            cameraBlendFactor = 0.0f;
        }
        if (gamePhase == 1004 && cameraSceneMode != 0) {
            cameraSceneMode = 0;
            cameraBlendDirection = 1;
            cameraBlendFactor = 0.0f;
        }
        if (gamePhase == 1007 && gamePhaseFrames == 1) {
            cameraSceneMode = 3;
            cameraBlendDirection = 0;
            cameraBlendFactor = 0.0f;
        }
        if (previousCameraSceneMode != cameraSceneMode && previousCameraSceneMode != -1) {
            cameraStartZoom = CZoom;
            cameraStartCenterY = Center.y;
            cameraStartCenterZ = Center.z;
            cameraStartRotationX = CRot.x;
        }
        if (cameraBlendFactor < 1.0f) {
            if (cameraBlendDirection == 0) {
                cameraBlendFactor = 1.0 - 0.95 * (1.0f - cameraBlendFactor);
                cameraBlendProgress = cameraBlendFactor;
            }
            else {
                cameraBlendFactor += 0.011111111111111112;
                cameraBlendFactor = (cameraBlendFactor > 1.0f) ? 1.0f : cameraBlendFactor;
                cameraBlendProgress = 0.5 + -0.5 * cosd(180.0f * cameraBlendFactor);
            }
            cameraBlendFactor = (cameraBlendFactor > 1.0f) ? 1.0f : cameraBlendFactor;
            CZoom = cameraBlendProgress * cameraScenePresets[cameraSceneMode][0] + cameraStartZoom * (1.0f - cameraBlendProgress);
            Center.y = cameraBlendProgress * cameraScenePresets[cameraSceneMode][1] + cameraStartCenterY * (1.0f - cameraBlendProgress);
            Center.z = cameraBlendProgress * cameraScenePresets[cameraSceneMode][2] + cameraStartCenterZ * (1.0f - cameraBlendProgress);
            CRot.x = cameraBlendProgress * cameraScenePresets[cameraSceneMode][3] + cameraStartRotationX * (1.0f - cameraBlendProgress);
        }
    }
}

const s32 introMessageIds[] = { MAKE_MESSID(43, 3), MAKE_MESSID(43, 4), MAKE_MESSID(43, 5), MAKE_MESSID(43, 6), MAKE_MESSID(43, 7),
    MAKE_MESSID(43, 8), MAKE_MESSID(43, 9), MAKE_MESSID(43, 10), 0 };

void IntroMessageProcess(void)
{
    PlayerRuntimeData *playerDataBySlot[2];
    s16 choiceResult;
    s16 messageWindowId;
    u32 messageId;
    s32 openingMessageWaitCount;
    s32 closingMessageWaitCount;
    s32 i;
    s32 openingWaitFrameIndex;
    s32 closingWaitFrameIndex;

    choiceResult = 0;
    for (i = 0; i < 2; i++) {
        playerDataBySlot[i] = playerObjects[i]->data;
    }
    HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, REFRESH_RATE);
    HuWinComKeyReset();
    messageWindowId = HuWinExCreateStyled(-10000.0f, 60.0f, 280, 120, -1, 1);
    HuWinExAnimIn(messageWindowId);
    HuWinBGTPLvlSet(messageWindowId, 0.8f);
    HuWinMesSpeedSet(messageWindowId, 1);
    HuWinDisablePlayerSet(
        messageWindowId, 0xF & ((playerDataBySlot[0]->controllerPort != -1) ? ~(1 << playerDataBySlot[0]->controllerPort) : 0xF) & ((playerDataBySlot[1]->controllerPort != -1) ? ~(1 << playerDataBySlot[1]->controllerPort) : 0xF));
    HuWinMesColSet(messageWindowId, 0);
    HuWinMesSet(messageWindowId, MAKE_MESSID(43, 1));
    if (((playerDataBySlot[0]->controllerPort == -1) || (playerDataBySlot[0]->characterId == -1)) && ((playerDataBySlot[1]->controllerPort == -1) || (playerDataBySlot[1]->characterId == -1))) {
#if VERSION_PAL
        openingMessageWaitCount = HuWinKeyWaitNumGet(DATA_MAKE_NUM(DATADIR_M412, 1));
        for (openingWaitFrameIndex = 0; openingWaitFrameIndex < openingMessageWaitCount; openingWaitFrameIndex++) {
#endif
            HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, REFRESH_RATE);
#if VERSION_PAL
        }
#endif
        }
    HuWinMesWait(messageWindowId);
    if (playerDataBySlot[0]->controllerPort != -1 || playerDataBySlot[1]->controllerPort != -1) {
        HuWinMesColSet(messageWindowId, 0);
        HuWinMesSet(messageWindowId, MAKE_MESSID(43, 2));
        if (playerDataBySlot[0]->controllerPort == -1 && playerDataBySlot[1]->controllerPort == -1) {
            HuPrcSleep(REFRESH_RATE);
            choiceResult = 1;
        }
        else {
            choiceResult = HuWinChoiceGet(messageWindowId, 0);
        }
    }
    else {
        choiceResult = 1;
    }
    if (choiceResult == 0) {
        i = 0;
        while ((messageId = introMessageIds[i++]) != 0) {
            HuWinMesColSet(messageWindowId, 0);
            HuWinMesSet(messageWindowId, messageId);
            if ((playerDataBySlot[0]->controllerPort == -1 || playerDataBySlot[0]->characterId == -1) && (playerDataBySlot[1]->controllerPort == -1 || playerDataBySlot[1]->characterId == -1)) {
                HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, REFRESH_RATE);
            }
            HuWinMesWait(messageWindowId);
        }
    }
    else {
        HuWinMesColSet(messageWindowId, 0);
        HuWinMesSet(messageWindowId, MAKE_MESSID(43, 10));
        if ((playerDataBySlot[0]->controllerPort == -1 || playerDataBySlot[0]->characterId == -1) && (playerDataBySlot[1]->controllerPort == -1 || playerDataBySlot[1]->characterId == -1)) {
#if VERSION_PAL
            closingMessageWaitCount = HuWinKeyWaitNumGet(DATA_MAKE_NUM(DATADIR_M412, 10));
            for (closingWaitFrameIndex = 0; closingWaitFrameIndex < closingMessageWaitCount; closingWaitFrameIndex++) {
#endif
                HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, REFRESH_RATE);
#if VERSION_PAL
            }
#endif
        }
        HuWinMesWait(messageWindowId);
    }
    HuWinExAnimOut(messageWindowId);
    HuWinExCleanup(messageWindowId);
    startMessageComplete = 0;
    HuPrcEnd();
}

void GameStateObjectUpdate(OMOBJ *object)
{
    float ambientLightLevel;
    PlayerRuntimeData *playerDataBySlot[2];
    s32 modelBrightness;
    s32 i;

    for (i = 0; i < 13; i++) {
        s32 scoreMessageIds[] = { 0x0000078E, 0x0000078F, 0x00000790, 0x00000791, 0x00000792, 0x00000793, 0x00000794, 0x00000795, 0x00000796, 0x00000797,
            0x00000798, 0x00000799, 0x0000079A };

        if (scoreEffectStates[i].activeEffectCount > 3) {
            scoreEffectStates[i].activeEffectCount = 3;
        }
        if (scoreEffectStates[i].activeEffectCount > 0) {
            modelBrightness = HuAudFXPlay(scoreMessageIds[i]);
            scoreEffectStates[i].activeEffectCount--;
        }
    }
    for (i = 0; i < 2; i++) {
        playerDataBySlot[i] = playerObjects[i]->data;
    }
    if (omSysExitReq != 0 && WipeStatGet() == 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = PlayerObjectCleanup;
    }
    switch (gamePhase) {
        case 1000:
            if (WipeStatGet() == 0) {
                gamePhase++;
                gamePhaseFrames = 0;
            }
            break;
        case 1001:
            gamePhaseFrames++;
            if (gamePhaseFrames == 1) {
                Hu3DModelAttrReset(boardObject->mdlId[1], HU3D_MOTATTR_PAUSE);
                Hu3DModelAttrReset(boardObject->mdlId[2], HU3D_MOTATTR_PAUSE);
            }
            if (gamePhaseFrames > 0 && gamePhaseFrames <= 40 && gamePhaseFrames % 10 == 0 && scoreEffectStates[12].activeEffectCount < 3) {
                scoreEffectStates[12].effectAges[scoreEffectStates[12].activeEffectCount] = 0.0f;
                scoreEffectStates[12].activeEffectCount++;
            }
            if (gamePhaseFrames == 50 && scoreEffectStates[10].activeEffectCount < 3) {
                scoreEffectStates[10].effectAges[scoreEffectStates[10].activeEffectCount] = 0.0f;
                scoreEffectStates[10].activeEffectCount++;
            }
            if (gamePhaseFrames == 200 && scoreEffectStates[11].activeEffectCount < 3) {
                scoreEffectStates[11].effectAges[scoreEffectStates[11].activeEffectCount] = 0.0f;
                scoreEffectStates[11].activeEffectCount++;
            }
            if (gamePhaseFrames >= 210) {
                ambientLightLevel = (gamePhaseFrames - 210) / 30.0;
                Hu3DModelAmbSet(playerObjects[0]->mdlId[0], ambientLightLevel, ambientLightLevel, ambientLightLevel);
                Hu3DModelAmbSet(playerObjects[1]->mdlId[0], ambientLightLevel, ambientLightLevel, ambientLightLevel);
            }
            if (gamePhaseFrames > 240) {
                gamePhase++;
                gamePhaseFrames = 0;
                Hu3DModelAttrSet(boardObject->mdlId[1], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrSet(boardObject->mdlId[2], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(boardObject->mdlId[0], HU3D_ATTR_DISPOFF);
                Hu3DModelAmbSet(playerObjects[0]->mdlId[0], 1.0f, 1.0f, 1.0f);
                Hu3DModelAmbSet(playerObjects[1]->mdlId[0], 1.0f, 1.0f, 1.0f);
            }
            break;
        case 1002:
            gamePhaseFrames++;
            if (gamePhaseFrames > 60) {
                gamePhase++;
                if (playerDataBySlot[0]->characterId != -1 && playerDataBySlot[1]->characterId != -1) {
                    gamePhase++;
                }
                gamePhaseSubstate = 0;
                gamePhaseFrames = 0;
                if (backgroundMusicSequenceId == -1) {
                    backgroundMusicSequenceId = HuAudSeqPlay(0x3D);
                }
            }
            break;
        case 1003:
            gamePhaseFrames++;
            if (gamePhaseFrames == 1) {
                HuPrcChildCreate(IntroMessageProcess, 100, 0x2000, 0, HuPrcCurrentGet());
                startMessageComplete = 1;
            }
            else if (startMessageComplete == 0) {
                gamePhase++;
                gamePhaseFrames = 0;
            }
            break;
        case 1004:
            gamePhaseFrames += (gamePhaseFrames >= 2);
            if (gamePhaseFrames == 5 && messageSequenceId < 0) {
                messageSequenceId = MGSeqCreate(3, 0);
            }
            if (gamePhaseFrames >= 5 && backgroundMusicSequenceId == -1 && (MGSeqStatGet(messageSequenceId) & 0x10)) {
                backgroundMusicSequenceId = HuAudSeqPlay(0x3E);
            }
            if (gamePhaseFrames >= 5 && MGSeqStatGet(messageSequenceId) == 0 && messageSequenceId >= 0) {
                gamePhase++;
                messageSequenceId = -1;
                gamePhaseFrames = 0;
            }
            break;
        case 1005:
            gamePhaseFrames++;
            if (countdownSequenceId != -1) {
                MGSeqParamSet(countdownSequenceId, 1, ((roundCountdownFrames < 0 ? 0 : roundCountdownFrames) + REFRESH_RATE - 1) / REFRESH_RATE);
                roundCountdownFrames--;
                if (roundCountdownFrames <= -REFRESH_RATE / 2) {
                    MGSeqParamSet(countdownSequenceId, 2, -1);
                    countdownSequenceId = -1;
                }
            }
            if (winnerPlayerIndex != -1) {
                gamePhase = 1006;
                gamePhaseFrames = 0;
            }
            break;
        case 1006:
            gamePhaseFrames++;
            if (messageSequenceId < 0 && gamePhaseFrames == 1) {
                if (countdownSequenceId != -1) {
                    MGSeqParamSet(countdownSequenceId, 2, -1);
                }
                if (timeoutSequenceId != -1) {
                    MGSeqParamSet(timeoutSequenceId, 2, -1);
                }
                messageSequenceId = MGSeqCreate(3, 1);
                if (backgroundMusicSequenceId == -1) {
                    break;
                }
                HuAudSeqFadeOut(backgroundMusicSequenceId, 100);
                backgroundMusicSequenceId = -1;
            }
            else if (MGSeqStatGet(messageSequenceId) == 0) {
                gamePhase++;
                gamePhaseFrames = 0;
            }
            break;
        case 1007:
            gamePhaseFrames++;
            if (gamePhaseFrames == 1) {
                if (winnerPlayerIndex != 4) {
                    if (playerDataBySlot[winnerPlayerIndex]->characterId != -1) {
                        HuAudSStreamPlay(1);
                    }
                    else {
                        HuAudSStreamPlay(3);
                    }
                    GWPlayerCoinWinSet(playerDataBySlot[winnerPlayerIndex]->playerConfigIndex, 10);
                }
                else if (winnerPlayerIndex == 4) {
                    HuAudSStreamPlay(4);
                }
            }
            if (REFRESH_RATE_F * 3.5f < gamePhaseFrames) {
                messageSequenceId = -1;
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                object->objFunc = PlayerObjectCleanup;
            }
            break;
    }
}

void ParticleEffectHook(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    s16 i;

    if (particle->count == 0) {
        particleData = particle->data;
        for (i = 0; i < particle->maxCnt; i++, particleData++) {
            particleData->time = i * 2 + 100;
            particleData->color.a = 0;
            particleData->scale = 0.0f;
        }
    }
    particleData = particle->data;
    for (i = 0; i < particle->maxCnt; i++, particleData++) {
        if (!particleData->scale) {
            break;
        }
    }
    particleData = particle->data;
    for (i = 0; i < particle->maxCnt; i++, particleData++) {
        if (particleData->time == 100) {
            particleData->time = (particleSpawnPosition[0] == 0.0f && particleSpawnPosition[1] == 0.0f && particleSpawnPosition[2] == 0.0f) ? 0 : particleData->time;
            particleData->pos.x = particleSpawnPosition[0] + 75.0 * (rand8() / 255.0);
            particleData->pos.y = particleSpawnPosition[1] + 75.0 * (rand8() / 255.0);
            particleData->pos.z = particleSpawnPosition[2] + 75.0 * (rand8() / 255.0);
            particleData->color.r = 0xFF;
            particleData->color.g = 0xF0;
            particleData->color.b = 0xA0;
            particleData->color.a = 0xFF;
            particleData->zRot = 0.785f;
            particleData->scale = 40.0f;
        }
        if (particleData->time <= 100) {
            particleData->pos.y -= 3.0f;
            particleData->scale = ((particleData->time > 50) ? (particleData->time - 50) / 50.0 : 0.0) * (rand8() * 80 / 255.0);
        }
        if (particleData->time == 0) {
            particleData->scale = 0.0f;
        }
        particleData->time -= (particleData->time > 0);
    }
}

void GameObjectInitialize(OMOBJ *object)
{
    PlayerRuntimeData *playerData;
    PlayerRuntimeData *opponentData;
    s32 boardPieceIndex;
    s32 pieceDataIndex;
    s32 i, j, k;
    s32 modelGroupOffset;

    memset(boardPieceStates, 0, sizeof(boardPieceStates));
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M448, 0));
    Hu3DModelShadowMapSet(object->mdlId[0]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(object->mdlId[0], 0);
    Hu3DMotionTimeSet(object->mdlId[0], 20.0f);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    object->mdlId[6] = Hu3DHookFuncCreate(ModelCaptureRenderHook);
    Hu3DModelLayerSet(object->mdlId[6], 6);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M448, 1));
    Hu3DModelShadowMapSet(object->mdlId[1]);
    Hu3DModelLayerSet(object->mdlId[1], 0);
    Hu3DModelPosSet(object->mdlId[1], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_MOTATTR_PAUSE);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M448, 2));
    Hu3DModelShadowMapSet(object->mdlId[2]);
    Hu3DModelLayerSet(object->mdlId[2], 0);
    Hu3DModelPosSet(object->mdlId[2], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_MOTATTR_PAUSE);
    {
        s32 playerModelResourceIds[] = { DATA_MAKE_NUM(DATADIR_M448, 9), DATA_MAKE_NUM(DATADIR_M448, 10), DATA_MAKE_NUM(DATADIR_M448, 11),
            DATA_MAKE_NUM(DATADIR_M448, 12), DATA_MAKE_NUM(DATADIR_M448, 13), DATA_MAKE_NUM(DATADIR_M448, 14), DATA_MAKE_NUM(DATADIR_M448, 16),
            DATA_MAKE_NUM(DATADIR_M448, 15), DATA_MAKE_NUM(DATADIR_M448, 17) };

        for (i = 0; i < 2; i++) {
            playerData = playerObjects[i]->data;
            object->mdlId[i + 3] = Hu3DModelCreateFile(playerModelResourceIds[(playerData->characterId == -1) ? 8 : playerData->characterId]);
            Hu3DModelShadowMapSet(object->mdlId[i + 3]);
            Hu3DModelLayerSet(object->mdlId[i + 3], 0);
            Hu3DModelRotSet(object->mdlId[i + 3], 0.0f, (playerData->characterId == -1) ? (1 - i) * 180 : i * 180, 0.0f);
        }
    }
    {
        s32 pieceScoreChanges[] = { -10, -5, -5, 3, 3, 3, 3, 3, 5, 5, 5, 5, 5, 10, 10, 10, 10, 10, 20, 20 };
        s32 pieceModelTypeIndices[] = { 0, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5 };
        s32 pieceModelResourceIds[] = { DATA_MAKE_NUM(DATADIR_M448, 3), DATA_MAKE_NUM(DATADIR_M448, 4), DATA_MAKE_NUM(DATADIR_M448, 5), DATA_MAKE_NUM(DATADIR_M448, 6),
            DATA_MAKE_NUM(DATADIR_M448, 7), DATA_MAKE_NUM(DATADIR_M448, 8) };
        s32 sharedPieceModelIndices[] = { -1, -1, -1, -1, -1, -1 };

        for (i = 0; i < 4; i++) {
            for (j = 0; j < 5; j++) {
                boardPieceIndex = j + i * 5;
                boardPieceStates[boardPieceIndex].column = j;
                boardPieceStates[boardPieceIndex].row = i;
                boardPieceStates[boardPieceIndex].fallCountdownOrScoreDigitIndex = 0;
                boardPieceStates[boardPieceIndex].ownerPlayerIndex = -1;
                boardPieceStates[boardPieceIndex].animationState = 1;
                pieceDataIndex = 19 - boardPieceIndex;
                boardPieceStates[boardPieceIndex].scoreChange = pieceScoreChanges[pieceDataIndex];
                pieceScoreChanges[pieceDataIndex] = 0;
                boardPieceStates[boardPieceIndex].animationFrames = 0;
                for (k = 0; k < 2; k++) {
                    modelGroupOffset = (k == 0) ? 7 : 27;
                    if (sharedPieceModelIndices[pieceModelTypeIndices[pieceDataIndex]] == -1) {
                        if (k == 1) {
                            sharedPieceModelIndices[pieceModelTypeIndices[pieceDataIndex]] = boardPieceIndex;
                        }
                        object->mdlId[modelGroupOffset + boardPieceIndex] = Hu3DModelCreateFile(pieceModelResourceIds[pieceModelTypeIndices[pieceDataIndex]]);
                    }
                    else {
                        object->mdlId[modelGroupOffset + boardPieceIndex] = Hu3DModelLink(object->mdlId[modelGroupOffset + sharedPieceModelIndices[pieceModelTypeIndices[pieceDataIndex]]]);
                    }
                    Hu3DModelPosSet(object->mdlId[modelGroupOffset + boardPieceIndex], 100.0 + 200.0 * (boardPieceStates[boardPieceIndex].column - 2.5), (k == 0) ? 4000 : 0,
                        100.0 + 200.0 * (boardPieceStates[boardPieceIndex].row - 2.0));
                    if (k == 0) {
                        Hu3DModelShadowSet(object->mdlId[modelGroupOffset + boardPieceIndex]);
                    }
                    else {
                        Hu3DModelAttrSet(object->mdlId[modelGroupOffset + boardPieceIndex], HU3D_ATTR_DISPOFF);
                        Hu3DModelShadowMapSet(object->mdlId[modelGroupOffset + boardPieceIndex]);
                    }
                }
            }
        }
    }
    object->mdlId[5] = Hu3DParticleCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M448, 30), HU_MEMNUM_OVL)), 70);
    Hu3DModelPosSet(object->mdlId[5], 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(object->mdlId[5], 1.0f, 1.0f, 1.0f);
    Hu3DModelLayerSet(object->mdlId[5], 3);
    Hu3DModelAttrReset(object->mdlId[5], HU3D_ATTR_DISPOFF);
    Hu3DParticleColSet(object->mdlId[5], 0xFF, 0xFF, 0xFF);
    Hu3DParticleScaleSet(object->mdlId[5], 1.0f);
    Hu3DParticleHookSet(object->mdlId[5], ParticleEffectHook);
    Hu3DParticleCntSet(object->mdlId[5], 0);
    Hu3DParticleBlendModeSet(object->mdlId[5], 1);
    for (i = 0; i < 10; i++) {
        s32 portraitBaseX[] = { 100, 485 };
        s32 portraitOffsetsX[] = { 0, 0, 16, -4, -36, 0, 0, 16, -4, -36 };
        s32 portraitOffsetsY[] = { 0, 0, 2, 2, -2, 0, 0, 2, 2, -2 };

        if (portraitResourceDirectories[i] == DATADIR_MGCONST) {
            opponentData = playerObjects[i == 4 ? 0 : (i == 9 ? 1 : 0)]->data;
            portraitSpriteIds[i]
                = espEntry((opponentData->characterId == -1) ? DATA_MAKE_NUM(DATADIR_M448, 29) : DATA_MAKE_NUM(portraitResourceDirectories[i], opponentData->characterId), 0, 0);
        }
        else {
            portraitSpriteIds[i] = espEntry(portraitResourceDirectories[i], 0, 0);
        }
        espDrawNoSet(portraitSpriteIds[i], 0);
        espAttrSet(portraitSpriteIds[i], HUSPR_ATTR_NOANIM | HUSPR_ATTR_DISPOFF);
        espBankSet(portraitSpriteIds[i], 0);
        espPriSet(portraitSpriteIds[i], 128 - i);
        espPosSet(portraitSpriteIds[i], portraitOffsetsX[i] + portraitBaseX[i < 5 ? 0 : 1], portraitOffsetsY[i] + 70);
        if (((i >= 5) ? i - 5 : i) != 0) {
            (void)1; // Required to match.
        }
        else {
            espTPLvlSet(portraitSpriteIds[i], 0.5f);
            espColorSet(portraitSpriteIds[i], 0, 0, 0);
        }
    }
    object->objFunc = MinigameUpdate;
}

void PlayerObjectInitialize(OMOBJ *object)
{
    s32 characterModelResourceId;
    s32 computerCharacterId;
    PlayerRuntimeData *playerData;
    PlayerRuntimeData *opponentData;
    s32 controllerPort;
    s32 i;

    characterModelResourceId = -1;
    computerCharacterId = -1;
    playerData = object->data;
    opponentData = playerObjects[1 - object->work[0]]->data;
    memset(playerData, 0, sizeof(*playerData));
    playerData->playerSlot = object->work[0];
    if (GWPlayerCfg->character == 14 || GWPlayerCfg[1].character == 14 || GWPlayerCfg[2].character == 14 || GWPlayerCfg[3].character == 14) {
        for (i = 0; i < 4; i++) {
            if (GWPlayerCfg[i].character == 14) {
                characterModelResourceId = i;
            }
        }
        computerCharacterId = (GWPlayerCfg[characterModelResourceId].group == 0) ? 1 : 0;
        if (playerData->playerSlot == 0) {
            playerData->playerConfigIndex = computerCharacterId;
            controllerPort = characterModelResourceId;
        }
        else {
            playerData->playerConfigIndex = characterModelResourceId;
            controllerPort = computerCharacterId;
        }
    }
    else {
        playerData->playerConfigIndex = (GWPlayerCfg[0].group == playerData->playerSlot) ? 0
            : GWPlayerCfg[1].group == playerData->playerSlot               ? 1
            : GWPlayerCfg[2].group == playerData->playerSlot               ? 2
            : GWPlayerCfg[3].group == playerData->playerSlot               ? 3
                                                                    : 4;
        controllerPort = (GWPlayerCfg[0].group == 1 - playerData->playerSlot) ? 0
            : (GWPlayerCfg[1].group == 1 - playerData->playerSlot)     ? 1
            : (GWPlayerCfg[2].group == 1 - playerData->playerSlot)     ? 2
            : (GWPlayerCfg[3].group == 1 - playerData->playerSlot)     ? 3
                                                                : 4;
    }
    if (playerData->playerConfigIndex == 4) {
        OSReport("m448:wrong player group\n");
        playerData->playerConfigIndex = playerData->playerSlot;
    }
    playerData->characterId = (characterModelResourceId != -1 && playerData->playerSlot == 1) ? -1 : GWPlayerCfg[playerData->playerConfigIndex].character;
    playerData->controllerPort = (playerData->characterId == -1 || GWPlayerCfg[playerData->playerConfigIndex].iscom) ? -1 : GWPlayerCfg[playerData->playerConfigIndex].pad_idx;
    playerData->difficultyLevel = (playerData->controllerPort != -1) ? 0 : GWPlayerCfg[playerData->playerConfigIndex].diff;
    playerData->behaviorState = 0;
    if (playerData->characterId != -1) {
        object->mdlId[0] = CharModelCreate(playerData->characterId, 2);
    }
    else {
        object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M448, 18));
    }
    Hu3DModelLayerSet(object->mdlId[0], 0);
    CharModelLayerSetAll2(1);
    Hu3DModelShadowSet(object->mdlId[0]);
    Hu3DModelAmbSet(object->mdlId[0], 0.0f, 0.0f, 0.0f);
    for (i = 0; i < 11; i++) {
        if (playerData->characterId != -1) {
            object->mtnId[i] = CharMotionCreate(playerData->characterId, characterMotionIds[i]);
        }
        else {
            object->mtnId[i] = Hu3DJointMotionFile(object->mdlId[0], boardMotionDataIds[i]);
        }
    }
    playerData->currentMotionIndex = (playerData->characterId != -1) ? 0 : 0;
    playerData->motionOverrideIndex = -1;
    Hu3DMotionSet(object->mdlId[0], object->mtnId[playerData->currentMotionIndex]);
    if (playerData->characterId != -1) {
        Hu3DMotionTimeSet(object->mdlId[0], (playerData->playerSlot == 0) ? 0 : 80);
        Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    }
    else {
        dustMotionIdA = CharNpcDustSet(object->mdlId[0], object->mtnId[2], 1, 14);
        dustMotionIdB = CharNpcDustSet(object->mdlId[0], object->mtnId[6], 5, 14);
    }
    object->trans.x = playerStartPositions[playerData->playerSlot][0];
    object->trans.y = playerStartPositions[playerData->playerSlot][1];
    object->trans.z = 0.0f;
    object->rot.y = (playerData->playerSlot == 0) ? 90 : 270;
    object->rot.y = (playerData->characterId != -1) ? 0 : (playerData->playerSlot == 0) ? 90 : 270;
    object->scale.x = object->scale.y = object->scale.z = 1.0f;
    object->rot.x = object->rot.z = 0.0f;
    object->objFunc = PlayerBehaviorUpdate;
}

void GameStateInitialize(OMOBJ *object)
{
    gamePhase = 1000;
    gamePhaseFrames = 0;
    boardTransitionState = 0;
    boardTransitionFrames = 0;
    captureBufferSize = GXGetTexBufferSize(640, 480, GX_TF_RGB565, GX_FALSE, 0);
    thumbnailBufferSize = GXGetTexBufferSize(160, 120, GX_TF_RGB565, GX_FALSE, 0U);
    captureBuffer = HuMemDirectMallocNum(HEAP_HEAP, captureBufferSize, HU_MEMNUM_OVL);
    memset(captureBuffer, 0, captureBufferSize);
    thumbnailBuffer = HuMemDirectMallocNum(HEAP_HEAP, thumbnailBufferSize, HU_MEMNUM_OVL);
    memset(thumbnailBuffer, 0, thumbnailBufferSize);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    object->objFunc = GameStateObjectUpdate;
}

void ObjectSetup(void)
{
    OMOBJ *playerObject;
    s32 i;

    OSReport("******* M448 ObjectSetup *********\n");
    objectManager = omInitObjMan(600, 0x2000);
    omGameSysInit(objectManager);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 30.0f, 20.0f, 25000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    CZoom = 2500.0f;
    Center.x = 0.0f;
    Center.y = 0.0f;
    Center.z = -100.0f;
    CRot.x = -40.0f;
    CRot.y = 0.0f;
    CRot.z = 0.0f;
    {
        Vec lightPosition = { 0.0f, 10000.0f, 0.0f };
        Vec lightDirection = { 0.0f, -1.0f, 0.0f };
        GXColor lightColor = { 0xFF, 0xFF, 0xFF, 0xFF };

        directionalLightId = Hu3DGLightCreateV(&lightPosition, &lightDirection, &lightColor);
        Hu3DGLightStaticSet(directionalLightId, 1);
        Hu3DGLightInfinitytSet(directionalLightId);
        directionalLightId = Hu3DGLightCreateV(&lightPosition, &lightDirection, &lightColor);
        Hu3DGLightStaticSet(directionalLightId, 1);
        Hu3DGLightInfinitytSet(directionalLightId);
    }
    {
        Vec shadowPosition;
        Vec shadowUpVector;
        Vec shadowTarget;

        shadowPosition.x = 0.0f;
        shadowPosition.y = 10000.0f;
        shadowPosition.z = 0.0f;
        shadowUpVector.x = 0.0f;
        shadowUpVector.y = 1.0f;
        shadowUpVector.z = 0.0f;
        shadowTarget.x = 0.0f;
        shadowTarget.y = 0.0f;
        shadowTarget.z = 0.5f;
        Hu3DShadowCreate(8.0f, 9000.0f, 11500.0f);
        Hu3DShadowTPLvlSet(0.75f);
        Hu3DShadowPosSet(&shadowPosition, &shadowUpVector, &shadowTarget);
    }
    messageSequenceId = -1;
    winnerPlayerIndex = -1;
    HuWinInit(1);
    boardObject = omAddObjEx(objectManager, 101, 47, 0, 1, GameObjectInitialize);
    for (i = 0; i < 2; i++) {
        playerObject = playerObjects[i] = omAddObjEx(objectManager, 100, 1, 11, 0, PlayerObjectInitialize);
        playerObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerRuntimeData), HU_MEMNUM_OVL);
        playerObject->work[0] = i;
    }
    viewExitObject = omAddObjEx(objectManager, 32730, 0, 0, -1, omOutView);
    gameStateObject = omAddObjEx(objectManager, 103, 0, 0, -1, GameStateInitialize);
}

void PlayerObjectCleanup(OMOBJ *object)
{
    s32 i;
    PlayerRuntimeData *playerData;

    if (WipeStatGet() != 0) {
        return;
    }
    if (captureBuffer) {
        HuMemDirectFree(captureBuffer);
    }
    captureBuffer = NULL;
    if (thumbnailBuffer) {
        HuMemDirectFree(thumbnailBuffer);
    }
    thumbnailBuffer = NULL;
    for (i = 0; i < 2; i++) {
        playerData = playerObjects[i]->data;
        if (playerData->characterId != -1) {
            CharModelKill(playerData->characterId);
        }
    }
    if (messageSequenceId >= 0) {
        MGSeqKill(messageSequenceId);
    }
    MGSeqKillAll();
    HuAudAllStop();
    omOvlReturnEx(1, 1);
}
