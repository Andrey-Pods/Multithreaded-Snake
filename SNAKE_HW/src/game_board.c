#include "game_board.h"
#include "debug.h"
#include "global.h"
#include <stdlib.h>

// just checks if a spot is actually on the board
static int is_valid_board_position(const game_board_t *board, int x, int y) {
	return board != NULL && board->cells != NULL && x >= 0 && x < board->size &&
	       y >= 0 && y < board->size;
}

// convert (x, y) into the 1d array index
static int board_cell_index(const game_board_t *board, int x, int y) {
	return y * board->size + x;
}

int board_init(game_board_t *board, int size, int max_snakes,
               unsigned int seed) {
	int x, y;

	if(board == NULL || size < BOARD_SIZE_MIN || size > BOARD_SIZE_MAX ||
	   max_snakes < MAX_PLAYERS_MIN || max_snakes > MAX_PLAYERS_MAX) {
		debug("board_init got bad args: board=%p size=%d max_snakes=%d",
		      (void *)board, size, max_snakes);
		return -1;
	}

	// set up the basic board fields first
	board->size = size;
	board->max_snakes = max_snakes;
	board->num_snakes = 0;
	board->rng_state = seed;
	board->apple.x = -1;
	board->apple.y = -1;
	board->cells = calloc((size_t)size * (size_t)size, sizeof(cell_t));
	if(board->cells == NULL) {
		debug("calloc failed in board_init for size=%d", size);
		return -1;
	}

	// every snake slot starts out unused
	for(x = 0; x < MAX_PLAYERS; x++) {
		board->snakes[x].id = x;
		board->snakes[x].length = 0;
		board->snakes[x].direction = DIR_RIGHT;
		board->snakes[x].next_direction = DIR_RIGHT;
		board->snakes[x].alive = 0;
	}

	// put walls around the outside edge
	for(y = 0; y < size; y++) {
		for(x = 0; x < size; x++) {
			if(x == 0 || y == 0 || x == size - 1 || y == size - 1) {
				board->cells[board_cell_index(board, x, y)] = CELL_WALL;
			}
		}
	}

	if(board_place_apple(board) != 0) {
		debug("could not place first apple during board_init");
		board_free(board);
		return -1;
	}

	debug("board initialized: size=%d max_snakes=%d seed=%u", size,
	      max_snakes, seed);
	return 0;
}

void board_free(game_board_t *board) {
	if(board == NULL) {
		return;
	}

	free(board->cells);
	board->cells = NULL;
	board->size = 0;
	board->num_snakes = 0;
	board->max_snakes = 0;
	board->apple.x = -1;
	board->apple.y = -1;
	debug("board memory freed");
}

unsigned int board_random(game_board_t *board) {
	if(board == NULL) {
		debug("board_random got NULL board");
		return 0;
	}

	board->rng_state = board->rng_state * 1103515245 + 12345;
	return (board->rng_state / 65536) % 32768;
}

int board_place_apple(game_board_t *board) {
	int i;
	int empty_count = 0;
	int target_empty;
	int current_empty = 0;
	int total_cells;

	if(board == NULL || board->cells == NULL || board->size <= 0) {
		debug("board_place_apple got invalid board");
		return -1;
	}

	// first count how many empty spots we have
	total_cells = board->size * board->size;
	for(i = 0; i < total_cells; i++) {
		if(board->cells[i] == CELL_EMPTY) {
			empty_count++;
		}
	}

	if(empty_count == 0) {
		debug("board_place_apple found no empty cells");
		return -1;
	}

	// pick one of the empty cells using the provided rng
	target_empty = (int)(board_random(board) % (unsigned int)empty_count);
	for(i = 0; i < total_cells; i++) {
		if(board->cells[i] != CELL_EMPTY) {
			continue;
		}
		if(current_empty == target_empty) {
			board->cells[i] = CELL_APPLE;
			board->apple.x = i % board->size;
			board->apple.y = i / board->size;
			debug("apple placed at (%d, %d)", board->apple.x, board->apple.y);
			return 0;
		}
		current_empty++;
	}

	debug("board_place_apple somehow failed after counting empty cells");
	return -1;
}

