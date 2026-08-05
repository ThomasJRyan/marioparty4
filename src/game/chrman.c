#include "types.h"
#include "game/chrman.h"
#include "game/armem.h"
#include "game/audio.h"
#include "game/data.h"
#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/humath.h"
#include "game/object.h"
#include "game/process.h"
#include "game/sprite.h"

#include "ext_math.h"
#include <string.h>

#ifndef __MWERKS__
#include "game/frand.h"
#endif

#include "data_num/effect.h"

typedef struct CharWork_s {
    /* 0x00 */ HU3DMODELID modelId;
    /* 0x02 */ s16 model;
    /* 0x04 */ s16 motNoCurr;
    /* 0x06 */ s16 motNoShiftCurr;
    /* 0x08 */ s16 motNoPrev;
    /* 0x0A */ s16 motNoShiftPrev;
    /* 0x0C */ HU3DMOTID motId[CHAR_MOT_MAX];
    /* 0x4C */ s16 motNoTbl[CHAR_MOT_MAX];
    /* 0x8C */ u8 voiceFlag[CHAR_MOT_MAX];
    /* 0xAC */ u32 attr;
    /* 0xB0 */ s8 stepFx;
    /* 0xB4 */ HuVecF pos;
    /* 0xC0 */ AMEM_PTR motAMemP;
    /* 0xC4 */ HUPROCESS *process;
} CHARWORK; // Size 0xC8

typedef struct EffectData_s {
    /* 0x00 */ s32 dataNum;
    /* 0x04 */ s16 maxCnt;
    /* 0x06 */ s16 blendMode;
    /* 0x08 */ s32 motCnt;
} EFFECTDATA; // Size 0xC

typedef struct EffectParam_s {
    /* 0x00 */ u32 attr;
    /* 0x04 */ GXColor colorBegin;
    /* 0x08 */ GXColor colorEnd;
    /* 0x0C */ HuVecF vel;
    /* 0x18 */ HuVecF velDecay;
    /* 0x24 */ float gravity;
    /* 0x28 */ u32 zero;
    /* 0x2C */ float scaleVel;
    /* 0x30 */ float alphaBase;
    /* 0x34 */ float colorWeight;
} EFFECTPARAM; // Size 0x38

static void UpdateChar(void);
static void UpdateCharAnim(s16 charNo, HU3DMODELID modelId, s16 motNo, u8 voiceFlag, s16 frameNo, HuVecF *ofs);
static s32 _CharFXPlay(s16 charNo, s16 seNo, u8 voiceFlag);
static void EffectInit(void);
static s16 EffectDustCreate(s16 modelId, float posX, float posY, float posZ, float scale, EFFECTPARAM *param);
static s16 EffectSmokeCreate(s16 modelId, float posX, float posY, float posZ, float scale, EFFECTPARAM *param);
static s16 EffectDotCreate(s16 modelId, float posX, float posY, float posZ, float scale, EFFECTPARAM *param);
static s16 EffectStarCreate(s16 modelId, float posX, float posY, float posZ, float scale, EFFECTPARAM *param);
static s16 EffectWarnCreate(s16 modelId, float posX, float posY, float posZ, float scale, EFFECTPARAM *param);
static s16 EffectBirdCreate(s16 modelId, float posX, float posY, float posZ, float scale, EFFECTPARAM *param);
static s16 EffectCreate(s16 type, s16 cameraBit, float posX, float posY, float posZ, float scale, EFFECTPARAM *param);
static void UpdateEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
static void RotateEffect(HU3DPARTICLEDATA *particleDataP);
static void PlayEffectSound(HU3DPARTICLEDATA *particleDataP);
static void CreateHookDust(void);
static void UpdateModelEffect(HU3DPARTICLEDATA *particleDataP);
static void UpdateNpcDust(void);
static s32 PlayStepVoice(s16 charNo, s16 seId, u8 voiceFlag);

static CHARWORK charWork[CHARNO_MAX];
static s16 effectMdl[CHAR_EFFECT_AND_PARTICLE_MAX];
static EFFECTPARAM *particleData[CHAR_EFFECT_AND_PARTICLE_MAX];
static HUPROCESS *itemHookProcess[CHAR_MOT_MAX];
//holds normal characters 0-7, then more characters 8-14
static u16 dustFlags[CHARNO_MAX  + CHAR_NPC_MAX];
static u8 lbl_801975CE[0x82]; // Unused?

static s32 skipAnimUpdate;
static void *effectAMemP;

static u8 lbl_801D35F0[8] = { 0x0C, 0x00, 0x17, 0x75, 0x1E, 0x1E, 0x1D, 0x18 };
static u8 lbl_801D35F8[8] = { 0x0D, 0x00, 0x17, 0x54, 0x1E, 0x00, 0x00, 0x23 };
static u8 lbl_801D3600[8] = { 0x0A, 0x00, 0x19, 0x63, 0x26, 0x00, 0x10, 0x0F };
static u8 lbl_801D3608[8] = { 0x0D, 0x00, 0x17, 0x69, 0x0D, 0x1E, 0x1D, 0x14 };
static u8 lbl_801D3610[8] = { 0x15, 0x1E, 0x16, 0x54, 0x0F, 0x62, 0x39, 0x0A };

static EFFECTDATA effectDataTbl[8] = {
    { EFFECT_ANM_danger, 10, 0, 2 },
    { EFFECT_ANM_hatena, 10, 0, 2 },
    { EFFECT_ANM_dust, 150, 1, 0 },
    { EFFECT_ANM_smoke, 70, 0, 0 },
    { EFFECT_ANM_star, 300, 1, 0 },
    { EFFECT_ANM_glow, 300, 1, 0 },
    { EFFECT_ANM_circle, 200, 1, 0 },
    { EFFECT_ANM_bird, 12, 0, 2 },
};

static s32 charDirTbl[CHARNO_MAX][3] = {
    { DATADIR_MARIOMDL0, DATADIR_MARIOMDL1, DATADIR_MARIOMOT },
    { DATADIR_LUIGIMDL0, DATADIR_LUIGIMDL1, DATADIR_LUIGIMOT },
    { DATADIR_PEACHMDL0, DATADIR_PEACHMDL1, DATADIR_PEACHMOT },
    { DATADIR_YOSHIMDL0, DATADIR_YOSHIMDL1, DATADIR_YOSHIMOT },
    { DATADIR_WARIOMDL0, DATADIR_WARIOMDL1, DATADIR_WARIOMOT },
    { DATADIR_DONKEYMDL0, DATADIR_DONKEYMDL1, DATADIR_DONKEYMOT },
    { DATADIR_DAISYMDL0, DATADIR_DAISYMDL1, DATADIR_DAISYMOT },
    { DATADIR_WALUIGIMDL0, DATADIR_WALUIGIMDL1, DATADIR_WALUIGIMOT },
};

static EFFECTPARAM dustEffParam
    = { 0, 0x80, 0x80, 0x80, 0xFF, 0x40, 0x20, 0x00, 0xFF, { 0.0f, 2.0f, 1.0f }, 0.95f, 0.95f, 0.95f, 0.0f, 0x00000000, 1.0f, -5.0f, 0.02f };

