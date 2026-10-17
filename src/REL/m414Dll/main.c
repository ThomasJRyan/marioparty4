#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/frand.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"

#ifndef __MWERKS__
#include "game/hsfex.h"
#endif

typedef struct EffectParticleContext {
    s32 effectActive;
    s32 playerIndex;
    float effectAmplitude;
    char reservedEffectData[0x38];
    s32 unusedEffectParameter;
    char reservedEffectMetadata[0x0C];
    s32 particleAnimationState;
    char reservedSpawnMetadata[0x24];
    float spawnCenterX;
    float spawnCenterY;
    float spawnCenterZ;
    char reservedTargetMetadata[0xB0];
    Vec playerTargetPositions[16];
    s32 targetPlayerIndex;
} EffectParticleContext; // size unknown

typedef struct PlayerSceneState {
    s16 sceneState[9];
    s16 secondaryModelId;
    s16 motionIds[6];
    s16 shadowModelId;
    s16 shadowMotionIds[5];
    s16 spriteGroupId;
    s16 spriteIndex;
    s16 playerModels[9];
    s16 sharedModelId;
    s32 playerIndex;
    s32 cameraId;
    s32 controllerPort;
    s32 playerProcessState;
    s32 particleAnimationState;
    s32 playerActionState;
    s32 animationTimer;
    float positionX;
    float positionY;
    float positionZ;
    float rotationX;
    float rotationY;
    float rotationZ;
    char playerIndices[4];
    float spawnCenterX;
    float spawnCenterY;
    float spawnCenterZ;
    float effectPosition;
    char effectIndices[4];
    float effectRotation;
    s16 playerCharacterIds[16];
    s16 playerMotionIds[16];
    s16 playerModelIds[16];
    s32 playerAnimationStates[16];
    Vec playerPositions[16];
    s32 playerCount;
    s32 targetPlayerIndex;
    s8 horizontalInput;
    s8 verticalInput;
    s8 leftTriggerInput;
    s8 rightTriggerInput;
    u16 effectFlags;
    char reservedEffectFlags[2];
    s32 effectCooldown;
} PlayerSceneState; // size 0x208

typedef struct PlayerAnimationContext {
    PlayerSceneState *sceneState;
    s32 playerIndex;
    union {
        s32 actionDirection;
        s32 *actionStatePointer;
    };
    s32 animationSequenceActive;
} PlayerAnimationContext; // size unknown

typedef struct CameraBobContext {
    s32 sceneState;
    s32 playerIndex;
    float targetPlayerIndex;
} CameraBobContext; // size unknown

typedef struct PlayerMotionContext {
    PlayerSceneState *sceneState;
    s32 playerIndex;
    union {
        s32 actionDirection;
        s32 *actionStatePointer;
    };
} PlayerMotionContext; // size unknown

s16 directionalLightId;
s16 backgroundModelId;
s16 playerInputHistory[16][4];
s32 roundState;
s32 playerRoundStates[4];
s32 winningCharacter;
s32 playerCharacterOrder[16];
s32 playerInputCounters[2];
float playerAngles[4];
float cameraOrbitDistances[4];
PlayerSceneState playerSceneStates[4];
HUPROCESS *playerProcesses[4];
HUPROCESS *gameSequenceProcess;
Vec cameraPositions[5];
Vec cameraLookAtPositions[5];
Vec cameraUpVectors[5];
OMOBJ *cameraUpdateObject;
Vec shadowLightPosition;
Vec shadowLightTarget;
Vec shadowLightDirection;

void UpdateCameraPositions(OMOBJ *object);
void UpdateSparkleParticlesPlayer0(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform);
void UpdateSparkleParticlesPlayer1(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform);
void UpdateSparkleParticlesPlayer2(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform);
void UpdateSparkleParticlesPlayer3(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform);
void UpdateStarParticlesPlayer0(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform);
void UpdateStarParticlesPlayer1(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform);
void UpdateStarParticlesPlayer2(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform);
void UpdateStarParticlesPlayer3(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform);
void UpdateRingParticlesPlayer0(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform);
void UpdateRingParticlesPlayer1(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform);
void UpdateRingParticlesPlayer2(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform);
void UpdateRingParticlesPlayer3(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform);
void InitializePlayerScenes();
void InitializePlayerModels();
void SetPlayerRotation(PlayerSceneState *playerState, float targetAngle);
s32 GetPlayerInput(s32 playerIndex, s32 inputPort);
void RunPlayerProcess();
void EndRound();
void RunGameSequence();

HU3DPARTICLEHOOK sparkleParticleHooks[] = { UpdateSparkleParticlesPlayer0, UpdateSparkleParticlesPlayer1, UpdateSparkleParticlesPlayer2, UpdateSparkleParticlesPlayer3 };

HU3DPARTICLEHOOK starParticleHooks[] = { UpdateStarParticlesPlayer0, UpdateStarParticlesPlayer1, UpdateStarParticlesPlayer2, UpdateStarParticlesPlayer3 };

HU3DPARTICLEHOOK ringParticleHooks[] = { UpdateRingParticlesPlayer0, UpdateRingParticlesPlayer1, UpdateRingParticlesPlayer2, UpdateRingParticlesPlayer3 };

s32 characterMotionIds[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

s32 characterModelDataIds[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0), DATA_MAKE_NUM(DATADIR_MARIOMOT, 79), DATA_MAKE_NUM(DATADIR_MARIOMOT, 72),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 23), DATA_MAKE_NUM(DATADIR_MARIOMOT, 24), DATA_MAKE_NUM(DATADIR_MARIOMOT, 54) };

float animationSpeeds[] = { 0.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f };

s32 victorySoundIds[] = { 75, 64, 67, 79, 73, 80 };

s32 cameraViewportX[] = { 230, 510, 230, 510 };

s32 cameraViewportY[] = { 205, 205, 420, 420 };

s32 playerCharacterColors[][3] = {
    // rgb color
    { 254, 0, 0 },
    { 1, 0, 254 },
    { 255, 123, 222 },
    { 0, 255, 1 },
    { 175, 50, 240 },
    { 225, 127, 80 },
    { 255, 255, 1 },
    { 0, 0, 0 },
};

void ObjectSetup(void)
{
    s32 index;
    s32 randomIndex;
    HUPROCESS *process;
    s32 randomIndex2;
    s32 randomIndex3;
    s32 value;

    process = omInitObjMan(50, 8192);
    omGameSysInit(process);
    Hu3DCameraCreate(31);
    Hu3DCameraPerspectiveSet(31, 30.0f, 20.0f, 25000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 10.0f, 20.0f, 309.0f, 219.0f, 0.0f, 1.0f);
    Hu3DCameraViewportSet(2, 322.0f, 20.0f, 309.0f, 219.0f, 0.0f, 1.0f);
    Hu3DCameraViewportSet(4, 10.0f, 242.0f, 309.0f, 219.0f, 0.0f, 1.0f);
    Hu3DCameraViewportSet(8, 322.0f, 242.0f, 309.0f, 219.0f, 0.0f, 1.0f);
    Hu3DCameraViewportSet(16, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 10, 20, 309, 219);
    Hu3DCameraScissorSet(2, 322, 20, 309, 219);
    Hu3DCameraScissorSet(4, 10, 242, 309, 219);
    Hu3DCameraScissorSet(8, 322, 242, 309, 219);
    Hu3DCameraScissorSet(16, 0, 0, 640, 480);
    cameraUpdateObject = omAddObjEx(process, 32730, 0, 0, -1, UpdateCameraPositions);
    cameraUpdateObject->work[0] = 4;

    for (index = 0; index < cameraUpdateObject->work[0]; index++) {
        playerAngles[index] = 0.0f;
        cameraOrbitDistances[index] = 1900.0f;
        cameraPositions[index].y = 455.0f;
        cameraLookAtPositions[index].x = index % 2 * 2000.0f;
        cameraLookAtPositions[index].z = index / 2 * 2000.0f;
        cameraLookAtPositions[index].y = 610.0f;
        cameraUpVectors[index].x = 0.0f;
        cameraUpVectors[index].y = 1.0f;
        cameraUpVectors[index].z = 0.0f;
    }
    Hu3DCameraPosSet(16, 0.0f, 265.0f, 180.0f, 0.0f, 1.0f, 0.0f, 0.0f, 265.0f, 0.0f);
    Hu3DLighInit();
    directionalLightId = Hu3DGLightCreate(10000.0f, 0.0f, 10000.0f, -1.0f, 0.0f, -1.0f, 255, 255, 255);
    Hu3DGLightPosAimSet(directionalLightId, 0.0f, 4330.13f, 2500.0f, 0.0f, 0.0f, 0.0f);
    Hu3DGLightInfinitytSet(directionalLightId);
    shadowLightPosition.x = 1000.0f;
    shadowLightPosition.y = 7000.0f;
    shadowLightPosition.z = 0.0f;
    shadowLightTarget.x = shadowLightTarget.z = 0.0f;
    shadowLightTarget.y = 1.0f;
    shadowLightDirection.x = 1000.0f;
    shadowLightDirection.y = 0.0f;
    shadowLightDirection.z = 1000.0f;
    Hu3DShadowCreate(30.0f, 20.0f, 50000.0f);
    Hu3DShadowTPLvlSet(0.8f);
    Hu3DShadowPosSet(&shadowLightPosition, &shadowLightTarget, &shadowLightDirection);
    Hu3DBGColorSet(0, 0, 0);
    winningCharacter = frandmod(6);

    for (index = 0; index < 3; index++) {
        playerCharacterOrder[index] = winningCharacter;
    }
    randomIndex = frandmod(6);

    for (; index < 16; index++, randomIndex++) {
        if (randomIndex == winningCharacter) {
            randomIndex++;
        }
        if (randomIndex >= 6) {
            if (winningCharacter == 0) {
                randomIndex = 1;
            }
            else {
                randomIndex = 0;
            }
        }
        playerCharacterOrder[index] = randomIndex;
    }
    for (index = 0; index < 50; index++) {
        randomIndex2 = frandmod(16);
        randomIndex3 = frandmod(16);
        value = playerCharacterOrder[randomIndex2];
        playerCharacterOrder[randomIndex2] = playerCharacterOrder[randomIndex3];
        playerCharacterOrder[randomIndex3] = value;
    }
    InitializePlayerScenes();
    gameSequenceProcess = HuPrcChildCreate(RunGameSequence, 4096, 8192, 0, HuPrcCurrentGet());
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
}

void UpdateCameraPositions(OMOBJ *object)
{
    s32 index;

    for (index = 0; index < object->work[0]; index++) {
        cameraPositions[index].x = (index % 2 * 2000.0f) + (cameraOrbitDistances[index] * sind(playerAngles[index]));
        cameraPositions[index].z = (index / 2 * 2000.0f) + (cameraOrbitDistances[index] * cosd(playerAngles[index]));

        Hu3DCameraPosSet(1 << index, cameraPositions[index].x, cameraPositions[index].y, cameraPositions[index].z, cameraUpVectors[index].x,
            cameraUpVectors[index].y, cameraUpVectors[index].z, cameraLookAtPositions[index].x, cameraLookAtPositions[index].y, cameraLookAtPositions[index].z);
    }
}

