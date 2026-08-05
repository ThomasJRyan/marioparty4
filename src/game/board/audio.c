#include "game/board/audio.h"
#include "game/audio.h"
#include "game/gamework_data.h"
#include "game/msm.h"
#include "game/board/main.h"

static s16 boardSeq[2][2];

static s32 boardFX = -1;

static s16 boardMusTbl[] = {
    0x000D, 0x0011, 0x0015,
    0x001C, 0x0021, 0x0025,
    0x0028, 0x0029, 0x002A
};

/**
 * @brief Start the current board's music and, where defined, its looping ambient effect.
 */
void BoardMusStartBoard(void) {
    s16 musIdx = boardMusTbl[GWBoardGet()];

    BoardMusStart(0, musIdx, 0x7F, 0);
    BoardAudFXPlay();
}

/**
 * @brief Start a music sequence on one of the two board music channels.
 * @param [in] boardNo Board audio channel index (0 or 1).
 * @param [in] musId Music sequence ID.
 * @param [in] vol Target volume; a negative value selects full volume (127).
 * @param [in] speed Music fade-in duration; zero starts immediately.
 */
void BoardMusStart(s32 boardNo, s32 musId, s8 vol, u16 speed) {
    s16 *board = boardSeq[boardNo];
    MSM_MUSPARAM param;

    if (musId == board[1]) {
        return;
    }
    if (board[0] != -1) {
        BoardAudSeqFadeOutFast(board[0]);
    }
    param.flag = MSM_MUSPARAM_CHAN;
    if (speed != 0) {
        param.flag |= MSM_MUSPARAM_FADESPEED;
    }
    if (vol < 0) {
        param.flag |= MSM_MUSPARAM_VOL;
        vol = 0x7F;
    }
    param.fadeSpeed = speed;
    param.vol = vol;
    param.chan = boardNo;
    board[0] = msmMusPlay(musId, &param);
    board[1] = musId;
}

/**
 * @brief Fade a board music channel out over 100 audio ticks.
 * @param [in] boardNo Board audio channel index (0 or 1).
 */
void BoardAudSeqFadeOutFast(s32 boardNo) {
    BoardAudSeqFadeOut(boardNo, 0x64);
}

/**
 * @brief Fade out the active sequence on a board music channel and clear its tracked handles.
 * @param [in] boardNo Board audio channel index (0 or 1).
 * @param [in] speed Sequence fade-out duration in audio ticks.
 */
void BoardAudSeqFadeOut(s32 boardNo, u16 speed) {
    s16 *board = boardSeq[boardNo];

    if (board[0] == -1) {
        return;
    }
    HuAudSeqFadeOut(board[0], speed);
    board[1] = board[0] = -1;
}

/**
 * @brief Fade a board music channel between the normal volume and the quieter event volume.
 * @param [in] boardNo Board audio channel index (0 or 1).
 * @param [in] isQuieter Nonzero to fade to volume 96; zero to fade to full volume 127.
 */
void BoardMusLoudSet(s32 boardNo, s32 isQuieter) {
    if (isQuieter != 0) {
        BoardMusVolPanSet(boardNo, 0x60, 0x1F4);
    } else {
        BoardMusVolPanSet(boardNo, 0x7F, 0x1F4);
    }
}

/**
 * @brief Fade the active sequence on a board music channel to a new volume.
 * @param [in] boardNo Board audio channel index (0 or 1).
 * @param [in] vol Target volume; a negative value selects full volume (127).
 * @param [in] fadeSpeed Audio fade duration.
 */
void BoardMusVolPanSet(s32 boardNo, s8 vol, u16 fadeSpeed) {
    s16 *board = boardSeq[boardNo];
    MSM_MUSPARAM param;

    if (board[0] == -1) {
        return;
    }
    param.flag = MSM_MUSPARAM_CHAN;
    param.flag |= MSM_MUSPARAM_VOL;
    if (vol < 0) {
        vol = 0x7F;
    }
    param.fadeSpeed = fadeSpeed;
    param.vol = vol;
    msmMusSetParam(board[0], &param);
}

/**
 * @brief Pause or resume the active sequence when its current status permits the transition.
 * @param [in] boardNo Board audio channel index (0 or 1).
 * @param [in] pause Nonzero to pause and zero to resume.
 * @param [in] speed Pause or resume transition duration in audio ticks.
 */
void BoardAudSeqPause(s32 boardNo, s32 pause, u16 speed) {
    s16 *board = boardSeq[boardNo];

    if (board[0] == -1) {
        return;
    }
    if (pause != 0) {
        if (BoardMusStatusGet(boardNo) == 3) {
            return;
        }
    } else if (BoardMusStatusGet(boardNo) != 3) {
        return;
    }
    HuAudSeqPause(board[0], pause, speed);
}

/**
 * @brief Query the audio-engine status of a board music channel.
 * @param [in] boardNo Board audio channel index (0 or 1).
 * @return Audio-engine sequence status, or 0 when the channel has no active sequence.
 */
s32 BoardMusStatusGet(s32 boardNo) {
    s16 *board = boardSeq[boardNo];

    if (board[0] == -1) {
        return 0;
    }
    return msmMusGetStatus(board[0]);
}

/**
 * @brief Mark both board music channels and the ambient effect as inactive without stopping audio.
 */
void BoardAudSeqClear(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        boardSeq[i][0] = boardSeq[i][1] = -1;
    }
    boardFX = -1;
}

/**
 * @brief Fade out both tracked music channels and stop the tracked ambient effect.
 */
void BoardAudSeqFadeOutAll(void) {
    s16 *sequence;
    s32 i;

    for (i = 0; i < 2; i++) {
        sequence = boardSeq[i];
        if (sequence[0] != -1) {
            HuAudSeqFadeOut(sequence[0], 0x64);
            sequence[1] = sequence[0] = -1;
        }
    }
    BoardAudFXStop();
}

/**
 * @brief Start the looping ambient effect used by Koopa's Seaside Soiree or Bowser's Gnarly Party.
 */
void BoardAudFXPlay(void) {
    s32 seId;

    switch (GWBoardGet()) {
        case BOARD_ID_MAIN5:
            seId = 0x48D;
            break;
        case BOARD_ID_MAIN6:
            seId = 0x4A2;
            break;
        default:
            return;
    }
    if (boardFX == -1) {
        boardFX = HuAudFXPlay(seId);
    }
}

/**
 * @brief Stop and clear the tracked board ambient sound-effect handle.
 */
void BoardAudFXStop(void) {
    if (boardFX != -1) {
        HuAudFXStop(boardFX);
        boardFX = -1;
    }
}
