#include "game_board.h"
#include "debug.h"
#include "global.h"

int board_init(game_board_t *board, int size, int max_snakes,
               unsigned int seed) {
	(void)board;
	(void)size;
	(void)max_snakes;
	(void)seed;
	return 0;
}

void board_free(game_board_t *board) { (void)board; }

unsigned int board_random(game_board_t *board) {
	(void)board;
	return 0;
}

int board_place_apple(game_board_t *board) {
	(void)board;
	return 0;
}

int board_add_snake(game_board_t *board, int *out_id) {
	(void)board;
	(void)out_id;
	return 0;
}

int board_remove_snake(game_board_t *board, int snake_id) {
	(void)board;
	(void)snake_id;
	return 0;
}

int board_tick(game_board_t *board) {
	(void)board;
	return 0;
}
