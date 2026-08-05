#include "game/gamework.h"
#include "game/flag.h"
#include "game/gamework_data.h"
#include <string.h>
#include "version.h"

#ifndef __MWERKS__
#include "game/pad.h"
#endif

s16 GwLanguage = 1;
s16 GwLanguageSave = -1;

GameStat GWGameStatDefault;
GameStat GWGameStat;
SystemState GWSystem;
PlayerState GWPlayer[4];
PlayerConfig GWPlayerCfg[4];

/** @brief Zeroes the runtime player configuration, player state, and system state. */
static inline void GWErase(void)
{
    memset(GWPlayerCfg, 0, sizeof(GWPlayerCfg));
    memset(GWPlayer, 0, sizeof(GWPlayer));
    memset(&GWSystem, 0, sizeof(GWSystem));
}

/**
 * @brief Assigns each player its matching character, controller port, and group defaults.
 * @details A player is marked CPU-controlled when its corresponding controller reports an error.
 */
static inline void InitPlayerConfig(void)
{
    PlayerConfig *config;
    s32 i;
    for (i = 0; i < 4; i++) {
        config = &GWPlayerCfg[i];
        config->character = i;
        config->pad_idx = i;
        config->diff = 0;
        config->group = i;
        if (!HuPadStatGet(i)) {
            config->iscom = 0;
        }
        else {
            config->iscom = 1;
        }
    }
}

/** @brief Restores runtime board options such as rumble, explanations, and save mode. */
static inline void ResetBoardSettings(void)
{
    GWRumbleSet(1);
    GWMGExplainSet(1);
    GWMGShowComSet(1);
    GWMessSpeedSet(1);
    GWSaveModeSet(0);
}

/** @brief Initializes persistent defaults, flags, runtime state, players, language, and board options. */
void GWInit(void)
{
    GWGameStatReset();
    _InitFlag();
    GWErase();
    InitPlayerConfig();
#if VERSION_JP
    GWGameStat.language = 0;
#elif VERSION_ENG
    GWGameStat.language = 1;
#else
    GWLanguageSet(GwLanguage);
#endif
    ResetBoardSettings();
}

/**
 * @brief Restores all fifteen minigame records to their game-specific initial values.
 * @param game_stat Persistent statistics record whose minigame records are reset.
 */
static inline void ResetMGRecord(GameStat *game_stat)
{
    game_stat->mg_record[0] = 300 * REFRESH_RATE;
    game_stat->mg_record[1] = 80;
    game_stat->mg_record[2] = 60 * REFRESH_RATE;
    game_stat->mg_record[3] = 120 * REFRESH_RATE;
    game_stat->mg_record[4] = 0;
    game_stat->mg_record[5] = 60 * REFRESH_RATE;
    game_stat->mg_record[6] = 300 * REFRESH_RATE;
    game_stat->mg_record[7] = 300 * REFRESH_RATE;
    game_stat->mg_record[8] = 300 * REFRESH_RATE;
    game_stat->mg_record[9] = 0;
    game_stat->mg_record[10] = 5 * REFRESH_RATE;
    game_stat->mg_record[11] = 0;
    game_stat->mg_record[12] = 0;
    game_stat->mg_record[13] = 0;
    game_stat->mg_record[14] = 0;
}

/**
 * @brief Clears per-board play counts, records, and character win counts.
 * @param game_stat Persistent statistics record whose nine board entries are cleared.
 */
static inline void ResetBoardRecord(GameStat *game_stat)
{
    s32 i;
    s32 j;

    for (i = 0; i < 9; i++) {
        for (j = 0; j < 8; j++) {
            game_stat->board_win_count[i][j] = 0;
        }
        game_stat->board_play_count[i] = 0;
        game_stat->board_max_stars[i] = 0;
        game_stat->board_max_coins[i] = 0;
    }
}

/**
 * @brief Marks every present entry as uncollected.
 * @param game_stat Persistent statistics record containing the 60 present flags.
 */
static inline void ResetPresent(GameStat *game_stat)
{
    s32 i;
    for (i = 0; i < 60; i++) {
        game_stat->present[i] = 0;
    }
    (void)i; // HACK to match GWResetGameStat
}