void AnimateCameraBob(void)
{
    float angle;
    float effectValue2;
    float effectValue5;
    float effectValue6;

    CameraBobContext *cameraBobContext;
    s32 index;

    cameraBobContext = HuPrcCurrentGet()->user_data;
    if (!cameraBobContext->sceneState) {
        effectValue3 = cameraPositions[cameraBobContext->playerIndex].y;
        effectValue4 = cameraLookAtPositions[cameraBobContext->playerIndex].y;

        for (index = 0, angle = 0.0f; index < 10; index++, angle += 36.0f) {
            effectValue = cameraBobContext->targetPlayerIndex * sind(angle);
            cameraPositions[cameraBobContext->playerIndex].y = effectValue3 + effectValue;
            cameraLookAtPositions[cameraBobContext->playerIndex].y = effectValue4 + effectValue;
            HuPrcVSleep();
        }
        cameraPositions[cameraBobContext->playerIndex].y = effectValue3;
        cameraLookAtPositions[cameraBobContext->playerIndex].y = effectValue4;
    }
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void StartCameraBob(s32 waitForGameState, s32 playerIndex, float bobAmplitude)
{
    CameraBobContext cameraBobContext;
    CameraBobContext *value;
    HUPROCESS *process;

    value = &cameraBobContext;
    process = HuPrcChildCreate(AnimateCameraBob, 8192, 8192, 0, HuPrcCurrentGet());
    process->user_data = value;
    value->sceneState = waitForGameState;
    value->playerIndex = playerIndex;
    value->targetPlayerIndex = bobAmplitude;
}

void UpdateSparkleParticles(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform, s32 playerIndex)
{
    float effectValue;
    float effectValue2;
    float angle;
    float effectValue3;
    double effectValue4;

    HU3DPARTICLEDATA *particleData;
    PlayerSceneState *playerState;
    s16 index;
    s16 index2;
    s16 index3;
    s32 value;

    if (particleSystem->count == 0) {
        particleData = particleSystem->data;

        for (index = 0; index < particleSystem->maxCnt; index++, particleData++) {
            particleData->accel.x = particleData->scale = 0.0f;
        }
    }
    particleData = particleSystem->data;
    playerState = &playerSceneStates[playerIndex];
    if (playerState->playerActionState == 1) {
        for (index2 = 0; index2 < 2; index2++) {
            effectValue3 = 2.4f;
            for (index3 = 0, angle = 0.0f; index3 < 4; index3++, angle += 90.0f) {
                for (index = 0; index < particleSystem->maxCnt; index++, particleData++) {
                    if (0.0f == particleData->accel.x) {
                        break;
                    }
                }
                if (index == particleSystem->maxCnt) {
                    break;
                }
                effectValue = angle + frandmod(70);
                effectValue2 = effectValue3 + (3.6f * frandf());
                particleData->pos.x = playerState->spawnCenterX + ((20.0f * index2) * (sind(effectValue) * cosd(playerAngles[playerState->playerIndex])));
                particleData->pos.y = playerState->spawnCenterY + ((20.0f * index2) * cosd(effectValue));
                particleData->pos.z = playerState->spawnCenterZ + (-(20.0f * index2) * (sind(playerAngles[playerState->playerIndex]) * sind(effectValue)));
                particleData->vel.x = effectValue2 * (sind(effectValue) * cosd(playerAngles[playerState->playerIndex]));
                particleData->vel.y = effectValue2 * cosd(effectValue);
                particleData->vel.z = -effectValue2 * (sind(playerAngles[playerState->playerIndex]) * sind(effectValue));
                particleData->accel.x = 30.0f;
                particleData->scale = 40.0f + (20.0f * frandf());
                particleData->color.r = frandmod(20) + 235;
                particleData->color.g = frandmod(50) + 205;
                particleData->color.b = frandmod(50);
                particleData->color.a = 255;
            }
        }
        playerState->playerActionState++;
    }
    particleData = particleSystem->data;
    for (index = 0; index < particleSystem->maxCnt; index++, particleData++) {
        if (0.0f != particleData->accel.x) {
            VECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
            if (particleData->accel.x > 10.0f) {
                particleData->accel.x = particleData->accel.x - 1.0f;
            }
            if (10.0f == particleData->accel.x) {
                alphaValue = particleData->color.a - 25;
                if (alphaValue < 0) {
                    alphaValue = 0;
                    particleData->accel.x = 0.0f;
                }
                particleData->color.a = alphaValue;
            }
            if (0.0f == particleData->accel.x) {
                particleData->scale = 0.0f;
            }
        }
    }
}

void UpdateSparkleParticlesPlayer0(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform)
{
    UpdateSparkleParticles(model, particleSystem, transform, 0);
}

void UpdateSparkleParticlesPlayer1(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform)
{
    UpdateSparkleParticles(model, particleSystem, transform, 1);
}

void UpdateSparkleParticlesPlayer2(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform)
{
    UpdateSparkleParticles(model, particleSystem, transform, 2);
}

void UpdateSparkleParticlesPlayer3(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform)
{
    UpdateSparkleParticles(model, particleSystem, transform, 3);
}

void UpdateStarParticles(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform, s32 playerIndex)
{
    HU3DPARTICLEDATA *particleData;
    s16 index;
    PlayerSceneState *playerState;
    s32 value;

    if (particleSystem->count == 0) {
        particleData = particleSystem->data;
        for (index = 0; index < particleSystem->maxCnt; index++, particleData++) {
            particleData->accel.x = particleData->scale = 0.0f;
        }
    }
    particleData = particleSystem->data;
    playerState = &playerSceneStates[playerIndex];
    if (playerState->playerActionState == 2) {
        for (index = 0; index < particleSystem->maxCnt; index++, particleData++) {
            if (0.0f == particleData->accel.x) {
                break;
            }
        }
        if (index != particleSystem->maxCnt) {
            particleData->pos.x = playerState->spawnCenterX;
            particleData->pos.y = playerState->spawnCenterY;
            particleData->pos.z = playerState->spawnCenterZ;
            particleData->accel.x = 30.0f;
            particleData->scale = 300.0f;
            particleData->color.r = 255;
            particleData->color.g = 255;
            particleData->color.b = 255;
            particleData->color.a = 255;
        }
        playerState->playerActionState++;
    }
    particleData = particleSystem->data;
    for (index = 0; index < particleSystem->maxCnt; index++, particleData++) {
        if (0.0f != particleData->accel.x) {
            if (particleData->accel.x > 25.0f) {
                particleData->accel.x -= 1.0f;
            }
            if (25.0f == particleData->accel.x) {
                alphaValue = particleData->color.a - 25;
                if (alphaValue < 0) {
                    alphaValue = 0;
                    particleData->accel.x = 0.0f;
                }
                particleData->color.a = alphaValue;
            }
            if (0.0f == particleData->accel.x) {
                particleData->scale = 0.0f;
            }
        }
    }
}

void UpdateStarParticlesPlayer0(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform)
{
    UpdateStarParticles(model, particleSystem, transform, 0);
}

void UpdateStarParticlesPlayer1(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform)
{
    UpdateStarParticles(model, particleSystem, transform, 1);
}

void UpdateStarParticlesPlayer2(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform)
{
    UpdateStarParticles(model, particleSystem, transform, 2);
}

void UpdateStarParticlesPlayer3(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform)
{
    UpdateStarParticles(model, particleSystem, transform, 3);
}

void UpdateRingParticles(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform, s32 playerIndex)
{
    float effectValue;
    float angle;

    HU3DPARTICLEDATA *particleData;
    PlayerSceneState *playerState;
    s16 index;
    s16 index2;
    s32 value;

    if (particleSystem->count == 0) {
        particleData = particleSystem->data;
        for (index = 0; index < particleSystem->maxCnt; index++, particleData++) {
            particleData->accel.x = particleData->scale = 0.0f;
        }
    }
    particleData = particleSystem->data;
    playerState = &playerSceneStates[playerIndex];
    if (playerState->playerActionState == 3) {
        for (index2 = 0, angle = 0.0f; index2 < 8; index2++, angle += 45.0f) {
            for (index = 0; index < particleSystem->maxCnt; index++, particleData++) {
                if (0.0f == particleData->accel.x) {
                    break;
                }
            }
            if (index == particleSystem->maxCnt) {
                break;
            }
            effectValue = angle + frandmod(30);
            particleData->pos.x = playerState->spawnCenterX + (120.0 * (sind(effectValue) * cosd(playerAngles[playerState->playerIndex])));
            particleData->pos.y = playerState->spawnCenterY + (120.0 * cosd(effectValue));
            particleData->pos.z = playerState->spawnCenterZ + (-120.0 * (sind(playerAngles[playerState->playerIndex]) * sind(effectValue)));
            particleData->zRot = (M_PI * (360.0f - effectValue)) / 180.0;
            particleData->accel.x = 30.0f;
            particleData->scale = 50.0f;
            particleData->color.r = 255;
            particleData->color.g = 255;
            particleData->color.b = 0;
            particleData->color.a = 255;
        }
        playerState->playerActionState++;
    }
    particleData = particleSystem->data;
    for (index = 0; index < particleSystem->maxCnt; index++, particleData++) {
        if (0.0f != particleData->accel.x) {
            alphaValue = particleData->color.a - 8;
            if (alphaValue < 0) {
                alphaValue = 0;
                particleData->accel.x = 0.0f;
            }
            particleData->color.a = alphaValue;
            if (0.0f == particleData->accel.x) {
                particleData->scale = 0.0f;
            }
        }
    }
}

void UpdateRingParticlesPlayer0(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform)
{
    UpdateRingParticles(model, particleSystem, transform, 0);
}

void UpdateRingParticlesPlayer1(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform)
{
    UpdateRingParticles(model, particleSystem, transform, 1);
}

void UpdateRingParticlesPlayer2(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform)
{
    UpdateRingParticles(model, particleSystem, transform, 2);
}

void UpdateRingParticlesPlayer3(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx transform)
{
    UpdateRingParticles(model, particleSystem, transform, 3);
}

void AnimatePlayerEffect(void)
{
    EffectParticleContext *effectContext;

    effectContext = HuPrcCurrentGet()->user_data;
    if (effectContext->playerIndex & 1) {
        HuAudFXPlay(1446);
    }
    else {
        HuAudFXPlay(1445);
    }
    effectContext->particleAnimationState = 1;
    effectContext->spawnCenterX = -800.0 * sind(playerAngles[effectContext->playerIndex]);
    effectContext->spawnCenterY = effectContext->playerTargetPositions[effectContext->targetPlayerIndex].x;
    effectContext->spawnCenterZ = -800.0 * cosd(playerAngles[effectContext->playerIndex]);
    effectContext->spawnCenterX += 2000.0f * (effectContext->playerIndex % 2);
    effectContext->spawnCenterZ += 2000.0f * (effectContext->playerIndex / 2);
    HuPrcSleep(30);
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void InitializePlayerScenes(void)
{
    s32 animationScores[6];
    ANIMDATA *animationDataList[4];

    float progress2;
    float effectValue;
    float effectValue2;
    float effectValue3;

    PlayerSceneState *playerState;
    s16 motionIndex;
    s32 index2;
    s32 index3;
    void *resourceData;
    HUPROCESS *process;
    ANIMDATA *animationData;
    s16 motionIndex2;
    s16 motionIndex3;
    s16 motionIndex4;
    ANIMDATA *animationData2;

    HuAudSndGrpSet(39);
    for (index3 = 0; index3 < 4; index3++) {
        playerState = &playerSceneStates[index3];
        playerState->playerIndex = index3;
        playerState->cameraId = index3;
        playerState->controllerPort = GWPlayerCfg[index3].pad_idx;
        if (!index3) {
            resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 8), HU_MEMNUM_OVL, HEAP_MODEL);
            modelId = Hu3DModelCreate(resourceData);
            motionIndex4 = modelId;
        }
        else {
            motionIndex = Hu3DModelLink(motionIndex4);
        }
        Hu3DModelCameraSet(motionIndex, 15);
        Hu3DModelPosSet(motionIndex, 2000.0f * (index3 % 2), 0.0f, 2000.0f * (index3 / 2));
        for (index2 = 0; index2 < 9; index2++) {
            if (!index3 && !index2) {
                resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 0), HU_MEMNUM_OVL, HEAP_MODEL);
                modelId = Hu3DModelCreate(resourceData);
                motionIndex2 = modelId;
            }
            else {
                motionIndex = Hu3DModelLink(motionIndex2);
            }
            playerState->playerModels[index2] = motionIndex;
            Hu3DModelCameraSet(motionIndex, 1 << playerState->cameraId);
            Hu3DModelShadowMapSet(motionIndex);
            Hu3DModelAttrSet(motionIndex, HU3D_ATTR_NOCULL);
            Hu3DModelRotSet(motionIndex, 0.0f, 40.0f * index2, 0.0f);
            Hu3DModelPosSet(motionIndex, 2000.0f * (index3 % 2), 0.0f, 2000.0f * (index3 / 2));
        }
        if (!index3) {
            resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 5), HU_MEMNUM_OVL, HEAP_MODEL);
            motionIndex = Hu3DModelCreate(resourceData);
        }
        else {
            motionIndex = Hu3DModelLink(playerSceneStates[0].sharedModelId);
        }
        playerState->sharedModelId = motionIndex;
        Hu3DModelCameraSet(motionIndex, 1 << playerState->cameraId);
        Hu3DModelShadowMapSet(motionIndex);
        Hu3DModelAttrSet(motionIndex, HU3D_ATTR_DISPOFF);
        Hu3DModelPosSet(motionIndex, 2000.0f * (index3 % 2), 0.0f, 2000.0f * (index3 / 2));
        for (index2 = 0; index2 < 9; index2++) {
            if (!index3) {
                resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 1) + animationSpeeds[index2], HU_MEMNUM_OVL, HEAP_MODEL);
                motionIndex = Hu3DModelCreate(resourceData);
            }
            else {
                motionIndex = Hu3DModelLink(playerSceneStates->sceneState[index2]);
            }
            playerState->sceneState[index2] = motionIndex;
            Hu3DModelRotSet(motionIndex, 0.0f, 40.0f * index2, 0.0f);
            Hu3DModelCameraSet(motionIndex, 1 << playerState->cameraId);
            Hu3DModelAttrSet(motionIndex, HU3D_ATTR_NOCULL);
            Hu3DModelPosSet(motionIndex, 2000.0f * (index3 % 2), 0.0f, 2000.0f * (index3 / 2));
            Hu3DModelAttrSet(motionIndex, HU3D_MOTATTR_PAUSE);
        }
        for (index2 = 0; index2 < 16; index2++) {
            if (!index3 && !index2) {
                resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 6), HU_MEMNUM_OVL, HEAP_MODEL);
                modelId = Hu3DModelCreate(resourceData);
                motionIndex3 = modelId;
            }
            else {
                resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 6), HU_MEMNUM_OVL, HEAP_MODEL);
                motionIndex = Hu3DModelCreate(resourceData);
                motionIndex3 = motionIndex;
            }
            Hu3DModelCameraSet(motionIndex, 1 << playerState->cameraId);
            playerState->playerModelIds[index2] = motionIndex;
            Hu3DModelPosSet(motionIndex, 2000.0f * (index3 % 2), 330.0f - (330.0f * (index2 & 1)), 2000.0f * (index3 / 2));
            Hu3DModelRotSet(motionIndex, 0.0f, 40.0f * ((index2 + 2) / 2), 0.0f);
            Hu3DModelAttrSet(motionIndex, HU3D_ATTR_NOCULL);
            Hu3DModelAttrSet(motionIndex, HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_SHAPE_PAUSE);
            Hu3DMotionSpeedSet(motionIndex, 3.0f);
            Hu3DMotionShapeSpeedSet(motionIndex, 3.0f);
        }
        if (!index3) {
            resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 9), HU_MEMNUM_OVL, HEAP_MODEL);
            motionIndex = Hu3DModelCreate(resourceData);
        }
        else {
            motionIndex = Hu3DModelLink(playerSceneStates[0].shadowModelId);
        }
        playerState->shadowModelId = motionIndex;
        Hu3DModelPosSet(motionIndex, 2000.0f * (index3 % 2), 0.0f, 2000.0f * (index3 / 2));
        Hu3DModelCameraSet(motionIndex, 1 << playerState->cameraId);
        Hu3DModelShadowMapObjSet(motionIndex, "m414_11_R-dai_1");
        for (index2 = 0; index2 < 5; index2++) {
            resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 10) + index2, HU_MEMNUM_OVL, HEAP_MODEL);
            playerState->shadowMotionIds[index2] = Hu3DJointMotion(motionIndex, resourceData);
        }
        Hu3DMotionSet(motionIndex, playerState->shadowMotionIds[0]);
        Hu3DMotionTimeSet(motionIndex, 0.0f);
        Hu3DModelAttrSet(motionIndex, HU3D_MOTATTR_PAUSE);
        Hu3DMotionSpeedSet(motionIndex, 2.0f);

        resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 15), HU_MEMNUM_OVL, HEAP_MODEL);
        animationData2 = HuSprAnimRead(resourceData);
        motionIndex = HuSprCreate(animationData2, 0x4000, 0);
        playerState->spriteGroupId = HuSprGrpCreate(1);
        HuSprGrpMemberSet(playerState->spriteGroupId, 0, motionIndex);
        HuSprAttrSet(playerState->spriteGroupId, 0, 4);
        HuSprColorSet(playerState->spriteGroupId, 0, playerCharacterColors[GWPlayerCfg[index3].character][0], playerCharacterColors[GWPlayerCfg[index3].character][1],
            playerCharacterColors[GWPlayerCfg[index3].character][2]);
        HuSprTPLvlSet(playerState->spriteGroupId, 0, 0.9f);
        HuSprGrpPosSet(playerState->spriteGroupId, 320.0f, 240.0f);
        if (!index3) {
            for (index2 = 0; index2 < 4; index2++) {
                resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 16) + index2, HU_MEMNUM_OVL, HEAP_MODEL);
                animationDataList[index2] = HuSprAnimRead(resourceData);
            }
        }
        playerState->spriteIndex = HuSprGrpCreate(8);
        motionIndex = HuSprCreate(animationDataList[2], 0x4000, 0);
        HuSprGrpMemberSet(playerState->spriteIndex, 0, motionIndex);
        HuSprPosSet(playerState->spriteIndex, 0, 0.0f, 0.0f);
        motionIndex = HuSprCreate(animationDataList[3], 0x4000, 0);
        HuSprGrpMemberSet(playerState->spriteIndex, 1, motionIndex);
        HuSprPosSet(playerState->spriteIndex, 1, 0.0f, 0.0f);
        HuSprTPLvlSet(playerState->spriteIndex, 1, 0.5f);

        for (index2 = 2; index2 < 8; index2++) {
            motionIndex = HuSprCreate(animationDataList[index2 & 1], 0x4000, 0);
            HuSprGrpMemberSet(playerState->spriteIndex, index2, motionIndex);
            if (index2 & 1) {
                HuSprAttrSet(playerState->spriteIndex, index2, 4);
            }
            HuSprPosSet(playerState->spriteIndex, index2, -62.0f + (30.0f * (index2 / 2)), -2.0f);
        }
        HuSprGrpPosSet(playerState->spriteIndex, cameraViewportX[playerState->playerIndex], cameraViewportY[playerState->playerIndex]);
        HuSprGrpScaleSet(playerState->spriteIndex, 0.0f, 0.0f);
        progress2 = 0.0f;
        if (!index3) {
            for (index2 = 0; index2 < 6; index2++) {
                animationScores[index2] = -1;
            }
        }
        progress2 += 40.0f;
        for (index2 = 0; index2 < 16; index2++) {
            if (!index3) {
                if (animationScores[playerCharacterOrder[index2]] == -1) {
                    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 22) + playerCharacterOrder[index2], HU_MEMNUM_OVL, HEAP_MODEL);
                    modelId = Hu3DModelCreate(resourceData);
                    animationScores[playerCharacterOrder[index2]] = modelId;
                }
                else {
                    motionIndex = Hu3DModelLink(animationScores[playerCharacterOrder[index2]]);
                }
            }
            else {
                motionIndex = Hu3DModelLink(animationScores[playerCharacterOrder[index2]]);
            }
            playerState->playerCharacterIds[index2] = motionIndex;
            playerState->playerAnimationStates[index2] = 0;
            Hu3DModelCameraSet(motionIndex, 1 << playerState->cameraId);
            Hu3DModelAttrSet(motionIndex, HU3D_MOTATTR_LOOP);
            resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 34) + playerCharacterOrder[index2], HU_MEMNUM_OVL, HEAP_MODEL);
            playerState->playerMotionIds[index2] = Hu3DJointMotion(motionIndex, resourceData);
            resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 28) + playerCharacterOrder[index2], HU_MEMNUM_OVL, HEAP_MODEL);
            Hu3DMotionSet(motionIndex, Hu3DJointMotion(motionIndex, resourceData));
            Hu3DModelAttrSet(motionIndex, HU3D_MOTATTR_LOOP);
            Hu3DModelAttrSet(motionIndex, HU3D_ATTR_NOCULL);
            effectValue2 = -950.0 * sind(progress2);
            effectValue2 = effectValue2;
            effectValue3 = -950.0 * cosd(progress2);
            effectValue3 = effectValue3;
            if (index2 & 1) {
                effectValue = 220.0f;
                progress2 += 40.0f;
            }
            else {
                effectValue = 570.0f;
            }
            if (!playerCharacterOrder[index2]) {
                effectValue += 70.0f;
            }
            HuSetVecF(&playerState->playerPositions[index2], effectValue2, effectValue, effectValue3);
            if (playerCharacterOrder[index2]) {
                (void)index2;
            }
            else {
                (void)index2;
            }
            Hu3DModelPosSet(motionIndex, effectValue2 + (2000.0f * (index3 % 2)), effectValue, effectValue3 + (2000.0f * (index3 / 2)));
            Hu3DModelRotSet(motionIndex, 0.0f, atan2d(-effectValue2, -effectValue3), 0.0f);
            if (playerCharacterOrder[index2] == 4 || playerCharacterOrder[index2] == 5) {
                Hu3DModelScaleSet(motionIndex, 0.6f, 0.6f, 0.6f);
            }
            if (playerCharacterOrder[index2] == 2) {
                Hu3DModelScaleSet(motionIndex, 1.2f, 1.2f, 1.2f);
            }
        }
        SetPlayerRotation(playerState, playerAngles[playerState->playerIndex]);
        playerState->secondaryModelId = CharModelCreate(characterMotionIds[GWPlayerCfg[index3].character], 8);
        Hu3DModelCameraSet(playerState->secondaryModelId, 1 << playerState->cameraId);
        playerState->rotationX = playerState->rotationZ = playerState->rotationY = 0.0f;
        playerState->effectRotation = 0.0f;
        playerState->positionX = 37.0 * sind(playerState->rotationY);
        playerState->positionY = 70.0f;
        playerState->positionZ = 37.0 * cosd(playerState->rotationY);
        Hu3DModelPosSet(playerState->secondaryModelId, playerState->positionX + (2000.0f * (index3 % 2)), playerState->positionY, playerState->positionZ + (2000.0f * (index3 / 2)));
        Hu3DModelRotSet(playerState->secondaryModelId, playerState->rotationX, playerState->rotationY, playerState->rotationZ);
        Hu3DModelShadowSet(playerState->secondaryModelId);
        for (index2 = 0; index2 < 6; index2++) {
            playerState->motionIds[index2] = CharMotionCreate(characterMotionIds[GWPlayerCfg[index3].character], characterModelDataIds[index2]);
        }
        CharMotionSet(characterMotionIds[GWPlayerCfg[index3].character], playerState->motionIds[0]);
        Hu3DModelAttrSet(playerState->secondaryModelId, HU3D_MOTATTR_LOOP);
        CharMotionDataClose(characterMotionIds[GWPlayerCfg[index3].character]);

        animationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 0), HU_MEMNUM_OVL));
        HuDataDirClose(DATA_MAKE_NUM(DATADIR_EFFECT, 0));
        motionIndex = Hu3DParticleCreate(animationData, 100);
        Hu3DModelPosSet(motionIndex, 0.0f, 0.0f, 0.0f);
        Hu3DModelScaleSet(motionIndex, 1.0f, 1.0f, 1.0f);
        Hu3DModelLayerSet(motionIndex, 1);
        Hu3DParticleColSet(motionIndex, 255, 255, 0);
        Hu3DParticleScaleSet(motionIndex, 0.0f);
        Hu3DParticleHookSet(motionIndex, sparkleParticleHooks[playerState->playerIndex]);
        Hu3DModelCameraSet(motionIndex, 1 << playerState->cameraId);

        animationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M414, 20), HU_MEMNUM_OVL));
        motionIndex = Hu3DParticleCreate(animationData, 1);
        Hu3DModelPosSet(motionIndex, 0.0f, 0.0f, 0.0f);
        Hu3DModelScaleSet(motionIndex, 1.0f, 1.0f, 1.0f);
        Hu3DModelLayerSet(motionIndex, 1);
        Hu3DParticleColSet(motionIndex, 255, 255, 255);
        Hu3DParticleScaleSet(motionIndex, 0.0f);
        Hu3DParticleHookSet(motionIndex, starParticleHooks[playerState->playerIndex]);
        Hu3DModelCameraSet(motionIndex, 1 << playerState->cameraId);

        animationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M414, 21), HU_MEMNUM_OVL));
        motionIndex = Hu3DParticleCreate(animationData, 8);
        Hu3DModelPosSet(motionIndex, 0.0f, 0.0f, 0.0f);
        Hu3DModelScaleSet(motionIndex, 1.0f, 1.0f, 1.0f);
        Hu3DModelLayerSet(motionIndex, 1);
        Hu3DParticleColSet(motionIndex, 255, 255, 0);
        Hu3DParticleScaleSet(motionIndex, 0.0f);
        Hu3DParticleHookSet(motionIndex, ringParticleHooks[playerState->playerIndex]);
        Hu3DModelCameraSet(motionIndex, 1 << playerState->cameraId);
        process = HuPrcChildCreate(RunPlayerProcess, 8192, 12288, 0, HuPrcCurrentGet());
        process->user_data = playerState;
        playerProcesses[index3] = process;
    }
    for (index3 = 0; index3 < 4; index3++) {
        playerState = &playerSceneStates[index3];
        resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 40), HU_MEMNUM_OVL, HEAP_MODEL);
        motionIndex = Hu3DModelCreate(resourceData);
        resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 41) + winningCharacter, HU_MEMNUM_OVL, HEAP_MODEL);
        Hu3DAnimCreate(resourceData, motionIndex, "gsp000");
        Hu3DModelPosSet(motionIndex, 2000.0f * (index3 % 2), 520.0f, -820.0f + (2000.0f * (index3 / 2)));
        Hu3DModelCameraSet(motionIndex, 1 << playerState->cameraId);
    }
    for (index3 = 0; index3 < 16; index3++) {
        for (index2 = 0; index2 < 4; index2++) {
            playerInputHistory[index3][index2] = frandmod(REFRESH_RATE * 2) + REFRESH_RATE;
        }
    }
    for (index3 = 0; index3 < 4; index3++) {
        for (index2 = 0; index2 < 16; index2++) {
            process = HuPrcChildCreate(InitializePlayerModels, 8192, 4096, 0, HuPrcCurrentGet());
            process->user_data = (void *)(index2 + (index3 << 16));
        }
    }

    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 7), HU_MEMNUM_OVL, HEAP_MODEL);
    motionIndex = Hu3DModelCreate(resourceData);
    backgroundModelId = motionIndex;
    Hu3DModelScaleSet(motionIndex, 0.5f, 0.5f, 0.5f);
    Hu3DModelCameraSet(motionIndex, 16);
    Hu3DModelAttrSet(motionIndex, HU3D_MOTATTR_PAUSE);

    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 40), HU_MEMNUM_OVL, HEAP_MODEL);
    motionIndex = Hu3DModelCreate(resourceData);

    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M414, 41) + winningCharacter, HU_MEMNUM_OVL, HEAP_MODEL);
    Hu3DAnimCreate(resourceData, motionIndex, "gsp000");
    Hu3DModelPosSet(motionIndex, 0.0f, 260.0f, -410.0f);
    Hu3DModelScaleSet(motionIndex, 0.5f, 0.5f, 0.5f);
    Hu3DModelCameraSet(motionIndex, 16);
    HuPrcChildCreate(EndRound, 4096, 4096, 0, HuPrcCurrentGet());
}

