#ifndef REL_M438DLL_H
#define REL_M438DLL_H

#include "game/animdata.h"
#include "game/hsfformat.h"
#include "game/object.h"

typedef struct M438CharacterCameraWork {
    u8 cameraMode;
    char reserved01[0x3];
    s32 controlFlags;
    float cameraAngle;
    float cameraDistance;
    Vec worldPosition;
    Vec modelRotation;
    Vec cameraOffset;
    Vec targetPosition;
    char reserved40[0xC];
} M438CharacterCameraWork; /* size = 0x4C */

typedef struct M438ParticleAnimationTrack {
    s8 frameIndex;
    s8 previousFrameIndex;
    char reserved02[0x2];
    float topLeftU;
    float topLeftV;
    float topRightU;
    float topRightV;
    float bottomRightU;
    float bottomRightV;
    float bottomLeftU;
    float bottomLeftV;
    u8 playbackFlags;
    float frameStep;
    float frameAccumulator;
} M438ParticleAnimationTrack;

typedef struct M438FireMaterial {
    Vec position;
    Vec rotation;
    Vec scale;
    Vec secondaryScale;
    u8 enabled;
    u8 textureCoordinateCount;
    u8 animationMode;
    u8 blendMode;
    u8 textureSlotCount;
    u32 stateFlags;
    s16 displayListSize;
    void *displayList;
    Vec *vertexCoordinates;
    GXColor *vertexColors;
    Vec *vertexNormals;
    GXColor baseColor;
    GXColor modulationColor;
    M438ParticleAnimationTrack *animationTracks;
    u8 renderFlags;
    char reserved5D[0x3];
    Vec textureOffset;
    Vec particleOffset;
    s16 animationFrameData[0x10];
} M438FireMaterial; /* size = 0x98 */

typedef struct M438FireQuadVertices {
    Vec topLeft;
    Vec topRight;
    Vec bottomRight;
    Vec bottomLeft;
} M438FireQuadVertices; // sizeof 0x30

typedef struct M438FireQuadColors {
    GXColor topLeft;
    GXColor topRight;
    GXColor bottomRight;
    GXColor bottomLeft;
} M438FireQuadColors; // sizeof 0x10

typedef struct M438FireQuadTextureCoordinates {
    HuVec2f topLeft;
    HuVec2f topRight;
    HuVec2f bottomRight;
    HuVec2f bottomLeft;
} M438FireQuadTextureCoordinates; // sizeof 0x20

typedef struct M438ParticleSystem {
    s16 systemId;
    s16 materialCount;
    u8 animationSlotCount;
    u32 stateFlags;
    Vec position;
    Vec rotation;
    Vec scale;
    void (*updateCallback)(struct M438ParticleSystem *);
    s16 *animationIds;
    char reserved38[0x4];
    M438FireMaterial *materials;
    char reserved40[0x8];
    s16 displayListSize;
    void *displayList;
    Vec *vertexBuffer;
    GXColor *colorBuffer;
    Vec *normalBuffer;
    M438ParticleAnimationTrack *animationTracks;
    u8 coordinateFlags;
    char reserved61[0x3];
    Vec textureOffset;
    Vec textureRotation;
    union {
        s16 animationFrameData[0x20];
        struct {
            s16 effectFlags;
            s16 effectTimer;
            float pathDirectionX;
            float pathDirectionZ;
            float pathLength;
            float effectHeight;
            float effectScale;
            char reserved94[0x28];
        };
    };
} M438ParticleSystem; /* size = 0xBC */

typedef struct M438ParticleAnimation {
    ANIMDATA *animationData;
    s16 textureMappingMode;
    s16 referenceCount;
    u32 resourceId;
    Vec position;
    Mtx textureMatrix;
    GXColor tintColor;
    u8 rowsPerFrame;
    u8 isTiled;
    s16 frameWidth;
    s16 frameHeight;
    char reserved52[0x2];
    float textureScaleX;
    float textureScaleY;
    float inverseTextureWidth;
    float inverseTextureHeight;
} M438ParticleAnimation; /* size = 0x64 */

typedef struct M438FireManager {
    Mtx inverseCameraMatrix;
    s16 particleSystemCapacity;
    s16 animationCapacity;
    s16 hookModelId;
    s16 activeParticleSystemId;
    s16 activeMaterialIndex;
    s16 activeTextureSlot;
    M438ParticleAnimation *animations;
    M438ParticleSystem *particleSystems;
} M438FireManager; /* size =  */

extern M438FireManager fireManager;

void SetupMapEffects(HUPROCESS *process);
void EmptyMapCallback();
s32 FindAvailableRockForAngle(float requestedAngle);
s32 FindAvailableRockAtAngle(float requestedAngle);
float ApproachAngle(float currentAngle, float targetAngle, float approachFactor);
float AngleDifference(float currentAngle, float targetAngle);

void InitializeFireManager(s16 particleSystemCapacity, s16 animationCapacity);
s16 CreateParticleSystem(u8 materialCount, u8 animationSlotCount);
void DestroyParticleSystem(s16 particleSystemId);
void SelectParticleMaterial(s16 particleSystemId, s16 materialIndex, u8 textureSlotIndex);
s16 LoadParticleAnimation(u32 resourceId, s16 columnCount, s16 rowCount);
u8 LoadAnimationToTextureSlotDefaultSize(s16 particleSystemId, u8 textureSlotIndex, u32 resourceId);
void ShutdownFireManager(void);
void SetParticleSystemPosition(s16 particleSystemId, float x, float y, float z);
void SetMaterialPosition(s16 particleSystemId, s16 materialIndex, float x, float y, float z);
void SetMaterialSecondaryScale(s16 particleSystemId, s16 materialIndex, float x, float y, float z);
void SetMaterialTextureRotation(s16 particleSystemId, s16 materialIndex, float x, float y, float z);
void SetMaterialRotation(s16 particleSystemId, s16 materialIndex, float x, float y, float z);
void SetMaterialScale(s16 particleSystemId, s16 materialIndex, float x, float y, float z);
void EnableParticleSystemFlags(s16 particleSystemId, s32 flags);
void EnableMaterialFlags(s16 particleSystemId, s16 materialIndex, s32 flags);
void EnableAllMaterialFlags(s16 particleSystemId, s32 flags);
void ClearParticleSystemFlags(s16 particleSystemId, u32 flags);
void SetMaterialColor(s16 particleSystemId, s16 materialIndex, u8 red, u8 green, u8 blue, u8 alpha);
void SetAllMaterialsEnabled(s16 particleSystemId, u8 enabled);
void SetMaterialTextureCoordinateCount(s16 particleSystemId, s16 materialIndex, u8 coordinateCount);
void SetMaterialAnimationFlags(s16 particleSystemId, s16 materialIndex, u8 textureSlotIndex, u8 animationFlags);
void SetMaterialAnimationSpeed(s16 particleSystemId, s16 materialIndex, u8 textureSlotIndex, float animationSpeed);

#endif