static EFFECTPARAM effectDotParam
    = { 0, 0xFF, 0x40, 0x40, 0x80, 0xFF, 0x40, 0x40, 0x80, { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0.0f, 0.0f, 0x00000000, -5.0f, 0.0f, 0.0f };

static EFFECTPARAM effectStarParam
    = { 0, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, { 0.0f, 0.0f, 0.0f }, 0.95f, 0.95f, 0.95f, 0.0f, 0x00000000, -0.5f, -10.0f, 0.0f };

static EFFECTPARAM effectWarnParam
    = { 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, { 0.0f, 20.0f, 0.0f }, 0.95f, 0.85f, 0.95f, 0.0f, 0x00000000, 1.0f, -5.0f, 0.0f };

static EFFECTPARAM effectSmokeParam
    = { 0, 0x80, 0x20, 0x20, 0xFF, 0x00, 0x00, 0x00, 0xFF, { 0.0f, 10.0f, 0.0f }, 1.0f, 0.95f, 1.0f, 0.0f, 0x00000000, 5.0f, -13.0f, 0.1f };

static s8 lbl_801309A0[32]
    = { 10, 32, -1, -1, 20, 40, -1, -1, 12, 37, -1, -1, 4, 25, -1, -1, 5, 30, -1, -1, 16, 1, 24, 39, 23, 50, -1, -1, 19, 39, -1, -1 };

static s8 lbl_801309C0[32]
    = { 15, 29, -1, -1, 5, 19, -1, -1, 5, 22, -1, -1, 12, 28, -1, -1, 18, 35, -1, -1, 1, 8, 12, -1, 0, 16, -1, -1, 12, 28, -1, -1 };

static s8 lbl_801309E0[16] = { 5, 7, 6, 9, 10, 11, 9, -1, 9, 11, 9, -1, 8, -1, 10, -1 };

/**
 * @brief Resets character model, motion, effect, and hook-process state.
 *
 * @details Loads the shared character-effect archive into ARAM on the first
 * initialization and marks all model and particle slots unused.
 */
void CharInit(void)
{
    s16 i;
    s16 j;
    
    for (i = 0; i < CHARNO_MAX; i++) {
        CHARWORK *workP = &charWork[i];
        workP->motAMemP = 0;
        for (j = 0; j < ARRAY_COUNT(workP->motId); j++) {
            workP->motId[j] = HU3D_MOTID_NONE;
        }
        workP->modelId = HU3D_MODELID_NONE;
        workP->process = NULL;
    }
    if (!effectAMemP) {
        effectAMemP = (void *)HuAR_DVDtoARAM(0x120000);
    }
    for (i = 0; i < CHAR_EFFECT_AND_PARTICLE_MAX; i++) {
        effectMdl[i] = -1;
        particleData[i] = NULL;
    }
    for (i = 0; i < ARRAY_COUNT(itemHookProcess); i++) {
        itemHookProcess[i] = NULL;
    }
}

/**
 * @brief Gets a character's motion archive address in ARAM.
 * @param charNo Character-table index from 0 through CHARNO_MAX - 1 selecting
 * the `charWork` entry that stores the motion archive's ARAM address.
 * @return ARAM address, or zero when the motion archive is not loaded.
 */
AMEM_PTR CharMotionAMemPGet(s16 charNo)
{
    return charWork[charNo].motAMemP;
}

/**
 * @brief Loads a character's motion directory into ARAM when needed.
 * @param charNo Character index; invalid indices and 0xFF are ignored.
 */
void CharMotionInit(s16 charNo)
{
    if (charNo >= CHARNO_MAX  || charNo < 0 || charNo == 0xFF) {
        return;
    } else {
        CHARWORK *workP = &charWork[charNo];
        if (!workP->motAMemP) {
            workP->motAMemP = HuAR_DVDtoARAM(charDirTbl[charNo][2]);
        }
    }
}

/**
 * @brief Frees a character's motion archive from ARAM.
 * @param charNo Character index whose cached motion archive is released.
 */
void CharMotionClose(s16 charNo)
{
    CHARWORK *workP = &charWork[charNo];
    if (workP->motAMemP) {
        HuARFree(workP->motAMemP);
        workP->motAMemP = 0;
    }
}

/**
 * @brief Closes a character's model and motion data directories.
 * @param charNo Character index, or CHARNO_NONE to close every character.
 */
void CharDataClose(s16 charNo)
{
    s16 i;

    if (charNo == CHARNO_NONE) {
        for (i = 0; i < CHARNO_MAX; i++) {
            CharDataClose(i);
        }
    }
    else {
        HuDataDirClose(charDirTbl[charNo][0]);
        HuDataDirClose(charDirTbl[charNo][1]);
        HuDataDirClose(charDirTbl[charNo][2]);
        CharMotionClose(charNo);
    }
}

/**
 * @brief Releases all character models, data directories, and effect ARAM.
 */
void CharClose(void)
{
    CharModelKill(CHARNO_NONE);
    CharDataClose(CHARNO_NONE);
    HuARFree((u32)effectAMemP);
    effectAMemP = NULL;
}

/**
 * @brief Creates one character model and its animation-update process.
 *
 * @param charNo Character index used to select model data and work storage.
 * @param model Character model-variant bit mask.
 * @return Model ID returned by Hu3DModelCreate.
 */
HU3DMODELID CharModelCreate(s16 charNo, s16 model)
{
    s16 unused = 0;
    CHARWORK *workP = &charWork[charNo];
    void *dataP;
    s16 *property;
    s16 modelId;
    s32 dataNum;

    if (workP->modelId != HU3D_MODELID_NONE) {
        Hu3DModelKill(workP->modelId);
    }
    if (model & CHAR_MODEL0) {
        dataNum = charDirTbl[charNo][0];
    }
    else if (model & CHAR_MODEL1) {
        dataNum = charDirTbl[charNo][1];
    }
    else if (model & CHAR_MODEL2) {
        dataNum = charDirTbl[charNo][1] | 1;
    }
    else {
        dataNum = charDirTbl[charNo][1] | 2;
    }
    dataP = HuDataSelHeapReadNum(dataNum, HU_MEMNUM_OVL, HEAP_MODEL);
    workP->modelId = modelId = Hu3DModelCreate(dataP);
    workP->process = HuPrcCreate(UpdateChar, 100, 16384, 0);
    workP->process->user_data = property = HuMemDirectMalloc(HEAP_HEAP, sizeof(s16));
    workP->model = model;
    workP->attr = 0;
    *property = charNo;
    workP->stepFx = 0;
    EffectInit();
    return modelId;
}

static void EyeBmpUpdate(s16 charNo);

/**
 * @brief Tracks the active character motions and emits frame-driven effects.
 *
 * @details Runs as the per-character process created by CharModelCreate and
 * updates both the primary and shift motion once per video frame.
 */
static void UpdateChar(void)
{
    HuVecF effect_position;
    s16 *property = HuPrcCurrentGet()->user_data;
    CHARWORK *workP = &charWork[*property];
    HU3DMODEL *modelP = &Hu3DData[workP->modelId];
    s16 updateBmpF = FALSE;
    s16 i;

    while (1) {
        for (i = 0; i < CHAR_MOT_MAX; i++) {
            if (workP->motId[i] == modelP->motId) {
                break;
            }
        }
        skipAnimUpdate = 0;
        if (i != CHAR_MOT_MAX) {
            workP->motNoCurr = workP->motNoTbl[i];
            UpdateCharAnim(*property, workP->modelId, workP->motNoTbl[i], workP->voiceFlag[i], modelP->motWork.time, &effect_position);
            workP->motNoPrev = workP->motNoTbl[i];
        }
        else {
            workP->motNoCurr = -1;
        }
        if (modelP->motIdShift != -1) {
            for (i = 0; i < CHAR_MOT_MAX; i++) {
                if (workP->motId[i] == modelP->motIdShift) {
                    break;
                }
            }
            skipAnimUpdate = 1;
            updateBmpF = TRUE;
            if (i != CHAR_MOT_MAX) {
                workP->motNoShiftCurr = workP->motNoTbl[i];
                UpdateCharAnim(*property, workP->modelId, workP->motNoTbl[i], workP->voiceFlag[i], modelP->motShiftWork.time, &effect_position);
                workP->motNoShiftPrev = workP->motNoTbl[i];
            }
            else {
                workP->motNoShiftCurr = -1;
            }
        }
        else if (updateBmpF) {
            EyeBmpUpdate(*property);
            updateBmpF = FALSE;
        }
        workP->pos = modelP->pos;
        HuPrcVSleep();
    }
}

/**
 * @brief Emits character sounds and particles associated with a motion frame.
 *
 * @param charNo Character index whose effect table is evaluated.
 * @param modelId Character model providing pose, scale, and hook transforms.
 * @param motNo Character motion data number used by the effect switch.
 * @param voiceFlag Per-motion flags that can suppress voice playback.
 * @param frameNo Current integer motion frame.
 * @param ofs Persistent previous hook position used to fill fast movement with
 * intermediate dot particles.
 */
static void UpdateCharAnim(s16 charNo, HU3DMODELID modelId, s16 motNo, u8 voiceFlag, s16 frameNo, HuVecF *ofs)
{
    HuVecF pos;
    HuVecF hitPos;
    HU3DMODEL *modelP;
    s16 effect_index;
    HU3DPARTICLE *effect_particles;
    HU3DMODEL *effect_model;
    HU3DPARTICLEDATA *effect_particle;
    CHARWORK *workP;
    Mtx hitMtx;
    u32 attrOld;
    s16 effect_limit;
    s16 i;
    
    modelP = &Hu3DData[modelId];
    workP = &charWork[charNo];
    attrOld = 0;
    if (skipAnimUpdate == 0 && (modelP->motAttr & HU3D_MOTATTR_PAUSE)) {
        return;
    }
    switch (motNo) {
        case 2:
            if (skipAnimUpdate == 0 && modelP->motWork.speed <= 0.5) {
                break;
            }
            if (skipAnimUpdate != 0 && modelP->motShiftWork.speed <= 0.5) {
                break;
            }
            if (!(frameNo & 0xF) && !(workP->attr & 0x10)) {
                dustEffParam.vel.x = 2.0 * -sind(modelP->rot.y);
                dustEffParam.vel.y = 1.0 + 0.1 * frandmod(10);
                dustEffParam.vel.z = 2.0 * -cosd(modelP->rot.y);
                pos.x = modelP->pos.x + modelP->scale.x * (frandmod(50) - 25);
                pos.y = modelP->pos.y;
                pos.z = modelP->pos.z + modelP->scale.x * (frandmod(50) - 25);
                EffectDustCreate(modelId, pos.x, pos.y, pos.z, frandmod(10) + 30, &dustEffParam);
            }
            for (i = 0; i < 4; i++) {
                if (lbl_801309A0[charNo * 4 + i] == frameNo) {
                    PlayStepVoice(charNo, 0x101, voiceFlag);
                    break;
                }
            }
            break;
        case 3:
            if (skipAnimUpdate == 0 && modelP->motWork.speed <= 0.5) {
                break;
            }
            if (skipAnimUpdate != 0 && modelP->motShiftWork.speed <= 0.5) {
                break;
            }
            if (!(frameNo & 3) && !(workP->attr & 0x10)) {
                dustEffParam.vel.x = 4.0 * -sind(modelP->rot.y);
                dustEffParam.vel.y = 2.0 + 0.1 * frandmod(10);
                dustEffParam.vel.z = 4.0 * -cosd(modelP->rot.y);
                pos.x = modelP->pos.x + modelP->scale.x * (frandmod(50) - 25);
                pos.y = modelP->pos.y;
                pos.z = modelP->pos.z + modelP->scale.x * (frandmod(50) - 25);
                EffectDustCreate(modelId, pos.x, pos.y, pos.z, frandmod(10) + 30, &dustEffParam);
            }
            for (i = 0; i < 4; i++) {
                if (lbl_801309C0[charNo * 4 + i] == frameNo) {
                    PlayStepVoice(charNo, 0x105, voiceFlag);
                    break;
                }
            }
            break;
        case 4:
            if (frameNo < 5 && !(workP->attr & 0x10)) {
                dustEffParam.vel.x = 6.0 * sind(modelP->rot.y);
                dustEffParam.vel.z = 6.0 * cosd(modelP->rot.y);
                for (i = 0; i < 5; i++) {
                    dustEffParam.vel.y = 2.0 + 0.1 * frandmod(10);
                    pos.x = modelP->pos.x + modelP->scale.x * (frandmod(50) - 25);
                    pos.y = modelP->pos.y;
                    pos.z = modelP->pos.z + modelP->scale.x * (frandmod(50) - 25);
                    EffectDustCreate(modelId, pos.x, pos.y, pos.z, 20.0f, &dustEffParam);
                }
            }
            if (frameNo == 0) {
                PlayStepVoice(charNo, 0x10D, voiceFlag);
            }
            break;
        case 6:
            if (frameNo == 5 && !(workP->attr & 0x10)) {
                for (i = 0; i < 8; i++) {
                    dustEffParam.vel.x = 4.0 * sind(45.0f * i) * modelP->scale.x;
                    dustEffParam.vel.y = 0.0f;
                    dustEffParam.vel.z = 4.0 * cosd(45.0f * i) * modelP->scale.x;
                    EffectDustCreate(modelId, modelP->pos.x, modelP->pos.y + 10.0f * modelP->scale.x, modelP->pos.z, 20.0f, &dustEffParam);
                }
                for (i = 0; i < 8; i++) {
                    dustEffParam.vel.x = 2.0 * sind(45.0f * i + 22.5) * modelP->scale.x;
                    dustEffParam.vel.y = 0.0f;
                    dustEffParam.vel.z = 2.0 * cosd(45.0f * i + 22.5) * modelP->scale.x;
                    EffectDustCreate(modelId, modelP->pos.x, modelP->pos.y + 10.0f * modelP->scale.x, modelP->pos.z, 20.0f, &dustEffParam);
                }
            }
            for (i = 0; i < 2; i++) {
                if (lbl_801309E0[charNo * 2 + i] == frameNo) {
                    PlayStepVoice(charNo, 0x10D, voiceFlag);
                    break;
                }
            }
            break;
        case 7:
            if (motNo == 7) {
                Hu3DModelObjMtxGet(modelId, CharModelItemHookGet(charNo, workP->model, 0), hitMtx);
                effect_limit = 10;
            }
            else {
                Hu3DModelObjMtxGet(modelId, CharModelItemHookGet(charNo, workP->model, 1), hitMtx);
                effect_limit = 13;
            }
            if (frameNo <= effect_limit && !(workP->attr & 0x10)) {
                effectDotParam.vel.x = 0.0f;
                effectDotParam.vel.y = 0.0f;
                effectDotParam.vel.z = 0.0f;
                effectDotParam.scaleVel = -5.0f;
                pos.x = hitMtx[0][3];
                pos.y = hitMtx[1][3];
                pos.z = hitMtx[2][3];
                if (frameNo != 0) {
                    VECSubtract(&pos, ofs, &hitPos);
                    effect_limit = 0.2 * sqrtf(hitPos.x * hitPos.x + hitPos.y * hitPos.y + hitPos.z * hitPos.z);
                    if (effect_limit > 5) {
                        effect_limit = 5;
                    }
                    if (effect_limit < 1) {
                        effect_limit = 1;
                    }
                    for (i = 1; i <= effect_limit; i++) {
                        hitPos.x = ofs->x + (pos.x - ofs->x) * ((float)i / effect_limit);
                        hitPos.y = ofs->y + (pos.y - ofs->y) * ((float)i / effect_limit);
                        hitPos.z = ofs->z + (pos.z - ofs->z) * ((float)i / effect_limit);
                        EffectDotCreate(modelId, hitPos.x, hitPos.y, hitPos.z, 50.0f, &effectDotParam);
                    }
                }
                else {
                    EffectDotCreate(modelId, pos.x, pos.y, pos.z, 40.0f, &effectDotParam);
                }
                *ofs = pos;
            }
            break;
        case 0xA:
            if (frameNo == 0) {
                if (!(workP->attr & 0x10)) {
                    for (i = 0; i < 8; i++) {
                        effectStarParam.vel.x = 10.0 * sind(45.0f * i) * modelP->scale.x;
                        effectStarParam.vel.y = 0.0f;
                        effectStarParam.vel.z = 10.0 * cosd(45.0f * i) * modelP->scale.x;
                        EffectStarCreate(
                            modelId, modelP->pos.x, modelP->pos.y + 10.0f * modelP->scale.x, modelP->pos.z, 40.0f, &effectStarParam);
                    }
                    for (i = 0; i < 8; i++) {
                        dustEffParam.vel.x = 4.0 * sind(45.0f * i + 22.5) * modelP->scale.x;
                        dustEffParam.vel.y = 0.0f;
                        dustEffParam.vel.z = 4.0 * cosd(45.0f * i + 22.5) * modelP->scale.x;
                        EffectDustCreate(
                            modelId, modelP->pos.x, modelP->pos.y + 10.0f * modelP->scale.x, modelP->pos.z, 20.0f, &dustEffParam);
                    }
                }
                _CharFXPlay(charNo, 0x119, voiceFlag);
            }
            break;
        case 0x1B:
            if (modelP->motIdShift != -1 && skipAnimUpdate == 0) {
                break;
            }
            if (!(workP->attr & 0x10)) {
                if (frameNo == 10) {
                    effect_index = EffectWarnCreate(modelId, modelP->pos.x, modelP->pos.y + 100.0f, modelP->pos.z, 20.0f, &effectWarnParam);
                    if (effect_index == -1) {
                        break;
                    }
                    effect_model = &Hu3DData[effectMdl[0]];
                    effect_particles = effect_model->hookData;
                    effect_particle = &effect_particles->data[effect_index];
                    effect_particle->parManId = 0;
                    effect_particle->vel.x = modelId;
                    if (charNo == 7) {
                        effect_particle->vel.y = 190.0f;
                    }
                    else if (charNo == 2 || charNo == 5 || charNo == 6) {
                        effect_particle->vel.y = 140.0f;
                    }
                    else {
                        effect_particle->vel.y = 120.0f;
                    }
                    effect_particle->accel.x = effect_particle->accel.z = 0.0f;
                    effect_particle->accel.y = 100.0f;
                }
                if (frameNo == 30) {
                    for (i = 0; i < 8; i++) {
                        dustEffParam.vel.x = 4.0 * sind(45.0f * i + 22.5) * modelP->scale.x;
                        dustEffParam.vel.y = 0.0f;
                        dustEffParam.vel.z = 4.0 * cosd(45.0f * i + 22.5) * modelP->scale.x;
                        EffectDustCreate(
                            modelId, modelP->pos.x, modelP->pos.y + 10.0f * modelP->scale.x, modelP->pos.z, 20.0f, &dustEffParam);
                    }
                }
            }
            break;
        case 0x1C:
            if (modelP->motIdShift != -1 && skipAnimUpdate == 0) {
                break;
            }
            if (!(workP->attr & 0x10) && frameNo == 0) {
                effect_index = EffectWarnCreate(modelId, modelP->pos.x, modelP->pos.y + 100.0f, modelP->pos.z, 20.0f, &effectWarnParam);
                if (effect_index == -1) {
                    break;
                }
                effect_model = &Hu3DData[effectMdl[0]];
                effect_particles = effect_model->hookData;
                effect_particle = &effect_particles->data[effect_index];
                effect_particle->parManId = 0;
                effect_particle->vel.x = modelId;
                if (charNo == 7) {
                    effect_particle->vel.y = 150.0f;
                }
                else if (charNo == 2 || charNo == 5 || charNo == 6) {
                    effect_particle->vel.y = 120.0f;
                }
                else {
                    effect_particle->vel.y = 100.0f;
                }
                effect_particle->accel.x = effect_particle->accel.z = 0.0f;
                effect_particle->accel.y = 100.0f;
            }
            break;
        case 0x15:
        case 0x16:
        case 0x79:
            if (!(workP->attr & 1) && !(workP->attr & 0x10)) {
                _CharFXPlay(charNo, 0x11A, voiceFlag);
                for (i = 0; i < 3; i++) {
                    effect_index = EffectBirdCreate(
                        modelId, modelP->pos.x, modelP->pos.y + 100.0f * modelP->scale.x, modelP->pos.z, 1.0f, &effectWarnParam);
                    if (effect_index == -1) {
                        break;
                    }
                    effect_model = &Hu3DData[effectMdl[7]];
                    effect_particles = effect_model->hookData;
                    effect_particle = &effect_particles->data[effect_index];
                    effect_particle->parManId = 1;
                    effect_particle->time = 0;
                    effect_particle->vel.x = charNo;
                    effect_particle->vel.y = i * 0x78;
                    workP->attr |= 1;
                }
            }
            break;
        case 0x13:
            if ((frameNo & 1) && !(workP->attr & 0x10)) {
                Hu3DModelObjMtxGet(modelId, CharModelItemHookGet(charNo, workP->model, 4), hitMtx);
                pos.x = hitMtx[0][3];
                pos.y = hitMtx[1][3];
                pos.z = hitMtx[2][3];
                EffectSmokeCreate(modelId, pos.x, pos.y, pos.z, 20.0f, &effectSmokeParam);
            }
            break;
        case 5:
            if (frameNo == 0) {
                if (workP->stepFx == 4) {
                    _CharFXPlay(charNo, 0x10A, voiceFlag);
                }
                else if (workP->stepFx == 5) {
                    _CharFXPlay(charNo, 0x10C, voiceFlag);
                }
                else {
                    _CharFXPlay(charNo, 0x115, voiceFlag);
                }
            }
            break;
        case 8:
            if (frameNo == 0) {
                _CharFXPlay(charNo, 0x118, voiceFlag);
            }
            break;
        case 0x14:
        case 0x50:
            if (frameNo == 0 && !(workP->attr & 0x14)) {
                _CharFXPlay(charNo, 0x123, voiceFlag);
            }
            break;
        case 0x3B:
        case 0x48:
            if (frameNo == 0 && !(workP->attr & 0x12)) {
                _CharFXPlay(charNo, 0x122, voiceFlag);
            }
            break;
        case 0x3F:
        case 0x53:
        case 0x57:
            if (frameNo == 0 && !(workP->attr & 0x12)) {
                _CharFXPlay(charNo, 0x124, voiceFlag);
            }
            workP->attr |= 2;
            attrOld |= 2;
            break;
        case 0x4B:
            if (frameNo == lbl_801D3600[charNo] && !(workP->attr & 0x12)) {
                _CharFXPlay(charNo, 0x124, voiceFlag);
                workP->attr |= 2;
                attrOld |= 2;
            }
            break;
        case 0x4C:
            if (frameNo == lbl_801D3608[charNo] && !(workP->attr & 0x12)) {
                _CharFXPlay(charNo, 0x124, voiceFlag);
                workP->attr |= 2;
                attrOld |= 2;
            }
            break;
        case 0x17:
            if (!(workP->attr & 0x12)) {
                if (omcurovl < DLL_w01dll && frameNo == lbl_801D35F0[charNo]) {
                    _CharFXPlay(charNo, 0x124, voiceFlag);
                    workP->attr |= 2;
                    attrOld |= 2;
                }
                else if (omcurovl >= DLL_w01dll && frameNo == lbl_801D35F8[charNo]) {
                    _CharFXPlay(charNo, 0x122, voiceFlag);
                    workP->attr |= 2;
                    attrOld |= 2;
                }
            }
            break;
        case 0x18:
            if (frameNo == lbl_801D3610[charNo] && !(workP->attr & 0x14)) {
                _CharFXPlay(charNo, 0x121, voiceFlag);
                workP->attr |= 4;
                attrOld |= 4;
            }
            break;
        case 0x2A:
        case 0x72:
            if (frameNo == 0 && !(workP->attr & 0x14)) {
                _CharFXPlay(charNo, 0x121, voiceFlag);
            }
            workP->attr |= 4;
            attrOld |= 4;
            break;
        case 0x49:
        case 0x4E:
            if (frameNo == 0 && !(workP->attr & 0x14)) {
                _CharFXPlay(charNo, 0x12E, voiceFlag);
            }
            workP->attr |= 4;
            attrOld |= 4;
            break;
    }
    if (skipAnimUpdate == 0) {
        if (!(attrOld & 4)) {
            workP->attr &= ~4;
        }
        if (!(attrOld & 2)) {
            workP->attr &= ~2;
        }
    }
}

/**
 * @brief Plays a character sound unless the motion's voice-disable bit is set.
 * @param charNo Character index used for voice selection and optional panning.
 * @param seNo Character-relative cue ID passed unchanged to CharFXPlay or
 * CharFXPlayPos; those functions add the character bank offset `charNo << 6`.
 * @param voiceFlag Bit 0 suppresses playback when set.
 * @return Audio handle when playback occurs; no defined value when suppressed.
 */
static s32 _CharFXPlay(s16 charNo, s16 seNo, u8 voiceFlag)
{
    CHARWORK *workP = &charWork[charNo];
    HU3DMODEL *modelP = &Hu3DData[workP->modelId];
    if (voiceFlag & 1) {
        return;
    }
    if (workP->attr & 8) {
        return CharFXPlayPos(charNo, seNo, &modelP->pos);
    }
    else {
        return CharFXPlay(charNo, seNo);
    }
}

/**
 * @brief Lazily creates the shared character particle models and work arrays.
 */
static void EffectInit(void)
{
    HU3DPARTICLEDATA *particleDataP;
    HU3DPARTICLE *particleP;
    void *data;
    ANIMDATA *anim;
    s16 effInitF;
    s16 i;
    s16 j;

    effInitF = FALSE;
    for (i = 0; i < CHAR_EFFECT_AND_PARTICLE_MAX; i++) {
        if (effectMdl[i] == -1) {
            data = HuDataSelHeapReadNum(effectDataTbl[i].dataNum, HU_MEMNUM_OVL, HEAP_MODEL);
            anim = HuSprAnimRead(data);
            effectMdl[i] = Hu3DParticleCreate(anim, effectDataTbl[i].maxCnt);
            if (i == CHAR_EFFECT_AND_PARTICLE_MAX - 1) {
                Hu3DParticleAnimModeSet(effectMdl[i], 0);
            }
            Hu3DParticleHookSet(effectMdl[i], UpdateEffect);
            if (!particleData[i]) {
                particleData[i] = HuMemDirectMalloc(HEAP_HEAP, effectDataTbl[i].maxCnt * sizeof(EFFECTPARAM));
            }
            Hu3DParticleBlendModeSet(effectMdl[i], effectDataTbl[i].blendMode);
            particleP = Hu3DData[effectMdl[i]].hookData;
            particleP->emitCnt = 0;
            particleP->work = particleData[i];
            particleP->count = 1;
            particleDataP = particleP->data;
            for (j = 0; j < particleP->maxCnt; j++, particleDataP++) {
                particleDataP->scale = 0.0f;
            }
            effInitF = TRUE;
        }
    }
    if (effInitF != 0) {
        HuDataDirClose(DATADIR_EFFECT);
    }
}

/**
 * @brief Allocates a dust particle scaled and camera-matched to a model.
 * @param modelId Source model for x scale and camera visibility.
 * @param posX World-space x coordinate.
 * @param posY World-space y coordinate.
 * @param posZ World-space z coordinate.
 * @param scale Base particle scale before applying the model's x scale.
 * @param param Initial dust motion, color, and lifetime parameters.
 * @return Particle slot index, or -1 when unavailable.
 */
static s16 EffectDustCreate(s16 modelId, float posX, float posY, float posZ, float scale, EFFECTPARAM *param)
{
    HU3DMODEL *modelP = &Hu3DData[modelId];
    if (effectMdl[2] == -1) {
        return -1;
    }
    scale *= modelP->scale.x;
    return EffectCreate(effectMdl[2], modelP->cameraBit, posX, posY, posZ, scale, param);
}

/**
 * @brief Allocates a smoke particle scaled and camera-matched to a model.
 * @param modelId Source model for x scale and camera visibility.
 * @param posX World-space x coordinate.
 * @param posY World-space y coordinate.
 * @param posZ World-space z coordinate.
 * @param scale Base particle scale before applying the model's x scale.
 * @param param Initial smoke motion, color, and lifetime parameters.
 * @return Particle slot index, or -1 when unavailable.
 */
static s16 EffectSmokeCreate(s16 modelId, float posX, float posY, float posZ, float scale, EFFECTPARAM *param)
{
    HU3DMODEL *modelP = &Hu3DData[modelId];
    if (effectMdl[3] == -1) {
        return -1;
    }
    scale *= modelP->scale.x;
    return EffectCreate(effectMdl[3], modelP->cameraBit, posX, posY, posZ, scale, param);
}

/**
 * @brief Allocates a dot particle scaled and camera-matched to a model.
 * @param modelId Source model for x scale and camera visibility.
 * @param posX World-space x coordinate.
 * @param posY World-space y coordinate.
 * @param posZ World-space z coordinate.
 * @param scale Base particle scale before applying the model's x scale.
 * @param param Initial dot motion, color, and lifetime parameters.
 * @return Particle slot index, or -1 when unavailable.
 */
static s16 EffectDotCreate(s16 modelId, float posX, float posY, float posZ, float scale, EFFECTPARAM *param)
{
    HU3DMODEL *modelP = &Hu3DData[modelId];
    if (effectMdl[6] == -1) {
        return -1;
    }
    scale *= modelP->scale.x;
    return EffectCreate(effectMdl[6], modelP->cameraBit, posX, posY, posZ, scale, param);
}

/**
 * @brief Allocates a star particle scaled and camera-matched to a model.
 * @param modelId Source model for x scale and camera visibility.
 * @param posX World-space x coordinate.
 * @param posY World-space y coordinate.
 * @param posZ World-space z coordinate.
 * @param scale Base particle scale before applying the model's x scale.
 * @param param Initial star motion, color, and lifetime parameters.
 * @return Particle slot index, or -1 when unavailable.
 */
static s16 EffectStarCreate(s16 modelId, float posX, float posY, float posZ, float scale, EFFECTPARAM *param)
{
    HU3DMODEL *modelP = &Hu3DData[modelId];
    if (effectMdl[4] == -1) {
        return -1;
    }
    scale *= modelP->scale.x;
    return EffectCreate(effectMdl[4], modelP->cameraBit, posX, posY, posZ, scale, param);
}

/**
 * @brief Allocates a warning-mark particle attached to a model's camera set.
 * @param modelId Source model for x scale and camera visibility.
 * @param posX World-space x coordinate.
 * @param posY World-space y coordinate.
 * @param posZ World-space z coordinate.
 * @param scale Base particle scale before applying the model's x scale.
 * @param param Initial warning-particle parameters.
 * @return Particle slot index, or -1 when unavailable.
 */
static s16 EffectWarnCreate(s16 modelId, float posX, float posY, float posZ, float scale, EFFECTPARAM *param)
{
    HU3DMODEL *modelP = &Hu3DData[modelId];
    if (effectMdl[0] == -1) {
        return -1;
    }
    scale *= modelP->scale.x;
    return EffectCreate(effectMdl[0], modelP->cameraBit, posX, posY, posZ, scale, param);
}

/**
 * @brief Allocates a circling-bird particle for a character model.
 * @param modelId Source model for x scale and camera visibility.
 * @param posX Initial world-space x coordinate.
 * @param posY Initial world-space y coordinate.
 * @param posZ Initial world-space z coordinate.
 * @param scale Base particle scale before applying the model's x scale.
 * @param param Initial bird-particle parameters.
 * @return Particle slot index, or -1 when unavailable.
 */
static s16 EffectBirdCreate(s16 modelId, float posX, float posY, float posZ, float scale, EFFECTPARAM *param)
{
    HU3DMODEL *modelP = &Hu3DData[modelId];
    if (effectMdl[7] == -1) {
        return -1;
    }
    scale *= modelP->scale.x;
    return EffectCreate(effectMdl[7], modelP->cameraBit, posX, posY, posZ, scale, param);
}

/**
 * @brief Initializes the next free slot in a shared particle model.
 * @param type Particle model ID containing the slot pool.
 * @param cameraBit Camera visibility mask copied to the particle.
 * @param posX Initial world-space x coordinate.
 * @param posY Initial world-space y coordinate.
 * @param posZ Initial world-space z coordinate.
 * @param scale Initial visible scale; zero denotes an unused slot.
 * @param param Initial velocity, color, acceleration, and decay settings.
 * @return Allocated particle slot index, or -1 when the pool is full.
 */
static s16 EffectCreate(s16 type, s16 cameraBit, float posX, float posY, float posZ, float scale, EFFECTPARAM *param)
{
    HU3DMODEL *modelP = &Hu3DData[type];
    HU3DPARTICLE *particleP = modelP->hookData;
    EFFECTPARAM *effParam = particleP->work;
    HU3DPARTICLEDATA *particleDataP = &particleP->data[particleP->emitCnt];
    s16 i;
    for (i = particleP->emitCnt; i < particleP->maxCnt; i++, particleDataP++) {
        if (!particleDataP->scale) {
            break;
        }
    }
    if (i >= particleP->maxCnt) {
        particleDataP = particleP->data;
        for (i = 0; i < particleP->maxCnt; i++, particleDataP++) {
            if (!particleDataP->scale) {
                break;
            }
        }
    }
    if (i != particleP->maxCnt) {
        effParam[i] = *param;
        particleDataP->cameraBit = cameraBit;
        particleDataP->pos.x = posX;
        particleDataP->pos.y = posY;
        particleDataP->pos.z = posZ;
        particleDataP->vel = param->vel;
        particleDataP->color.r = param->colorBegin.r;
        particleDataP->color.g = param->colorBegin.g;
        particleDataP->color.b = param->colorBegin.b;
        particleDataP->color.a = param->colorBegin.a;
        particleDataP->scaleBase = scale;
        particleDataP->scale = scale;
        particleDataP->time = 0;
        particleDataP->parManId = HU3D_PARMANID_NONE;
        particleP->emitCnt = i;
    }
    else {
        return -1;
    }
    return i;
}

/**
 * @brief Advances every active shared character particle by one frame.
 * @param model Particle model being rendered; not otherwise consulted.
 * @param particle Particle pool and per-slot effect parameters to update.
 * @param matrix Current model matrix supplied by the particle hook.
 */
static void UpdateEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    EFFECTPARAM *effParam = particle->work;
    HU3DPARTICLEDATA *particleDataP;
    s16 color_component;
    s16 i;

    if (particle->count == 0) {
        particleDataP = particle->data;
        for (i = 0; i < particle->maxCnt; i++, particleDataP++) {
            particleDataP->scale = 0.0f;
        }
    }
    particleDataP = particle->data;
    for (i = 0; i < particle->maxCnt; i++, particleDataP++) {
        if (particleDataP->scale) {
            if (particleDataP->parManId == -1) {
                particleDataP->vel.x *= effParam[i].velDecay.x;
                particleDataP->vel.y *= effParam[i].velDecay.y;
                particleDataP->vel.z *= effParam[i].velDecay.z;
                VECAdd(&particleDataP->vel, &particleDataP->pos, &particleDataP->pos);
                particleDataP->vel.y += effParam[i].gravity;
                color_component = particleDataP->color.r + effParam[i].colorWeight * (effParam[i].colorEnd.r - effParam[i].colorBegin.r);
                if (color_component < 0) {
                    color_component = 0;
                }
                else if (color_component > 0xFF) {
                    color_component = 0xFF;
                }
                particleDataP->color.r = color_component;
                color_component = particleDataP->color.g + effParam[i].colorWeight * (effParam[i].colorEnd.g - effParam[i].colorBegin.g);
                if (color_component < 0) {
                    color_component = 0;
                }
                else if (color_component > 0xFF) {
                    color_component = 0xFF;
                }
                particleDataP->color.g = color_component;
                color_component = particleDataP->color.b + effParam[i].colorWeight * (effParam[i].colorEnd.b - effParam[i].colorBegin.b);
                if (color_component < 0) {
                    color_component = 0;
                }
                else if (color_component > 0xFF) {
                    color_component = 0xFF;
                }
                particleDataP->color.b = color_component;
                color_component = particleDataP->color.a + effParam[i].alphaBase;
                if (color_component < 1) {
                    particleDataP->scale = 0.0f;
                }
                particleDataP->color.a = color_component;
                if (particleDataP->scale) {
                    if (effParam[i].attr & 1) {
                        particleDataP->scale = particleDataP->scaleBase * (((particleDataP->time + i) & 1) ? 1.0 : 0.5);
                    }
                    else {
                        particleDataP->scale = particleDataP->scaleBase;
                    }
                    particleDataP->scaleBase += effParam[i].scaleVel;
                    if (particleDataP->scaleBase <= 0.01f) {
                        particleDataP->scale = 0.0f;
                    }
                }
                particleDataP->time++;
            }
            else {
                switch (particleDataP->parManId) {
                    case 0:
                        RotateEffect(particleDataP);
                        break;
                    case 1:
                        PlayEffectSound(particleDataP);
                        break;
                    case 2:
                        UpdateModelEffect(particleDataP);
                        break;
                }
            }
        }
    }
    DCStoreRangeNoSync(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

/**
 * @brief Updates a warning particle that expands, follows, and fades on a model.
 * @param particleDataP Particle whose velocity fields encode the model ID and
 * vertical offset and whose acceleration encodes its local displacement.
 */
static void RotateEffect(HU3DPARTICLEDATA *particleDataP)
{
    HU3DMODEL *modelP = &Hu3DData[(s32)particleDataP->vel.x];
    float effect_scale;

    if (particleDataP->time < 8) {
        effect_scale = 0.3 + sind(40.0f + 10.0f * (particleDataP->time + 1));
        particleDataP->scale = 50.0f * effect_scale * modelP->scale.x;
        particleDataP->color.a = 0xFF;
        effect_scale = 0.3 + sind(15.0f * (particleDataP->time + 1));
    }
    else {
        effect_scale = 0.3 + sind(135);
    }
    effect_scale *= modelP->scale.x;
    particleDataP->pos.x = modelP->pos.x + particleDataP->accel.x * effect_scale;
    particleDataP->pos.y = modelP->pos.y + particleDataP->vel.y * modelP->scale.x + particleDataP->accel.y * effect_scale;
    particleDataP->pos.z = modelP->pos.z + particleDataP->accel.z * effect_scale;
    if (particleDataP->time > 20) {
        particleDataP->color.a -= 32;
        particleDataP->scale -= 8.0f * modelP->scale.x;
        if (particleDataP->scale < 0.0f) {
            particleDataP->scale = 0.0f;
        }
    }
    particleDataP->time++;
}

static float voiceParam[16]
    = { 110.0f, 160.0f, 110.0f, 160.0f, 150.0f, 180.0f, 130.0f, 160.0f, 130.0f, 160.0f, 150.0f, 160.0f, 150.0f, 180.0f, 120.0f, 210.0f };

/**
 * @brief Updates one bird particle orbiting a character and plays its cue.
 * @param particleDataP Bird particle whose velocity stores character index and
 * orbit phase.
 */
static void PlayEffectSound(HU3DPARTICLEDATA *particleDataP)
{
    HU3DMODEL *modelP;
    CHARWORK *workP;
    s16 orbit_angle;
    s16 char_no;
    s16 height;

    char_no = particleDataP->vel.x;
    workP = &charWork[char_no];
    modelP = &Hu3DData[workP->modelId];
    if (particleDataP->time < 0x14 && particleDataP->scale < 40.0f * modelP->scale.x) {
        particleDataP->scale += 4.0f * modelP->scale.x;
    }
    particleDataP->color.a = 0xFF;
    if (workP->motNoCurr == 0x16 || workP->motNoShiftCurr == 0x16) {
        height = voiceParam[char_no * 2];
    }
    else {
        height = voiceParam[char_no * 2 + 1];
    }
    orbit_angle = (particleDataP->time * 5) % 360;
    particleDataP->pos.x = modelP->pos.x + 40.0 * sind(particleDataP->vel.y + orbit_angle) * modelP->scale.x;
    particleDataP->pos.y = modelP->pos.y + height * modelP->scale.x;
    particleDataP->pos.z = modelP->pos.z + 40.0 * cosd(particleDataP->vel.y + orbit_angle) * modelP->scale.x;
    particleDataP->time++;
    if (particleDataP->time >= 0x8F) {
        particleDataP->time = 0x48;
    }
    if (workP->motNoCurr != 0x15 && workP->motNoCurr != 0x16 && workP->motNoCurr != 0x79 && particleDataP->time > 0x1E) {
        particleDataP->scale -= 4.0f * modelP->scale.x;
        if (particleDataP->scale < 0.0f) {
            particleDataP->scale = 0.0f;
            workP->attr &= ~1;
            if (particleDataP->vel.y == 0.0) {
                _CharFXPlay(char_no, 0x100, workP->attr);
            }
        }
    }
}

/**
 * @brief Loads and attaches a motion to a character's model.
 *
 * @param charNo Character index receiving the motion.
 * @param data_num Packed data-file number; character motion directories are
 * resolved through ARAM first and other directories through normal data I/O.
 * @return New motion ID, or HU3D_MOTID_NONE when no model or slot is available.
 */
HU3DMOTID CharMotionCreate(s16 charNo, s32 data_num)
{
    CHARWORK *workP = &charWork[charNo];
    s16 i;
    s16 motNo;
    u32 dir;
    void *data;

    if (workP->modelId == -1) {
        return -1;
    }
    for (motNo = 0; motNo < 32; motNo++) {
        if (workP->motId[motNo] == -1) {
            break;
        }
    }
    if (motNo == CHAR_MOT_MAX) {
        return HU3D_MOTID_NONE;
    }
    dir = data_num & 0xFFFF0000;
    for (i = 0; i < CHARNO_MAX; i++) {
        if (dir == charDirTbl[i][2]) {
            break;
        }
    }
    if (i != CHARNO_MAX  || dir == 0) {
        data_num &= 0xFFFF;
        data = HuAR_ARAMtoMRAMFileRead(data_num | charDirTbl[charNo][2], HU_MEMNUM_OVL, HEAP_MODEL);
        if (!data) {
            data = HuDataSelHeapReadNum(data_num | charDirTbl[charNo][2], HU_MEMNUM_OVL, HEAP_MODEL);
        }
        workP->motNoTbl[motNo] = data_num;
    }
    else {
        data = HuDataSelHeapReadNum(data_num, HU_MEMNUM_OVL, HEAP_MODEL);
        workP->motNoTbl[motNo] = HU3D_MOTID_NONE;
    }
    workP->motId[motNo] = Hu3DJointMotion(workP->modelId, data);
    workP->voiceFlag[motNo] = 0;
    return workP->motId[motNo];
}

/**
 * @brief Associates a loaded character motion with its effect-table number.
 * @param charNo Character index owning the motion.
 * @param motId Loaded motion ID to find in the character's motion slots.
 * @param motNo Motion data number consumed by UpdateCharAnim.
 */
void CharMotionNoSet(s16 charNo, HU3DMOTID motId, s32 motNo)
{
    CHARWORK *workP = &charWork[charNo];
    s16 i;

    for (i = 0; i < ARRAY_COUNT(workP->motId); i++) {
        if (workP->motId[i] == motId) {
            break;
        }
    }
    if (i != ARRAY_COUNT(workP->motId)) {
        workP->motNoTbl[i] = motNo;
    }
}

/**
 * @brief Detaches and destroys one loaded character motion.
 * @param charNo Character index owning the motion slot.
 * @param motId Motion ID to destroy.
 */
void CharMotionKill(s16 charNo, u32 motId)
{
    CHARWORK *workP = &charWork[charNo];
    s16 i;

    for (i = 0; i < ARRAY_COUNT(workP->motId); i++) {
        if (workP->motId[i] == motId) {
            break;
        }
    }
    workP->motId[i] = -1;
    Hu3DMotionKill(motId);
}

/**
 * @brief Closes cached motion-directory data for one or all characters.
 * @param charNo Character index, or CHARNO_NONE to process every character.
 */
void CharMotionDataClose(s16 charNo)
{
    s16 i;

    if (charNo == CHARNO_NONE) {
        for (i = 0; i < CHARNO_MAX; i++) {
            CharMotionDataClose(i);
        }
    }
    else {
        HuDataDirClose(charDirTbl[charNo][2]);
    }
}

/**
 * @brief Closes cached model and motion directories for one or all characters.
 * @param charNo Character index, or CHARNO_NONE to process every character.
 */
void CharModelDataClose(s16 charNo)
{
    s16 i;

    if (charNo == CHARNO_NONE) {
        for (i = 0; i < CHARNO_MAX; i++) {
            CharModelDataClose(i);
            // Required to match.
            (void)i;
            (void)charNo;
        }
    }
    else {
        HuDataDirClose(charDirTbl[charNo][0]);
        HuDataDirClose(charDirTbl[charNo][1]);
        HuDataDirClose(charDirTbl[charNo][2]);
    }
}

/**
 * @brief Destroys a character model, its motions, and update processes.
 *
 * @details CHARNO_NONE destroys every character. Shared effect models are also
 * destroyed after the final character model is gone.
 *
 * @param charNo Character index, or CHARNO_NONE for all characters.
 */
void CharModelKill(s16 charNo)
{
    CHARWORK *workP;
    s16 i;

    if (charNo == CHARNO_NONE) {
        for (i = 0; i < CHARNO_MAX; i++) {
            CharModelKill(i);
        }
        for (i = 0; i < ARRAY_COUNT(dustFlags); i++) {
            dustFlags[i] = 0;
        }
        return;
    }
    CharMotionDataClose(charNo);
    workP = &charWork[charNo];
    for (i = 0; i < ARRAY_COUNT(workP->motId); i++) {
        if (workP->motId[i] != HU3D_MOTID_NONE) {
            Hu3DMotionKill(workP->motId[i]);
        }
        workP->motId[i] = HU3D_MOTID_NONE;
    }
    if (workP->modelId != HU3D_MODELID_NONE) {
        Hu3DModelKill(workP->modelId);
    }
    workP->modelId = HU3D_MODELID_NONE;
    for (i = 0; i < CHARNO_MAX; i++) {
        if (charWork[i].modelId != HU3D_MODELID_NONE) {
            break;
        }
    }
    if (i == CHARNO_MAX) {
        for (i = 0; i < CHAR_EFFECT_AND_PARTICLE_MAX; i++) {
            if (effectMdl[i] != HU3D_MODELID_NONE) {
                Hu3DModelKill(effectMdl[i]);
            }
            effectMdl[i] = HU3D_MODELID_NONE;
        }
    }
    if (workP->process) {
        HuMemDirectFree(workP->process->user_data);
        HuPrcKill(workP->process);
        for (i = 0; i < ARRAY_COUNT(itemHookProcess); i++) {
            if (itemHookProcess[i]) {
                HuPrcKill(itemHookProcess[i]);
            }
            itemHookProcess[i] = NULL;
        }
        workP->process = NULL;
    }
}

/**
 * @brief Starts a character motion after resetting animated eye textures.
 * @param charNo Character index whose model is updated.
 * @param motId Motion ID to make active.
 */
void CharMotionSet(s16 charNo, HU3DMOTID motId)
{
    CHARWORK *workP = &charWork[charNo];
    HU3DMOTION *motP = &Hu3DMotion[motId];
    EyeBmpUpdate(charNo);
    Hu3DMotionSet(workP->modelId, motId);
}

/**
 * @brief Resets texture transforms for the active model's eye materials.
 * @param charNo Character index used to select the model and eye bitmap names.
 */
static void EyeBmpUpdate(s16 charNo)
{
    CHARWORK *workP = &charWork[charNo];
    HU3DMODEL *modelP = &Hu3DData[workP->modelId];
    HSFATTRIBUTE *attrP = modelP->hsf->attribute;
    s16 modelBit;
    s16 i;
    char **eyeBmp;
    
    for (i = 0, modelBit = 1; i < 4; i++, modelBit <<= 1) {
        if (modelBit & workP->model) {
            break;
        }
    }
    if (i < 4) {
        eyeBmp = CharModelEyeBmpGet(charNo, workP->model);
        for (i = 0; i < modelP->hsf->attributeNum; i++, attrP++) {
            if ((attrP->bitmap->name[0] == eyeBmp[0][0] && strcmp(attrP->bitmap->name, eyeBmp[0]) == 0)
            || (attrP->bitmap->name[0] == eyeBmp[1][0] && strcmp(attrP->bitmap->name, eyeBmp[1]) == 0)) {
                if (attrP->animWorkP) {
                    HU3DATTRANIM *particleDataP = attrP->animWorkP;
                    particleDataP->trans3D.x = particleDataP->trans3D.y = particleDataP->trans3D.z = 0.0f;
                    particleDataP->rot.x = particleDataP->rot.y = particleDataP->rot.z = 0.0f;
                }
            }
        }
    }
}

char *charEyeBmpNameTbl[64] = { "s3c000m1_eyes", "s3c000m1_eyes", "s3c000m1_eyes", "s3c000m1_eyes", "s3c000m2_eyes", "s3c000m2_eyes", "s3c000m3_eyes",
    "s3c000m3_eyes", "S3c001m0_eye", "S3c001m0_eye", "S3c001m1_eye", "S3c001m1_eye", "c001m3_eye", "c001m3_eye", "c001m3_eye", "c001m3_eye",
    "s3c002m0_r_eye", "s3c002m0_l_eye", "s3c002m1_r_eye", "s3c002m1_l_eye", "s3c002m2_r_eye", "s3c002m2_l_eye", "", "", "eye1", "eye2", "S3c003m1",
    "S3c003m1", "eye1", "eye2", "eye1", "eye2", "GC-eyes", "GC-eyes", "s3c004m1_eye", "s3c004m1_eye", "Clswario_eye_l1_AUTO12",
    "Clswario_eye_l1_AUTO13", "", "", "m_donkey_eye4", "m_donkey_eye5", "S3donkey_eye", "S3donkey_eye", "m_donkey_eye1", "m_donkey_eye2", "", "",
    "GC-eyes", "GC-eyes", "s3c007m1_Eye_L", "s3c007m1_Eye_R", "mat87", "mat89", "", "", "clswaluigi_eye_l1_AUTO1", "clswaluigi_eye_l1_AUTO2",
    "s3c007_m1_eye", "s3c007_m1_eye", "clswaluigi_eye_l1_AUTO9", "clswaluigi_eye_l1_AUTO10", "", "" };

char **CharModelEyeBmpGet(s16 charNo, s16 model)
{
    s16 bit;
    s16 i;

    for (i = 0, bit = 1; i < 3; i++, bit <<= 1) {
        if (bit & model) {
            break;
        }
    }
    return &charEyeBmpNameTbl[charNo * (CHAR_MODEL_MAX * 2) + i * 2];
}

static char *hookNameTbl[CHARNO_MAX * 5]
    = { "a-itemhook-r", "a-itemhook-l", "a-itemhook-fr", "a-itemhook-fl", "a-itemhook-body", "a-itemhook-r", "a-itemhook-l", "a-itemhook-fr",
          "a-itemhook-fl", "a-itemhook-body", "a-itemhook-r", "a-itemhook-l", "a-itemhook-fr", "a-itemhook-fl", "a-itemhook-body", "a-itemhook-r",
          "a-itemhook-l", "a-itemhook-fr", "a-itemhook-fl", "a-itemhook-body", "a-itemhook-r", "a-itemhook-l", "a-itemhook-fr", "a-itemhook-fl",
          "a-itemhook-body", "a-itemhook-r", "a-itemhook-l", "a-itemhook-fr", "a-itemhook-fl", "a-itemhook-body", "a-itemhook-r", "a-itemhook-l",
          "a-itemhook-fr", "a-itemhook-fl", "a-itemhook-body", "a-itemhook-r", "a-itemhook-l", "a-itemhook-fr", "a-itemhook-fl", "a-itemhook-body" };

/**
 * @brief Gets the named item attachment point for a character.
 * @param charNo Character index selecting the hook-name table row.
 * @param model Model-variant mask; currently does not alter the returned row.
 * @param hookNo Hook index: right, left, front-right, front-left, or body.
 * @return Pointer to the static HSF object name; ownership remains internal.
 */
char *CharModelItemHookGet(s16 charNo, s16 model, s16 hookNo)
{
    s16 i;
    s16 bit;

    for (i = 0, bit = 1; i < 3; i++, bit <<= 1) {
        if (bit & model) {
            break;
        }
    }
    return hookNameTbl[charNo * 5 + hookNo];
}

/**
 * @brief Sets a character model's active motion time.
 * @param charNo Character-table index from 0 through CHARNO_MAX - 1 whose
 * `charWork.modelId` selects the model passed to Hu3DMotionTimeSet.
 * @param time Motion time in frames.
 */
void CharMotionTimeSet(s16 charNo, float time)
{
    CHARWORK *workP = &charWork[charNo];

    Hu3DMotionTimeSet(workP->modelId, time);
}

/**
 * @brief Gets a character model's active motion time.
 * @param charNo Character-table index from 0 through CHARNO_MAX - 1 whose
 * active model supplies the motion time returned by Hu3DMotionTimeGet.
 * @return Current motion time in frames.
 */
float CharMotionTimeGet(s16 charNo)
{
    CHARWORK *workP = &charWork[charNo];

    return Hu3DMotionTimeGet(workP->modelId);
}

/**
 * @brief Gets the duration of a character model's active motion.
 * @param charNo Character-table index from 0 through CHARNO_MAX - 1 whose
 * active model supplies the duration returned by Hu3DMotionMaxTimeGet.
 * @return Motion duration in frames.
 */
float CharMotionMaxTimeGet(s16 charNo)
{
    CHARWORK *workP = &charWork[charNo];

    return Hu3DMotionMaxTimeGet(workP->modelId);
}

/**
 * @brief Checks whether a character model's active motion has ended.
 * @param charNo Character-table index from 0 through CHARNO_MAX - 1 whose
 * active model is passed to Hu3DMotionEndCheck.
 * @return Nonzero when the motion is at its end; zero otherwise.
 */
s32 CharMotionEndCheck(s16 charNo)
{
    CHARWORK *workP = &charWork[charNo];

    return Hu3DMotionEndCheck(workP->modelId);
}

/**
 * @brief Gets the blend-target motion ID for a character model.
 * @param charNo Character-table index from 0 through CHARNO_MAX - 1 whose
 * active model supplies the blend-target ID from Hu3DMotionShiftIDGet.
 * @return Shift motion ID, or HU3D_MOTID_NONE when no shift is active.
 */
s16 CharMotionShiftIDGet(s16 charNo)
{
    CHARWORK *workP = &charWork[charNo];

    return Hu3DMotionShiftIDGet(workP->modelId);
}

/**
 * @brief Blends a character model to another motion.
 * @param charNo Character-table index from 0 through CHARNO_MAX - 1 whose
 * `charWork.modelId` receives the destination motion blend.
 * @param motId Destination motion ID.
 * @param start Starting frame in the destination motion.
 * @param end Blend duration in frames.
 * @param attr Motion attributes applied to the destination motion.
 */
void CharMotionShiftSet(s16 charNo, HU3DMOTID motId, float start, float end, u32 attr)
{
    CHARWORK *workP = &charWork[charNo];
    HU3DMOTION *motP = &Hu3DMotion[motId];

    Hu3DMotionShiftSet(workP->modelId, motId, start, end, attr);
}

/**
 * @brief Gets the current time of a character's shift motion.
 * @param charNo Character-table index from 0 through CHARNO_MAX - 1 whose
 * active model supplies the shift-motion time from Hu3DMotionShiftTimeGet.
 * @return Shift-motion time in frames.
 */
float CharMotionShiftTimeGet(s16 charNo)
{
    CHARWORK *workP = &charWork[charNo];

    return Hu3DMotionShiftTimeGet(workP->modelId);
}

/**
 * @brief Sets a character model's motion playback rate.
 * @param charNo Character-table index from 0 through CHARNO_MAX - 1 whose
 * active model receives the new Hu3D motion speed.
 * @param speed Motion frames advanced per update relative to normal playback.
 */
void CharMotionSpeedSet(s16 charNo, float speed)
{
    CHARWORK *workP = &charWork[charNo];

    Hu3DMotionSpeedSet(workP->modelId, speed);
}

/**
 * @brief Assigns every live shared character effect model to a render layer.
 * @param layerNo Render layer from 0 through HU3D_LAYER_MAX - 1 assigned to
 * each live model in the eight-entry shared character-effect pool.
 */
void CharEffectLayerSet(s16 layerNo)
{
    s16 i;

    for (i = 0; i < CHAR_EFFECT_AND_PARTICLE_MAX; i++) {
        if (effectMdl[i] != HU3D_MODELID_NONE) {
            Hu3DModelLayerSet(effectMdl[i], layerNo);
        }
    }
}

/**
 * @brief Starts a hook-dust worker in the first free tracked process slot.
 * @return New process pointer, or NULL when all hook-process slots are in use.
 */
static inline HUPROCESS *CharModelItemHookCreateInlineFunc(void)
{
    s16 i;

    for (i = 0; i < ARRAY_COUNT(itemHookProcess); i++) {
        if (!itemHookProcess[i]) {
            break;
        }
    }
    if (i == ARRAY_COUNT(itemHookProcess)) {
        return NULL;
    }
    else {
        itemHookProcess[i] = HuPrcCreate(CreateHookDust, 0x64, 0x2000, 0);
        return itemHookProcess[i];
    }
}

typedef struct HookDustWork_s {
    u16 cameraBit;
    HU3DMODELID modelId;
} HOOKDUSTWORK; // Size 4

/**
 * @brief Spawns dust at a named object attached to a character model.
 *
 * @param charNo Character index owning the attachment object.
 * @param objName HSF object name whose constant geometry bounds locate the dust.
 */
void CharModelHookDustCreate(s16 charNo, char *objName)
{
    CHARWORK *workP = &charWork[charNo];
    HU3DMODEL *modelP = &Hu3DData[workP->modelId];
    HSFOBJECT *objPtr = Hu3DModelObjPtrGet(workP->modelId, objName);
    HSFCONSTDATA *constData;
    HUPROCESS *process;
    HOOKDUSTWORK *hookDustWork;
    s16 hookMdlId;
    Mtx hookMtx;
    HuVecF transform_values;

    Hu3DModelObjMtxGet(workP->modelId, objName, hookMtx);
    constData = objPtr->constData;
    hookMdlId = constData->hookMdlId;
    if (hookMdlId != HU3D_MODELID_NONE) {
        Hu3DModelHookObjReset(workP->modelId, objName);
        process = CharModelItemHookCreateInlineFunc();
        if (!process) {
            Hu3DModelAttrSet(hookMdlId, HU3D_ATTR_DISPOFF);
            return;
        }
        process->user_data = hookDustWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(HOOKDUSTWORK), HU_MEMNUM_OVL);
        modelP = &Hu3DData[hookMdlId];
        Hu3DMtxTransGet(hookMtx, &transform_values);
        Hu3DModelPosSetV(hookMdlId, &transform_values);
        Hu3DMtxRotGet(hookMtx, &transform_values);
        mtxRot(modelP->mtx, transform_values.x, transform_values.y, transform_values.z);
        Hu3DMtxScaleGet(hookMtx, &transform_values);
        Hu3DModelScaleSetV(hookMdlId, &transform_values);
        hookDustWork->modelId = hookMdlId;
        hookDustWork->cameraBit = modelP->cameraBit;
    }
}

/**
 * @brief Computes an attached object's world bounds and emits its dust burst.
 *
 * @details Runs as a short-lived process using HOOKDUSTWORK in user_data, then
 * clears its tracked process slot before terminating.
 */
static void CreateHookDust(void)
{
    Mtx rootMtx;
    HuVecF pos;
    HUPROCESS *process;
    s16 j;
    s16 i;
    HOOKDUSTWORK *hookDustWork = HuPrcCurrentGet()->user_data;
    HU3DMODEL *modelP = &Hu3DData[hookDustWork->modelId];
    Hu3DModelObjMtxGet(hookDustWork->modelId, "", rootMtx);
    pos.x = PGMinPos.x + (PGMaxPos.x - PGMinPos.x) / 2;
    pos.y = PGMinPos.y + (PGMaxPos.y - PGMinPos.y) / 2;
    pos.z = PGMinPos.z + (PGMaxPos.z - PGMinPos.z) / 2;
    for (i = 0; i < 40; i++) {
        modelP->pos.y += 4.0f;
        mtxRotCat(modelP->mtx, 24.0f, 0.0f, 0.0f);
        modelP->scale.x *= 0.95f;
        modelP->scale.y *= 0.95f;
        modelP->scale.z *= 0.95f;
        HuPrcVSleep();
    }
    Hu3DModelObjMtxGet(hookDustWork->modelId, "", rootMtx);
    pos.x = PGMinPos.x + (PGMaxPos.x - PGMinPos.x) / 2;
    pos.y = PGMinPos.y + (PGMaxPos.y - PGMinPos.y) / 2;
    pos.z = PGMinPos.z + (PGMaxPos.z - PGMinPos.z) / 2;
    Hu3DModelAttrSet(hookDustWork->modelId, HU3D_ATTR_DISPOFF);
    dustEffParam.vel.x = 0.0f;
    dustEffParam.vel.y = 0.0f;
    dustEffParam.vel.z = 0.0f;
    EffectDustCreate(hookDustWork->modelId, pos.x, pos.y, pos.z, 40.0f, &dustEffParam);
    for (i = 0; i < 8; i++) {
        dustEffParam.vel.x = frandmod(10) - 5;
        dustEffParam.vel.y = frandmod(10) - 5;
        dustEffParam.vel.z = frandmod(10) - 5;
        EffectDustCreate(hookDustWork->modelId, pos.x, pos.y, pos.z, 20.0f, &dustEffParam);
    }
    PSMTXIdentity(modelP->mtx);
    process = HuPrcCurrentGet();
    for (j = 0; j < ARRAY_COUNT(itemHookProcess); j++) {
        if (itemHookProcess[j] == process) {
            HuPrcKill(process);
            itemHookProcess[j] = NULL;
        }
    }
    while (1) {
        HuPrcVSleep();
    }
}

static EFFECTPARAM modelSmokeEffParam
    = { 0, 0xFF, 0xFF, 0xFF, 0xFF, 0x40, 0x20, 0x00, 0xFF, { 0.0f, 2.0f, 1.0f }, 0.95f, 0.95f, 0.95f, 0.0f, 0x00000000, 1.0f, -5.0f, 0.02f };

/**
 * @brief Emits expanding radial and scattered smoke around a world position.
 * @param cameraBit Camera visibility mask copied to each smoke particle.
 * @param pos World-space center retained as the particles' motion origin.
 */
void CharEffectSmokeCreate(s16 cameraBit, HuVecF *pos)
{
    s16 effectNo;
    s16 i;
    HU3DPARTICLEDATA *particleDataP;
    HU3DPARTICLE *particleP;
    HU3DMODEL *modelP;

    for (i = 0; i < CHAR_EFFECT_AND_PARTICLE_MAX; i++) {
        effectNo = EffectCreate(effectMdl[3], cameraBit, pos->x, pos->y, pos->z, 20.0f, &modelSmokeEffParam);
        if (effectNo == -1) {
            break;
        }
        modelP = &Hu3DData[effectMdl[3]];
        particleP = modelP->hookData;
        particleDataP = &particleP->data[effectNo];
        particleDataP->parManId = 2;
        particleDataP->vel.x = 30.0 * sind(i * 45);
        particleDataP->vel.y = 30.0 * cosd(i * 45);
        particleDataP->vel.z = 0.0f;
        particleDataP->accel = *pos;
        particleDataP->speedDecay = 1.15f;
        particleDataP->colorIdx = 0.1f * (frandmod(20) - 10);
        particleDataP->color.a = 0xFF - frandmod(3) * 16;
    }
    for (i = 0; i < CHAR_EFFECT_AND_PARTICLE_MAX; i++) {
        effectNo = EffectCreate(effectMdl[3], cameraBit, pos->x, pos->y, pos->z, 10.0f, &modelSmokeEffParam);
        if (effectNo == -1) {
            break;
        }
        modelP = &Hu3DData[effectMdl[3]];
        particleP = modelP->hookData;
        particleDataP = &particleP->data[effectNo];
        particleDataP->parManId = 2;
        particleDataP->vel.x = frandmod(100) - 50;
        particleDataP->vel.y = frandmod(100) - 50;
        particleDataP->vel.z = frandmod(100) - 50;
        particleDataP->accel = *pos;
        particleDataP->speedDecay = 1.15f;
        particleDataP->colorIdx = 0.1f * (frandmod(20) - 10);
        particleDataP->color.a = 0xFF - frandmod(3) * 16;
    }
    effectNo = EffectCreate(effectMdl[3], cameraBit, pos->x, pos->y, pos->z, 10.0f, &modelSmokeEffParam);
    if (effectNo != -1) {
        modelP = &Hu3DData[effectMdl[3]];
        particleP = modelP->hookData;
        particleDataP = &particleP->data[effectNo];
        particleDataP->parManId = 2;
        particleDataP->vel.x = 0.0f;
        particleDataP->vel.y = 0.0f;
        particleDataP->vel.z = 0.0f;
        particleDataP->accel = *pos;
        particleDataP->colorIdx = 0.0f;
        particleDataP->speedDecay = 1.15f;
        particleDataP->color.a = 0xFF;
    }
}

/**
 * @brief Advances a radial model effect and fades it after eight frames.
 * @param particleDataP Particle whose acceleration stores its origin and whose
 * velocity stores the radial offset.
 */
static void UpdateModelEffect(HU3DPARTICLEDATA *particleDataP)
{
    float speed;
    float angle;
    s16 alpha;

    angle = 20.0f + 3.75f * particleDataP->time;
    if (angle > 90.0f) {
        angle = 90.0f;
    }
    speed = sind(angle);
    particleDataP->pos.x = particleDataP->accel.x + particleDataP->vel.x * speed;
    particleDataP->pos.y = particleDataP->accel.y + particleDataP->vel.y * speed;
    particleDataP->pos.z = particleDataP->accel.z + particleDataP->vel.z * speed;
    particleDataP->scale *= particleDataP->speedDecay;
    particleDataP->speedDecay -= 0.01;
    if (particleDataP->speedDecay < 1.0f) {
        particleDataP->speedDecay = 1.0f;
    }
    if (particleDataP->time > 8) {
        alpha = particleDataP->color.a;
        alpha -= 8;
        if (alpha < 0) {
            particleDataP->color.a = 0;
            particleDataP->scale = 0.0f;
        }
        else {
            particleDataP->color.a = alpha;
        }
    }
    particleDataP->time++;
}

static EFFECTPARAM coinEffParam
    = { 1, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, { 0.0f, 2.0f, 1.0f }, 0.95f, 1.0f, 0.95f, -0.1f, 0x00000000, -0.2f, -8.0f, 0.0f };

/**
 * @brief Emits a sixteen-particle coin glow ring from a world position.
 * @param cameraBit Camera visibility mask copied to each glow particle.
 * @param pos World-space center of the ring.
 */
void CharEffectCoinGlowCreate(s16 cameraBit, HuVecF *pos)
{
    s16 i;
    for (i = 0; i < 16; i++) {
        s16 angle = i * (360.0f / 16.0f);
        s16 effectNo;
        coinEffParam.vel.x = 5.0 * sind(angle);
        coinEffParam.vel.y = 0.1f * (frandmod(100) - 50);
        coinEffParam.vel.z = 5.0 * cosd(angle);
        effectNo = EffectCreate(effectMdl[5], cameraBit, pos->x, pos->y, pos->z, 30.0f, &coinEffParam);
        if (effectNo == -1) {
            break;
        }
    }
}

static EFFECTPARAM modelHitEffParam
    = { 0, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x80, 0x00, 0xFF, { 0.0f, 2.0f, 1.0f }, 0.95f, 0.95f, 0.95f, 0.0f, 0x00000000, -0.2f, -16.0f, 0.05f };

static EFFECTPARAM hitGlowEffParam
    = { 0, 0xE0, 0x20, 0x20, 0xFF, 0xE0, 0x20, 0x20, 0xFF, { 0.0f, 2.0f, 1.0f }, 1.0f, 1.0f, 1.0f, 0.0f, 0x00000000, -0.2f, -12.0f, 0.05f };

/**
 * @brief Emits a hit burst just beyond a character's right item hook.
 * @param charNo Character index supplying the hook transform and camera mask.
 */
void CharModelHitCreate(s16 charNo)
{
    CHARWORK *workP = &charWork[charNo];
    HU3DMODEL *modelP = &Hu3DData[workP->modelId];
    s16 motNo;
    Mtx mtx;
    HuVecF pos;
    HuVecF radius;

    for (motNo = 0; motNo < ARRAY_COUNT(workP->motId); motNo++) {
        if (workP->motId[motNo] == modelP->motId) {
            break;
        }
    }
    if (motNo != ARRAY_COUNT(workP->motId)) {
        Hu3DModelObjMtxGet(workP->modelId, "test11_tex_we-itemhook-r", mtx);
        pos.x = mtx[0][3];
        pos.y = mtx[1][3];
        pos.z = mtx[2][3];
        mtxRot(mtx, modelP->rot.x, modelP->rot.y, modelP->rot.z);
        radius.x = mtx[0][2];
        radius.y = mtx[1][2];
        radius.z = mtx[2][2];
        VECScale(&radius, &radius, 20.0f);
        VECAdd(&pos, &radius, &pos);
        CharEffectHitCreate(modelP->cameraBit, &pos, &modelP->rot);
    }
}

/**
 * @brief Emits paired yellow star and red glow particles in a hit ring.
 * @param cameraBit Camera visibility mask copied to every particle.
 * @param pos World-space center of the burst.
 * @param rot Euler rotation in degrees defining the ring's facing direction.
 */
void CharEffectHitCreate(s16 cameraBit, HuVecF *pos, HuVecF *rot)
{
    Mtx mtx;
    HuVecF radius;
    HuVecF dir;
    HuVecF vel;
    s16 effectNo;
    s16 i;
    
    mtxRot(mtx, rot->x, rot->y, rot->z);
    radius.x = mtx[0][2];
    radius.y = mtx[1][2];
    radius.z = mtx[2][2];
    for (i = 0; i < 8; i++) {
        float angle = i * 45;
        dir.x = radius.x * radius.y * (1.0 - cosd(angle)) - radius.z * sind(angle);
        dir.y = radius.y * radius.y + (1.0f - radius.y * radius.y) * cosd(angle);
        dir.z = radius.y * radius.z * (1.0 - cosd(angle)) + radius.x * sind(angle);
        VECNormalize(&dir, &dir);
        VECScale(&dir, &modelHitEffParam.vel, 10.0f);
        effectNo = EffectCreate(effectMdl[4], cameraBit, pos->x, pos->y, pos->z, 20.0f, &modelHitEffParam);
        if (effectNo == -1) {
            break;
        }
        VECScale(&radius, &vel, -2.0 - 0.1 * frandmod(20));
        VECScale(&dir, &dir, 2.0f);
        VECAdd(&dir, &vel, &hitGlowEffParam.vel);
        effectNo = EffectCreate(effectMdl[6], cameraBit, pos->x, pos->y, pos->z, 20.0f, &hitGlowEffParam);
        if (effectNo == -1) {
            break;
        }
    }
}

static EFFECTPARAM shoeHitEffParam
    = { 0, 0x20, 0x20, 0xFF, 0xFF, 0x80, 0xFF, 0x20, 0xFF, 0.0f, 2.0f, 1.0f, 0.95f, 0.95f, 0.95f, 0.0f, 0x00000000, -0.2f, -16.0f, 0.06f };

static EFFECTPARAM shoeHitGlowEffParam
    = { 0, 0x20, 0xE0, 0x20, 0xFF, 0x20, 0xE0, 0x20, 0xFF, 0.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0x00000000, -0.2f, -12.0f, 0.05f };

/**
 * @brief Emits a shoe-hit burst beyond a character's right shoe attachment.
 * @param charNo Character index supplying the shoe transform and camera mask.
 */
void CharModelShoeHitCreate(s16 charNo)
{
    Mtx mtx;
    HuVecF pos;
    HuVecF radius;
    CHARWORK *workP = &charWork[charNo];
    HU3DMODEL *modelP = &Hu3DData[workP->modelId];

    Hu3DModelObjMtxGet(workP->modelId, "test11_tex_we-ske_R_shoe1", mtx);
    pos.x = mtx[0][3];
    pos.y = mtx[1][3];
    pos.z = mtx[2][3];
    mtxRot(mtx, modelP->rot.x, modelP->rot.y, modelP->rot.z);
    radius.x = mtx[0][2];
    radius.y = mtx[1][2];
    radius.z = mtx[2][2];
    VECScale(&radius, &radius, 30.0f);
    VECAdd(&pos, &radius, &pos);
    CharEffectShoeHitCreate(modelP->cameraBit, &pos, &modelP->rot);
}

/**
 * @brief Emits paired blue star and green glow particles in a hit ring.
 * @param cameraBit Camera visibility mask copied to every particle.
 * @param pos World-space center of the burst.
 * @param rot Euler rotation in degrees defining the ring's facing direction.
 */
void CharEffectShoeHitCreate(s16 cameraBit, HuVecF *pos, HuVecF *rot)
{
    Mtx mtx;
    HuVecF radius;
    HuVecF dir;
    HuVecF vel;
    s16 effectNo;
    s16 i;
    
    mtxRot(mtx, rot->x, rot->y, rot->z);
    radius.x = mtx[0][2];
    radius.y = mtx[1][2];
    radius.z = mtx[2][2];
    for (i = 0; i < 8; i++) {
        float angle = i * 45;
        dir.x = radius.x * radius.y * (1.0 - cosd(angle)) - radius.z * sind(angle);
        dir.y = radius.y * radius.y + (1.0f - radius.y * radius.y) * cosd(angle);
        dir.z = radius.y * radius.z * (1.0 - cosd(angle)) + radius.x * sind(angle);
        VECNormalize(&dir, &dir);
        VECScale(&dir, &shoeHitEffParam.vel, 10.0f);
        effectNo = EffectCreate(effectMdl[4], cameraBit, pos->x, pos->y, pos->z, 20.0f, &shoeHitEffParam);
        if (effectNo == -1) {
            break;
        }
        VECScale(&radius, &vel, -2.0 - 0.1 * frandmod(20));
        VECScale(&dir, &dir, 2.0f);
        VECAdd(&dir, &vel, &shoeHitGlowEffParam.vel);
        effectNo = EffectCreate(effectMdl[6], cameraBit, pos->x, pos->y, pos->z, 20.0f, &shoeHitGlowEffParam);
        if (effectNo == -1) {
            break;
        }
    }
}

/**
 * @brief Assigns all shared character effect models to a render layer.
 * @param layerNo Render layer from 0 through HU3D_LAYER_MAX - 1 forwarded to
 * CharEffectLayerSet for all live shared character-effect models.
 */
void CharModelLayerSetAll2(s16 layerNo)
{
    CharEffectLayerSet(layerNo);
}

/**
 * @brief Enables or suppresses voice cues for one loaded character motion.
 * @param charNo Character index owning the motion.
 * @param motion Motion ID whose per-slot voice-disable bit is changed.
 * @param voiceOn TRUE to permit voice playback; FALSE to suppress it.
 */
void CharMotionVoiceOnSet(s16 charNo, s16 motion, BOOL voiceOn)
{
    CHARWORK *workP = &charWork[charNo];
    s16 i;

    if (workP->modelId == HU3D_MODELID_NONE) {
        return;
    }
    for (i = 0; i < ARRAY_COUNT(workP->motId); i++) {
        if (workP->motId[i] == motion) {
            break;
        }
    }
    if (i != ARRAY_COUNT(workP->motId)) {
        if (!voiceOn) {
            workP->voiceFlag[i] |= 1;
        }
        else {
            workP->voiceFlag[i] &= ~1;
        }
    }
}

/**
 * @brief Selects positional or non-positional character sound playback.
 * @param charNo Character-table index from 0 through CHARNO_MAX - 1 whose
 * `charWork.attr` selects positional or non-positional audio playback.
 * @param voicePanAuto TRUE to pan sounds from the model position; FALSE to use
 * ordinary character sound playback.
 */
void CharModelVoicePanAutoSet(s16 charNo, BOOL voicePanAuto)
{
    CHARWORK *workP = &charWork[charNo];
    if (voicePanAuto) {
        workP->attr |= 8;
    }
    else {
        workP->attr &= ~8;
    }
}

/**
 * @brief Enables or suppresses frame-driven character effects.
 * @param charNo Character index, or an NPC dust-flag index used by callers.
 * @param fxFlag TRUE to enable effects; FALSE to set the suppression bit.
 */
void CharModelFxFlagSet(s16 charNo, BOOL fxFlag)
{
    CHARWORK *workP = &charWork[charNo];

    if (charNo >= CHARNO_MAX ) {
        if (!fxFlag) {
            dustFlags[charNo] |= 0x10;
        }
        else {
            dustFlags[charNo] &= ~0x10;
        }
        return;
    }
    if (!fxFlag) {
        workP->attr |= 0x10;
    }
    else {
        workP->attr &= ~0x10;
    }
}

typedef struct NpcDustWork_s {
    HU3DMODELID modelId;
    HU3DMOTID motId;
    s16 type;
    s16 npcNo;
} NPCDUSTWORK;

/**
 * @brief Starts a child process that emits motion-timed NPC dust and sounds.
 * @param modelId NPC model to monitor and use as the particle origin.
 * @param motId Motion ID during which updates are allowed.
 * @param type Dust cadence and sound-table selector in the range 0 through 5.
 * @param npcNo NPC identifier, or CHAR_NPC_NONE to omit NPC-specific sounds.
 * @return No defined value in the current implementation.
 */
s32 CharNpcDustSet(HU3DMODELID modelId, HU3DMOTID motId, s16 type, s16 npcNo)
{
    HUPROCESS *parent = HuPrcCurrentGet();
    HUPROCESS *process = HuPrcChildCreate(UpdateNpcDust, 0x64, 0x2000, 0, parent);
    
    if (process) {
        NPCDUSTWORK *work = HuMemDirectMallocNum(HEAP_HEAP, sizeof(NPCDUSTWORK), HU_MEMNUM_OVL);
        process->user_data = work;
        work->modelId = modelId;
        work->motId = motId;
        work->type = type;
        work->npcNo = npcNo;
        EffectInit();
    }
}

/**
 * @brief Starts an NPC dust process with NPC-specific voice cues disabled.
 * @param modelId NPC model to monitor and use as the particle origin.
 * @param motId Motion ID during which updates are allowed.
 * @param type Dust cadence selector in the range 0 through 5.
 * @return Indeterminate value preserved from the original implementation.
 */
s32 CharNpcDustVoiceOffSet(HU3DMODELID modelId, HU3DMOTID motId, s16 type)
{
    s32 undefined_result; // ! - uninitialized

    CharNpcDustSet(modelId, motId, type, CHAR_NPC_NONE);
    return undefined_result;
}

static s8 npcSeTimeTbl[] = { 0x07, 0x20, 0x10, 0x28, 0x01, 0x1E, 0xE7, 0xE7, 0x01, 0x1E, 0x01, 0x1A, 0x01, 0x14, 0x13, 0x20, 0x02, 0x13 };

static u16 lbl_80131158[] = {
    0x0051,
    0x0053,
    0x0057,
    0x0101,
    0x0059,
    0x005B,
    0x0055,
    0x0069,
    0x006A,
};

// Unused?
static s8 lbl_8013116A[] = { 0x01, 0x20, 0x01, 0x15, 0x0A, 0x1E, 0xE7, 0xE7, 0x01, 0x1E, 0x01, 0x10, 0x01, 0x11, 0xE7, 0xE7, 0xE7, 0xE7 };

static u16 lbl_8013117C[] = { 0x0052, 0x0054, 0x0058, 0x0101, 0x005A, 0x005C, 0x0056, 0x0069, 0x006A };

static s8 lbl_8013118E[] = { 0x03, 0x37, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0x01, 0x1A, 0xE7, 0xE7 };

static u16 lbl_8013119C[] = { 0x0051, 0x0053, 0x0057, 0x0101, 0x0059, 0x0068, 0x0055 };

static s8 lbl_801311AA[] = { 0x04, 0x11, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0x01, 0x10, 0xE7, 0xE7 };

static u16 lbl_801311B8[] = { 0x0052, 0x0054, 0x0058, 0x0101, 0x005A, 0x0067, 0x0056 };

static s8 lbl_801311C6[] = { 0x01, 0x17, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7 };

static u16 lbl_801311D4[] = { 0x0051, 0x0053, 0x0057, 0x0101, 0x0059, 0x005B, 0x0055 };

/**
 * @brief Emits NPC particles and sounds at configured active-motion frames.
 *
 * @details Runs continuously as the process created by CharNpcDustSet. Its
 * user_data identifies the watched model, motion, effect type, and NPC table.
 */
static void UpdateNpcDust(void)
{
    HuVecF pos;
    HU3DMODEL *modelP;
    NPCDUSTWORK *work;
    s16 modelId;
    s16 time;
    s16 npcNo;
    s16 i;

    work = HuPrcCurrentGet()->user_data;
    modelId = work->modelId;
    time = 0;
    modelP = &Hu3DData[work->modelId];
    npcNo = work->npcNo - 8;
    while (1) {
        HuPrcVSleep();
        if (work->motId != Hu3DMotionIDGet(modelId)) {
            continue;
        }
        if (modelP->attr & 1) {
            continue;
        }
        time = Hu3DMotionTimeGet(modelId);
        switch (work->type) {
            case 0:
                if (!(time & 0xF) && !(dustFlags[npcNo] & 0x10)) {
                    dustEffParam.vel.x = 2.0 * -sind(modelP->rot.y);
                    dustEffParam.vel.y = 1.0 + 0.1 * frandmod(10);
                    dustEffParam.vel.z = 2.0 * -cosd(modelP->rot.y);
                    pos.x = modelP->pos.x + (frandmod(50) - 25);
                    pos.y = modelP->pos.y;
                    pos.z = modelP->pos.z + (frandmod(50) - 25);
                    EffectDustCreate(modelId, pos.x, pos.y, pos.z, frandmod(10) + 30, &dustEffParam);
                }
                if (work->npcNo != CHAR_NPC_NONE) {
                    for (i = 0; i < 2; i++) {
                        if (npcSeTimeTbl[npcNo * 2 + i] == time * 2) {
                            HuAudFXPlay(lbl_80131158[npcNo]);
                            break;
                        }
                    }
                }
                break;
            case 1:
                if (!(time & 3) && !(dustFlags[npcNo] & 0x10)) {
                    dustEffParam.vel.x = 4.0 * -sind(modelP->rot.y);
                    dustEffParam.vel.y = 2.0 + 0.1 * frandmod(10);
                    dustEffParam.vel.z = 4.0 * -cosd(modelP->rot.y);
                    pos.x = modelP->pos.x + (frandmod(50) - 25);
                    pos.y = modelP->pos.y;
                    pos.z = modelP->pos.z + (frandmod(50) - 25);
                    EffectDustCreate(modelId, pos.x, pos.y, pos.z, frandmod(10) + 30, &dustEffParam);
                }
                if (work->npcNo != CHAR_NPC_NONE) {
                    for (i = 0; i < 2; i++) {
                        if (npcSeTimeTbl[npcNo * 2 + i] == time * 2) {
                            HuAudFXPlay(lbl_8013117C[npcNo]);
                            break;
                        }
                    }
                }
                break;
            case 2:
                if (npcNo != CHAR_NPC_NONE) {
                    if (!(time & 0x1F) && !(dustFlags[npcNo] & 0x10)) {
                        dustEffParam.vel.x = 2.0 * -sind(modelP->rot.y);
                        dustEffParam.vel.y = 1.0 + 0.1 * frandmod(10);
                        dustEffParam.vel.z = 2.0 * -cosd(modelP->rot.y);
                        pos.x = modelP->pos.x + (frandmod(50) - 25);
                        pos.y = modelP->pos.y;
                        pos.z = modelP->pos.z + (frandmod(50) - 25);
                        EffectDustCreate(modelId, pos.x, pos.y, pos.z, frandmod(10) + 30, &dustEffParam);
                    }
                    for (i = 0; i < 2; i++) {
                        if (lbl_8013118E[npcNo * 2 + i] == time * 2) {
                            HuAudFXPlay(lbl_8013119C[npcNo]);
                            break;
                        }
                    }
                }
                break;
            case 3:
                if (npcNo != CHAR_NPC_NONE) {
                    if (!(time & 3) && !(dustFlags[npcNo] & 0x10)) {
                        dustEffParam.vel.x = 2.0 * -sind(modelP->rot.y);
                        dustEffParam.vel.y = 1.0 + 0.1 * frandmod(10);
                        dustEffParam.vel.z = 2.0 * -cosd(modelP->rot.y);
                        pos.x = modelP->pos.x + (frandmod(50) - 25);
                        pos.y = modelP->pos.y;
                        pos.z = modelP->pos.z + (frandmod(50) - 25);
                        EffectDustCreate(modelId, pos.x, pos.y, pos.z, frandmod(10) + 30, &dustEffParam);
                    }
                    for (i = 0; i < 2; i++) {
                        if (lbl_801311AA[npcNo * 2 + i] == time * 2) {
                            HuAudFXPlay(lbl_801311B8[npcNo]);
                            break;
                        }
                    }
                }
                break;
            case 4:
                if (npcNo != CHAR_NPC_NONE) {
                    if (!(time & 7) && !(dustFlags[npcNo] & 0x10)) {
                        dustEffParam.vel.x = 2.0 * -sind(modelP->rot.y);
                        dustEffParam.vel.y = 1.0 + 0.1 * frandmod(10);
                        dustEffParam.vel.z = 2.0 * -cosd(modelP->rot.y);
                        pos.x = modelP->pos.x + (frandmod(50) - 25);
                        pos.y = modelP->pos.y;
                        pos.z = modelP->pos.z + (frandmod(50) - 25);
                        EffectDustCreate(modelId, pos.x, pos.y, pos.z, frandmod(10) + 30, &dustEffParam);
                    }
                    for (i = 0; i < 2; i++) {
                        if (lbl_801311C6[npcNo * 2 + i] == time * 2) {
                            HuAudFXPlay(lbl_801311D4[npcNo]);
                            break;
                        }
                    }
                }
                break;
            case 5:
                if (time != 0) {
                    break;
                }
                if (dustFlags[npcNo] & 0x10) {
                    break;
                }
                for (i = 0; i < CHAR_EFFECT_AND_PARTICLE_MAX; i++) {
                    effectStarParam.vel.x = 10.0 * sind(45.0f * i) * modelP->scale.x;
                    effectStarParam.vel.y = 0.0f;
                    effectStarParam.vel.z = 10.0 * cosd(45.0f * i) * modelP->scale.x;
                    EffectStarCreate(
                        modelId, modelP->pos.x, modelP->pos.y + 10.0f * modelP->scale.x, modelP->pos.z, 40.0f, &effectStarParam);
                }
                for (i = 0; i < CHAR_EFFECT_AND_PARTICLE_MAX; i++) {
                    dustEffParam.vel.x = 4.0 * sind(45.0f * i + 22.5) * modelP->scale.x;
                    dustEffParam.vel.y = 0.0f;
                    dustEffParam.vel.z = 4.0 * cosd(45.0f * i + 22.5) * modelP->scale.x;
                    EffectDustCreate(
                        modelId, modelP->pos.x, modelP->pos.y + 10.0f * modelP->scale.x, modelP->pos.z, 20.0f, &dustEffParam);
                }
                if (npcNo != CHAR_NPC_NONE) {
                    HuAudFXPlay(0x61);
                }
                break;
        }
    }
}

/**
 * @brief Selects the footstep sound offset for a character.
 * @param charNo Character-table index from 0 through CHARNO_MAX - 1 whose
 * `charWork.stepFx` stores the footstep cue variant.
 * @param stepFx Footstep variant; values 4 and 5 map to dedicated sound IDs.
 */
void CharModelStepFxSet(s16 charNo, s32 stepFx)
{
    CHARWORK *workP = &charWork[charNo];

    workP->stepFx = stepFx;
}

/**
 * @brief Applies the character's footstep variant and plays the resulting cue.
 * @param charNo Character-table index from 0 through CHARNO_MAX - 1 selecting
 * the stored footstep variant, model position, and positional-audio attribute.
 * @param seId Character-relative base footstep cue. The stored `stepFx` value is
 * added to it except that 4 and 5 replace it with 0x109 and 0x10B respectively;
 * CharFXPlay or CharFXPlayPos then adds the `charNo << 6` bank offset.
 * @param voiceFlag Bit 0 suppresses playback when set.
 * @return Audio handle when playback occurs; no defined value when suppressed.
 */
static s32 PlayStepVoice(s16 charNo, s16 seId, u8 voiceFlag)
{
    CHARWORK *workP = &charWork[charNo];
    HU3DMODEL *modelP = &Hu3DData[workP->modelId];
    if (voiceFlag & 1) {
        return;
    }
    if (workP->stepFx == 4) {
        seId = 0x109;
    }
    else if (workP->stepFx == 5) {
        seId = 0x10B;
    }
    else {
        seId += workP->stepFx;
    }
    if (workP->attr & 8) {
        return CharFXPlayPos(charNo, seId, &modelP->pos);
    }
    else {
        return CharFXPlay(charNo, seId);
    }
}
