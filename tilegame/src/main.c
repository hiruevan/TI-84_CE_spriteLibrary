#include <graphx.h>
#include <keypadc.h>
#include <tice.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "sprite_utils.h"
#include "map.h"
#include "sprites.h"

// ==================== CONSTANTS ====================
#define TILE_SIZE 16
#define VIEW_W 20
#define VIEW_H 15
#define MOVE_SPEED 4
#define GRASS_COLOR 16

// Buffers
#define SCREEN_W 320
#define SCREEN_H 240
#define WORLD_BUF_W (SCREEN_W + TILE_SIZE * 2)
#define WORLD_BUF_H (SCREEN_H + TILE_SIZE * 2)

#define PLAYER_Y_OFFSET (7 * TILE_SIZE)
#define PLAYER_X_OFFSET (9.5 * TILE_SIZE + 2)

#define BASE_ENCOUNTER_RATE 100
#define STEP_INCREMENT 2

#define MAX_ENEMY_NAME_LEN 16
#define MAX_MESSAGE_LEN 100

// ==================== ENUMS ====================
typedef enum {
    STATE_EXPLORATION,
    STATE_BATTLE,
    STATE_PAUSED
} GameState;

typedef enum {
    AREA_OVERWORLD,
    AREA_TOWN,
    AREA_CAVE
} AreaID;

typedef enum {
    BATTLE_MENU_MAIN,
    BATTLE_MENU_MAGIC,
    BATTLE_MENU_ITEM,
    BATTLE_ANIMATING,
    BATTLE_MESSAGE,
    BATTLE_VICTORY,
    BATTLE_GAME_OVER,
    BATTLE_EXIT
} BattleState;

typedef enum {
    INPUT_NONE,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_SELECT,
    INPUT_CANCEL
} InputAction;

typedef enum {
    ENEMY_GOBLIN,
    ENEMY_ORC,
    ENEMY_SKELETON,
    ENEMY_IMP
} EnemyType;

// ==================== STRUCTS ====================
typedef struct {
    int x, y;
    int w, h;
    const char** items;
    int item_count;
    int cursor;
    bool active;
} Menu;

typedef struct {
    char text[MAX_MESSAGE_LEN];
    int timer;
    bool active;
} MessageBox;

typedef struct {
    int hp;
    int max_hp;
    int mp;
    int max_mp;
    int tile_x, tile_y;
    int x, y;
    int target_x, target_y;
    bool is_moving;
} Player;

typedef struct {
    char name[MAX_ENEMY_NAME_LEN];
    int hp;
    int max_hp;
} Enemy;

typedef struct {
    const uint8_t (*map)[WORLD_MAP_W];

    int width;
    int height;

    int spawn_x;
    int spawn_y;

    int encounter_rate;

    AreaID id;
} Area;

typedef struct {
    int step_counter;
    bool in_battle;
    bool player_escaped;
    bool enemy_turn_pending;    
} BattleSystem;

// ==================== GLOBALS ====================

// Tile Sprites
static gfx_sprite_t* tile_sprites[19];

// Area Maps
// static AreaID current_area = AREA_OVERWORLD;
// static Area overworld_map = { // overworld
//     .map = world_map,
//     .width = WORLD_MAP_W, .height = WORLD_MAP_H,
//     .spawn_x = 0, .spawn_y = 0,
//     .encounter_rate = 1,
//     .AreaID = AREA_OVERWORLD
// }

// Game state
static GameState game_state = STATE_EXPLORATION;
static BattleState battle_state = BATTLE_MENU_MAIN;

// Player
static Player player = {
    .hp = 50, .max_hp = 50,
    .mp = 20, .max_mp = 20,
    .tile_x = 1, .tile_y = 1,
    .x = 16, .y = 16,
    .target_x = 16, .target_y = 16,
    .is_moving = false
};

// Enemy
static Enemy enemy = {
    .name = "Goblin",
    .hp = 20,
    .max_hp = 20
};