/**
 * @brief Clears continue-state and unlock-related persistent flags.
 * @param game_stat Persistent statistics record containing the flags.
 */
static inline void ResetFlag(GameStat *game_stat)
{
    game_stat->story_continue = 0;
    game_stat->party_continue = 0;
    game_stat->open_w06 = 0;
    game_stat->veryHardUnlock = 0;
    game_stat->customPackEnable = 0;
    game_stat->musicAllF = 0;
}

/**
 * @brief Restores the story and party pause-menu options to their defaults.
 * @param game_stat Persistent statistics record containing both pause configurations.
 */
static inline void ResetPauseConfig(GameStat *game_stat)
{
    game_stat->story_pause.explain_mg = game_stat->party_pause.explain_mg = 1;
    game_stat->story_pause.show_com_mg = game_stat->party_pause.show_com_mg = 1;
    game_stat->story_pause.mg_list = game_stat->party_pause.mg_list = 0;
    game_stat->story_pause.mess_speed = game_stat->party_pause.mess_speed = 1;
    game_stat->story_pause.save_mode = game_stat->party_pause.save_mode = 0;
}

/**
 * @brief Constructs the default persistent game statistics and copies them into the active record.
 * @details The reset includes language, audio, records, unlock flags, pause options, and runtime board options.
 */
void GWGameStatReset(void)
{
    GameStat *game_stat = &GWGameStatDefault;
    memset(game_stat, 0, sizeof(GameStat));
    game_stat->unk_00 = 0;
#if VERSION_JP
    game_stat->language = 0;
#elif VERSION_ENG
    game_stat->language = 1;
#else
    game_stat->language = GwLanguage;
#endif
    game_stat->sound_mode = 1;
    game_stat->rumble = 1;
    game_stat->total_stars = 0;
    game_stat->create_time = 0;
    game_stat->mg_custom[0] = 0;
    game_stat->mg_custom[1] = 0;
    game_stat->mg_avail[0] = 0;
    game_stat->mg_avail[1] = 0;
    ResetMGRecord(game_stat);
    ResetBoardRecord(game_stat);
    ResetPresent(game_stat);
    ResetFlag(game_stat);
    ResetPauseConfig(game_stat);
    memcpy(&GWGameStat, &GWGameStatDefault, sizeof(GameStat));
    ResetBoardSettings();
}

/**
 * @brief Returns the runtime message delay, repairing values outside the region-specific range.
 * @return Message delay in frames after validation.
 */
s32 GWMessDelayGet(void)
{
#if VERSION_NTSC
    if (GWSystem.mess_delay > 48) {
        GWSystem.mess_speed = 1;
        GWSystem.mess_delay = 32;
    }
#else
    if (GWSystem.mess_delay > 64) {
        GWSystem.mess_speed = 1;
        GWSystem.mess_delay = 48;
    }
#endif
    return GWSystem.mess_delay;
}

/**
 * @brief Stores a minigame record unless game-work writes are locked by system flag 1:12.
 * @param index Zero-based index in the fifteen-entry record table.
 * @param value Minigame-specific record value, commonly measured in frames or points.
 */
void GWMGRecordSet(s32 index, u32 value)
{
    if (!_CheckFlag(FLAG_ID_MAKE(1, 12))) {
        GWGameStat.mg_record[index] = value;
    }
}

/**
 * @brief Reads a minigame record.
 * @param index Zero-based index in the fifteen-entry record table.
 * @return Stored minigame-specific record value.
 */
u32 GWMGRecordGet(s32 index)
{
    return GWGameStat.mg_record[index];
}

/**
 * @brief Looks up the opaque interface color assigned to a playable character.
 * @param character Character index from 0 through 7.
 * @param color Destination for the character's RGBA color.
 */
void GWCharColorGet(s32 character, GXColor *color)
{
    GXColor char_color[] = { { 227, 67, 67, 255 }, { 68, 67, 227, 255 }, { 241, 158, 220, 255 }, { 67, 228, 68, 255 }, { 138, 60, 180, 255 },
        { 146, 85, 55, 255 }, { 227, 228, 68, 255 }, { 40, 40, 40, 255 } };
    *color = char_color[character];
}