void UpdatePlayerScenes(void)
{
    float progress;

    s32 index;
    PlayerSceneState *playerState;
    s32 index2;

    for (index2 = 0, progress = 0.0f; index2 < 15; index2++) {
        progress += 0.06666667f;
        for (index = 0; index < 4; index++) {
            playerState = &playerSceneStates[index];
            HuSprGrpScaleSet(playerState->spriteIndex, progress, progress);
        }
        HuPrcVSleep();
    }
    for (index = 0; index < 4; index++) {
        playerState = &playerSceneStates[index];
        HuSprGrpScaleSet(playerState->spriteIndex, 1.0f, 1.0f);
    }
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunRoundSequence(void)
{
    float progress;
    s32 index;
    s32 index2;
    PlayerSceneState *playerState;

    for (index = 0, progress = 1.0f; index < 15; index++) {
        progress -= 0.06666667f;
        for (index2 = 0; index2 < 4; index2++) {
            playerState = &playerSceneStates[index2];
            HuSprGrpScaleSet(playerState->spriteIndex, progress, progress);
        }
        HuPrcVSleep();
    }
    for (index2 = 0; index2 < 4; index2++) {
        playerState = &playerSceneStates[index2];
        HuSprGrpScaleSet(playerState->spriteIndex, 0.0f, 0.0f);
    }
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunPlayerSequence(void)
{
    float scale;
    float scale2;
    double scale3;

    PlayerSceneState *playerState;
    s32 value;
    s32 value2;

    playerState = HuPrcCurrentGet()->user_data;
    HuAudFXPlay(9);
    for (value = 30; value != 0; value--) {
        scale = 1.0f - ((value - 1) / 30.0f);
        scale = (0.5 * scale) + (0.5 * (1.0f - ((1.0f - scale) * (1.0f - scale))));
        if (scale < 0.75) {
            scale3 = scale / 0.75;
        }
        else {
            scale3 = 1.0;
        }
        scale2 = scale3;
        scale = 1.0f - ((1.0f - scale) * (1.0f - scale));
        scale2 = 1.0f - ((1.0f - scale2) * (1.0f - scale2));
        value2 = (playerState->animationTimer * 2) + 3;
        HuSprTPLvlSet(playerState->spriteIndex, value2, scale2);
        HuSprScaleSet(playerState->spriteIndex, value2, 4.0f - (3.0f * scale), 4.0f - (3.0f * scale));
        HuPrcVSleep();
    }
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void SetPlayerAnimation(s32 playerIndex, s32 motionId)
{
    PlayerSceneState *playerState;
    s32 index;
    HUPROCESS *process;
    s32 value3;

    playerState = &playerSceneStates[playerIndex];
    value3 = playerRoundStates[playerState->playerIndex];
    if (motionId == 0) {
        playerRoundStates[playerState->playerIndex]++;
    }
    else {
        playerRoundStates[playerState->playerIndex]--;
    }
    for (index = 0; index < 3; index++) {
        if (index < playerRoundStates[playerState->playerIndex]) {
            if (motionId == 0 && value3 == value2) {
                process = HuPrcChildCreate(RunPlayerSequence, 8192, 8192, 0, gameSequenceProcess);
                process->user_data = value;
                value->animationTimer = value2;
            }
            HuSprAttrSet(playerState->spriteIndex, (index * 2) + 2, HUSPR_ATTR_DISPOFF);
            HuSprAttrReset(playerState->spriteIndex, (index * 2) + 3, HUSPR_ATTR_DISPOFF);
        }
        else {
            HuSprAttrReset(playerState->spriteIndex, (index * 2) + 2, HUSPR_ATTR_DISPOFF);
            HuSprAttrSet(playerState->spriteIndex, (index * 2) + 3, HUSPR_ATTR_DISPOFF);
        }
    }
}

void InitializePlayerModels(void)
{
    float effectValue2;
    float effectValue3;
    float effectValue4;
    float effectValue5;

    PlayerSceneState *playerState;
    s32 value;
    s32 value2;
    s32 value3;
    s32 value4;

    value = (s32)HuPrcCurrentGet()->user_data;
    value4 = (value >> 16);
    value = (u16)value;

    while (roundState == 0) {
        HuPrcVSleep();
    }
    playerState = &playerSceneStates[value4];
    value2 = 0;
    while (TRUE) {
        HuPrcSleep(playerInputHistory[value][value2++]);
        if (value2 >= 4) {
            value2 = 0;
        }
        if (playerState->targetPlayerIndex == value) {
            continue;
        }
        if (roundState == 5) {
            break;
        }

        effectValue2 = 40.0f * ((value + 2) / 2);
        if (playerState->rotationY - effectValue >= 0.0f) {
            effectValue4 = playerState->rotationY - effectValue;
        }
        else {
            effectValue4 = -(playerState->rotationY - effectValue2);
        }
        if (effectValue4 < 45.0f || (0.0f == playerState->rotationY && value >= 14)) {
            if (value == GetPlayerInput(playerState->playerIndex, 0) || value == GetPlayerInput(playerState->playerIndex, 1)) {
                if (playerState->playerIndex & 1) {
                    HuAudFXPlay(1434);
                }
                else {
                    HuAudFXPlay(1433);
                }
            }
            else {
                HuAudFXPlay(1450);
            }
        }
        Hu3DModelAttrReset(playerState->playerModelIds[value], HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV | HU3D_MOTATTR_SHAPE_PAUSE | HU3D_MOTATTR_SHAPE_REV);
        do {
            value3 = playerInputHistory[value][value2++];
            if (value2 >= 4) {
                value2 = 0;
            }
            while (value3) {
                if (roundState == 5) {
                    break;
                }
                HuPrcVSleep();
                value3--;
            }
        } while (roundState != 5 && playerState->targetPlayerIndex != -1 && winningCharacter == playerCharacterOrder[playerState->targetPlayerIndex]);

        Hu3DModelAttrReset(playerState->playerModelIds[value], HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_SHAPE_PAUSE);
        Hu3DModelAttrSet(playerState->playerModelIds[value], HU3D_MOTATTR_REV | HU3D_MOTATTR_SHAPE_REV);
        HuPrcSleep(10);
        effectValue3 = 40.0f * ((value + 2) / 2);
        if (playerState->rotationY - effectValue >= 0.0f) {
            effectValue5 = playerState->rotationY - effectValue;
        }
        else {
            effectValue5 = -(playerState->rotationY - effectValue3);
        }
        if (effectValue5 < 45.0f || (0.0f == playerState->rotationY && value >= 14)) {
            if (value == GetPlayerInput(playerState->playerIndex, 0) || value == GetPlayerInput(playerState->playerIndex, 1)) {
                if (playerState->playerIndex & 1) {
                    HuAudFXPlay(1436);
                }
                else {
                    HuAudFXPlay(1435);
                }
            }
            else {
                HuAudFXPlay(1452);
            }
        }
    }
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunPlayerAnimations(void)
{
    s32 animationScores[1];

    float effectValue;
    float effectValue2;
    float effectValue3;
    float effectValue4;
    float effectValue5;
    float effectValue6;
    float effectValue7;
    float effectValue8;
    float effectValue9;
    float effectValue10;

    PlayerSceneState *playerState;
    PlayerAnimationContext *playerAnimationContext;
    s32 index;
    s32 value;

    playerAnimationContext = HuPrcCurrentGet()->user_data;
    playerState = playerAnimationContext->sceneState;
    value = playerAnimationContext->sceneState->playerIndex;
    HuPrcSleep(30);
    if (playerState->playerIndex & 1) {
        HuAudFXPlay(1444);
    }
    else {
        HuAudFXPlay(1443);
    }
    if (playerState->playerIndex & 1) {
        HuAudFXPlay(1448);
    }
    else {
        HuAudFXPlay(1447);
    }
    if (playerAnimationContext->targetPlayerIndex) {
        effectValue8 = -200.0 * (sind(playerAngles[value]) * cos(0.0));
        effectValue6 = 150.0 + (-200.0 * sin(0.0));
        effectValue9 = -200.0 * (cos(0.0) * cosd(playerAngles[value]));
    }
    else {
        effectValue8 = -200.0 * (sind(playerAngles[value]) * cosd(20));
        effectValue6 = 150.0 + -200.0 * sind(-20);
        effectValue9 = -200.0 * (cosd(-20) * cosd(playerAngles[value]));
    }
    if (!playerCharacterOrder[playerAnimationContext->playerIndex]) {
        effectValue6 += 70.0f;
    }
    effectValue = playerState->playerPositions[playerAnimationContext->playerIndex].x;
    effectValue2 = playerState->playerPositions[playerAnimationContext->playerIndex].y;
    effectValue3 = playerState->playerPositions[playerAnimationContext->playerIndex].z;

    effectValue4 = (effectValue8 - effectValue) / 30.0f;
    effectValue10 = (effectValue6 - effectValue2) / 30.0f;
    effectValue5 = (effectValue9 - effectValue3) / 30.0f;

    for (index = 0; index < 30; index++) {
        effectValue += effectValue4;
        effectValue2 += effectValue10;
        effectValue3 += effectValue5;
        Hu3DModelPosSet(playerState->playerCharacterIds[playerAnimationContext->playerIndex], effectValue + (2000.0f * (value % 2)), effectValue2, effectValue3 + (2000.0f * (value / 2)));
        HuPrcVSleep();
    }
    HuPrcSleep(10);
    HuAudFXPlay(victorySoundIds[winningCharacter]);
    effectValue4 = 500.0 * sind(playerAngles[value] - 20.0f);
    effectValue4 = effectValue4;
    effectValue5 = 500.0 * cosd(playerAngles[value] - 20.0f);
    effectValue5 = effectValue5;
    effectValue4 /= 30.0f;
    effectValue5 /= 30.0f;

    for (index = 0, effectValue7 = -20.0f; index < 30.0f; index++) {
        effectValue += effectValue4;
        effectValue2 -= effectValue7;
        effectValue3 += effectValue5;
        effectValue7 += 2.0f;
        Hu3DModelPosSet(playerState->playerCharacterIds[playerAnimationContext->playerIndex], effectValue + (2000.0f * (value % 2)), effectValue2, effectValue3 + (2000.0f * (value / 2)));
        HuPrcVSleep();
    }
    playerState->playerAnimationStates[playerAnimationContext->playerIndex] = 1;
    Hu3DModelAttrSet(playerState->playerCharacterIds[playerAnimationContext->playerIndex], HU3D_ATTR_DISPOFF);
    for (index = 0, animationScores[0] = 0; index < 4; index++) {
        if (playerRoundStates[index] >= 3) {
            break;
        }
    }
    if (index >= 4) {
        SetPlayerAnimation(playerState->playerIndex, 0);
    }
    CharMotionSpeedSet(characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], 1.0f);
    if (playerAnimationContext->targetPlayerIndex) {
        CharMotionShiftSet(characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], playerState->motionIds[1], 60.0f, 5.0f, HU3D_MOTATTR_NONE);
    }
    else {
        CharMotionShiftSet(characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], playerState->motionIds[1], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    }
    HuPrcSleep(5);
    Hu3DModelAttrSet(playerState->secondaryModelId, HU3D_MOTATTR_PAUSE);
    CharMotionSpeedSet(characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], 2.0f);
    playerAnimationContext->animationSequenceActive = 0;
    HuPrcSleep(5);
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

u32 GetPlayerTimingValue(s32 playerIndex, s32 frameCount, s32 halveDuration)
{
    switch (GWPlayerCfg[playerIndex].diff) {
        case 0:
            frameCount *= 12;
            break;
        case 1:
            frameCount *= 10;
            break;
        case 2:
            frameCount *= 10;
            break;
        default:
            frameCount *= 8;
            break;
    }
    if (halveDuration) {
        frameCount / 2;
    }
    return frameCount;
}

s32 GetPlayerAnimationState(PlayerSceneState *playerState)
{
    s32 animationScores[4];

    s32 index;
    s32 value3;
    s32 value4;
    s32 value5;
    s32 value6;
    s32 value7;

    for (index = 0; index < 3; index++) {
        animationScores[index] = -1;
    }

    for (index = value3 = 0; index < 16; index++) {
        if (winningCharacter == playerCharacterOrder[index] && !playerState->playerAnimationStates[index]) {
            animationScores[value3++] = index / 2;
        }
    }
    value4 = GetPlayerInput(playerState->playerIndex, 0) / 2;
    if (GetPlayerInput(playerState->playerIndex, 0) == -1) {
        value4 = 8;
    }
    for (index = 0, value3 = 100, value6 = -1; index < 3; index++) {
        if (animationScores[index] == -1) {
            break;
        }
        if (value4 - savedValue[index] >= 0) {
            value7 = value4 - savedValue[index];
        }
        else {
            value7 = -(value4 - animationScores[index]);
        }
        value5 = value7;
        if (value3 > value) {
            value3 = value;
            value6 = value2;
        }
    }
    if (!index) {
        return 0;
    }
    return value4 - animationScores[value6];
}

inline s32 GetPlayerAnimationScore(PlayerSceneState *playerState)
{
    s32 animationScores[4];

    s32 index;
    s32 value3;
    s32 value4;
    s32 value5;
    s32 value6;

    for (index = 0; index < 3; index++) {
        animationScores[index] = -1;
    }

    for (index = value3 = 0; index < 16; index++) {
        if (winningCharacter == playerCharacterOrder[index] && !playerState->playerAnimationStates[index]) {
            animationScores[value3++] = index / 2;
        }
    }
    value4 = GetPlayerInput(playerState->playerIndex, 0) / 2;
    if (GetPlayerInput(playerState->playerIndex, 0) == -1) {
        value4 = 8;
    }
    for (index = 0, value3 = 100, value6 = -1; index < 3; index++) {
        if (animationScores[index] == -1) {
            break;
        }
        value5 = ABS_INV(value4 - animationScores[index]);
        if (value3 > value) {
            value3 = value;
            value6 = value2;
        }
    }

    if (!index) {
        return 0;
    }

    return value4 - animationScores[value6];
}

void UpdatePlayerAnimationState(PlayerSceneState *playerState)
{
    float effectValue;

    u32 value;

    playerState->horizontalInput = playerState->verticalInput = 0;
    playerState->effectRotation = playerState->playerActionState = 0;
    playerState->effectFlags = 0;

    switch (GWPlayerCfg[playerState->playerIndex].diff) {
        case 0:
            value = 45;
            break;
        case 1:
            value = 55;
            break;
        case 2:
            value = 70;
            break;
        default:
            value = 80;
            break;
    }
    if (playerState->effectCooldown) {
        playerState->effectCooldown--;
        return;
    }
    effectValue = GetPlayerAnimationScore(playerState);
    if (effectValue > 0.0f) {
        playerState->playerActionState = 50;
        playerState->effectCooldown = GetPlayerTimingValue(playerState->playerIndex, 5, 1) + 20 + GetPlayerTimingValue(playerState->playerIndex, frandmod(10), 0);
    }
    if (effectValue < 0.0f) {
        playerState->rightTriggerInput = 50;
        playerState->effectCooldown = GetPlayerTimingValue(playerState->playerIndex, 5, 1) + 20 + GetPlayerTimingValue(playerState->playerIndex, frandmod(10), 0);
    }
    if (0.0f == effectValue) {
        playerState->effectCooldown = GetPlayerTimingValue(playerState->playerIndex, 1, 1) + 10 + GetPlayerTimingValue(playerState->playerIndex, frandmod(5), 0);

        if (playerState->playerCount == 1) {
            if (winningCharacter == playerCharacterOrder[GetPlayerInput(playerState->playerIndex, 0)] && !playerState->playerAnimationStates[GetPlayerInput(playerState->playerIndex, 0)]) {
                if (!(Hu3DModelMotionAttrGet(playerState->playerModelIds[GetPlayerInput(playerState->playerIndex, 0)]) & 0x40000004)) {
                    playerState->effectFlags = 256;
                }
            }
            else {
                playerState->verticalInput = -50;
            }
        }
        else if (winningCharacter == playerCharacterOrder[GetPlayerInput(playerState->playerIndex, 1)] && !playerState->playerAnimationStates[GetPlayerInput(playerState->playerIndex, 1)]) {
            if (!(Hu3DModelMotionAttrGet(playerState->playerModelIds[GetPlayerInput(playerState->playerIndex, 1)]) & 0x40000004)) {
                playerState->effectFlags = 256;
            }
        }
        else {
            playerState->verticalInput = 50;
        }
    }
    if (frandmod(100) > value) {
        switch (frandmod(5)) {
            case 0:
                playerState->playerActionState = 50;
                break;
            case 1:
                playerState->rightTriggerInput = 50;
                break;
            case 2:
                break;
            case 3:
                playerState->verticalInput = -50;
                break;
            default:
                playerState->verticalInput = -50;
                break;
        }
    }
}

s8 GetPlayerStickInput(s32 playerIndex, s32 verticalAxis)
{
    PlayerSceneState *playerState;
    s8 stickValue;

    playerState = &playerSceneStates[playerIndex];
    stickValue = 0;
    if (roundState < 4) {
        if (!GWPlayerCfg[playerIndex].iscom) {
            (verticalAxis) ? (stickValue = HuPadStkY[playerState->controllerPort]) : (stickValue = HuPadStkX[playerState->controllerPort]);
        }
        else {
            (verticalAxis) ? (stickValue = playerState->verticalInput) : (stickValue = playerState->horizontalInput);
        }
    }
    return stickValue;
}

u8 GetPlayerTriggerInput(s32 playerIndex, s32 triggerIndex)
{
    PlayerSceneState *playerState;
    s8 value;

    playerState = &playerSceneStates[playerIndex];
    value = 0;
    if (roundState < 4) {
        if (!GWPlayerCfg[playerIndex].iscom) {
            (triggerIndex) ? (value = HuPadTrigR[playerState->controllerPort]) : (value = HuPadTrigL[playerState->controllerPort]);
        }
        else {
            (triggerIndex) ? (value = playerState->playerActionState) : (value = playerState->effectRotation);
        }
    }
    return value;
}

u16 GetPlayerButtonsDown(s32 playerIndex)
{
    PlayerSceneState *playerState;
    u16 value;

    playerState = &playerSceneStates[playerIndex];
    value = 0;
    if (roundState < 4) {
        if (!GWPlayerCfg[playerIndex].iscom) {
            value = HuPadBtnDown[playerState->controllerPort];
        }
        else {
            value = playerState->effectFlags;
        }
    }
    return value;
}

void SetPlayerRotation(PlayerSceneState *playerState, float targetAngle)
{
    float effectValue2;
    float progress;
    float effectValue3;
    float effectValue4;
    float effectValue5;
    float effectValue6;

    s32 index;

    effectValue3 = 60.0f;
    effectValue2 = 180.0f + targetAngle;
    if (effectValue2 > 360.0f) {
        effectValue2 -= 360.0f;
    }
    for (index = 0, progress = 40.0f; index < 16; index++) {
        if (progress < effectValue || effectValue2 < effectValue) {
            progress += effectValue;
            effectValue2 += effectValue;
        }
        if (progress - effectValue >= 0.0f) {
            effectValue5 = progress - effectValue;
        }
        else {
            effectValue5 = -(progress - effectValue2);
        }
        effectValue4 = effectValue5;
        if (effectValue4 < effectValue3 || playerState->playerAnimationStates[index] == 1) {
            Hu3DModelAttrSet(playerState->playerCharacterIds[index], HU3D_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrReset(playerState->playerCharacterIds[index], HU3D_ATTR_DISPOFF);
        }
        if (effectValue4 < effectValue3) {
            Hu3DModelAttrSet(playerState->playerModelIds[index], HU3D_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrReset(playerState->playerModelIds[index], HU3D_ATTR_DISPOFF);
        }
        if (index & 1) {
            progress += 40.0f;
        }
    }
    effectValue2 = 180.0f + targetAngle;
    if (effectValue2 > 360.0f) {
        effectValue2 -= 360.0f;
    }
    for (index = 0, progress = 0.0f; index < 9; progress += 40.0f, index++) {
        if (progress < effectValue || effectValue2 < effectValue) {
            progress += effectValue;
            effectValue2 += effectValue;
        }
        if (progress - effectValue >= 0.0f) {
            effectValue6 = progress - effectValue;
        }
        else {
            effectValue6 = -(progress - effectValue2);
        }
        effectValue4 = effectValue6;
        if (effectValue4 < effectValue3) {
            Hu3DModelAttrSet(playerState->sceneState[index], HU3D_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrReset(playerState->sceneState[index], HU3D_ATTR_DISPOFF);
        }
    }
}

void AnimatePlayerMovement(void)
{
    float effectValue;
    float angle;
    float effectValue2;
    float progress;
    float effectValue3;
    float effectValue4;

    PlayerMotionContext *playerMotionContext;
    s32 index;
    s32 index2;
    s32 value;

    playerMotionContext = HuPrcCurrentGet()->user_data;
    HuPrcSleep(10);
    value = playerMotionContext->sceneState->playerIndex;
    effectValue4 = playerAngles[value];
    SetPlayerRotation(playerMotionContext->sceneState, playerAngles[value]);

    if (playerMotionContext->playerIndex) {
        angle = 40.0f + playerAngles[value];
        effectValue2 = 40.0f;
    }
    else {
        angle = playerAngles[value] - 40.0f;
        effectValue2 = -40.0f;
    }
    if (angle >= 360.0f) {
        angle -= 360.0f;
    }
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    for (index = 0; index < 9; index++) {
        Hu3DModelAttrSet(playerMotionContext->sceneState->sceneState[index], HU3D_MOTATTR_PAUSE);
    }
    for (index = 0, progress = 0.0f; index < 10; index++, progress += 9.0f) {
        effectValue3 = (effectValue2 / 2) * (1.0 - cosd(progress));
        playerAngles[value] = effectValue4 + effectValue3;
        for (index2 = 0; index2 < 9; index2++) {
            if (!playerMotionContext->playerIndex) {
                effectValue = (effectValue3 / effectValue2) * Hu3DMotionMaxTimeGet(playerMotionContext->sceneState->sceneState[index2]);
            }
            else {
                effectValue = Hu3DMotionMaxTimeGet(playerMotionContext->sceneState->sceneState[index2])
                    - (effectValue3 / effectValue2) * Hu3DMotionMaxTimeGet(playerMotionContext->sceneState->sceneState[index2]);
            }
            Hu3DMotionTimeSet(playerMotionContext->sceneState->sceneState[index2], ABS_INV(effectValue));
        }
        HuPrcVSleep();
    }
    for (index = 0, progress = 0.0f; index < 10; index++, progress += 9.0f) {
        effectValue3 = (effectValue2 / 2) + ((effectValue2 / 2) * sind(progress));
        playerAngles[value] = effectValue4 + effectValue3;
        for (index2 = 0; index2 < 9; index2++) {
            if (!playerMotionContext->playerIndex) {
                effectValue = (effectValue3 / effectValue2) * Hu3DMotionMaxTimeGet(playerMotionContext->sceneState->sceneState[index2]);
            }
            else {
                effectValue = Hu3DMotionMaxTimeGet(playerMotionContext->sceneState->sceneState[index2])
                    - (effectValue3 / effectValue2) * Hu3DMotionMaxTimeGet(playerMotionContext->sceneState->sceneState[index2]);
            }
            Hu3DMotionTimeSet(playerMotionContext->sceneState->sceneState[index2], ABS_INV(effectValue));
        }
        HuPrcVSleep();
    }
    if (!playerMotionContext->playerIndex) {
        for (index = 0; index < 9; index++) {
            Hu3DMotionTimeSet(playerMotionContext->sceneState->sceneState[index], Hu3DMotionMaxTimeGet(playerMotionContext->sceneState->sceneState[index]));
        }
    }
    else {
        for (index = 0; index < 9; index++) {
            Hu3DMotionTimeSet(playerMotionContext->sceneState->sceneState[index], 0.0f);
        }
    }
    playerAngles[value] = angle;
    *playerMotionContext->actionStatePointer = 0;
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void AnimatePlayerMotion(void)
{
    float angle;
    float progress;
    float effectValue;
    float effectValue2;

    PlayerMotionContext *playerMotionContext;
    s32 index;
    s32 value;

    playerMotionContext = HuPrcCurrentGet()->user_data;
    value = playerMotionContext->sceneState->playerIndex;
    effectValue2 = playerAngles[value];
    if (playerMotionContext->playerIndex) {
        angle = 40.0f + playerAngles[value];
        effectValue = 40.0f;
    }
    else {
        angle = playerAngles[value] - 40.0f;
        effectValue = -40.0f;
    }
    if (angle >= 360.0f) {
        angle -= 360.0f;
    }
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    for (index = 0, progress = 0.0f; index < 15; index++, progress += 6.0f) {
        playerMotionContext->sceneState->rotationY = effectValue2 + ((effectValue / 2) * (1.0 - cosd(progress)));
        HuPrcVSleep();
    }
    for (index = 0, progress = 0.0f; index < 15; index++, progress += 6.0f) {
        playerMotionContext->sceneState->rotationY = (effectValue / 2) + (effectValue2 + ((effectValue / 2) * sind(progress)));
        HuPrcVSleep();
    }
    playerMotionContext->sceneState->rotationY = angle;
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void UpdatePlayerInput(void)
{
    float progress;
    float effectValue;
    float effectValue2;

    PlayerSceneState *playerState;
    s32 index;

    playerState = HuPrcCurrentGet()->user_data;
    for (index = 0, progress = 0.0f; index < 10; index++, progress += 18.0f) {
        effectValue = 50.0 * sind(playerAngles[playerState->playerIndex]) * sind(progress);
        effectValue2 = 50.0 * cosd(playerAngles[playerState->playerIndex]) * sind(progress);
        Hu3DModelPosSet(playerState->secondaryModelId, (2000.0f * (playerState->playerIndex % 2)) + (playerState->positionX + effectValue), playerState->positionY,
            (2000.0f * (playerState->playerIndex / 2)) + (playerState->positionZ + effectValue2));
        Hu3DModelPosSet(playerState->shadowModelId, effectValue + (2000.0f * (playerState->playerIndex % 2)), 0.0f, effectValue2 + (2000.0f * (playerState->playerIndex / 2)));
        HuPrcVSleep();
    }
    Hu3DModelPosSet(
        playerState->secondaryModelId, playerState->positionX + (2000.0f * (playerState->playerIndex % 2)), playerState->positionY, playerState->positionZ + (2000.0f * (playerState->playerIndex / 2)));
    Hu3DModelPosSet(playerState->shadowModelId, 2000.0f * (playerState->playerIndex % 2), 0.0f, 2000.0f * (playerState->playerIndex / 2));
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

s32 GetPlayerInput(s32 playerIndex, s32 inputPort)
{
    float effectValue;
    s32 index;

    for (index = 0, effectValue = 40.0f; index < 16; effectValue += 40.0f, index += 2) {
        if (effectValue == playerAngles[playerIndex]) {
            if (inputPort) {
                index++;
            }
            break;
        }
    }
    if (index >= 16) {
        index = -1;
    }
    return index;
}

s32 IsPlayerInputActive(PlayerSceneState *playerState, s32 characterIndex)
{
    s32 value;

    value = 1;
    if (Hu3DModelMotionAttrGet(playerState->playerModelIds[characterIndex]) & 0x40000004) {
        value = 0;
    }
    return value;
}

void RunPlayerProcess(void)
{
    PlayerMotionContext playerMotionContext;
    PlayerAnimationContext playerAnimationContext;

    PlayerSceneState *playerState;
    HUPROCESS *process;
    s32 selectedCharacterIndex;
    s32 selectedInputDirection;

    playerState = HuPrcCurrentGet()->user_data;
    playerState->playerProcessState = 0;
    playerState->playerCount = 1;
    playerState->targetPlayerIndex = -1;
    playerState->effectCooldown = 0;

    while (!roundState) {
        HuPrcVSleep();
    }
    roundState = 2;

    while (roundState == 2) {
        HuPrcVSleep();
    }
    playerState->playerActionState = 0;
    playerRoundStates[playerState->playerIndex] = 0;

    while (TRUE) {
        playerInputCounters[0] = playerInputCounters[1] = 0;
        if (GWPlayerCfg[playerState->playerIndex].iscom == 1) {
            UpdatePlayerAnimationState(playerState);
        }
        if (playerState->playerCount == 3 && Hu3DMotionEndCheck(playerState->shadowModelId)) {
            if (0.0f == Hu3DMotionTimeGet(playerState->shadowModelId)) {
                Hu3DMotionSet(playerState->shadowModelId, playerState->shadowMotionIds[0]);
                playerState->playerCount = 1;
            }
            else {
                playerState->playerCount = 2;
                Hu3DMotionSet(playerState->shadowModelId, playerState->shadowMotionIds[0]);
                Hu3DMotionTimeSet(playerState->shadowModelId, Hu3DMotionMaxTimeGet(playerState->shadowModelId));
            }
        }
        if (playerState->playerCount == 4 && Hu3DMotionEndCheck(playerState->shadowModelId)) {
            playerState->playerCount = 1;
            playerState->targetPlayerIndex = -1;
            Hu3DMotionSet(playerState->shadowModelId, playerState->shadowMotionIds[0]);
            Hu3DModelAttrSet(playerState->shadowModelId, HU3D_MOTATTR_PAUSE);
            Hu3DModelAttrReset(playerState->shadowModelId, HU3D_MOTATTR_REV);
            Hu3DMotionSpeedSet(playerState->shadowModelId, 2.0f);
        }
        if (playerState->playerCount == 5 && Hu3DMotionEndCheck(playerState->shadowModelId)) {
            playerState->playerCount = 2;
            playerState->targetPlayerIndex = -1;
            Hu3DMotionSet(playerState->shadowModelId, playerState->shadowMotionIds[0]);
            Hu3DModelAttrReset(playerState->shadowModelId, HU3D_MOTATTR_REV);
            Hu3DMotionTimeSet(playerState->shadowModelId, Hu3DMotionMaxTimeGet(playerState->shadowModelId));
            Hu3DMotionSpeedSet(playerState->shadowModelId, 2.0f);
        }
        if ((GetPlayerButtonsDown(playerState->playerIndex) & PAD_BUTTON_A) && !playerState->playerActionState) {
            if (GetPlayerInput(playerState->playerIndex, 0) != -1) {
                if (playerState->playerCount == 1) {
                    selectedCharacterIndex = GetPlayerInput(playerState->playerIndex, 0);
                    if (!playerState->playerAnimationStates[selectedCharacterIndex]) {
                        if (playerState->playerIndex & 1) {
                            HuAudFXPlay(1442);
                        }
                        else {
                            HuAudFXPlay(1441);
                        }
                        omVibrate(playerState->playerIndex, 12, 6, 6);
                        playerState->playerCount = 4;
                        playerState->targetPlayerIndex = selectedCharacterIndex;
                        Hu3DMotionSet(playerState->shadowModelId, playerState->shadowMotionIds[1]);
                        Hu3DModelAttrReset(playerState->shadowModelId, HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV);
                        Hu3DMotionSpeedSet(playerState->shadowModelId, 1.0f);
                        process = HuPrcChildCreate(UpdatePlayerInput, 8192, 8192, 0, HuPrcCurrentGet());
                        process->user_data = playerState;
                        if (winningCharacter == playerCharacterOrder[selectedCharacterIndex] && IsPlayerInputActive(playerState, selectedCharacterIndex)) {
                            Hu3DModelAttrReset(
                                playerState->playerModelIds[selectedCharacterIndex], HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV | HU3D_MOTATTR_SHAPE_PAUSE | HU3D_MOTATTR_SHAPE_REV);
                            process = HuPrcChildCreate(RunPlayerAnimations, 8192, 8192, 0, HuPrcCurrentGet());
                            process->user_data = &playerAnimationContext;
                            playerAnimationContext.sceneState = playerState;
                            playerAnimationContext.targetPlayerIndex = 0;
                            playerAnimationContext.animationSequenceActive = 1;
                            playerAnimationContext.playerIndex = GetPlayerInput(playerState->playerIndex, playerAnimationContext.targetPlayerIndex);
                            while (playerAnimationContext.animationSequenceActive) {
                                HuPrcVSleep();
                            }
                        }
                        else {
                            HuPrcSleep(10);
                            Hu3DModelAttrReset(playerState->playerModelIds[selectedCharacterIndex], HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_SHAPE_PAUSE);
                            Hu3DModelAttrSet(playerState->playerModelIds[selectedCharacterIndex], HU3D_MOTATTR_REV | HU3D_MOTATTR_SHAPE_REV);
                            HuPrcSleep(10);
                            omVibrate(playerState->playerIndex, 12, 4, 2);
                            process = HuPrcChildCreate(AnimatePlayerEffect, 8192, 8192, 0, HuPrcCurrentGet());
                            process->user_data = playerState;

                            StartCameraBob(0, playerState->playerIndex, 5.0f);
                            Hu3DModelAttrSet(playerState->shadowModelId, HU3D_MOTATTR_REV);
                            HuPrcSleep(10);
                            if (playerState->playerIndex & 1) {
                                HuAudFXPlay(1444);
                            }
                            else {
                                HuAudFXPlay(1443);
                            }
                            Hu3DModelAttrReset(playerState->playerModelIds[selectedCharacterIndex], HU3D_MOTATTR_REV);
                            HuPrcSleep(10);
                        }
                    }
                }
                else if (playerState->playerCount == 2) {
                    selectedCharacterIndex = GetPlayerInput(playerState->playerIndex, 1);
                    if (!playerState->playerAnimationStates[selectedCharacterIndex]) {
                        if (playerState->playerIndex & 1) {
                            HuAudFXPlay(1442);
                        }
                        else {
                            HuAudFXPlay(1441);
                        }
                        omVibrate(playerState->playerIndex, 12, 6, 6);
                        playerState->playerCount = 5;
                        playerState->targetPlayerIndex = selectedCharacterIndex;
                        Hu3DMotionSet(playerState->shadowModelId, playerState->shadowMotionIds[2]);
                        Hu3DMotionSpeedSet(playerState->shadowModelId, 1.0f);
                        process = HuPrcChildCreate(UpdatePlayerInput, 8192, 8192, 0, HuPrcCurrentGet());
                        process->user_data = playerState;
                        if (winningCharacter == playerCharacterOrder[selectedCharacterIndex] && IsPlayerInputActive(playerState, selectedCharacterIndex)) {
                            Hu3DModelAttrReset(
                                playerState->playerModelIds[selectedCharacterIndex], HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV | HU3D_MOTATTR_SHAPE_PAUSE | HU3D_MOTATTR_SHAPE_REV);
                            process = HuPrcChildCreate(RunPlayerAnimations, 8192, 8192, 0, HuPrcCurrentGet());
                            process->user_data = &playerAnimationContext;
                            playerAnimationContext.sceneState = playerState;
                            playerAnimationContext.targetPlayerIndex = 1;
                            playerAnimationContext.animationSequenceActive = 1;
                            playerAnimationContext.playerIndex = GetPlayerInput(playerState->playerIndex, playerAnimationContext.targetPlayerIndex);
                            while (playerAnimationContext.animationSequenceActive) {
                                HuPrcVSleep();
                            }
                        }
                        else {
                            HuPrcSleep(10);
                            Hu3DModelAttrReset(playerState->playerModelIds[selectedCharacterIndex], HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_SHAPE_PAUSE);
                            Hu3DModelAttrSet(playerState->playerModelIds[selectedCharacterIndex], HU3D_MOTATTR_REV | HU3D_MOTATTR_SHAPE_REV);
                            HuPrcSleep(10);
                            omVibrate(playerState->playerIndex, 12, 4, 2);
                            process = HuPrcChildCreate(AnimatePlayerEffect, 8192, 8192, 0, HuPrcCurrentGet());
                            process->user_data = playerState;
                            StartCameraBob(0, playerState->playerIndex, 5.0f);
                            Hu3DModelAttrSet(playerState->shadowModelId, HU3D_MOTATTR_REV);
                            HuPrcSleep(15);
                            if (playerState->playerIndex & 1) {
                                HuAudFXPlay(1444);
                            }
                            else {
                                HuAudFXPlay(1443);
                            }
                            Hu3DModelAttrReset(playerState->playerModelIds[selectedCharacterIndex], HU3D_MOTATTR_REV);
                            HuPrcSleep(15);
                        }
                    }
                }
            }
        }
        if (GetPlayerStickInput(playerState->playerIndex, 1) > 20 && (playerState->playerCount == 2 || playerState->playerCount == 0)) {
            Hu3DModelAttrReset(playerState->shadowModelId, HU3D_MOTATTR_PAUSE);
            Hu3DModelAttrSet(playerState->shadowModelId, HU3D_MOTATTR_REV);
            playerState->playerCount = 3;
            Hu3DModelAttrReset(playerState->secondaryModelId, HU3D_MOTATTR_PAUSE);
            Hu3DModelAttrSet(playerState->secondaryModelId, HU3D_MOTATTR_REV);
            Hu3DMotionSpeedSet(playerState->secondaryModelId, 2.0f);
        }
        if (GetPlayerStickInput(playerState->playerIndex, 1) < -20 && (playerState->playerCount == 1 || playerState->playerCount == 0)) {
            Hu3DModelAttrReset(playerState->shadowModelId, HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV);
            playerState->playerCount = 3;
            Hu3DModelAttrReset(playerState->secondaryModelId, HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV);
            Hu3DMotionSpeedSet(playerState->secondaryModelId, 2.0f);
        }
        if (!playerState->playerActionState) {
            selectedInputDirection = -1;
            if (GetPlayerTriggerInput(playerState->playerIndex, 0) > 20 && selectedInputDirection == -1) {
                selectedInputDirection = 1;
            }
            if (GetPlayerTriggerInput(playerState->playerIndex, 1) > 20 && selectedInputDirection == -1) {
                selectedInputDirection = 0;
            }
            if (selectedInputDirection != -1) {
                if (selectedInputDirection) {
                    if (playerState->playerIndex & 1) {
                        HuAudFXPlay(1440);
                    }
                    else {
                        HuAudFXPlay(1439);
                    }
                }
                else if (playerState->playerIndex & 1) {
                    HuAudFXPlay(1438);
                }
                else {
                    HuAudFXPlay(1437);
                }
                playerState->playerActionState = 1;
                process = HuPrcChildCreate(UpdatePlayerInput, 8192, 8192, 0, HuPrcCurrentGet());
                process->user_data = &playerMotionContext;
                playerMotionContext.sceneState = playerState;
                playerMotionContext.playerIndex = selectedInputDirection;
                playerMotionContext.targetplayerindexPtr = &playerState->playerActionState;
                process = HuPrcChildCreate(AnimatePlayerMotion, 8192, 8192, 0, HuPrcCurrentGet());
                process->user_data = &playerMotionContext;
                playerMotionContext.sceneState = playerState;
                playerMotionContext.playerIndex = selectedInputDirection;
            }
        }
        playerState->positionX = 37.0 * sind(playerState->rotationY);
        playerState->positionZ = 37.0 * cosd(playerState->rotationY);

        Hu3DModelPosSet(playerState->secondaryModelId, playerState->positionX + (2000.0f * (playerState->playerIndex % 2)), playerState->positionY,
            playerState->positionZ + (2000.0f * (playerState->playerIndex / 2)));
        Hu3DModelRotSet(playerState->secondaryModelId, playerState->rotationX, playerState->rotationY + playerState->effectRotation, playerState->rotationZ);
        Hu3DModelRotSet(playerState->shadowModelId, playerState->rotationX, playerState->rotationY, playerState->rotationZ);
        HuPrcVSleep();
    }
}

void UpdatePlayerSprites(void)
{
    Vec worldPosition;
    Vec screenPosition;

    float effectValue;
    float effectValue2;
    float effectValue3;

    PlayerSceneState *playerState;

    playerState = HuPrcCurrentGet()->user_data;

    while (TRUE) {
        if (playerState->playerCount != 4 && playerState->playerCount != 5) {
            worldPosition.x = (2000.0f * (playerState->playerIndex % 2)) + (-950.0 * sind(playerState->rotationY));
            worldPosition.z = (2000.0f * (playerState->playerIndex / 2)) + (-950.0 * cosd(playerState->rotationY));
            effectValue3 = Hu3DMotionMaxTimeGet(playerState->shadowModelId);
            worldPosition.y = 220.0f + (350.0f * (1.0f - (Hu3DMotionTimeGet(playerState->shadowModelId) / effectValue3)));
            Hu3D3Dto2D(&worldPosition, 1 << playerState->cameraId, &screenPosition);
            effectValue = (282.0f * (playerState->playerIndex % 2)) + (screenPosition.x / 2);
            effectValue2 = (225.0f * (playerState->playerIndex / 2)) + (screenPosition.y / 2);
            effectValue += 3.0f;
            effectValue2 -= 15.0f;
            HuSprGrpPosSet(playerState->spriteGroupId, effectValue, effectValue2);
        }
        HuPrcVSleep();
    }
}
void RunVictorySequence(void)
{
    float scale;
    float scale2;
    float scale3;
    float scale4;
    float scale5;

    PlayerSceneState *playerState;
    s32 index;
    s32 index2;
    PlayerSceneState *value;
    HUPROCESS *process;

    HuAudFXPlay(victorySoundIds[winningCharacter]);
    HuPrcSleep(REFRESH_RATE);
    HuAudFXPlay(1449);
    scale = scale2 = 0.0f;
    scale3 = 640.0f;
    scale4 = 480.0f;
    for (index2 = 0; index2 < 30; index2++) {
        Hu3DCameraViewportSet(16, scale, scale2, scale3, scale4, 0.0f, 1.0f);
        Hu3DCameraScissorSet(16, scale, scale2, scale3, scale4);
        scale += 10.666667f;
        scale2 += 8.0f;
        scale3 -= 21.333334f;
        scale4 -= 16.0f;
        HuPrcVSleep();
    }
    Hu3DCameraViewportSet(16, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(16, 0, 0, 0, 0);
    Hu3DModelAttrSet(backgroundModelId, HU3D_ATTR_DISPOFF);
    HuPrcSleep(30);
    for (index2 = 0; index2 < 45; index2++) {
        for (index = 0; index < 4; index++) {
            cameraOrbitDistances[index] -= 23.777779f;
            cameraPositions[index].y -= 3.5555556f;
            cameraLookAtPositions[index].y -= 6.2222223f;
        }
        HuPrcVSleep();
    }
    for (index = 0; index < 4; index++) {
        playerState = &playerSceneStates[index];
        CharMotionSet(characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], playerState->motionIds[5]);
        CharMotionTimeSet(characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], 61.0f);
        CharMotionSpeedSet(characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], 2.0f);
    }
    for (index2 = 0; index2 < 30; index2++) {
        for (index = 0; index < 4; index++) {
            playerState = &playerSceneStates[index];
            playerState->effectRotation += 6.0f;
            Hu3DModelRotSet(playerState->secondaryModelId, playerState->rotationX, playerState->rotationY + playerState->effectRotation, playerState->rotationZ);
        }
        HuPrcVSleep();
    }
    playerState->effectRotation = 180.0f;
    Hu3DModelRotSet(playerState->secondaryModelId, playerState->rotationX, playerState->rotationY + playerState->effectRotation, playerState->rotationZ);
    CharMotionSpeedSet(characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], 1.0f);
    for (index = 0; index < 4; index++) {
        playerState = &playerSceneStates[index];
        CharMotionShiftSet(characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], playerState->motionIds[1], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    }
    HuPrcSleep(5);
    for (index = 0; index < 4; index++) {
        playerState = &playerSceneStates[index];
        Hu3DModelAttrSet(playerState->secondaryModelId, HU3D_MOTATTR_PAUSE);
    }
    HuPrcChildCreate(UpdatePlayerScenes, 4096, 8192, 0, HuPrcCurrentGet());
    for (index = 0; index < 4; index++) {
        value = &playerSceneStates[index];
        process = HuPrcChildCreate(UpdatePlayerSprites, 8192, 8192, 0, HuPrcCurrentGet());
        process->user_data = value;
        HuSprAttrReset(value->spriteGroupId, 0, HUSPR_ATTR_DISPOFF);
    }
    for (index2 = 0, scale5 = 0.0f; index2 < 15; index2++) {
        scale5 += 0.06666667f;
        for (index = 0; index < 4; index++) {
            value = &playerSceneStates[index];
            HuSprScaleSet(value->spriteGroupId, 0, scale5, scale5);
        }
        HuPrcVSleep();
    }
}

void ShowWinningPlayer(s32 playerIndex)
{
    float progress;
    float effectValue;
    float effectValue2;
    float effectValue3;

    PlayerSceneState *playerState;
    PlayerSceneState *playerState2;
    s32 index;
    s32 index2;
    s32 index3;

    if (playerIndex != -1) {
        for (index2 = 0, index = 0; index2 < 16; index2++) {
            if (winningCharacter == playerCharacterOrder[index2]) {
                playerState2 = &playerSceneStates[playerIndex];
                switch (index) {
                    case 0:
                        effectValue = 35.0f + playerState2->rotationY;
                        effectValue3 = 240.0f;
                        break;
                    case 1:
                        effectValue = playerState2->rotationY - 35.0f;
                        effectValue3 = 240.0f;
                        break;
                    default:
                        effectValue = playerState2->rotationY;
                        effectValue3 = 240.0f;
                        break;
                }
                Hu3DMotionSet(playerState2->playerCharacterIds[index2], playerState2->playerMotionIds[index2]);
                progress = 0.0f;
                if (!playerCharacterOrder[index2]) {
                    progress += 70.0f;
                }
                Hu3DModelPosSet(playerState2->playerCharacterIds[index2], (2000.0f * (playerIndex % 2) + (250.0 * sind(effectValue))), progress,
                    (2000.0f * (playerIndex / 2)) + (250.0 * cosd(effectValue)));
                Hu3DModelRotSet(playerState2->playerCharacterIds[index2], playerState2->rotationX, playerState2->rotationY, playerState2->rotationZ);
                if (winningCharacter == 2) {
                    Hu3DModelScaleSet(playerState2->playerCharacterIds[index2], 1.0f, 1.0f, 1.0f);
                }
                Hu3DModelAttrReset(playerState2->playerCharacterIds[index2], HU3D_ATTR_DISPOFF);
                Hu3DModelShadowSet(playerState2->playerCharacterIds[index2]);
                if (index++ >= 2) {
                    break;
                }
            }
        }
    }
    for (index3 = 0; index3 < 4; index3++) {
        playerState2 = &playerSceneStates[index3];
        progress = 40.0f + playerAngles[playerState2->playerIndex];
        if (progress >= 360.0f) {
            progress -= 360.0f;
        }
        index = 2.0f + (playerAngles[playerState2->playerIndex] / 40.0f);
        if (index >= 9) {
            index -= 9;
        }
        for (index2 = 0; index2 < 6; index2++) {
            Hu3DModelAttrSet(playerState2->playerModels[index], HU3D_ATTR_DISPOFF);
            index++;
            if (index >= 9) {
                index = 0;
            }
        }
        Hu3DModelAttrReset(playerState2->sharedModelId, HU3D_ATTR_DISPOFF);
        Hu3DModelRotSet(playerState2->sharedModelId, 0.0f, playerAngles[playerState2->playerIndex], 0.0f);
    }
    for (index2 = 0; index2 < 30; index2++) {
        for (index = 0; index < 4; index++) {
            cameraLookAtPositions[index].y -= 7.6666665f;
        }
        HuPrcVSleep();
    }
    HuAudFXPlay(victorySoundIds[winningCharacter]);
    for (index = 0; index < 4; index++) {
        playerState = &playerSceneStates[index];
        CharMotionShiftSet(characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], playerState->motionIds[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    }
    HuPrcSleep(5);
    for (index = 0; index < 4; index++) {
        playerState = &playerSceneStates[index];
        CharMotionSet(characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], playerState->motionIds[5]);
        Hu3DModelAttrReset(playerState->secondaryModelId, HU3D_MOTATTR_LOOP);
        CharMotionTimeSet(characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], 61.0f);
        CharMotionSpeedSet(characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], 2.0f);
    }
    for (effectValue2 = 37.0f, index2 = 0; index2 < 30; index2++, effectValue2 += 1.6666666f) {
        for (index = 0; index < 4; index++) {
            playerState = &playerSceneStates[index];
            playerState->effectRotation += 6.0f;
            Hu3DModelRotSet(playerState->secondaryModelId, playerState->rotationX, playerState->rotationY + playerState->effectRotation, playerState->rotationZ);
            playerState->positionX = effectValue2 * sind(playerState->rotationY);
            playerState->positionZ = effectValue2 * cosd(playerState->rotationY);
            Hu3DModelPosSet(playerState->secondaryModelId, playerState->positionX + (2000.0f * (playerState->playerIndex % 2)), playerState->positionY,
                playerState->positionZ + (2000.0f * (playerState->playerIndex / 2)));
        }
        HuPrcVSleep();
    }
    playerState->effectRotation = 0.0f;
    Hu3DModelRotSet(playerState->secondaryModelId, playerState->rotationX, playerState->rotationY + playerState->effectRotation, playerState->rotationZ);
    CharMotionSpeedSet(characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], 1.0f);
    for (index = 0; index < 4; index++) {
        playerState = &playerSceneStates[index];
        CharMotionShiftSet(
            characterMotionIds[GWPlayerCfg[playerState->playerIndex].character], playerState->motionIds[playerState->playerIndex == playerIndex ? 3 : 4], 0.0f, 5.0f, 0);
    }
}

void WaitForPlayerProcess(void)
{
    PlayerSceneState *playerState;

    playerState = HuPrcCurrentGet()->user_data;
    while (TRUE) {
        if (playerState->playerCount == 4 || playerState->playerCount == 5 || playerState->playerActionState) {
            HuPrcVSleep();
        }
        else {
            break;
        }
    }
    HuPrcCurrentGet()->user_data = NULL;
    HuPrcKill(playerProcesses[playerState->playerIndex]);
    while (TRUE) {
        HuPrcVSleep();
    }
}

void WaitForAllPlayerProcesses(void)
{
    HUPROCESS *processList[4];

    s32 index;
    s32 value;

    for (index = 0; index < 4; index++) {
        processList[index] = HuPrcChildCreate(WaitForPlayerProcess, 8192, 8192, 0, HuPrcCurrentGet());
        processList[index]->user_data = &playerSceneStates[index];
    }
    while (TRUE) {
        for (index = 0; index < 4; index++) {
            if (processList[index]->user_data) {
                break;
            }
        }
        if (index < 4) {
            HuPrcVSleep();
        }
        else {
            break;
        }
    }
    HuPrcCurrentGet()->user_data = NULL;
    while (TRUE) {
        HuPrcVSleep();
    }
}

void EndRound(void)
{
    while (TRUE) {
        if (omSysExitReq == 1) {
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
            HuAudFadeOut(1000);
            HuPrcSleep(60);
            MGSeqKillAll();
            omOvlReturnEx(1, 1);
            while (TRUE) {
                HuPrcVSleep();
            }
        }
        HuPrcVSleep();
    }
}

void RunGameSequence(void)
{
    float scale;

    s32 index;
    s32 value2;
    s32 value3;
    s16 messageId;
    s16 motionIndex;
    s32 index2;
    HUPROCESS *process;
    s16 messageId2;
    PlayerSceneState *playerState;
    s32 sequenceId;
    PlayerSceneState *playerState2;

    roundState = 0;
    HuPrcSleep(1.0f + wipeData.duration);
    RunVictorySequence();
    HuPrcSleep(30);
    roundState = 1;

    while (roundState == 1) {
        HuPrcVSleep();
    }

    sequenceId = HuAudSeqPlay(72);
    messageId = MGSeqCreate(3, 0);
    while (MGSeqStatGet(messageId)) {
        HuPrcVSleep();
    }
    motionIndex = REFRESH_RATE * 60 + REFRESH_RATE - 1;
    messageId2 = MGSeqCreate(1, motionIndex / REFRESH_RATE, -1, -1);
    roundState = 3;
    while (TRUE) {
        for (index = 0, value2 = 0; index < 4; index++) {
            playerState2 = &playerSceneStates[index];
            if (playerRoundStates[value] >= 3) {
                value2++;
                value3 = value;
            }
        }
        if (value2 > 0) {
            break;
        }
        if (motionIndex < REFRESH_RATE - 1) {
            value3 = -1;
            break;
        }
        MGSeqParamSet(messageId2, 1, motionIndex-- / REFRESH_RATE);
        HuPrcVSleep();
    }
    roundState = 4;
    HuAudSeqFadeOut(sequenceId, 100);
    MGSeqParamSet(messageId2, 2, -1);
    messageId = MGSeqCreate(3, 1);
    process = HuPrcChildCreate(WaitForAllPlayerProcesses, 8192, 8192, 0, HuPrcCurrentGet());
    process->user_data = (void *)1;

    while (process->user_data) {
        HuPrcVSleep();
    }
    for (index = 0, value2 = 0; index < 4; index++) {
        playerState2 = &playerSceneStates[index];
        if (playerRoundStates[value] >= 3) {
            value2++;
            value3 = value;
        }
    }

    while (MGSeqStatGet(messageId)) {
        HuPrcVSleep();
    }
    roundState = 5;
    HuPrcChildCreate(RunRoundSequence, 4096, 8192, 0, HuPrcCurrentGet());
    for (index = 0, scale = 1.0f; index < 15; index++) {
        for (scale -= 0.06666667f, index2 = 0; index2 < 4; index2++) {
            playerState = &playerSceneStates[index2];
            HuSprScaleSet(playerState->spriteGroupId, 0, scale, scale);
        }
        HuPrcVSleep();
    }
    for (index = 0; index < 4; index++) {
        HuSprAttrSet(playerSceneStates[index].spriteGroupId, 0, 4);
    }
    HuPrcSleep(15);
    ShowWinningPlayer(value3);
    if (value3 != -1) {
        HuAudSStreamPlay(1);
        GWPlayerCoinWinSet(value3, 10);
        messageId = MGSeqCreate(5, 3, GWPlayerCfg[value3].character, -1, -1, -1);
    }
    else {
        HuAudSStreamPlay(4);
        messageId = MGSeqCreate(3, 2);
    }
    HuPrcSleep(210);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    HuPrcSleep(60);
    HuAudFadeOut(1);
    MGSeqKillAll();
    omOvlReturnEx(1, 1);
    while (TRUE) {
        HuPrcVSleep();
    }
}