// Battle system
static BattleSystem battle = {
    .step_counter = 0,
    .in_battle = false,
    .player_escaped = false,
    .enemy_turn_pending = false
};

// Camera
static int cam_x = 0, cam_y = 0;
static int last_cam_x = -1, last_cam_y = -1;

// UI
static MessageBox global_message = {"", 0, false};
static Menu battle_menu;
static Menu pause_menu;

static const char* battle_commands[] = {"Fight", "Magic", "Item", "Run"};
static const char* pause_items[] = {"Continue", "Items", "Status", "Quit Game"};

// ==================== TILE SYSTEM ====================
void init_tile_sprites(void) { // Everything but grass, grass has index 0, and everything else is offset by 1
    tile_sprites[0] = wall_tile;
    tile_sprites[1] = treasure_chest;
    tile_sprites[2] = open_treasure_chest;
    tile_sprites[3] = mountain0;
    tile_sprites[4] = mountain1;
    tile_sprites[5] = mountain2;
    tile_sprites[6] = mountain3;
    tile_sprites[7] = mountain4;
    tile_sprites[8] = mountain5;
    tile_sprites[9] = mountain6;
    tile_sprites[10] = mountain7;
    tile_sprites[11] = mountain8;
    tile_sprites[12] = cave;
}

// ==================== COLLISION DETECTION ====================
static inline bool is_valid_tile(int tx, int ty) {
    return tx >= 0 && ty >= 0 && tx < WORLD_MAP_W && ty < WORLD_MAP_H;
}

static inline bool is_solid_tile(int tx, int ty) {
    return !is_valid_tile(tx, ty) || world_map[ty][tx] > 0;
}

static inline bool can_encounter(int tx, int ty) {
    return is_valid_tile(tx, ty) && world_map[ty][tx] == 0;
}

// ==================== MESSAGE SYSTEM ====================
void show_message(const char* msg, int duration) {
    strncpy(global_message.text, msg, MAX_MESSAGE_LEN - 1);
    global_message.text[MAX_MESSAGE_LEN - 1] = '\0';
    global_message.timer = duration;
    global_message.active = true;
}

void update_message_box(void) {
    if (global_message.active) {
        global_message.timer--;
        if (global_message.timer <= 0) {
            global_message.active = false;
            global_message.text[0] = '\0';
        }
    }
}

bool is_message_active(void) {
    return global_message.active;
}

void skip_message(void) {
    global_message.timer = 0;
    global_message.active = false;
}

// ==================== UI DRAWING ====================
void draw_window(int x, int y, int w, int h) {
    gfx_SetColor(2);
    gfx_FillRectangle(x, y, w, h);
    gfx_SetColor(255);
    gfx_Rectangle(x, y, w, h);
    gfx_Rectangle(x + 1, y + 1, w - 2, h - 2);
}

void draw_text(int x, int y, const char* text) {
    gfx_SetTextFGColor(255);
    gfx_SetTextBGColor(0);
    gfx_SetTextTransparentColor(0);
    gfx_SetTextXY(x, y);
    gfx_PrintString(text);
}

void draw_hp_bar(int x, int y, int current, int max, uint8_t color) {
    int bar_width = 60;
    int fill_width = max > 0 ? (current * bar_width) / max : 0;
    
    gfx_SetColor(255);
    gfx_Rectangle(x, y, bar_width + 2, 6);
    
    gfx_SetColor(0);
    gfx_FillRectangle(x + 1, y + 1, bar_width, 4);
    
    if (fill_width > 0) {
        gfx_SetColor(color);
        gfx_FillRectangle(x + 1, y + 1, fill_width, 4);
    }
}

void draw_menu(Menu* menu) {
    draw_window(menu->x, menu->y, menu->w, menu->h);
    
    for (int i = 0; i < menu->item_count; i++) {
        int item_y = menu->y + 8 + (i * 18);
        
        if (i == menu->cursor && menu->active) {
            gfx_SetColor(255);
            gfx_FillRectangle(menu->x + 8, item_y, 6, 6);
        }
        
        draw_text(menu->x + 20, item_y, menu->items[i]);
    }
}

