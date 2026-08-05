#include "game/board/model.h"
#include "game/board/main.h"
#include "game/chrman.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/object.h"

#include <string.h>

#define BOARD_MODEL_MAX 256
#define BOARD_MOT_MAX 32

typedef struct board_model {
    struct {
        u8 visible : 1;
        u8 visible_old : 1;
        u8 field00_bit2 : 1;
        u8 field00_bit3 : 1;
        u8 field00_bit4 : 1;
    };
    s8 mot_count;
    s8 character;
    u8 alpha;
    u8 layer;
    u16 camera;
    s16 index;
    s16 id;
    s16 mot_id[BOARD_MOT_MAX];
    s16 curr_mot;
    HSFDATA *data;
    HSFDATA *mot_data[BOARD_MOT_MAX];
    float unidentified_value_D4;
    s32 data_num;
    float mot_start;
    float mot_end;
    Vec pos;
    Vec rot;
    Vec scale;
} BoardModel;

static s16 modelDataNum;
static OMOBJ *modelMgrObj;
static BoardModel *modelDataList;

static void ModelMgrFunc(OMOBJ *object);

static BoardModel *SearchBoardModel(s16 id);
static void KillBoardModel(BoardModel *model);
static s32 CreateBoardModel(BoardModel *model, s32 data_num, s32 link);
static s32 CreateBoardModelMotion(BoardModel *model, s32 count, s32 *data_num);

void BoardModelDummyUpdate(void);
void BoardModelVisibilityUpdate(void);

typedef struct modelmgr_work {
    u8 kill : 1;
} ModelMgrWork;

/**
 * @brief Allocate the board-model table and create its per-frame manager object.
 */
void BoardModelInit(void)
{
    ModelMgrWork *work;
    s32 i;
    modelDataList = HuMemDirectMallocNum(HEAP_HEAP, BOARD_MODEL_MAX * sizeof(BoardModel), HU_MEMNUM_OVL);
    reflectMapNo = 0;
    modelDataNum = 0;
    memset(modelDataList, 0, BOARD_MODEL_MAX * sizeof(BoardModel));
    for (i = 0; i < BOARD_MODEL_MAX; i++) {
        modelDataList[i].index = -1;
        modelDataList[i].visible = 0;
    }
    modelMgrObj = omAddObjEx(boardObjMan, 32258, 0, 0, -1, ModelMgrFunc);
    work = omObjGetWork(modelMgrObj, ModelMgrWork);
    work->kill = 0;
    omSetStatBit(modelMgrObj, OM_STAT_NOPAUSE | 0x80);
}

/**
 * @brief Destroy every board model, free the model table, and stop its manager.
 */
void BoardModelKillAll(void)
{
    if (modelMgrObj) {
        ModelMgrWork *work = omObjGetWork(modelMgrObj, ModelMgrWork);
        work->kill = 1;
    }
    if (modelDataList) {
        BoardModel *model;
        for (model = modelDataList; model < &modelDataList[BOARD_MODEL_MAX]; model++) {
            KillBoardModel(model);
        }
        if (modelDataList) {
            HuMemDirectFree(modelDataList);
            modelDataList = NULL;
        }
    }
}

/**
 * @brief Synchronize model visibility each frame and remove the manager during board shutdown.
 * @param [in] object Model-manager object deleted after its kill flag or board shutdown is observed.
 */
static void ModelMgrFunc(OMOBJ *object)
{
    ModelMgrWork *work = omObjGetWork(modelMgrObj, ModelMgrWork);
    if (work->kill || BoardIsKill()) {
        modelMgrObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }
    BoardModelDummyUpdate();
    BoardModelVisibilityUpdate();
}

/**
 * @brief Provide the intentionally empty board-model update hook.
 */
void BoardModelDummyUpdate(void) { }

/**
 * @brief Synchronize each wrapper's requested visibility with the engine model display attribute.
 */
void BoardModelVisibilityUpdate(void)
{
    s32 i;
    BoardModel *model;
    u32 temp[2];
    Vec pos, target;
    temp[1] = temp[0] = 0;
    BoardCameraPosGet(&pos);
    BoardCameraTargetGet(&target);
    for (i = 0, model = modelDataList; i < BOARD_MODEL_MAX; i++, model++) {
        u32 attr;
        if (model->index == -1) {
            continue;
        }
        attr = Hu3DModelAttrGet(model->id);
        if (!model->visible) {
            if (!(attr & 0x1)) {
                Hu3DModelAttrSet(model->id, HU3D_ATTR_DISPOFF);
            }
        }
        else {
            if (attr & 0x1) {
                Hu3DModelAttrReset(model->id, HU3D_ATTR_DISPOFF);
            }
        }
    }
}

/**
 * @brief Temporarily hide all board models or restore their saved visibility states.
 * @param [in] flag Nonzero saves each live model's visibility and hides it; zero restores the saved state.
 */
void BoardModelHideSetAll(s32 flag)
{
    BoardModel *model;
    if (!modelDataList) {
        return;
    }

    for (model = modelDataList; model < &modelDataList[BOARD_MODEL_MAX]; model++) {
        if (model->index == -1) {
            continue;
        }
        if (flag) {
            model->visible_old = model->visible;
            model->visible = 0;
        }
        else {
            model->visible = model->visible_old;
        }
    }
}

/**
 * @brief Set the ambient RGB multipliers of every live board model.
 * @param [in] r Red ambient-light multiplier.
 * @param [in] g Green ambient-light multiplier.
 * @param [in] b Blue ambient-light multiplier.
 */
void BoardModelAmbSetAll(float r, float g, float b)
{
    BoardModel *model;
    for (model = modelDataList; model < &modelDataList[BOARD_MODEL_MAX]; model++) {
        if (model->index == -1) {
            continue;
        }
        BoardModelAmbSet(model->index, r, g, b);
    }
}

/**
 * @brief Assign every non-character board model to an engine camera mask.
 * @param [in] camera Engine camera visibility mask applied to non-character models.
 */
