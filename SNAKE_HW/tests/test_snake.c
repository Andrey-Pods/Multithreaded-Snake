#include <criterion/criterion.h>
#include <criterion/logging.h>

#include "game_board.h"
#include "global.h"
#include "snake.h"

/* ---------------------------------------------------------------------------
 * Helper: set up a board with one snake at a known position
 * ---------------------------------------------------------------------------
 */
static game_board_t setup_board_with_snake(int size, unsigned int seed,
                                           int *out_id) {
	game_board_t board;
	board_init(&board, size, 4, seed);
	board_add_snake(&board, out_id);
	return board;
}

/* ========================================================================= */
/*  snake_set_direction tests                                                */
/* ========================================================================= */

Test(snake_suite, set_direction_valid) {
	snake_t snake = {0};
	setup_board_with_snake(20, 42,  NULL); //Not used, just here to avoid compiler error
	snake.direction = DIR_RIGHT;
	snake.next_direction = DIR_RIGHT;
	snake.alive = 1;
	int ret = snake_set_direction(&snake, DIR_UP);
	cr_assert_eq(ret, 0, "Setting non-opposite direction should succeed");
	cr_assert_eq(snake.next_direction, DIR_UP,
	             "next_direction should be updated to DIR_UP");
}