void draw_message_box_centered(const char* message) {
    int box_w = 280;
    int box_h = 30;
    int box_x = (320 - box_w) / 2;
    int box_y = 100;
    
    draw_window(box_x, box_y, box_w, box_h);
    draw_text(box_x + 8, box_y + 8, message);
}

// ==================== INPUT HANDLING ====================
InputAction get_input(void) {
    static bool key_released = true;
    
    kb_Scan();
    
    bool any_key = (kb_Data[1] & kb_2nd) || (kb_Data[7] & kb_Up) || 
                   (kb_Data[7] & kb_Down) || (kb_Data[7] & kb_Left) ||
                   (kb_Data[7] & kb_Right) || (kb_Data[6] & kb_Mode);
    
    if (!any_key) {
        key_released = true;
        return INPUT_NONE;
    }
    
    if (!key_released) return INPUT_NONE;
    key_released = false;
    
    if (kb_Data[7] & kb_Up) return INPUT_UP;
    if (kb_Data[7] & kb_Down) return INPUT_DOWN;
    if (kb_Data[7] & kb_Left) return INPUT_LEFT;
    if (kb_Data[7] & kb_Right) return INPUT_RIGHT;
    if (kb_Data[1] & kb_2nd) return INPUT_SELECT;
    if (kb_Data[6] & kb_Mode) return INPUT_CANCEL;
    
    return INPUT_NONE;
}

int handle_menu_input(Menu* menu, InputAction input) {
    if (!menu->active) return -1;
    
    switch (input) {
        case INPUT_UP:
            menu->cursor--;
            if (menu->cursor < 0) menu->cursor = menu->item_count - 1;
            break;
            
        case INPUT_DOWN:
            menu->cursor++;
            if (menu->cursor >= menu->item_count) menu->cursor = 0;
            break;
            
        case INPUT_SELECT:
            return menu->cursor;
            
        case INPUT_CANCEL:
            return -2;
            
        default:
            break;
    }
    
    return -1;
}

// ==================== ENEMY SYSTEM ====================
void spawn_enemy(void) {
    EnemyType type = rand() % 4;
    
    switch(type) {
        case ENEMY_GOBLIN:
            strcpy(enemy.name, "Goblin");
            enemy.max_hp = 15 + (rand() % 10);
            break;
        case ENEMY_ORC:
            strcpy(enemy.name, "Orc");
            enemy.max_hp = 25 + (rand() % 10);
            break;
        case ENEMY_SKELETON:
            strcpy(enemy.name, "Skeleton");
            enemy.max_hp = 20 + (rand() % 8);
            break;
        case ENEMY_IMP:
            strcpy(enemy.name, "Imp");
            enemy.max_hp = 12 + (rand() % 6);
            break;
    }
    
    enemy.hp = enemy.max_hp;
}

void draw_enemy_sprite(void) {
    gfx_Sprite(slime_sprite, 25, 40);
}

// ==================== BATTLE ENCOUNTER ====================
void trigger_battle_encounter(void) {
    battle.in_battle = true;
    battle.step_counter = 0;
    battle_state = BATTLE_MENU_MAIN;
    
    spawn_enemy();
    
    // Battle transition effect
    for (uint8_t i = 0; i < 4; i++) {
        gfx_FillScreen(0);
        gfx_SwapDraw();
        delay(40);
        gfx_FillScreen(255);
        gfx_SwapDraw();
        delay(40);
    }
    gfx_FillScreen(0);
}

void check_encounter(void) {
    if (!can_encounter(player.tile_x, player.tile_y)) {
        battle.step_counter = 0;
        return;
    }
    
    battle.step_counter += STEP_INCREMENT;
    
    int encounter_chance = BASE_ENCOUNTER_RATE - (battle.step_counter / 4);
    if (encounter_chance < 4) encounter_chance = 4;
    
    if ((rand() % encounter_chance) == 0) {
        trigger_battle_encounter();
    }
}