void BoardModelCameraSetAll(u16 camera)
{
    BoardModel *model;
    for (model = modelDataList; model < &modelDataList[BOARD_MODEL_MAX]; model++) {
        if (model->index == -1) {
            continue;
        }
        if (model->character != -1) {
            continue;
        }
        Hu3DModelCameraSet(model->id, camera);
        model->camera = camera;
    }
}

/**
 * @brief Allocate a wrapper, create its model and motions, and initialize transform and display state.
 * @param [in] character Character index.
 * @param [in] data_num Packed model-data ID used to create or link the engine model.
 * @param [in] mot_list Array of motion data identifiers terminated by -1; NULL creates only the base motion.
 * @param [in] link Nonzero to reuse an already loaded model as an engine link.
 * @param [in] unused Unused; retained to match the original interface.
 * @return Board-model handle, or -1 if allocation, model creation, or motion loading fails.
 */
static s16 CreateInstance(s32 character, s32 data_num, s32 *mot_list, s32 link, s32 unused)
{
    BoardModel *model;
    s32 i;
    model = NULL;
    for (i = 0; i < BOARD_MODEL_MAX; i++) {
        if (modelDataList[i].index == -1) {
            model = &modelDataList[i];
            break;
        }
    }
    if (!model) {
        return -1;
    }
    memset(model, 0, sizeof(BoardModel));
    model->index = i + 1;
    model->character = character;
    modelDataNum++;
    if (CreateBoardModel(model, data_num, link) == 0) {
        s32 count;
        count = 0;
        if (mot_list) {
            for (count = 0; mot_list[count] >= 0; count++)
                ;
            if (count >= BOARD_MOT_MAX) {
                count = BOARD_MOT_MAX;
            }
        }
        for (i = 0; i < BOARD_MOT_MAX; i++) {
            model->mot_id[i] = -1;
        }
        if (CreateBoardModelMotion(model, count, mot_list) == 0) {
            model->visible = 1;
            model->visible_old = model->visible;
            model->field00_bit2 = 0;
            model->field00_bit4 = 0;
            model->unidentified_value_D4 = 100.0f;
            model->scale.x = model->scale.y = model->scale.z = 1.0f;
            model->curr_mot = 0;
            Hu3DModelCameraSet(model->id, 1);
            Hu3DModelLayerSet(model->id, 0);
            model->field00_bit3 = 0;
            model->alpha = 255;
            BoardModelPosSet(model->index, 0.0f, 0.0f, 0.0f);
            BoardModelRotSet(model->index, 0.0f, 0.0f, 0.0f);
            BoardModelScaleSet(model->index, 1.0f, 1.0f, 1.0f);
            BoardModelAmbSet(model->index, 1.0f, 1.0f, 1.0f);
            return model->index;
        }
    }
    BoardModelKill(model->index);
    return -1;
}

/**
 * @brief Create a board model from a data resource and optional motion list.
 * @param [in] data_num Packed model-data ID used to create or link the engine model.
 * @param [in] mot_list Array of motion data identifiers terminated by -1; NULL creates only the base motion.
 * @param [in] link Nonzero to reuse an already loaded model as an engine link.
 * @return Board-model handle, or -1 if allocation, model creation, or motion loading fails.
 */
s16 BoardModelCreate(s32 data_num, s32 *mot_list, s32 link)
{
    return CreateInstance(-1, data_num, mot_list, link, 0);
}

/**
 * @brief Create a character-backed board model with optional motions.
 * @param [in] character Character index.
 * @param [in] data_num Packed character model-data ID passed through model creation.
 * @param [in] mot_list Array of motion data identifiers terminated by -1; NULL creates only the base motion.
 * @param [in] link Nonzero to reuse an already loaded model as an engine link.
 * @return Board-model handle, or -1 if allocation, model creation, or motion loading fails.
 */
s16 BoardModelCreateCharacter(s32 character, s32 data_num, s32 *mot_list, s32 link)
{
    return CreateInstance(character, data_num, mot_list, link, 0);
}

/**
 * @brief Create and position a board model from a BoardModelParam record.
 * @param [in] param Model creation record.
 * @param [in] pos Initial world position, or NULL to retain the origin.
 * @param [in] rot Euler rotation in degrees.
 * @return Board-model handle, or -1 if allocation, model creation, or motion loading fails.
 */
s16 BoardModelCreateParam(BoardModelParam *param, Vec *pos, Vec *rot)
{
    float unk_param;
    s16 model = BoardModelCreate(param->data_num, NULL, param->unk4.link);
    if (model == -1) {
        return -1;
    }
    BoardModelMotionStart(model, 0, (param->unk4.pause) ? 0x40000001 : 0);
    if (!param->unk4.start_mot) {
        BoardModelMotionSpeedSet(model, 0.0f);
    }
    BoardModelExistDupe(model, param->unk4.field04_bit0);
    BoardModelVisibilitySet(model, param->unk4.visible);
    if (pos) {
        BoardModelPosSetV(model, pos);
    }
    if (rot) {
        BoardModelRotSetV(model, rot);
    }
    OSs16tof32(&param->unk6, &unk_param);
    fn_8006DDE8(model, unk_param);
    return model;
}

/**
 * @brief Destroy a board model and release its slot.
 * @param [in] model Board-model handle.
 */
void BoardModelKill(s16 model)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (model_ptr) {
        KillBoardModel(model_ptr);
    }
}

/**
 * @brief Translate a board-model handle to its engine model ID.
 * @param [in] model Board-model handle.
 * @return Engine model ID, or -1 for an invalid board-model handle.
 */
s16 BoardModelIDGet(s16 model)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        return model_ptr->id;
    }
}

/**
 * @brief Store a value in an unidentified one-bit board-model wrapper field.
 * @param [in] model Board-model handle.
 * @param [in] value Value assigned to the one-bit field; its higher-level meaning is not established.
 * @return 0 after storing the field, or -1 when model is invalid.
 */
