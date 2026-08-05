#include "game/esprite.h"
#include "game/data.h"
#include "game/sprite.h"

typedef struct esprite_s {
    s16 memberNo;
    s16 animNo;
} ESPRITE;

typedef struct espanim_s {
    /* 0x00 */ u32 dataNum;
    /* 0x04 */ u16 useCnt;
    /* 0x08 */ ANIMDATA *anim;
} ESPANIM; // Size 0xC

ESPRITE esprite[HUSPR_MAX];
ESPANIM espanim[HUSPR_MAX];

static HUSPRGRPID gid;

/**
 * @brief Initialize the extended sprite subsystem.
 */
void espInit(void) {
    s32 i;

    gid = HuSprGrpCreate(0x180);
    for (i = 0; i < HUSPR_MAX; i++) {
        esprite[i].memberNo = i;
        esprite[i].animNo = -1;
    }
    for (i = 0; i < HUSPR_MAX; i++) {
        espanim[i].useCnt = 0;
    }
}

/**
 * @brief Load or reuse sprite animation data and create an extended sprite.
 *
 * @param [in] dataNum Packed data-file identifier for the sprite animation.
 * @param [in] prio Initial HuSpr priority assigned when the underlying sprite is created.
 * @param [in] bank Initial bank selected in the ANIMDATA used by HuSprCreate().
 *
 * @return An extended-sprite slot index, or `-1` if no slot, animation slot,
 *         data buffer, or underlying sprite could be allocated.
 */
s16 espEntry(unsigned int dataNum, s16 prio, s16 bank) {
    ESPANIM *animFree;
    ESPANIM *anim;
    ESPRITE *esp;
    void *data;
    s16 sprNo;
    s16 i;
    s32 animFreeId;

    esp = esprite;
    for (i = 0; i < HUSPR_MAX; esp++, i++) {
        if (esp->animNo == -1) {
            break;
        }
    }
    if (i == HUSPR_MAX) {
        return -1;
    }
    anim = espanim;
    animFree = NULL;
    for (animFreeId = 0; animFreeId < HUSPR_MAX; anim++, animFreeId++) {
        if (anim->useCnt != 0) {
            if (anim->dataNum == dataNum) {
                animFree = NULL;
                break;
            }
        } else if (animFree == NULL) {
            animFree = anim;
        }
    }
    if (animFreeId == HUSPR_MAX) {
        if (animFree == NULL) {
            return -1;
        }
        data = HuDataSelHeapReadNum(dataNum, HU_MEMNUM_OVL, HEAP_MODEL);
        if (data == NULL) {
            return -1;
        }
        animFree->dataNum = dataNum;
        animFree->anim = HuSprAnimRead(data);
        anim = animFree;
    }
    sprNo = HuSprCreate(anim->anim, prio, bank);
    if (sprNo == -1) {
        if (animFree != NULL) {
            HuSprAnimKill(anim->anim);
        }
        return -1;
    }
    anim->useCnt++;
    esp->animNo = anim - espanim;
    HuSprGrpMemberSet(gid, esp->memberNo, sprNo);
    return i;
}

/**
 * @brief Remove an extended sprite and release its animation reference.
 *
 * @param [in] espId Slot returned by espEntry(); its sprite-group member is
 *                   killed and its shared animation reference count decremented.
 */
void espKill(s16 espId) {
    HuSprGrpMemberKill(gid, esprite[espId].memberNo);
    espanim[esprite[espId].animNo].useCnt--;
    esprite[espId].animNo = -1;
}

/**
 * @brief Get the underlying sprite group used by extended sprites.
 *
 * @return The sprite-group identifier created by espInit().
 */
s16 espGrpIDGet(void) {
    return gid;
}

/**
 * @brief Make an extended sprite visible.
 *
 * @param [in] espId Slot returned by espEntry(); attribute bit 4 is cleared on
 *                   its underlying sprite-group member.
 */
void espDispOn(s16 espId) {
    HuSprAttrReset(gid, esprite[espId].memberNo, 4);
}

/**
 * @brief Hide an extended sprite.
 *
 * @param [in] espId Slot returned by espEntry(); attribute bit 4 is set on its
 *                   underlying sprite-group member.
 */
void espDispOff(s16 espId) {
    HuSprAttrSet(gid, esprite[espId].memberNo, 4);
}

