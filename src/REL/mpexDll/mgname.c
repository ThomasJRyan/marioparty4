#include "ext_math.h"
#include "game/armem.h"
#include "game/audio.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

#include "REL/mpexDll.h"

#define FONT_CHAR_MAX 100

#if VERSION_PAL
static s32 FontCharFile[0x130] = {
#else
static s32 FontCharFile[0xC6] = {
#endif
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x26),
    DATA_MAKE_NUM(DATADIR_INST, 0x27),
    DATA_MAKE_NUM(DATADIR_INST, 0x28),
    DATA_MAKE_NUM(DATADIR_INST, 0x29),
    DATA_MAKE_NUM(DATADIR_INST, 0x2A),
    DATA_MAKE_NUM(DATADIR_INST, 0x2B),
    DATA_MAKE_NUM(DATADIR_INST, 0x2C),
    DATA_MAKE_NUM(DATADIR_INST, 0x2D),
    DATA_MAKE_NUM(DATADIR_INST, 0x2E),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x2F),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
#if VERSION_NTSC
    DATA_MAKE_NUM(DATADIR_INST, 0x34),
#endif
    DATA_MAKE_NUM(DATADIR_INST, 0x35),
    DATA_MAKE_NUM(DATADIR_INST, 0x36),
    DATA_MAKE_NUM(DATADIR_INST, 0x37),
    DATA_MAKE_NUM(DATADIR_INST, 0x38),
    DATA_MAKE_NUM(DATADIR_INST, 0x39),
    DATA_MAKE_NUM(DATADIR_INST, 0x3A),
    DATA_MAKE_NUM(DATADIR_INST, 0x3B),
    DATA_MAKE_NUM(DATADIR_INST, 0x3C),
    DATA_MAKE_NUM(DATADIR_INST, 0x3D),
    DATA_MAKE_NUM(DATADIR_INST, 0x3E),
    DATA_MAKE_NUM(DATADIR_INST, 0x3F),
    DATA_MAKE_NUM(DATADIR_INST, 0x40),
    DATA_MAKE_NUM(DATADIR_INST, 0x41),
    DATA_MAKE_NUM(DATADIR_INST, 0x42),
    DATA_MAKE_NUM(DATADIR_INST, 0x43),
    DATA_MAKE_NUM(DATADIR_INST, 0x44),
    DATA_MAKE_NUM(DATADIR_INST, 0x45),
    DATA_MAKE_NUM(DATADIR_INST, 0x46),
    DATA_MAKE_NUM(DATADIR_INST, 0x47),
    DATA_MAKE_NUM(DATADIR_INST, 0x48),
    DATA_MAKE_NUM(DATADIR_INST, 0x49),
    DATA_MAKE_NUM(DATADIR_INST, 0x4A),
    DATA_MAKE_NUM(DATADIR_INST, 0x4B),
    DATA_MAKE_NUM(DATADIR_INST, 0x4C),
    DATA_MAKE_NUM(DATADIR_INST, 0x4D),
#if VERSION_PAL
    DATA_MAKE_NUM(DATADIR_INST, 0x4E),
    DATA_MAKE_NUM(DATADIR_INST, 0x43),
    DATA_MAKE_NUM(DATADIR_INST, 0x32),
    DATA_MAKE_NUM(DATADIR_INST, 0x43),
    DATA_MAKE_NUM(DATADIR_INST, 0x43),
    DATA_MAKE_NUM(DATADIR_INST, 0x43),
    DATA_MAKE_NUM(DATADIR_INST, 0x43),
#else
    DATA_MAKE_NUM(DATADIR_INST, 0x42),
    DATA_MAKE_NUM(DATADIR_INST, 0x32),
    DATA_MAKE_NUM(DATADIR_INST, 0x42),
    DATA_MAKE_NUM(DATADIR_INST, 0x42),
    DATA_MAKE_NUM(DATADIR_INST, 0x42),
    DATA_MAKE_NUM(DATADIR_INST, 0x42),
    DATA_MAKE_NUM(DATADIR_INST, 0x4E),
