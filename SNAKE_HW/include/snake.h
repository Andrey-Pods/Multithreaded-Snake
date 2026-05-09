/*
 * DO NOT MODIFY THIS FILE
 * Snake data structures and function prototypes - CSE 320 HW2
 */
#ifndef SNAKE_H
#define SNAKE_H

#define MAX_SNAKE_LENGTH 256

/* ---------------------------------------------------------------------------
 * Position on the game board
 * --------------------------------------------------------------------------- */
typedef struct {
    int x;  /* Column (0 = left edge) */
    int y;  /* Row    (0 = top edge)  */
} position_t;

/* ---------------------------------------------------------------------------
 * Movement direction
 * --------------------------------------------------------------------------- */
typedef enum {
    DIR_UP    = 0,
    DIR_DOWN  = 1,
    DIR_LEFT  = 2,
    DIR_RIGHT = 3
} direction_t;

/* ---------------------------------------------------------------------------
 * Snake state
 * --------------------------------------------------------------------------- */
typedef struct {
    int          id;                        /* Unique player ID (0-indexed)            */
    position_t   body[MAX_SNAKE_LENGTH];    /* body[0] = head                          */
    int          length;                    /* Current body length                     */
    direction_t  direction;                 /* Direction the snake is currently moving  */
    direction_t  next_direction;            /* Buffered direction from client input     */
    int          alive;                     /* 1 = alive, 0 = dead                     */
} snake_t;

/* Forward declaration (full definition in game_board.h) */
struct game_board;

/* ---------------------------------------------------------------------------
 * Function prototypes
 * --------------------------------------------------------------------------- */

/*
 * Set the snake's next_direction, unless the requested direction is
 * directly opposite to the current direction (in which case it is ignored).
 *
 * Returns  0 on success, -1 on error.
 */
int snake_set_direction(snake_t *snake, direction_t dir);

/*
 * Advance the snake by one cell in its current direction.
 *
 * Returns  0  on normal movement,
 *          1  on death (collision with wall or snake),
 *          2  on apple eaten (snake grows),
 *         -1  on error.
 */
int snake_advance(struct game_board *board, int snake_id);

#endif /* SNAKE_H */