/**
 * @brief Set attribute bits on an extended sprite.
 *
 * @param [in] espId Slot returned by espEntry() whose underlying member receives `attr`.
 * @param [in] attr HuSpr attribute mask to enable.
 */
void espAttrSet(s16 espId, u16 attr) {
    HuSprAttrSet(gid, esprite[espId].memberNo, attr);
}

/**
 * @brief Clear attribute bits on an extended sprite.
 *
 * @param [in] espId Slot returned by espEntry() whose underlying member clears `attr`.
 * @param [in] attr HuSpr attribute mask to clear.
 */
void espAttrReset(s16 espId, u16 attr) {
    HuSprAttrReset(gid, esprite[espId].memberNo, attr);
}

/**
 * @brief Set an extended sprite's screen position.
 *
 * @param [in] espId Slot returned by espEntry() whose underlying member is repositioned.
 * @param [in] posX Horizontal coordinate assigned to the underlying group member.
 * @param [in] posY Vertical coordinate assigned to the underlying group member.
 */
void espPosSet(s16 espId, float posX, float posY) {
    HuSprPosSet(gid, esprite[espId].memberNo, posX, posY);
}

/**
 * @brief Set an extended sprite's horizontal and vertical scale.
 *
 * @param [in] espId Slot returned by espEntry() whose underlying member is rescaled.
 * @param [in] scaleX Horizontal multiplier assigned to the underlying group member.
 * @param [in] scaleY Vertical multiplier assigned to the underlying group member.
 */
void espScaleSet(s16 espId, float scaleX, float scaleY) {
    HuSprScaleSet(gid, esprite[espId].memberNo, scaleX, scaleY);
}

/**
 * @brief Set an extended sprite's screen-plane rotation.
 *
 * @param [in] espId Slot returned by espEntry() whose underlying member is rotated.
 * @param [in] zRot Rotation about the screen Z axis.
 */
void espZRotSet(s16 espId, float zRot) {
    HuSprZRotSet(gid, esprite[espId].memberNo, zRot);
}

/**
 * @brief Set an extended sprite's transparency level.
 *
 * @param [in] espId Slot returned by espEntry() whose underlying member receives
 *                   the transparency multiplier.
 * @param [in] tpLvl Transparency multiplier passed to HuSprTPLvlSet().
 */
void espTPLvlSet(s16 espId, float tpLvl) {
    HuSprTPLvlSet(gid, esprite[espId].memberNo, tpLvl);
}

/**
 * @brief Set an extended sprite's RGB modulation color.
 *
 * @param [in] espId Slot returned by espEntry() whose underlying member receives
 *                   the RGB modulation values.
 * @param [in] r Red color component.
 * @param [in] g Green color component.
 * @param [in] b Blue color component.
 */
void espColorSet(s16 espId, u8 r, u8 g, u8 b) {
    HuSprColorSet(gid, esprite[espId].memberNo, r, g, b);
}

/**
 * @brief Set an extended sprite's animation playback speed.
 *
 * @param [in] espId Slot returned by espEntry() whose underlying animation speed changes.
 * @param [in] speed Animation frames advanced per update.
 */
void espSpeedSet(s16 espId, float speed) {
    HuSprSpeedSet(gid, esprite[espId].memberNo, speed);
}

/**
 * @brief Select an extended sprite's animation bank.
 *
 * @param [in] espId Slot returned by espEntry() whose underlying animation bank changes.
 * @param [in] bank Bank selected from that sprite's ANIMDATA.
 */
void espBankSet(s16 espId, s16 bank) {
    HuSprBankSet(gid, esprite[espId].memberNo, bank);
}

/**
 * @brief Assign an extended sprite to an underlying draw pass.
 *
 * @param [in] espId Slot returned by espEntry() whose underlying draw pass changes.
 * @param [in] drawNo HuSpr draw-pass selector assigned to the sprite-group member.
 */
void espDrawNoSet(s16 espId, s16 drawNo) {
    HuSprDrawNoSet(gid, esprite[espId].memberNo, drawNo);
}

/**
 * @brief Set an extended sprite's draw priority.
 *
 * @param [in] espId Slot returned by espEntry() whose underlying draw priority changes.
 * @param [in] pri HuSpr priority assigned to the underlying group member.
 */
void espPriSet(s16 espId, s16 pri) {
    HuSprPriSet(gid, esprite[espId].memberNo, pri);
}