/**
 * @brief Sets a board's play count, clamped to 99.
 * @param board Board-record index.
 * @param value New play count.
 */
void GWBoardPlayCountSet(s32 board, u8 value)
{
    if (value > 99) {
        value = 99;
    }
    GWGameStat.board_play_count[board] = value;
}

/**
 * @brief Adds to a board's play count, saturating at 99.
 * @param board Board-record index.
 * @param value Unsigned number of plays to add.
 */
void GWBoardPlayCountAdd(s32 board, u8 value)
{
    value += GWGameStat.board_play_count[board];
    if (value > 99) {
        value = 99;
    }
    GWGameStat.board_play_count[board] = value;
}

/**
 * @brief Reads a board's accumulated play count.
 * @param board Board-record index.
 * @return Stored play count in the range 0 through 99.
 */
u8 GWBoardPlayCountGet(s32 board)
{
    return GWGameStat.board_play_count[board];
}

/**
 * @brief Stores the star record for a board.
 * @param board Board-record index.
 * @param value Star record to store; this routine does not clamp it.
 */
void GWBoardMaxStarsSet(s32 board, s16 value)
{
    GWGameStat.board_max_stars[board] = value;
}

/**
 * @brief Reads the star record for a board.
 * @param board Board-record index.
 * @return Stored star record.
 */
s32 GWBoardMaxStarsGet(s32 board)
{
    return GWGameStat.board_max_stars[board];
}

/**
 * @brief Stores the coin record for a board.
 * @param board Board-record index.
 * @param value Coin record to store; this routine does not clamp it.
 */
void GWBoardMaxCoinsSet(s32 board, s32 value)
{
    GWGameStat.board_max_coins[board] = value;
}

/**
 * @brief Reads the coin record for a board.
 * @param board Board-record index.
 * @return Stored coin record.
 */
s32 GWBoardMaxCoinsGet(s32 board)
{
    return GWGameStat.board_max_coins[board];
}

/**
 * @brief Increments a character's win count on a board, saturating at 99.
 * @param character Character index within the board record.
 * @param board Board-record index.
 * @return Updated win count in the range 0 through 99.
 */
s32 GWBoardWinCountInc(s32 character, s32 board)
{
    s32 win_count = GWGameStat.board_win_count[board][character] + 1;
    if (win_count > 99) {
        win_count = 99;
    }
    GWGameStat.board_win_count[board][character] = win_count;
    return win_count;
}

/**
 * @brief Reads a character's win count on a board.
 * @param character Character index within the board record.
 * @param board Board-record index.
 * @return Stored win count.
 */
s32 GWBoardWinCountGet(s32 character, s32 board)
{
    return GWGameStat.board_win_count[board][character];
}

/**
 * @brief Stores a character's win count on a board without clamping.
 * @param character Character index within the board record.
 * @param board Board-record index.
 * @param value Win count to store.
 */
void GWBoardWinCountSet(s32 character, s32 board, s32 value)
{
    GWGameStat.board_win_count[board][character] = value;
}

/**
 * @brief Tests the persistent availability bit for a minigame.
 * @param id Minigame overlay ID, with 401 corresponding to bit zero.
 * @return 1 when the minigame is available; otherwise 0.
 */
s32 GWMGAvailGet(s32 id)
{
    s32 word;
    s32 bit;
    id -= 401;
    word = id >> 5;
    bit = id % 32;
    if (GWGameStat.mg_avail[word] & (1 << bit)) {
        return 1;
    }
    else {
        return 0;
    }
}

/**
 * @brief Marks a minigame as available in persistent statistics.
 * @param id Minigame overlay ID, with 401 corresponding to bit zero.
 */
void GWMGAvailSet(s32 id)
{
    s32 word;
    s32 bit;
    id -= 401;
    word = id >> 5;
    bit = id % 32;
    GWGameStat.mg_avail[word] |= (1 << bit);
}

/**
 * @brief Tests whether a minigame belongs to the custom pack.
 * @param id Minigame overlay ID, with 401 corresponding to bit zero.
 * @return 1 when selected for the custom pack; otherwise 0.
 */
