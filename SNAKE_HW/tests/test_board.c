#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdlib.h>

#include "game_board.h"
#include "global.h"

/* ========================================================================= */
/*  board_init tests                                                         */
/* ========================================================================= */

Test(board_suite, init_basic) {
	game_board_t board;
	int ret = board_init(&board, 20, 4, 42);
	cr_assert_eq(ret, 0, "board_init should return 0 on success");
	cr_assert_eq(board.size, 20);
	cr_assert_eq(board.max_snakes, 4);
	cr_assert_eq(board.num_snakes, 0);
	cr_assert_not_null(board.cells);
	board_free(&board);
}