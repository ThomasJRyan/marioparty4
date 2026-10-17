#ifndef REL_ZTARDLL_H
#define REL_ZTARDLL_H

#include "game/minigame_seq.h"

#include "dolphin/types.h"

s32 CreateCharacterNameSprites(SeqWork *sequenceWork, char *mess);
void LoadSelectedCharacterAssets(void);
void StartPlayerSelectionTasks(HUPROCESS *parentProcess);
s32 GetPlayerSelectionValue(s32 selectionMode, s32 restorePlayerSettings);
void OpenSelectionWindow(void);
void CloseSelectionWindow(void);
void SetSelectionWindowMessage(s32 messageId, s32 waitFrames, s32 instantText);
void InsertSelectionWindowMessage(u32 messageId, s32 insertPosition);
s32 ShowSelectionConfirmation(s32 choiceCount);
void OpenMessageWindowCentered(u32 messageId, s32 windowPositionMode);
void OpenMessageWindow(u32 messageId);
void CloseMessageWindow(void);

#endif