// ==================== BATTLE GRAPHICS ====================
void draw_battle_stats(void) {
    draw_window(5, 150, 150, 85);
    
    char buffer[32];
    
    draw_text(12, 158, "Hero");
    
    draw_text(12, 172, "HP");
    sprintf(buffer, "%d/%d", player.hp, player.max_hp);
    draw_text(80, 172, buffer);
    draw_hp_bar(12, 184, player.hp, player.max_hp, 28);
    
    draw_text(12, 194, "MP");
    sprintf(buffer, "%d/%d", player.mp, player.max_mp);
    draw_text(80, 194, buffer);
    draw_hp_bar(12, 206, player.mp, player.max_mp, 97);
}

void draw_enemy_stats(void) {
    draw_window(160, 20, 150, 50);
    
    draw_text(168, 28, enemy.name);
    
    char buffer[32];
    sprintf(buffer, "HP:%d/%d", enemy.hp, enemy.max_hp);
    draw_text(168, 42, buffer);
    
    draw_hp_bar(168, 54, enemy.hp, enemy.max_hp, 224);
}

void draw_battle_screen(void) {
    if (battle_state == BATTLE_VICTORY) {
        gfx_FillScreen(0);
        draw_window(60, 90, 200, 60);
        draw_text(130, 110, "Victory!");
        
        char exp_msg[32];
        sprintf(exp_msg, "Gained %d EXP", enemy.max_hp * 10);
        draw_text(100, 130, exp_msg);
        return;
    }
    
    if (battle_state == BATTLE_GAME_OVER) {
        gfx_FillScreen(0);
        draw_window(60, 90, 200, 60);
        gfx_SetTextFGColor(224);
        draw_text(90, 110, "You were defeated");
        gfx_SetTextFGColor(255);
        return;
    }
    
    gfx_FillScreen(0);
    draw_enemy_sprite();
    draw_enemy_stats();
    draw_battle_stats();
    draw_menu(&battle_menu);
    
    if (global_message.active) {
        draw_message_box_centered(global_message.text);
    }
}

// ==================== BATTLE ACTIONS ====================
void execute_player_attack(void) {
    int damage = 8 + (rand() % 10);
    enemy.hp -= damage;
    if (enemy.hp < 0) enemy.hp = 0;
    
    char msg[MAX_MESSAGE_LEN];
    sprintf(msg, "You hit for %d damage!", damage);
    show_message(msg, 60);
    
    if (enemy.hp > 0) {
        battle.enemy_turn_pending = true;
    }
}

void execute_player_magic(void) {
    if (player.mp < 5) {
        show_message("Not enough MP!", 40);
        return;
    }
    
    player.mp -= 5;
    int damage = 12 + (rand() % 8);
    enemy.hp -= damage;
    if (enemy.hp < 0) enemy.hp = 0;
    
    char msg[MAX_MESSAGE_LEN];
    sprintf(msg, "Fire spell! %d damage!", damage);
    show_message(msg, 60);
    
    if (enemy.hp > 0) {
        battle.enemy_turn_pending = true;
    }
}

void execute_enemy_turn(void) {
    if (enemy.hp <= 0) return;
    
    int damage = 4 + (rand() % 7);
    player.hp -= damage;
    if (player.hp < 0) player.hp = 0;
    
    char msg[MAX_MESSAGE_LEN];
    sprintf(msg, "%s attacks for %d!", enemy.name, damage);
    show_message(msg, 60);
}

void try_run(void) {
    if ((rand() % 3) == 0) {
        show_message("Got away safely!", 40);
        battle.player_escaped = true;
    } else {
        show_message("Can't escape!", 40);
        battle.enemy_turn_pending = true;
    }
}

// ==================== BATTLE LOGIC ====================
void init_battle_menu(void) {
    battle_menu.x = 160;
    battle_menu.y = 150;
    battle_menu.w = 155;
    battle_menu.h = 85;
    battle_menu.items = battle_commands;
    battle_menu.item_count = 4;
    battle_menu.cursor = 0;
    battle_menu.active = true;
}