s32 fn_8006DC1C(s16 model, s32 value)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        model_ptr->field00_bit3 = value;
        return 0;
    }
}

/**
 * @brief Enable or disable Z-buffer writes for a board model.
 * @param [in] model Board-model handle.
 * @param [in] pass Nonzero sets HU3D_ATTR_ZWRITE_OFF; zero clears it.
 * @return 0 after toggling the Z-write-disable attribute, or -1 when model is not a live board-model handle.
 */
s32 BoardModelPassSet(s16 model, s32 pass)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        if (pass != 0) {
            Hu3DModelAttrSet(model_ptr->id, HU3D_ATTR_ZWRITE_OFF);
        }
        else {
            Hu3DModelAttrReset(model_ptr->id, HU3D_ATTR_ZWRITE_OFF);
        }
        return 0;
    }
}

/**
 * @brief Set one board model's ambient RGB multipliers.
 * @param [in] model Board-model handle.
 * @param [in] r Red ambient-light multiplier.
 * @param [in] g Green ambient-light multiplier.
 * @param [in] b Blue ambient-light multiplier.
 * @return 0 after applying the RGB multipliers, or -1 when model is not a live board-model handle.
 */
s32 BoardModelAmbSet(s16 model, float r, float g, float b)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        Hu3DModelAmbSet(model_ptr->id, r, g, b);
        return 0;
    }
}

/**
 * @brief Check whether a board-model handle resolves to a live wrapper; the second argument is unused.
 * @param [in] model Board-model handle.
 * @param [in] unused Unused; retained to match the original interface.
 * @return 0 when model resolves to a live wrapper, or -1 when it does not.
 */
s32 BoardModelExistCheck(s16 model, s32 unused)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        return 0;
    }
}

/**
 * @brief Check whether a board-model handle resolves to a live wrapper; the second argument is unused.
 * @param [in] model Board-model handle.
 * @param [in] unused Unused; retained to match the original interface.
 * @return 0 when model resolves to a live wrapper, or -1 when it does not.
 */
s32 BoardModelExistDupe(s16 model, s32 unused)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        return 0;
    }
}

/**
 * @brief Store a value in the unidentified offset-D4 field, replacing negative input with -1.0f.
 * @param [in] model Board-model handle.
 * @param [in] value Value stored in the unidentified offset-D4 field; negative inputs are replaced with -1.0f.
 * @return 0 after storing the wrapper field, or -1 for an invalid board-model handle.
 */
s32 fn_8006DDE8(s16 model, float value)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        if (value < 0) {
            value = -1;
        }
        model_ptr->unidentified_value_D4 = value;
        return 0;
    }
}

/**
 * @brief Load one additional motion and return its wrapper slot.
 * @param [in] model Board-model handle.
 * @param [in] data_num Packed motion-data ID loaded and attached to the model.
 * @return Motion slot index from 1 through 31, or -1 for an invalid model, a full motion table, or motion-data load failure.
 */
s32 BoardModelMotionCreate(s16 model, s32 data_num)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    s32 i;
    if (!model_ptr) {
        return -1;
    }
    else {
        s16 id;
        if (model_ptr->mot_count >= BOARD_MOT_MAX) {
            return -1;
        }
        for (i = 1; i < BOARD_MOT_MAX; i++) {
            if (model_ptr->mot_id[i] == -1) {
                break;
            }
        }
        if (model_ptr->character == -1) {
            model_ptr->mot_data[i] = HuDataSelHeapReadNum(data_num, HU_MEMNUM_OVL, HEAP_MODEL);
            if (!model_ptr->mot_data[i]) {
                return -1;
            }
            model = Hu3DJointMotion(model_ptr->id, model_ptr->mot_data[i]);
        }
        else {
            model = CharMotionCreate(model_ptr->character, data_num);
            CharMotionDataClose(model_ptr->character);
        }
        model_ptr->mot_id[i] = model;
        model_ptr->mot_count++;
        return i;
    }
}

/**
 * @brief Detach and release one motion slot.
 * @param [in] model Board-model handle.
 * @param [in] motion Wrapper motion slot.
 * @return 0 after clearing the slot, or -1 when model is invalid or motion is outside slots 1 through 31.
 */
s32 BoardModelMotionKill(s16 model, s32 motion)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        if (motion >= BOARD_MOT_MAX || motion <= 0) {
            return -1;
        }
        if (motion == model_ptr->curr_mot && model_ptr->curr_mot != 0) {
            BoardModelMotionStart(model, 0, 0);
        }
        if (model_ptr->mot_id[motion] > 0) {
            if (model_ptr->character == -1) {
                Hu3DMotionKill(model_ptr->mot_id[motion]);
            }
            else {
                CharMotionKill(model_ptr->character, model_ptr->mot_id[motion]);
            }
            model_ptr->mot_id[motion] = -1;
        }
        model_ptr->mot_data[motion] = NULL;
        model_ptr->mot_count--;
        return 0;
    }
}

/**
 * @brief Set the playback start and end times for the current motion.
 * @param [in] model Board-model handle.
 * @param [in] start Starting motion frame; a negative value leaves the stored start unchanged.
 * @param [in] end Ending motion frame; a negative value leaves the stored end unchanged.
 * @return 0 after applying the stored playback interval, or -1 when model is invalid.
 */
s32 BoardModelMotionStartEndSet(s16 model, s16 start, s16 end)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        if (start >= 0) {
            OSs16tof32(&start, &model_ptr->mot_start);
        }
        if (end >= 0) {
            OSs16tof32(&end, &model_ptr->mot_end);
        }
        Hu3DMotionStartEndSet(model_ptr->id, model_ptr->mot_start, model_ptr->mot_end);
        return 0;
    }
}

/**
 * @brief Enable or disable looping for the current motion.
 * @param [in] model Board-model handle.
 * @param [in] flag Nonzero sets HU3D_MOTATTR_LOOP; zero clears it.
 * @return 0 after updating the loop attribute, or -1 when model is invalid.
 */
