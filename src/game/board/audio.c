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
    s16 music_id = boardMusTbl[GWBoardGet()];

    BoardMusStart(0, music_id, 0x7F, 0);
    BoardAudFXPlay();
}

/**
 * @brief Start a music sequence on one of the two board music channels.
 * @param [in] board_no Board music channel normally selected for playback and tracking; when replacing an active entry, the baseline body instead passes its stored sequence handle to BoardAudSeqFadeOutFast as the channel argument.
 * @param [in] music_id Requested sequence ID to play and store; a match with the tracked ID returns without changing the tracked selection.
 * @param [in] vol Value written to MSM_MUSPARAM.vol; negative input is replaced by 127 and enables MSM_MUSPARAM_VOL, while nonnegative input does not add that flag and msmMusPlay uses its base volume 127 path.
 * @param [in] speed Music fade-in duration; zero starts immediately.
 */
void BoardMusStart(s32 board_no, s32 music_id, s8 vol, u16 speed) {
    s16 *board = boardSeq[board_no];
    MSM_MUSPARAM param;

    if (music_id == board[1]) {
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
    param.chan = board_no;
    board[0] = msmMusPlay(music_id, &param);
    board[1] = music_id;
}

/**
 * @brief Forward a board music channel to BoardAudSeqFadeOut with the literal duration argument 0x64.
 * @param [in] board_no Board music channel forwarded unchanged to BoardAudSeqFadeOut.
 */
void BoardAudSeqFadeOutFast(s32 board_no) {
    BoardAudSeqFadeOut(board_no, 0x64);
}

/**
 * @brief Fade out the active sequence on a board music channel and clear its tracked handles.
 * @param [in] board_no Board music channel whose active sequence is faded and tracking state is cleared.
 * @param [in] speed Fade-duration argument forwarded to HuAudSeqFadeOut.
 */
void BoardAudSeqFadeOut(s32 board_no, u16 speed) {
    s16 *board = boardSeq[board_no];

    if (board[0] == -1) {
        return;
    }
    HuAudSeqFadeOut(board[0], speed);
    board[1] = board[0] = -1;
}

/**
 * @brief Request volume 96 or 127 for a board music channel.
 * @param [in] board_no Board music channel passed to BoardMusVolPanSet for the volume request.
 * @param [in] is_quieter Nonzero requests volume 96; zero requests volume 127.
 */
void BoardMusLoudSet(s32 board_no, s32 is_quieter) {
    if (is_quieter != 0) {
        BoardMusVolPanSet(board_no, 0x60, 0x1F4);
    } else {
        BoardMusVolPanSet(board_no, 0x7F, 0x1F4);
    }
}

/**
 * @brief Submit a volume parameter for the active sequence stored in one board music channel.
 * @param [in] board_no Used only to select the boardSeq entry containing the active sequence handle; the body enables MSM_MUSPARAM_CHAN but does not initialize MSM_MUSPARAM.chan.
 * @param [in] vol Target volume; a negative value selects full volume (127).
 * @param [in] fade_speed Value written to MSM_MUSPARAM.fadeSpeed and consumed by msmMusSetParam's VOL branch as pauseOffMaxTime = fadeSpeed / 15 to interpolate volume when nonzero; this setter path does not require or add MSM_MUSPARAM_FADESPEED.
 */
void BoardMusVolPanSet(s32 board_no, s8 vol, u16 fade_speed) {
    s16 *board = boardSeq[board_no];
    MSM_MUSPARAM param;

    if (board[0] == -1) {
        return;
    }
    param.flag = MSM_MUSPARAM_CHAN;
    param.flag |= MSM_MUSPARAM_VOL;
    if (vol < 0) {
        vol = 0x7F;
    }
    param.fadeSpeed = fade_speed;
    param.vol = vol;
    msmMusSetParam(board[0], &param);
}

/**
 * @brief Pause or resume the active sequence when its current status permits the transition.
 * @param [in] board_no Board music channel whose active sequence and paused state are updated.
 * @param [in] pause Nonzero to pause and zero to resume.
 * @param [in] speed Pause or resume transition argument forwarded to HuAudSeqPause.
 */
void BoardAudSeqPause(s32 board_no, s32 pause, u16 speed) {
    s16 *board = boardSeq[board_no];

    if (board[0] == -1) {
        return;
    }
    if (pause != 0) {
        if (BoardMusStatusGet(board_no) == 3) {
            return;
        }
    } else if (BoardMusStatusGet(board_no) != 3) {
        return;
    }
    HuAudSeqPause(board[0], pause, speed);
}

/**
 * @brief Query the audio-engine status of a board music channel.
 * @param [in] board_no Board music channel whose active sequence status is queried.
 * @return Audio-engine sequence status, or 0 when the channel has no active sequence.
 */
s32 BoardMusStatusGet(s32 board_no) {
    s16 *board = boardSeq[board_no];

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
    s16 *sequence_state;
    s32 i;

    for (i = 0; i < 2; i++) {
        sequence_state = boardSeq[i];
        if (sequence_state[0] != -1) {
            HuAudSeqFadeOut(sequence_state[0], 0x64);
            sequence_state[1] = sequence_state[0] = -1;
        }
    }
    BoardAudFXStop();
}

/**
 * @brief Start the looping ambient effect used by Koopa's Seaside Soiree or Bowser's Gnarly Party.
 */
void BoardAudFXPlay(void) {
    s32 sound_effect_id;

    switch (GWBoardGet()) {
        case BOARD_ID_MAIN5:
            sound_effect_id = 0x48D;
            break;
        case BOARD_ID_MAIN6:
            sound_effect_id = 0x4A2;
            break;
        default:
            return;
    }
    if (boardFX == -1) {
        boardFX = HuAudFXPlay(sound_effect_id);
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