void handle_battle_input(void) {
    InputAction input = get_input();
    
    if (global_message.active) {
        if (input == INPUT_SELECT) {
            skip_message();
        }
        return;
    }
    
    if (battle_state == BATTLE_MENU_MAIN) {
        battle_menu.active = true;
        int selection = handle_menu_input(&battle_menu, input);
        
        if (selection >= 0) {
            battle_menu.active = false;
            
            switch(selection) {
                case 0:
                    execute_player_attack();
                    break;
                case 1:
                    execute_player_magic();
                    break;
                case 2:
                    show_message("No items!", 30);
                    break;
                case 3:
                    try_run();
                    break;
            }
        }
    }
}

void reset_player_after_death(void) {
    player.hp = player.max_hp;
    player.mp = player.max_mp;
    player.tile_x = 1;
    player.tile_y = 1;
    player.x = 16;
    player.y = 16;
    player.target_x = 16;
    player.target_y = 16;
}

void update_battle(void) {
    update_message_box();
    
    if (battle_state == BATTLE_MESSAGE && !global_message.active) {
        if (battle.player_escaped) {
            battle_state = BATTLE_EXIT;
            battle.player_escaped = false;
            battle.enemy_turn_pending = false;
            return;
        }
        
        if (enemy.hp <= 0) {
            battle_state = BATTLE_VICTORY;
            show_message("Victory!", 90);
            battle.enemy_turn_pending = false;
            return;
        }
        
        if (player.hp <= 0) {
            battle_state = BATTLE_GAME_OVER;
            show_message("You were defeated...", 90);
            battle.enemy_turn_pending = false;
            return;
        }
        
        if (battle.enemy_turn_pending) {
            battle.enemy_turn_pending = false;
            battle_state = BATTLE_MESSAGE;
            execute_enemy_turn();
            return;
        }
        
        battle_state = BATTLE_MENU_MAIN;
        battle_menu.active = true;
    }
    
    if (is_message_active() && battle_state == BATTLE_MENU_MAIN) {
        battle_state = BATTLE_MESSAGE;
        battle_menu.active = false;
    }
    
    if ((battle_state == BATTLE_VICTORY || battle_state == BATTLE_EXIT) && !global_message.active) {
        battle.in_battle = false;
    }
    
    if (battle_state == BATTLE_GAME_OVER && !global_message.active) {
        reset_player_after_death();
        battle.in_battle = false;
    }
}

// ==================== WORLD RENDERING ====================
void draw_world(void) {
    gfx_FillScreen(GRASS_COLOR);

    int start_tx = cam_x >> 4;
    int start_ty = cam_y >> 4;
    int offset_x = -(cam_x & 15);
    int offset_y = -(cam_y & 15);

    for (uint8_t y = 0; y <= VIEW_H; y++) {
        int ty = start_ty + y;
        if (ty < 0 || ty >= WORLD_MAP_H) continue;

        const uint8_t *row_ptr = world_map[ty];
        int draw_y = offset_y + (y << 4);
        int current_draw_x = offset_x;

        for (uint8_t x = 0; x <= VIEW_W; x++) {
            int tx = start_tx + x;
            if (tx >= 0 && tx < WORLD_MAP_W && row_ptr[tx]-1 >= 0) {
                gfx_Sprite(tile_sprites[row_ptr[tx]-1], current_draw_x, draw_y);
            }
            current_draw_x += 16;
        }
    }
}

// ==================== MOVEMENT ====================
void update_movement(void) {
    if (!player.is_moving) return;
    
    if (player.x != player.target_x) {
        player.x += (player.x < player.target_x) ? MOVE_SPEED : -MOVE_SPEED;
    }
    if (player.y != player.target_y) {
        player.y += (player.y < player.target_y) ? MOVE_SPEED : -MOVE_SPEED;
    }
    
    bool just_landed = player.is_moving && 
                      (player.x == player.target_x && player.y == player.target_y);
    
    player.is_moving = (player.x != player.target_x || player.y != player.target_y);
    
    if (just_landed) {
        check_encounter();
    }
}