s32 BoardModelMotionUpdateSet(s16 model, s32 flag)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        if (flag) {
            Hu3DModelAttrSet(model_ptr->id, HU3D_MOTATTR_LOOP);
        }
        else {
            Hu3DModelAttrReset(model_ptr->id, HU3D_MOTATTR_LOOP);
        }
        return 0;
    }
}

/**
 * @brief Enable or disable character voice triggers for a motion.
 * @param [in] model Board-model handle.
 * @param [in] motion Wrapper motion slot.
 * @param [in] flag Nonzero enables voice triggers; zero disables them.
 * @return -1 when model is invalid; otherwise 0, including for an out-of-range motion or non-character model.
 */
s32 BoardModelVoiceEnableSet(s16 model, s32 motion, s32 flag)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        if (motion >= BOARD_MOT_MAX || motion < 0) {
            return 0;
        }
        if (model_ptr->character == -1) {
            return 0;
        }
        CharMotionVoiceOnSet(model_ptr->character, model_ptr->mot_id[motion], flag);
        return 0;
    }
}

/**
 * @brief Start a wrapper motion with engine motion attributes.
 * @param [in] model Board-model handle.
 * @param [in] motion Wrapper motion slot.
 * @param [in] attr Engine attribute bit mask.
 * @return -1 when model is invalid; otherwise 0, including when motion is outside slots 0 through 31.
 */
s32 BoardModelMotionStart(s16 model, s32 motion, u32 attr)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        if (motion >= BOARD_MOT_MAX || motion < 0) {
            return 0;
        }
        Hu3DMotionSpeedSet(model_ptr->id, 1.0f);
        if (model_ptr->character == -1) {
            Hu3DMotionSet(model_ptr->id, model_ptr->mot_id[motion]);
        }
        else {
            CharMotionSet(model_ptr->character, model_ptr->mot_id[motion]);
        }
        model_ptr->mot_start = 0.0f;
        if (model_ptr->character == -1) {
            model_ptr->mot_end = Hu3DMotionMaxTimeGet(model_ptr->id);
        }
        else {
            model_ptr->mot_end = CharMotionMaxTimeGet(model_ptr->character);
        }
        model_ptr->field00_bit4 = 0;
        if (attr & 0x40000024) {
            if (model_ptr->character == -1) {
                Hu3DMotionTimeSet(model_ptr->id, model_ptr->mot_end);
            }
            else {
                CharMotionTimeSet(model_ptr->character, model_ptr->mot_end);
            }
        }
        Hu3DModelAttrReset(model_ptr->id, HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrSet(model_ptr->id, attr);
        model_ptr->curr_mot = motion;
        return 0;
    }
}

/**
 * @brief Get the engine's current motion-shift ID.
 * @param [in] model Board-model handle.
 * @return Current engine motion-shift ID, or -1 when model is invalid.
 */
s32 BoardModelMotionShiftIDGet(s16 model)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        return Hu3DMotionShiftIDGet(model_ptr->id);
    }
}

/**
 * @brief Blend from the current motion to another wrapper motion.
 * @param [in] model Board-model handle.
 * @param [in] motion Wrapper motion slot.
 * @param [in] time Motion start time in frames.
 * @param [in] shift_time Motion blend duration in frames.
 * @param [in] attr Engine attribute bit mask.
 * @return 0 after starting the blend, or -1 when model is invalid or motion is outside slots 0 through 31.
 */
s32 BoardModelMotionShiftSet(s16 model, s32 motion, float time, float shift_time, u32 attr)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        if (motion >= BOARD_MOT_MAX || motion < 0) {
            return -1;
        }
        model_ptr->mot_start = 0.0f;
        if (model_ptr->character == -1) {
            model_ptr->mot_end = Hu3DMotionMaxTimeGet(model_ptr->id);
        }
        else {
            model_ptr->mot_end = CharMotionMaxTimeGet(model_ptr->character);
        }
        model_ptr->field00_bit4 = 0;
        if (model_ptr->character == -1) {
            Hu3DMotionShiftSet(model_ptr->id, model_ptr->mot_id[motion], time, shift_time, attr);
        }
        else {
            CharMotionShiftSet(model_ptr->character, model_ptr->mot_id[motion], time, shift_time, attr);
        }
        model_ptr->curr_mot = motion;
        return 0;
    }
}

/**
 * @brief Add engine model attribute bits.
 * @param [in] model Board-model handle.
 * @param [in] attr Engine attribute bit mask.
 * @return 0 after setting the attribute bits, or -1 when model is invalid.
 */
s32 BoardModelAttrSet(s16 model, u32 attr)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        Hu3DModelAttrSet(model_ptr->id, attr);
        return 0;
    }
}

/**
 * @brief Remove engine model attribute bits.
 * @param [in] model Board-model handle.
 * @param [in] attr Engine attribute bit mask.
 * @return 0 after clearing the attribute bits, or -1 when model is invalid.
 */
s32 BoardModelAttrReset(s16 model, u32 attr)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        Hu3DModelAttrReset(model_ptr->id, attr);
        return 0;
    }
}

/**
 * @brief Set the current motion playback time in frames.
 * @param [in] model Board-model handle.
 * @param [in] time Motion playback time in frames.
 * @return 0 after setting playback time, or -1 when model is invalid.
 */
s32 BoardModelMotionTimeSet(s16 model, float time)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        if (model_ptr->character == -1) {
            Hu3DMotionTimeSet(model_ptr->id, time);
        }
        else {
            CharMotionTimeSet(model_ptr->character, time);
        }
        return 0;
    }
}

/**
 * @brief Get the current motion playback time in frames.
 * @param [in] model Board-model handle.
 * @return Current motion playback time in frames, or -1.0 for an invalid model.
 */