#endif
    DATA_MAKE_NUM(DATADIR_INST, 0x4F),
    DATA_MAKE_NUM(DATADIR_INST, 0x50),
    DATA_MAKE_NUM(DATADIR_INST, 0x51),
    DATA_MAKE_NUM(DATADIR_INST, 0x52),
    DATA_MAKE_NUM(DATADIR_INST, 0x53),
    DATA_MAKE_NUM(DATADIR_INST, 0x54),
    DATA_MAKE_NUM(DATADIR_INST, 0x55),
    DATA_MAKE_NUM(DATADIR_INST, 0x56),
    DATA_MAKE_NUM(DATADIR_INST, 0x57),
    DATA_MAKE_NUM(DATADIR_INST, 0x58),
    DATA_MAKE_NUM(DATADIR_INST, 0x59),
    DATA_MAKE_NUM(DATADIR_INST, 0x5A),
    DATA_MAKE_NUM(DATADIR_INST, 0x5B),
    DATA_MAKE_NUM(DATADIR_INST, 0x5C),
    DATA_MAKE_NUM(DATADIR_INST, 0x5D),
    DATA_MAKE_NUM(DATADIR_INST, 0x5E),
    DATA_MAKE_NUM(DATADIR_INST, 0x5F),
    DATA_MAKE_NUM(DATADIR_INST, 0x60),
    DATA_MAKE_NUM(DATADIR_INST, 0x61),
    DATA_MAKE_NUM(DATADIR_INST, 0x62),
    DATA_MAKE_NUM(DATADIR_INST, 0x63),
    DATA_MAKE_NUM(DATADIR_INST, 0x64),
    DATA_MAKE_NUM(DATADIR_INST, 0x65),
    DATA_MAKE_NUM(DATADIR_INST, 0x66),
    DATA_MAKE_NUM(DATADIR_INST, 0x67),
#if VERSION_PAL
    DATA_MAKE_NUM(DATADIR_INST, 0x68),
#endif
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x2F),
    DATA_MAKE_NUM(DATADIR_INST, 0x33),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
#if VERSION_PAL
    DATA_MAKE_NUM(DATADIR_INST, 0x69),
    DATA_MAKE_NUM(DATADIR_INST, 0x6A),
    DATA_MAKE_NUM(DATADIR_INST, 0x6B),
    DATA_MAKE_NUM(DATADIR_INST, 0x6C),
    DATA_MAKE_NUM(DATADIR_INST, 0x6D),
    DATA_MAKE_NUM(DATADIR_INST, 0x6E),
    DATA_MAKE_NUM(DATADIR_INST, 0x6F),
    DATA_MAKE_NUM(DATADIR_INST, 0x70),
    DATA_MAKE_NUM(DATADIR_INST, 0x71),
    DATA_MAKE_NUM(DATADIR_INST, 0x72),
    DATA_MAKE_NUM(DATADIR_INST, 0x73),
    DATA_MAKE_NUM(DATADIR_INST, 0x74),
    DATA_MAKE_NUM(DATADIR_INST, 0x75),
    DATA_MAKE_NUM(DATADIR_INST, 0x76),
    DATA_MAKE_NUM(DATADIR_INST, 0x77),
#else
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
#endif
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x30),
    DATA_MAKE_NUM(DATADIR_INST, 0x31),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
#if VERSION_PAL
    DATA_MAKE_NUM(DATADIR_INST, 0x8F),
    DATA_MAKE_NUM(DATADIR_INST, 0x8E),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x78),
    DATA_MAKE_NUM(DATADIR_INST, 0x79),
    DATA_MAKE_NUM(DATADIR_INST, 0x7A),
    DATA_MAKE_NUM(DATADIR_INST, 0x7B),
    DATA_MAKE_NUM(DATADIR_INST, 0x7C),
    DATA_MAKE_NUM(DATADIR_INST, 0x7D),
    DATA_MAKE_NUM(DATADIR_INST, 0x7E),
    DATA_MAKE_NUM(DATADIR_INST, 0x7F),
    DATA_MAKE_NUM(DATADIR_INST, 0x80),
    DATA_MAKE_NUM(DATADIR_INST, 0x81),
    DATA_MAKE_NUM(DATADIR_INST, 0x82),
    DATA_MAKE_NUM(DATADIR_INST, 0x83),
    DATA_MAKE_NUM(DATADIR_INST, 0x84),
    DATA_MAKE_NUM(DATADIR_INST, 0x85),
    DATA_MAKE_NUM(DATADIR_INST, 0x86),
    DATA_MAKE_NUM(DATADIR_INST, 0x87),
    DATA_MAKE_NUM(DATADIR_INST, 0x88),
    DATA_MAKE_NUM(DATADIR_INST, 0x89),
    DATA_MAKE_NUM(DATADIR_INST, 0x8A),
    DATA_MAKE_NUM(DATADIR_INST, 0x8B),
    DATA_MAKE_NUM(DATADIR_INST, 0x8C),
    DATA_MAKE_NUM(DATADIR_INST, 0x8D),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
    DATA_MAKE_NUM(DATADIR_INST, 0x25),
