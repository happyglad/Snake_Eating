#ifndef SNAKE_GAME_H
#define SNAKE_GAME_H

#include <stdint.h>

/* Snake movement directions */
typedef enum {
    DIR_UP = 0,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} SnakeDir;

/**
 * @brief Initialize the snake game state.
 * Loads high score, displays start screen.
 */
void snake_game_init(void);

/**
 * @brief Process commands from USART/serial input.
 * @param cmd The command character ('U', 'D', 'L', 'R', 'S').
 */
void snake_game_on_command(char cmd);

/**
 * @brief Handle left turn event or start game (mapped to KEY1).
 */
void snake_game_turn_left_or_start(void);

/**
 * @brief Handle right turn event or start game (mapped to KEY2).
 */
void snake_game_turn_right_or_start(void);

/**
 * @brief Game tick update function.
 * Called in the main execution loop to handle game state updates, timings, and rendering.
 */
void snake_game_tick(void);

#endif /* SNAKE_GAME_H */