float BoardModelMotionTimeGet(s16 model)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        float time;
        if (model_ptr->character == -1) {
            time = Hu3DData[model_ptr->id].motWork.time;
        }
        else {
            time = CharMotionTimeGet(model_ptr->character);
        }
        return time;
    }
}

/**
 * @brief Get the current motion's maximum playback time in frames.
 * @param [in] model Board-model handle.
 * @return Current motion's maximum time in frames, or -1.0 when model is invalid.
 */
float BoardModelMotionMaxTimeGet(s16 model)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        float time;
        if (model_ptr->character == -1) {
            time = Hu3DMotionMaxTimeGet(model_ptr->id);
        }
        else {
            time = CharMotionMaxTimeGet(model_ptr->character);
        }
        return time;
    }
}

/**
 * @brief Get the current motion playback multiplier.
 * @param [in] model Board-model handle.
 * @return Current motion playback multiplier, or -1.0 for an invalid model.
 */
float BoardModelMotionSpeedGet(s16 model)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        HU3DMODEL *hsf_model = &Hu3DData[model_ptr->id];
        return hsf_model->motWork.speed;
    }
}

/**
 * @brief Set the current motion playback multiplier.
 * @param [in] model Board-model handle.
 * @param [in] speed Motion playback multiplier; 1.0 is normal speed.
 * @return 0 after setting normal and shift playback speeds, or -1 when model is invalid.
 */
s32 BoardModelMotionSpeedSet(s16 model, float speed)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        HU3DMODEL *hsf_model;
        if (model_ptr->character == -1) {
            Hu3DMotionSpeedSet(model_ptr->id, speed);
        }
        else {
            CharMotionSpeedSet(model_ptr->character, speed);
        }
        hsf_model = &Hu3DData[model_ptr->id];
        hsf_model->motShiftWork.speed = speed;
        return 0;
    }
}

/**
 * @brief Check whether the current motion reached its configured end.
 * @param [in] model Board-model handle.
 * @return 1 when model is invalid; otherwise the engine or character motion-end result.
 */
s32 BoardModelMotionEndCheck(s16 model)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return 1;
    }
    else {
        s32 result;
        if (model_ptr->character == -1) {
            result = Hu3DMotionEndCheck(model_ptr->id);
        }
        else {
            result = CharMotionEndCheck(model_ptr->character);
        }
        return result;
    }
}

/**
 * @brief Attach and start a shape-motion resource on the model.
 * @param [in] model Board-model handle.
 * @param [in] motion Wrapper motion slot.
 * @param [in] attr Engine attribute bit mask.
 * @return 0 after attaching the shape motion, or -1 when model is invalid, character-backed, or motion is at least 32.
 */
s32 BoardModelMotionShapeSet(s16 model, s32 motion, u32 attr)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        HU3DMODEL *hsf_model;
        if (model_ptr->character != -1) {
            return -1;
        }
        if (motion >= BOARD_MOT_MAX) {
            return -1;
        }
        hsf_model = &Hu3DData[model_ptr->id];
        hsf_model->motShapeWork.speed = 1.0f;
        Hu3DMotionShapeSet(model_ptr->id, model_ptr->mot_id[motion]);
        if (attr & 0x40000100) {
            float max_time = Hu3DMotionMaxTimeGet(model_ptr->id);
            hsf_model->motShapeWork.time = max_time;
        }
        Hu3DModelAttrSet(model_ptr->id, attr);
        return 0;
    }
}

/**
 * @brief Get shape-motion playback time in frames.
 * @param [in] model Board-model handle.
 * @return Current shape-motion time in frames, or -1.0 when model is invalid or character-backed.
 */
float BoardModelMotionShapeTimeGet(s16 model)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        HU3DMODEL *hsf_model;
        if (model_ptr->character != -1) {
            return -1;
        }
        hsf_model = &Hu3DData[model_ptr->id];
        return hsf_model->motShapeWork.time;
    }
}

/**
 * @brief Get the attached shape motion's maximum time in frames.
 * @param [in] model Board-model handle.
 * @return Shape motion's maximum time in frames; 0.0 when none is attached, or -1.0 when model is invalid or character-backed.
 */
float BoardModelMotionShapeMaxTimeGet(s16 model)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        HU3DMODEL *hsf_model;
        HU3DMOTION *motion;
        HSFMOTION *motion_hsf;
        if (model_ptr->character != -1) {
            return -1;
        }
        hsf_model = &Hu3DData[model_ptr->id];
        if (hsf_model->motIdShape == -1) {
            return 0;
        }
        motion = &Hu3DMotion[hsf_model->motIdShape];
        motion_hsf = motion->hsf->motion;
        return motion_hsf->maxTime;
    }
}

/**
 * @brief Set the shape-motion playback multiplier.
 * @param [in] model Board-model handle.
 * @param [in] speed Shape-motion playback multiplier; 1.0 is normal speed.
 * @return 0 after setting shape-motion speed, or -1 when model is invalid or character-backed.
 */
s32 BoardModelMotionShapeSpeedSet(s16 model, float speed)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        HU3DMODEL *hsf_model;
        if (model_ptr->character != -1) {
            return -1;
        }
        hsf_model = &Hu3DData[model_ptr->id];
        hsf_model->motShapeWork.speed = speed;
        return 0;
    }
}

/**
 * @brief Get the shape-motion playback multiplier; the second argument is unused.
 * @param [in] model Board-model handle.
 * @param [in] unused_speed Unused; retained to match the original interface.
 * @return Stored shape-motion playback multiplier, or -1.0 when model is invalid.
 */
float BoardModelMotionShapeSpeedGet(s16 model, float unused_speed)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        HU3DMODEL *hsf_model;
        hsf_model = &Hu3DData[model_ptr->id];
        return hsf_model->motShapeWork.speed;
    }
}

/**
 * @brief Check whether shape-motion playback reached an endpoint.
 * @param [in] model Board-model handle.
 * @return 1 when model is invalid or character-backed; otherwise 1 at the direction-dependent endpoint and 0 before it.
 */