int board_add_snake(game_board_t *board, int *out_id) {
	int id;
	int start_x;
	int start_y;
	int interior_size;
	int start_index;
	int step;
	int found_index = -1;
	snake_t *snake;

	if(board == NULL || board->cells == NULL || out_id == NULL ||
	   board->size < BOARD_SIZE_MIN || board->max_snakes < MAX_PLAYERS_MIN ||
	   board->max_snakes > MAX_PLAYERS_MAX) {
		debug("board_add_snake got invalid args");
		return -1;
	}

	// find the first free snake id
	for(id = 0; id < board->max_snakes; id++) {
		if(!board->snakes[id].alive) {
			break;
		}
	}
	if(id == board->max_snakes) {
		debug("board_add_snake failed because board is full");
		return -1;
	}

	// starting spots are based on quadrants
	switch(id % 4) {
	case 0:
		start_x = board->size / 4;
		start_y = board->size / 4;
		break;
	case 1:
		start_x = (3 * board->size) / 4;
		start_y = board->size / 4;
		break;
	case 2:
		start_x = board->size / 4;
		start_y = (3 * board->size) / 4;
		break;
	case 3:
	default:
		start_x = (3 * board->size) / 4;
		start_y = (3 * board->size) / 4;
		break;
	}

	// if the preferred spot is taken, walk through the inside of the board
	interior_size = board->size - 2;
	start_index = (start_y - 1) * interior_size + (start_x - 1);
	for(step = 0; step < interior_size * interior_size; step++) {
		int candidate_index = (start_index + step) % (interior_size * interior_size);
		int x = (candidate_index % interior_size) + 1;
		int y = (candidate_index / interior_size) + 1;

		if(board->cells[board_cell_index(board, x, y)] == CELL_EMPTY) {
			found_index = candidate_index;
			start_x = x;
			start_y = y;
			break;
		}
	}

	if(found_index < 0) {
		debug("board_add_snake could not find an empty starting cell");
		return -1;
	}

	// basic one-segment snake to start
	snake = &board->snakes[id];
	snake->id = id;
	snake->body[0].x = start_x;
	snake->body[0].y = start_y;
	snake->length = 1;
	snake->direction = DIR_RIGHT;
	snake->next_direction = DIR_RIGHT;
	snake->alive = 1;
	board->cells[board_cell_index(board, start_x, start_y)] = (cell_t)(CELL_SNAKE_0 + id);
	board->num_snakes++;
	*out_id = id;
	debug("added snake %d at (%d, %d)", id, start_x, start_y);

	return 0;
}

int board_remove_snake(game_board_t *board, int snake_id) {
	int i;
	snake_t *snake;

	if(board == NULL || board->cells == NULL || snake_id < 0 ||
	   snake_id >= board->max_snakes) {
		debug("board_remove_snake got invalid snake id %d", snake_id);
		return -1;
	}

	snake = &board->snakes[snake_id];
	if(!snake->alive) {
		debug("board_remove_snake called on dead snake %d", snake_id);
		return -1;
	}

	// clear every body segment from the board
	for(i = 0; i < snake->length; i++) {
		position_t pos = snake->body[i];

		if(is_valid_board_position(board, pos.x, pos.y)) {
			board->cells[board_cell_index(board, pos.x, pos.y)] = CELL_EMPTY;
		}
	}

	snake->alive = 0;
	snake->length = 0;
	snake->direction = DIR_RIGHT;
	snake->next_direction = DIR_RIGHT;
	if(board->num_snakes > 0) {
		board->num_snakes--;
	}
	debug("removed snake %d", snake_id);

	return 0;
}

int board_tick(game_board_t *board) {
	int i;

	if(board == NULL || board->cells == NULL) {
		debug("board_tick got invalid board");
		return -1;
	}

	// move snakes in id order
	for(i = 0; i < MAX_PLAYERS; i++) {
		if(board->snakes[i].alive) {
			if(snake_advance(board, i) < 0) {
				debug("snake_advance failed for snake %d during board_tick", i);
				return -1;
			}
		}
	}

	return 0;
}