#endif
};

s32 CreateTextSpriteGroup(SeqWork *sequenceWork, char *text, s16 useAlternateFont)
{
    char *currentCharacter;
    s16 textWidth;
    s16 *characterVerticalOffset;
    s16 characterCount;
    s32 *characterFileTable;
    s16 characterIndex;
    s16 spriteGroupIndex;
    s16 *characterHorizontalOffsets;
    ANIMDATA **characterAnimations;
    s16 spriteGroupId;
    s16 spriteId;
    s32 characterFileId;

    characterFileTable = FontCharFile;

    for (spriteGroupIndex = 0; spriteGroupIndex < 0x10; spriteGroupIndex++) {
        if (sequenceWork->spr_grp[spriteGroupIndex] == -1) {
            break;
        }
    }
    if (spriteGroupIndex == 0x10) {
        return -1;
    }
    characterAnimations = HuMemDirectMalloc(HEAP_HEAP, (FONT_CHAR_MAX * sizeof(ANIMDATA*)));
    characterHorizontalOffsets = HuMemDirectMalloc(HEAP_HEAP, FONT_CHAR_MAX * sizeof(*characterHorizontalOffsets));
    characterVerticalOffset = HuMemDirectMalloc(HEAP_HEAP, FONT_CHAR_MAX * sizeof(*characterVerticalOffset));

    for (currentCharacter = text, textWidth = 0, characterCount = 0; currentCharacter[0] != 0; currentCharacter++) {
        if (currentCharacter[0] == 0x20 || currentCharacter[0] == 0x10) {
            textWidth += 0xE;
        }
        else if (currentCharacter[0] < 0x30
#if VERSION_NTSC
             || currentCharacter[0] == 0x80 || currentCharacter[0] == 0x81
#endif
            ) {
            continue;
        }
        else {
#if VERSION_NTSC
            if (currentCharacter[1] == 0x80) {
                if ((currentCharacter[0] >= 0x96) && (currentCharacter[0] <= 0xA4)) {
                    characterFileId = characterFileTable[currentCharacter[0] + 0x6A];
                }
                else if ((currentCharacter[0] >= 0xAA) && (currentCharacter[0] <= 0xAE)) {
                    characterFileId = characterFileTable[currentCharacter[0] + 0x65];
                }
                else if ((currentCharacter[0] >= 0xD6) && (currentCharacter[0] <= 0xE4)) {
                    characterFileId = characterFileTable[currentCharacter[0] + 0x43];
                }
                else if ((currentCharacter[0] >= 0xEA) && (currentCharacter[0] <= 0xEE)) {
                    characterFileId = characterFileTable[currentCharacter[0] + 0x3E];
                }
            }
            else if (currentCharacter[1] == 0x81) {
                if ((currentCharacter[0] >= 0xAA) && (currentCharacter[0] <= 0xAE)) {
                    characterFileId = characterFileTable[currentCharacter[0] + 0x6A];
                }
                else if ((currentCharacter[0] >= 0xEA) && (currentCharacter[0] <= 0xEE)) {
                    characterFileId = characterFileTable[currentCharacter[0] + 0x43];
                }
            }
            else {
                characterFileId = characterFileTable[currentCharacter[0]];
            }
#else
            characterFileId = characterFileTable[currentCharacter[0]];
#endif
            characterAnimations[characterCount] = HuSprAnimReadFile(characterFileId);
            characterHorizontalOffsets[characterCount] = textWidth;
            if ((currentCharacter[0] >= 0x61) && (currentCharacter[0] <= 0x7A)) {
                characterVerticalOffset[characterCount] = 2;
                textWidth += 0x12;
            }
            else if ((currentCharacter[0] == 0xC2) || (currentCharacter[0] == 0xC3)
#if VERSION_PAL
                || (currentCharacter[0] == 0xC6)
#endif
            ) {
                characterVerticalOffset[characterCount] = 0;
                textWidth += 0x12;
            }
#if VERSION_PAL
            else if (currentCharacter[0] == 0xC7) {
                characterVerticalOffset[characterCount] = 0;
                textWidth += 0x18;
            }
            else if ((currentCharacter[0] == 0x5C) || (currentCharacter[0] == 0x85)) {
                characterVerticalOffset[characterCount] = 0;
                textWidth += 8;
            }
            else if ((currentCharacter[0] >= 0x90) && (currentCharacter[0] <= 0x9F)) {
                characterVerticalOffset[characterCount] = -2;
                textWidth += 0x18;
            }
            else if ((currentCharacter[0] >= 0xD0) && (currentCharacter[0] <= 0xEF)) {
                characterVerticalOffset[characterCount] = -2;
                textWidth += 0x12;
            }
#else
            else if (currentCharacter[0] == 0x5C) {
                characterVerticalOffset[characterCount] = 0;
                textWidth += 8;
            }
            else if ((currentCharacter[0] >= 0x87) && (currentCharacter[0] <= 0x8F)) {
                characterVerticalOffset[characterCount] = 4;
                textWidth += 0x18;
            }
            else if ((currentCharacter[0] >= 0xC7) && (currentCharacter[0] <= 0xCF)) {
                characterVerticalOffset[characterCount] = 4;
                textWidth += 0x18;
            }
#endif
            else if ((currentCharacter[0] == 0x3D) || (currentCharacter[0] == 0x84)) {
                characterVerticalOffset[characterCount] = 0;
                textWidth += 0x14;
            }
            else {
                characterVerticalOffset[characterCount] = 0;
                textWidth += 0x1C;
            }
            characterCount++;
        }
    }
    spriteGroupId = HuSprGrpCreate(characterCount);
    sequenceWork->spr_grp[spriteGroupIndex] = spriteGroupId;
    sequenceWork->alt_word_len = textWidth;
    textWidth = (textWidth / 2) - 0xE;
    for (characterIndex = 0; characterIndex < characterCount; characterIndex++) {
        spriteId = HuSprCreate(characterAnimations[characterIndex], 0, 0);
        HuSprGrpMemberSet(spriteGroupId, characterIndex, spriteId);
        HuSprPosSet(spriteGroupId, characterIndex, characterHorizontalOffsets[characterIndex] - textWidth, characterVerticalOffset[characterIndex]);
    }

    sequenceWork->word_len = characterCount;
    HuMemDirectFree(characterAnimations);
    HuMemDirectFree(characterHorizontalOffsets);
    HuMemDirectFree(characterVerticalOffset);
    return spriteGroupId;
}