s32 BoardModelMotionShapeEndCheck(s16 model)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return 1;
    }
    else {
        HU3DMODEL *hsf_model;
        if (model_ptr->character != -1) {
            return 1;
        }
        hsf_model = &Hu3DData[model_ptr->id];
        if (hsf_model->attr & 0x40000100) {
            return hsf_model->motShapeWork.time <= 0.0f;
        }
        return BoardModelMotionShapeMaxTimeGet(model) <= hsf_model->motShapeWork.time;
    }
}

/**
 * @brief Set the shape-motion playback start and end times.
 * @param [in] model Board-model handle.
 * @param [in] start Starting motion frame; a negative value leaves the stored start unchanged.
 * @param [in] end Ending shape-motion frame; a negative value leaves the stored end unchanged.
 * @return 0 after applying the shape-motion interval, or -1 when model is invalid or character-backed.
 */
s32 BoardModelMotionShapeStartEndSet(s16 model, s16 start, s16 end)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        if (model_ptr->character != -1) {
            return -1;
        }
        if (start >= 0) {
            OSs16tof32(&start, &model_ptr->mot_start);
        }
        if (end >= 0) {
            OSs16tof32(&end, &model_ptr->mot_end);
        }
        Hu3DMotionShapeStartEndSet(model_ptr->id, model_ptr->mot_start, model_ptr->mot_end);
        return 0;
    }
}

/**
 * @brief Set model translucency from 0 (transparent) to 255 (opaque).
 * @param [in] model Board-model handle.
 * @param [in] alpha Opacity from 0 (transparent) to 255 (opaque).
 * @return 0 after applying alpha, or -1 when model is invalid.
 */
s32 BoardModelAlphaSet(s16 model, u8 alpha)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        float tp_lvl;
        model_ptr->alpha = alpha;
        OSu8tof32(&alpha, &tp_lvl);
        tp_lvl = tp_lvl * (1.0f / 255.0f);
        Hu3DModelTPLvlSet(model_ptr->id, tp_lvl);
        return 0;
    }
}

/**
 * @brief Copy a transform matrix into the engine model.
 * @param [in] model Board-model handle.
 * @param [in] src Caller-owned source matrix.
 * @return 0 after copying src, or -1 when model or src is invalid.
 */
s32 BoardModelMtxSet(s16 model, Mtx *src)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        HU3DMODEL *hsf_model;
        if (!src) {
            return -1;
        }
        hsf_model = &Hu3DData[model_ptr->id];
        MTXCopy(*src, hsf_model->mtx);
        return 0;
    }
}

/**
 * @brief Copy the engine model transform matrix to caller storage.
 * @param [in] model Board-model handle.
 * @param [out] dst Caller-owned matrix receiving the engine model transform.
 * @return 0 after copying the matrix, or -1 when model or dst is invalid.
 */
s32 BoardModelMtxGet(s16 model, Mtx *dst)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        HU3DMODEL *hsf_model;
        if (!dst) {
            return -1;
        }
        hsf_model = &Hu3DData[model_ptr->id];
        MTXCopy(hsf_model->mtx, *dst);
        return 0;
    }
}

/**
 * @brief Set the model's normalized Y rotation in degrees.
 * @param [in] model Board-model handle.
 * @param [in] rot Y-axis Euler angle in degrees; values are normalized before storage.
 * @return BoardModelRotSet's status: 0 for a valid model, or -1 when model is invalid.
 */
s32 BoardModelRotYSet(s16 model, float rot)
{
    Vec rot_orig;
    BoardModelRotGet(model, &rot_orig);
    return BoardModelRotSet(model, rot_orig.x, BoardDAngleCalc(rot), rot_orig.z);
}

/**
 * @brief Get the model's Y rotation in degrees.
 * @param [in] model Board-model handle.
 * @return Stored Y rotation in degrees for a valid model; indeterminate for an invalid handle because the local vector remains uninitialized.
 */
float BoardModelRotYGet(s16 model)
{
    Vec rot;
    BoardModelRotGet(model, &rot);
    return rot.y;
}

/**
 * @brief Set model position from a vector.
 * @param [in] model Board-model handle.
 * @param [in] pos World-position vector copied immediately; ownership remains with the caller.
 * @return BoardModelPosSet's status, or -1 when pos is NULL.
 */
s32 BoardModelPosSetV(s16 model, Vec *pos)
{
    if (!pos) {
        return -1;
    }
    else {
        return BoardModelPosSet(model, pos->x, pos->y, pos->z);
    }
}

/**
 * @brief Set model position in world units.
 * @param [in] model Board-model handle.
 * @param [in] x World-space X coordinate.
 * @param [in] y World-space Y coordinate.
 * @param [in] z World-space Z coordinate.
 * @return 0 after updating the stored and engine positions, or -1 when model is invalid.
 */
s32 BoardModelPosSet(s16 model, float x, float y, float z)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        model_ptr->pos.x = x;
        model_ptr->pos.y = y;
        model_ptr->pos.z = z;
        Hu3DModelPosSetV(model_ptr->id, &model_ptr->pos);
        return 0;
    }
}

/**
 * @brief Set model Euler rotation from a vector.
 * @param [in] model Board-model handle.
 * @param [in] rotation Euler rotation in degrees copied immediately; ownership remains with the caller.
 * @return BoardModelRotSet's status, or -1 when rotation is NULL.
 */
s32 BoardModelRotSetV(s16 model, Vec *rotation)
{
    if (!rotation) {
        return -1;
    }
    else {
        return BoardModelRotSet(model, rotation->x, rotation->y, rotation->z);
    }
}

/**
 * @brief Set normalized model Euler rotation in degrees.
 * @param [in] model Board-model handle.
 * @param [in] x X-axis Euler rotation in degrees.
 * @param [in] y Y-axis Euler rotation in degrees.
 * @param [in] z Z-axis Euler rotation in degrees.
 * @return 0 after updating the stored and engine rotations, or -1 when model is invalid.
 */
