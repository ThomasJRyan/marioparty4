#include "REL/mstory3Dll.h"
#include "game/data.h"
#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/sprite.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

#include "ext_math.h"

#define frand5() (((s32)frand() & 0x1F))
#define frand8() (((s32)frand() & 0xFF))

typedef struct {
    /* 0x00 */ Vec* destinationPositions;
    /* 0x04 */ Vec emissionOrigin;
    /* 0x10 */ Vec destinationCenter;
    /* 0x1C */ u16 textureWidth;
    /* 0x1E */ u16 textureHeight;
    /* 0x20 */ u16 particleCount;
    /* 0x22 */ s16 particleModelId;
    /* 0x24 */ float particleLifetimeFrames;
    /* 0x28 */ ANIMDATA* sourceAnimation;
} ParticleTextEffectConfig; // Size 0x2C

typedef struct {
    /* 0x00 */ s16 completionState;
    /* 0x02 */ char unreferencedParticleProcessPadding[0x1A];
    /* 0x1C */ ParticleTextEffectConfig* effectConfig;
} ParticleTextEffectProcessState; // Size 0x20

Void Story3InitializeParticleTextBitmap(s32 animationFileId, s32 bitmapDataId, ParticleTextEffectConfig* effectConfig, Vec* destinationCenter);
Void Story3RunParticleTextEffect(Void);
Void Story3UpdateParticleTextParticles(HU3DMODEL* model, HU3DPARTICLE* particle, Mtx matrix);

ParticleTextEffectConfig particleTextEffectConfigs[2];
s32 particleTextEffectInitialized;
s32 particleTextEffectComplete;
s32 particleTextEffectStopping;
s16 particleTextSpriteGroup;

Void Story3CreateParticleTextEffect(s32 animationFileId, Vec* emissionOrigin, Vec* destinationCenter, s16 lifetimeFrames) {
    ANIMDATA* spriteAnimation;
    s16 spriteId;

    Story3InitializeParticleTextBitmap(animationFileId + 1, animationFileId + 3, &particleTextEffectConfigs[0], destinationCenter);
    particleTextEffectConfigs[0].emissionOrigin = *emissionOrigin;
    particleTextEffectConfigs[0].particleLifetimeFrames = lifetimeFrames - 20;
    Story3InitializeParticleTextBitmap(animationFileId + 2, animationFileId + 3, &particleTextEffectConfigs[1], destinationCenter);
    particleTextEffectConfigs[1].emissionOrigin = *emissionOrigin;
    particleTextEffectConfigs[1].particleLifetimeFrames = lifetimeFrames - 20;
    spriteAnimation = HuSprAnimReadFile(animationFileId);
    particleTextSpriteGroup = HuSprGrpCreate(1);
    spriteId = HuSprCreate(spriteAnimation, 0, 0);
    HuSprGrpMemberSet(particleTextSpriteGroup, 0, spriteId);
    HuSprPosSet(particleTextSpriteGroup, 0, destinationCenter->x, destinationCenter->y);
    HuSprTPLvlSet(particleTextSpriteGroup, 0, 0.0f);
    HuSprAttrSet(particleTextSpriteGroup, 0, 8);
    HuPrcChildCreate(Story3RunParticleTextEffect, 100, 0x2000, 0, HuPrcCurrentGet());
    particleTextEffectInitialized = 0;
    particleTextEffectComplete = 0;
    particleTextEffectStopping = 0;
}

Void Story3MarkParticleTextEffectReady(Void) {
    particleTextEffectInitialized = 1;
}

s32 Story3IsParticleTextEffectComplete(Void) {
    return particleTextEffectComplete;
}

Void Story3StopParticleTextEffect(Void) {
    particleTextEffectStopping = 1;
}

s32 Story3GetParticleTextSpriteGroup(Void) {
    return particleTextSpriteGroup;
}