s32 GWMGCustomGet(s32 id)
{
    s32 word;
    s32 bit;
    id -= 401;
    word = id >> 5;
    bit = id % 32;
    if (GWGameStat.mg_custom[word] & (1 << bit)) {
        return 1;
    }
    else {
        return 0;
    }
}

/**
 * @brief Adds a minigame to the custom pack bitset.
 * @param id Minigame overlay ID, with 401 corresponding to bit zero.
 */
void GWMGCustomSet(s32 id)
{
    s32 word;
    s32 bit;
    id -= 401;
    word = id >> 5;
    bit = id % 32;
    GWGameStat.mg_custom[word] |= (1 << bit);
}

/**
 * @brief Removes a minigame from the custom pack bitset.
 * @param id Minigame overlay ID, with 401 corresponding to bit zero.
 */
void GWMGCustomReset(s32 id)
{
    s32 word;
    s32 bit;
    id -= 401;
    word = id >> 5;
    bit = id % 32;
    GWGameStat.mg_custom[word] &= ~(1 << bit);
}

/**
 * @brief Reads a player's current coin count.
 * @param player Player index from 0 through 3.
 * @return Current coin count.
 */
s16 GWCoinsGet(s32 player)
{
    return GWPlayer[player].coins;
}

/**
 * @brief Sets a player's coins and updates their lifetime maximum.
 * @details Values are clamped to 0 through 999; writes are ignored while system flag 1:12 is set.
 * @param player Player index from 0 through 3.
 * @param value Desired coin count.
 */
void GWCoinsSet(s32 player, s16 value)
{
    if (!_CheckFlag(FLAG_ID_MAKE(1, 12))) {
        if (value < 0) {
            value = 0;
        }
        if (value > 999) {
            value = 999;
        }
        if (value > GWPlayer[player].coins_max) {
            GWPlayer[player].coins_max = value;
        }
        GWPlayer[player].coins = value;
    }
}

/**
 * @brief Adds a signed amount to a player's coin count using normal clamping and write-lock rules.
 * @param player Player index from 0 through 3.
 * @param amount Signed coin delta.
 */
void GWCoinsAdd(s32 player, s16 amount)
{
    GWCoinsSet(player, GWPlayer[player].coins + amount);
}

/**
 * @brief Sets a player's stars and updates their lifetime maximum.
 * @param player Player index from 0 through 3.
 * @param value Desired star count, clamped to 0 through 999.
 */
void GWStarsSet(s32 player, s16 value)
{
    if (value < 0) {
        value = 0;
    }
    if (value > 999) {
        value = 999;
    }
    if (value > GWPlayer[player].stars_max) {
        GWPlayer[player].stars_max = value;
    }
    GWPlayer[player].stars = value;
}

/**
 * @brief Adds a signed amount to a player's star count with 0-through-999 clamping.
 * @param player Player index from 0 through 3.
 * @param amount Signed star delta.
 */
void GWStarsAdd(s32 player, s16 amount)
{
    GWStarsSet(player, GWPlayer[player].stars + amount);
}

/**
 * @brief Reads a player's current star count.
 * @param player Player index from 0 through 3.
 * @return Current star count.
 */
s32 GWStarsGet(s32 player)
{
    return GWPlayer[player].stars;
}

/**
 * @brief Sets the persistent total-star count.
 * @param value Desired total, clamped to 0 through 10000.
 */
void GWTotalStarsSet(s16 value)
{
    if (value < 0) {
        value = 0;
    }
    if (value > 10000) {
        value = 10000;
    }
    GWGameStat.total_stars = value;
}

/**
 * @brief Adds a signed amount to the persistent total-star count.
 * @param amount Signed star delta applied with 0-through-10000 clamping.
 */
void GWTotalStarsAdd(s16 amount)
{
    GWTotalStarsSet(GWGameStat.total_stars + amount);
}

/**
 * @brief Reads the persistent total-star count.
 * @return Total collected stars in the range 0 through 10000.
 */
u16 GWTotalStarsGet(void)
{
    return GWGameStat.total_stars;
}