s32 BoardModelRotSet(s16 model, float x, float y, float z)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        model_ptr->rot.x = x;
        model_ptr->rot.y = y;
        model_ptr->rot.z = z;
        Hu3DModelRotSetV(model_ptr->id, &model_ptr->rot);
        return 0;
    }
}

/**
 * @brief Set model scale from a vector.
 * @param [in] model Board-model handle.
 * @param [in] scale Per-axis scale factors copied immediately; ownership remains with the caller.
 * @return BoardModelScaleSet's status, or -1 when scale is NULL.
 */
s32 BoardModelScaleSetV(s16 model, Vec *scale)
{
    if (!scale) {
        return -1;
    }
    else {
        return BoardModelScaleSet(model, scale->x, scale->y, scale->z);
    }
}

/**
 * @brief Set model scale factors.
 * @param [in] model Board-model handle.
 * @param [in] x X-axis scale factor.
 * @param [in] y Y-axis scale factor.
 * @param [in] z Z-axis scale factor.
 * @return 0 after updating the stored and engine scales, or -1 when model is invalid.
 */
s32 BoardModelScaleSet(s16 model, float x, float y, float z)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        model_ptr->scale.x = x;
        model_ptr->scale.y = y;
        model_ptr->scale.z = z;
        Hu3DModelScaleSetV(model_ptr->id, &model_ptr->scale);
        return 0;
    }
}

/**
 * @brief Copy model position to caller storage.
 * @param [in] model Board-model handle.
 * @param [out] dst Caller-owned vector receiving the stored world position.
 * @return 0 after copying the position, or -1 when model or dst is invalid.
 */
s32 BoardModelPosGet(s16 model, Vec *dst)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr || !dst) {
        return -1;
    }
    else {
        *dst = model_ptr->pos;
        return 0;
    }
}

/**
 * @brief Copy model Euler rotation to caller storage.
 * @param [in] model Board-model handle.
 * @param [out] dst Caller-owned vector receiving the stored Euler rotation in degrees.
 * @return 0 after copying the rotation, or -1 when model or dst is invalid.
 */
s32 BoardModelRotGet(s16 model, Vec *dst)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr || !dst) {
        return -1;
    }
    else {
        *dst = model_ptr->rot;
        return 0;
    }
}

/**
 * @brief Copy model scale to caller storage.
 * @param [in] model Board-model handle.
 * @param [out] dst Caller-owned vector receiving the stored per-axis scale factors.
 * @return 0 after copying the scale, or -1 when model or dst is invalid.
 */
s32 BoardModelScaleGet(s16 model, Vec *dst)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr || !dst) {
        return -1;
    }
    else {
        *dst = model_ptr->scale;
        return 0;
    }
}

/**
 * @brief Set the wrapper visibility state applied on the next model-manager update.
 * @param [in] model Board-model handle.
 * @param [in] visible Value stored in the wrapper's one-bit visibility field; callers use 1 for visible and 0 for hidden.
 * @return 0 after updating requested visibility, or -1 when model is invalid.
 */
s32 BoardModelVisibilitySet(s16 model, s32 visible)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        model_ptr->visible = visible;
        return 0;
    }
}

/**
 * @brief Get the wrapper's requested visibility state.
 * @param [in] model Board-model handle.
 * @return Stored visibility value, or 0 when model is invalid.
 */
s32 BoardModelVisibilityGet(s16 model)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return 0;
    }
    else {
        return model_ptr->visible;
    }
}

/**
 * @brief Assign a model to an engine camera mask.
 * @param [in] model Board-model handle.
 * @param [in] camera Engine camera visibility mask.
 * @return 0 after storing and applying the camera mask, or -1 when model is invalid.
 */
s32 BoardModelCameraSet(s16 model, u16 camera)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        Hu3DModelCameraSet(model_ptr->id, camera);
        model_ptr->camera = camera;
        return 0;
    }
}

/**
 * @brief Assign a model to a render layer.
 * @param [in] model Board-model handle.
 * @param [in] layer Engine model render-layer index.
 * @return 0 after storing and applying the render layer, or -1 when model is invalid.
 */
s32 BoardModelLayerSet(s16 model, u8 layer)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        model_ptr->layer = layer;
        Hu3DModelLayerSet(model_ptr->id, layer);
        return 0;
    }
}

/**
 * @brief Get a model's render layer.
 * @param [in] model Board-model handle.
 * @return Stored render-layer index, or -1 for an invalid model.
 */
s32 BoardModelLayerGet(s16 model)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        return model_ptr->layer;
    }
}

/**
 * @brief Assign all live board models to a render layer.
 * @param [in] layer Engine model render-layer index applied to every live non-character model.
 */
void BoardModelLayerSetAll(s32 layer)
{
    BoardModel *model;
    for (model = modelDataList; model < &modelDataList[BOARD_MODEL_MAX]; model++) {
        if (model->index == -1 || model->character != -1) {
            continue;
        }
        Hu3DModelLayerSet(model->id, layer);
    }
}

/**
 * @brief Attach another board model to a named object in the parent model.
 * @param [in] model Board-model handle.
 * @param [in] name Name of the model object used as a hook point.
 * @param [in] hook Child board-model handle.
 * @return 0 after attaching hook, or -1 when the parent model, child model, or name is invalid.
 */
s32 BoardModelHookSet(s16 model, char *name, s16 hook)
{
    BoardModel *model_ptr = SearchBoardModel(model);

    if (!model_ptr) {
        return -1;
    }
    else {
        BoardModel *hook_ptr = SearchBoardModel(hook);
        if (!hook_ptr || !name) {
            return -1;
        }
        Hu3DModelHookSet(model_ptr->id, name, hook_ptr->id);
        BoardModelPosSet(hook, 0, 0, 0);
        return 0;
    }
}