Void Story3InitializeParticleTextBitmap(s32 animationFileId, s32 bitmapDataId, ParticleTextEffectConfig* effectConfig, Vec* destinationCenter) {
    Vec pixelPosition;
    ANIMBMP* bitmapData;
    s32 bitmapPixelCount;
    s32 particleModelId;
    s32 opaquePixelCount;
    u16* bitmapPixel;
    ANIMDATA* bitmapAnimation;
    Vec* destinationPosition;
    ParticleTextEffectProcessState* particleHookData;
    s16 pixelColumn;
    s16 pixelRow;

    bitmapAnimation = HuSprAnimReadFile(animationFileId);
    effectConfig->sourceAnimation = bitmapAnimation;
    bitmapData = bitmapAnimation->bmp;
    bitmapPixelCount = bitmapData->sizeX * bitmapData->sizeY;
    bitmapPixel = bitmapData->data;
    for (pixelRow = opaquePixelCount = 0; pixelRow < bitmapData->sizeY; pixelRow += 2) {
        for (pixelColumn = 0; pixelColumn < bitmapData->sizeX; pixelColumn += 2) {
            bitmapPixel = (u16*) bitmapData->data + ((pixelRow % 4) * 4 + (pixelRow / 4) * bitmapData->sizeX * 4 + (pixelColumn / 4) * 16 + pixelColumn % 4);
            if (*bitmapPixel != 0x8000) {
                opaquePixelCount++;
            }
        }
    }
    effectConfig->particleCount = opaquePixelCount;
    destinationPosition = effectConfig->destinationPositions = HuMemDirectMallocNum(HEAP_HEAP, opaquePixelCount * sizeof(*destinationPosition), HU_MEMNUM_OVL);
    effectConfig->textureWidth = bitmapData->sizeX;
    effectConfig->textureHeight = bitmapData->sizeY;
    Hu3D2Dto3D(destinationCenter, 1, &effectConfig->destinationCenter);
    for (pixelRow = 0; pixelRow < bitmapData->sizeY; pixelRow += 2) {
        for (pixelColumn = 0; pixelColumn < bitmapData->sizeX; pixelColumn += 2) {
            bitmapPixel = (u16*) bitmapData->data + ((pixelRow % 4) * 4 + (pixelRow / 4) * bitmapData->sizeX * 4 + (pixelColumn / 4) * 16 + pixelColumn % 4);
            if (*bitmapPixel != 0x8000) {
                pixelPosition = *destinationCenter;
                pixelPosition.x = pixelPosition.x + pixelColumn - bitmapData->sizeX / 2;
                pixelPosition.y = pixelPosition.y + pixelRow - bitmapData->sizeY / 2;
                Hu3D2Dto3D(&pixelPosition, 1, destinationPosition);
                destinationPosition++;
            }
        }
    }
    HuSprAnimKill(bitmapAnimation);
    bitmapAnimation = HuSprAnimRead(HuDataRead(bitmapDataId));
    particleModelId = Hu3DParticleCreate(bitmapAnimation, opaquePixelCount);
    effectConfig->particleModelId = particleModelId;
    Hu3DModelPosSet(particleModelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelLayerSet(particleModelId, 1);
    Hu3DParticleColSet(particleModelId, 0xFF, 0xFF, 0xA);
    Hu3DParticleScaleSet(particleModelId, 20.0f);
    Hu3DParticleHookSet(particleModelId, Story3UpdateParticleTextParticles);
    Hu3DParticleBlendModeSet(particleModelId, 1);
    Hu3DParticleAttrSet(particleModelId, 2);
    particleHookData = Hu3DData[particleModelId].hookData;
    particleHookData->effectConfig = effectConfig;
    particleHookData->completionState = 0;
    OSReport("Particle Count %d\n", opaquePixelCount);
}

Void Story3RunParticleTextEffect(Void) {
    float pulseAngle;
    float pulseAngularSpeed;
    float spriteAlpha;
    s16* firstParticleHookState;
    s16* secondParticleHookState;
    s16 i;

    pulseAngle = 0.0f;
    pulseAngularSpeed = 0.0f;
    while (particleTextEffectInitialized == 0) {
        HuPrcVSleep();
    }
    Hu3DParticleAttrReset(particleTextEffectConfigs->particleModelId, 2);
    HuPrcSleep(20);
    Hu3DParticleAttrReset(particleTextEffectConfigs[1].particleModelId, 2);
    firstParticleHookState = Hu3DData[particleTextEffectConfigs[0].particleModelId].hookData;
    secondParticleHookState = Hu3DData[particleTextEffectConfigs[1].particleModelId].hookData;
    while (*firstParticleHookState == 0 || *secondParticleHookState == 0) {
        HuPrcVSleep();
    }
    for (i = 0; i <= 20; i++) {
        spriteAlpha = i / 20.0f;
        HuSprTPLvlSet(particleTextSpriteGroup, 0, spriteAlpha);
        HuPrcVSleep();
    }
    OSReport("Finish\n");
    for (i = 0; i < 2; i++) {
        Hu3DModelKill(particleTextEffectConfigs[i].particleModelId);
        HuSprAnimKill(particleTextEffectConfigs[i].sourceAnimation);
    }
    particleTextEffectComplete = 1;
    while (TRUE) {
        if (particleTextEffectStopping != 0) {
            break;
        }
        pulseAngle += pulseAngularSpeed;
        if (pulseAngle > 360.0f) {
            pulseAngle -= 360.0f;
        }
        pulseAngularSpeed += 0.5;
        if (pulseAngularSpeed > 10.0f) {
            pulseAngularSpeed = 10.0f;
        }
        spriteAlpha = 1.0 + 0.25 * sind(pulseAngle);
        HuSprScaleSet(particleTextSpriteGroup, 0, spriteAlpha, spriteAlpha);
        HuPrcVSleep();
    }
    HuSprGrpKill(particleTextSpriteGroup);
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

Void Story3UpdateParticleTextParticles(HU3DMODEL* model, HU3DPARTICLE* particle, Mtx matrix) {
    Vec particleDelta;
    float particleJitter;
    float travelSpeed;
    s16 spawnIndex;
    s16 particleIndex;
    ParticleTextEffectConfig* effectConfig;
    HU3DPARTICLEDATA* particleData;
    s16 colorChannel;
    s16 greenChannel;
    s16 blueChannel;
    s16 travelFrames;
    s16 accelerationFrames;
    s16 coastFrames;
    s16 arrivedParticleCount;

    effectConfig = particle->work;
    if (particle->count == 0) {
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->scale = 0.0f;
        }
    }
    if (particle->attr & 2) {
        return;
    }
    travelFrames = effectConfig->particleLifetimeFrames - (effectConfig->particleLifetimeFrames / 5.0f);
    accelerationFrames = travelFrames / 2;
    coastFrames = travelFrames - accelerationFrames;
    PSVECSubtract(&effectConfig->emissionOrigin, &effectConfig->destinationCenter, &particleDelta);
    travelSpeed = PSVECMag(&particleDelta) / coastFrames;
    for (spawnIndex = 0; spawnIndex < particle->maxCnt / (effectConfig->particleLifetimeFrames / 5.0f); spawnIndex++) {
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            if (!particleData->scale) {
                break;
            }
        }
        if (particleIndex == particle->maxCnt) {
            break;
        }
        particleData->pos = effectConfig->emissionOrigin;
        colorChannel = (frand8() * 360) >> 8;
        particleJitter = 0.2 * frand5();
        particleData->vel.x = particleJitter * sind(colorChannel);
        particleData->vel.y = 0.1 * (frand5() + 100);
        particleData->vel.z = particleJitter * cosd(colorChannel);
        particleData->color.r = 0xF0;
        particleData->color.g = 0x80;
        particleData->color.b = 0x60;
        particleData->scale = 10.0f;
        particleData->time = 0;
        particleData->parManId = 0;
        particleData->accel.x = frandmod(360);
    }
    particleData = particle->data;
    for (particleIndex = arrivedParticleCount = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (!particleData->scale) {
            continue;
        }
        if (particleData->time == 0) {
            PSVECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
            particleData->vel.x *= 0.999f;
            particleData->vel.y -= 0.5f;
            particleData->vel.z *= 0.999f;
            if (particleData->parManId > accelerationFrames - (accelerationFrames / 5.0)) {
                PSVECScale(&particleData->vel, &particleData->vel, 1.0 - ((particleData->parManId - (accelerationFrames - accelerationFrames / 5.0)) / (accelerationFrames / 5.0)));
            }
            if (particleData->parManId == accelerationFrames) {
                particleData->time++;
            }
            if (particleData->pos.y <= 0.0f) {
                particleData->vel.y = 0.5f * -particleData->vel.y;
                particleData->pos.y = 0.0f;
            }
        } else if (particleData->time == 1) {
            particleDelta = effectConfig->destinationPositions[particleIndex];
            PSVECSubtract(&particleDelta, &particleData->pos, &particleDelta);
            if (PSVECMag(&particleDelta) <= 1.0 + travelSpeed) {
                arrivedParticleCount++;
                particleData->pos = effectConfig->destinationPositions[particleIndex];
                particleData->time++;
                continue;
            }
            PSVECNormalize(&particleDelta, &particleDelta);
            particleData->vel.x = particleDelta.x * travelSpeed + sind(particleData->accel.x);
            particleData->vel.y = particleDelta.y * travelSpeed;
            particleData->vel.z = particleDelta.z * travelSpeed;
            PSVECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
            particleData->accel.x += 20.0f;
        } else {
            arrivedParticleCount++;
        }
        if (particleData->time >= 1) {
            colorChannel = particleData->color.r;
            colorChannel += (colorChannel - 0xFF) / 20;
            if (colorChannel > 0xFF) {
                particleData->color.r = 0xFF;
            } else {
                particleData->color.r = colorChannel;
            }
            greenChannel = particleData->color.g;
            greenChannel += (greenChannel - 0xFF) / 20;
            if (greenChannel > 0xFF) {
                particleData->color.g = 0xFF;
            } else {
                particleData->color.g = greenChannel;
            }
            blueChannel = particleData->color.b;
            blueChannel += (blueChannel - 0x80) / 20;
            if (blueChannel > 0x80) {
                particleData->color.b = 0x80;
            } else {
                particleData->color.b = blueChannel;
            }
        }
        particleData->scale -= 0.2f;
        if (particleData->scale < 5.0f) {
            particleData->scale = 5.0f;
        }
        particleData->parManId++;
    }
    DCStoreRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
    if (arrivedParticleCount >= particle->maxCnt) {
        particle->dataCnt++;
    }
}
