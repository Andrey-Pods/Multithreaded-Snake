/*
 * DO NOT MODIFY THIS FILE
 * Game board data structures and function prototypes - CSE 320 HW2
 */
#ifndef GAME_BOARD_H
#define GAME_BOARD_H

#include "snake.h"

#define MAX_PLAYERS 8

/* ---------------------------------------------------------------------------
 * Cell types on the game board
 * --------------------------------------------------------------------------- */
typedef enum {
    CELL_EMPTY   = 0,
    CELL_WALL    = 1,
    CELL_APPLE   = 2,
    CELL_SNAKE_0 = 10,
    CELL_SNAKE_1 = 11,
    CELL_SNAKE_2 = 12,
    CELL_SNAKE_3 = 13,
    CELL_SNAKE_4 = 14,
    CELL_SNAKE_5 = 15,
    CELL_SNAKE_6 = 16,
    CELL_SNAKE_7 = 17
} cell_t;

/* ---------------------------------------------------------------------------
 * Game board state
 *
 * The cells array is a flattened 2D grid in row-major order.
 * Access cell (x, y) as:  board->cells[y * board->size + x]
 * --------------------------------------------------------------------------- */
typedef struct game_board {
    int           size;                  /* Board dimension N (N x N grid)       */
    cell_t       *cells;                 /* Flattened 2D cell array              */
    snake_t       snakes[MAX_PLAYERS];   /* All snake slots                      */
    int           num_snakes;            /* Number of alive snakes               */
    int           max_snakes;            /* Maximum allowed concurrent snakes    */
    position_t    apple;                 /* Current apple position               */
    unsigned int  rng_state;             /* PRNG state for deterministic apples  */
} game_board_t;

/* ---------------------------------------------------------------------------
 * Function prototypes
 * --------------------------------------------------------------------------- */

/*
 * Initialize the game board: allocate cells, place walls, place first apple.
 * Returns 0 on success, -1 on error.
 */
int board_init(game_board_t *board, int size, int max_snakes, unsigned int seed);

/*
 * Free dynamically allocated board memory.
 */
void board_free(game_board_t *board);

/*
 * Deterministic PRNG.  MUST be used for all random number generation.
 * Returns the next pseudo-random number.
 */
unsigned int board_random(game_board_t *board);

/*
 * Place an apple on a random empty cell.
 * Returns 0 on success, -1 if no empty cells.
 */
int board_place_apple(game_board_t *board);

/*
 * Add a new snake to the board.  Writes the assigned ID into *out_id.
 * Returns 0 on success, -1 on error (board full, no empty cells, etc.).
 */
int board_add_snake(game_board_t *board, int *out_id);

/*
 * Remove a snake from the board (clears its cells, marks it dead).
 * Returns 0 on success, -1 on error.
 */
int board_remove_snake(game_board_t *board, int snake_id);

/*
 * Advance the game by one tick (move all alive snakes).
 * Returns 0 on success, -1 on error.
 */
int board_tick(game_board_t *board);

#endif /* GAME_BOARD_H */