/**
 * @brief Detach the model currently hooked to a board model.
 * @param [in] model Board-model handle.
 * @return 0 after detaching the hook, or -1 when model is invalid.
 */
s32 BoardModelHookReset(s16 model)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        Hu3DModelHookReset(model_ptr->id);
        return 0;
    }
}

/**
 * @brief Reset a named hook object on a board model.
 * @param [in] model Board-model handle.
 * @param [in] name Name of the model object used as a hook point.
 * @return 0 after forwarding name to the engine reset routine, or -1 when model is invalid.
 */
s32 BoardModelHookObjReset(s16 model, char *name)
{
    BoardModel *model_ptr = SearchBoardModel(model);
    if (!model_ptr) {
        return -1;
    }
    else {
        Hu3DModelHookObjReset(model_ptr->id, name);
        return 0;
    }
}

/**
 * @brief Find the live wrapper for a board-model handle.
 * @param [in] id Board-model handle to locate.
 * @return Matching live wrapper, or NULL for an invalid handle.
 */
static BoardModel *SearchBoardModel(s16 id)
{
    BoardModel *model;
    if (id <= 0 || id > BOARD_MODEL_MAX || !modelDataList || BoardIsKill()) {
        return NULL;
    }
    model = &modelDataList[id - 1];
    if (model->index != id) {
        return NULL;
    }
    return model;
}

/**
 * @brief Release one wrapper's engine model, motions, loaded data, and slot.
 * @param [in] model Board-model handle.
 */
static void KillBoardModel(BoardModel *model)
{
    s32 i;
    if (model->index == -1) {
        return;
    }
    if (model->character == -1) {
        model->mot_id[0] = -1;
        if (model->id >= 0) {
            Hu3DModelKill(model->id);
            model->id = -1;
        }
        for (i = 1; i < BOARD_MOT_MAX; i++) {
            if (model->mot_id[i] >= 0) {
                if (i != 0) {
                    Hu3DMotionKill(model->mot_id[i]);
                }
                model->mot_id[i] = -1;
            }
            model->mot_data[i] = NULL;
        }
        model->data = NULL;
        model->data_num = 0;
    }
    else {
        for (i = 0; i < BOARD_MOT_MAX; i++) {
            model->mot_id[i] = -1;
            model->mot_data[i] = NULL;
        }
        if (model->data) {
            if (model->id >= 0) {
                model->id = -1;
            }
            model->data = NULL;
        }
        CharModelKill(model->character);
        model->character = -1;
    }
    modelDataNum--;
    model->index = -1;
    model->visible = 0;
}

/**
 * @brief Find an existing wrapper that uses a given model data resource.
 * @param [in] data_num Packed model-data ID matched against live wrappers.
 * @return First wrapper using data_num, or NULL when none exists.
 */
static BoardModel *FindLinkModel(s32 data_num)
{
    BoardModel *model;
    for (model = modelDataList; model < &modelDataList[BOARD_MODEL_MAX]; model++) {
        if (model->data_num == data_num) {
            return model;
        }
    }
    return NULL;
}

/**
 * @brief Create or link the engine model for a wrapper.
 * @param [in] model Board-model handle.
 * @param [in] data_num Packed model-data ID loaded directly or used to find a link source.
 * @param [in] link Nonzero to reuse an already loaded model as an engine link.
 * @return 0 after linking or creating the engine model; -1 when model data cannot load or engine model creation fails.
 */
static s32 CreateBoardModel(BoardModel *model, s32 data_num, s32 link)
{
    BoardModel *linked_mdl = NULL;
    if (link && model->character == -1) {
        linked_mdl = FindLinkModel(data_num);
        if (linked_mdl) {
            model->id = Hu3DModelLink(linked_mdl->id);
            model->data = NULL;
            model->data_num = 0;
            return 0;
        }
    }
    if (model->character == -1) {
        model->data = HuDataSelHeapReadNum(data_num, HU_MEMNUM_OVL, HEAP_MODEL);
        if (!model->data) {
            s32 size_data, size_dvd;
            size_data = HuMemHeapSizeGet(HEAP_MODEL) - HuMemUsedMallocSizeGet(HEAP_MODEL);
            size_dvd = HuMemHeapSizeGet(HEAP_DVD) - HuMemUsedMallocSizeGet(HEAP_DVD);
            return -1;
        }
    }
    else {
        model->data = NULL;
    }
    if (model->character == -1) {
        model->id = Hu3DModelCreate(model->data);
    }
    else {
        model->id = CharModelCreate(model->character, 2);
    }
    Hu3DModelAttrSet(model->id, HU3D_ATTR_NOCULL);
    if (model->id < 0) {
        return -1;
    }
    else {
        model->data_num = data_num;
        return 0;
    }
}

/**
 * @brief Attach a list of data-backed or character motions to a wrapper.
 * @param [in] model Board-model handle.
 * @param [in] count Number of packed motion-data IDs in data_num.
 * @param [in] data_num Array of packed motion-data IDs loaded and attached in order.
 * @return 0 after attaching all motions; -1 when motion data cannot load or an engine or character motion cannot be created.
 */
static s32 CreateBoardModelMotion(BoardModel *model, s32 count, s32 *data_num)
{
    void *data;
    s32 i;
    model->mot_id[0] = Hu3DData[model->id].motId;
    if (!data_num) {
        model->mot_count = 1;
        return 0;
    }
    data = NULL;
    for (i = 0; i < count; i++) {
        s16 index;
        if (model->character == -1) {
            data = HuDataSelHeapReadNum(data_num[i], HU_MEMNUM_OVL, HEAP_MODEL);
            if (!data) {
                return -1;
            }
            index = Hu3DJointMotion(model->id, data);
        }
        else {
            index = CharMotionCreate(model->character, data_num[i]);
        }
        if (index < 0) {
            return -1;
        }
        model->mot_id[i + 1] = index;
        model->mot_data[i + 1] = data;
    }
    model->mot_count = count + 1;
    return 0;
}