void try_move(int dx, int dy) {
    if (player.is_moving) return;

    int new_tx = player.tile_x + dx;
    int new_ty = player.tile_y + dy;

    if (!is_solid_tile(new_tx, new_ty)) {
        player.tile_x = new_tx;
        player.tile_y = new_ty;
        player.target_x = new_tx << 4;
        player.target_y = new_ty << 4;
        player.is_moving = true;
    }
}

// ==================== PAUSE MENU ====================
void init_pause_menu(void) {
    pause_menu.x = 110;
    pause_menu.y = 70;
    pause_menu.w = 100;
    pause_menu.h = 90;
    pause_menu.items = pause_items;
    pause_menu.item_count = 4;
    pause_menu.cursor = 0;
    pause_menu.active = true;
}

void draw_pause_screen(void) {
    draw_world();
    gfx_TransparentSprite(player_sprite, PLAYER_X_OFFSET, PLAYER_Y_OFFSET);
    
    draw_menu(&pause_menu);
    
    draw_window(pause_menu.x - 10, pause_menu.y - 30, pause_menu.w + 20, 25);
    draw_text(pause_menu.x + 20, pause_menu.y - 22, "PAUSED");
}

bool handle_pause_menu(void) {
    InputAction input = get_input();
    int selection = handle_menu_input(&pause_menu, input);
    
    if (selection >= 0) {
        switch(selection) {
            case 0: // Continue
                game_state = STATE_EXPLORATION;
                draw_world();
                gfx_TransparentSprite(player_sprite, PLAYER_X_OFFSET, PLAYER_Y_OFFSET);
                gfx_SwapDraw();
                return false;
                
            case 1: // Items
                show_message("No items yet!", 40);
                break;
                
            case 2: // Status
                show_message("Status screen coming soon!", 40);
                break;
                
            case 3: // Quit Game
                return true;
        }
    } else if (selection == -2) {
        game_state = STATE_EXPLORATION;
    }
    
    return false;
}

// ==================== EXPLORATION MODE ====================
void handle_exploration(void) {
    if (battle.in_battle) {
        game_state = STATE_BATTLE;
        init_battle_menu();
        return;
    }
    
    kb_Scan();
    
    if (kb_Data[7] & kb_Left)  try_move(-1, 0);
    if (kb_Data[7] & kb_Right) try_move(1, 0);
    if (kb_Data[7] & kb_Up)    try_move(0, -1);
    if (kb_Data[7] & kb_Down)  try_move(0, 1);

    cam_x = player.x - 152;
    cam_y = player.y - 112;

    if (cam_x != last_cam_x || cam_y != last_cam_y) {
        draw_world();
        gfx_TransparentSprite(player_sprite, PLAYER_X_OFFSET, PLAYER_Y_OFFSET);
        gfx_SwapDraw();

        last_cam_x = cam_x;
        last_cam_y = cam_y;
    }

    update_movement();
}

// ==================== MAIN LOOP ====================
int main(void) {
    gfx_Begin();
    gfx_SetDrawBuffer();
    setup_xlibc_palette();

    init_tile_sprites();
    init_pause_menu();
    srand(rtc_Time());

    while (true) {
        kb_Scan();
        
        if (kb_Data[6] & kb_Clear && game_state == STATE_EXPLORATION) {
            game_state = STATE_PAUSED;
        }

        switch (game_state) {
            case STATE_PAUSED:
                draw_pause_screen();
                gfx_SwapDraw();
                
                if (handle_pause_menu()) {
                    gfx_End();
                    return 0;
                }
                
                update_message_box();
                break;
                
            case STATE_BATTLE:
                draw_battle_screen();
                gfx_SwapDraw();
                handle_battle_input();
                update_battle();
                
                if (!battle.in_battle) {
                    game_state = STATE_EXPLORATION;
                }
                break;
                
            case STATE_EXPLORATION:
                handle_exploration();
                break;
        }
    }
    
    gfx_End();
    return 0;
}