s32 CreateMinigameNameSprites(s32 messageId)
{
    SeqWork nameSequenceWork;
    s16 wordLengths[2];
    char *messageCursor;
    s32 messageEnded;
    s16 lineIndex;
    char *lineStart;
    s32 useAlternateFont;
    s32 spriteGroupId;
    s16 language;

    language = GWGameStat.language;
    minigameNameLanguage = language;
    for (messageEnded = 0; messageEnded < 0x10; messageEnded++) {
        nameSequenceWork.sprite[messageEnded] = nameSequenceWork.spr_grp[messageEnded] = -1;
    }
    lineStart = MessData_MesPtrGet(messDataPtr, messageId);
    messageCursor = lineStart;
    messageEnded = 0;
    lineIndex = 0;

    while (TRUE) {
        if (messageCursor[0] == 0 || messageCursor[0] == 0xA) {
            if (messageCursor[0] == 0) {
                messageEnded = 1;
            }
            messageCursor[0] = 0;
            if (minigameNameLanguage == 0) {
                useAlternateFont = 1;
            }
            else {
                useAlternateFont = 0;
            }
            spriteGroupId = CreateTextSpriteGroup(&nameSequenceWork, lineStart, useAlternateFont);
            HuSprGrpPosSet(nameSequenceWork.spr_grp[lineIndex], 288.0f, 240.0f);
            wordLengths[lineIndex] = nameSequenceWork.alt_word_len;
            lineIndex++;
            if (messageEnded == 0) {
                lineStart = messageCursor + 1;
            }
            else {
                break;
            }
        }
        messageCursor++;
    }
    return spriteGroupId;
}
