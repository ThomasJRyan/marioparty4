#ifndef REL_M407DLL_H
#define REL_M407DLL_H

#include "game/object.h"

typedef void (*VoidFuncs)(void);
typedef void (*ObjFuncs)(OMOBJ *);

void initialize_player_system(HUPROCESS *process);
void play_player_idle_motion(void);
void play_player_walk_motion(void);
void play_player_turn_motion(void);
void play_player_reaction_motion(void);
void play_player_jump_motion(void);
void play_player_land_motion(void);
void set_player_motion_action(u8, s32);
void play_player_motion_action(u8);
void stop_player_motion(void);
#ifndef __MWERKS__
s32 get_player_motion_state(u8 player_index);
#endif
s32 get_player_action_state(u8);
s32 is_player_idle(u8);
void vibrate_player_controller(s16, s16, s16);

void initialize_map_scene(HUPROCESS *process, s32 map_section_index);
void get_map_scene_state(void);

void initialize_camera_system(HUPROCESS *process);
void reset_camera_transition(void);
void set_camera_center(float, float, float);
void get_camera_center(float *center_x, float *center_y, float *center_z);
s32 is_camera_transition_active(void);
void start_camera_transition(Vec *start_center, Vec *start_rotation, float *start_zoom, s32 frame_count, s32 transition_mode);

void initialize_whomp_system(HUPROCESS *process);
void clear_whomp_scores(void);
void reset_whomp_counts(void);
s32 get_whomp_score_total(void);
#ifndef __MWERKS__
void spawn_player_whomp(u8 player_index);
s16 get_player_whomp_count(u8 player_index);
s16 get_whomp_score_display_mode(void);
#endif

void initialize_whomp_score_system(HUPROCESS *process);
void reset_whomp_score_display(void);

void initialize_effect_system(HUPROCESS *process);
void reset_effect_pool(void);

void initialize_minigame_sequence(HUPROCESS *process);
void update_minigame_sequence(OMOBJ *obj);

void initialize_score_display(HUPROCESS *process, s16 initial_score);
void reset_score_display(void);
void set_score_display_mode(s32 display_mode);

#endif
