# Homework 5 - CSE 320 - Spring 2026
#### Professor Dan Benz

Readme version 1.0, see Piazza for any updates

### **Due Date: Sunday May 10th @ 11:59 PM**

**Read the entire doc before you start**

## Introduction

In this assignment, you will write a **multi-threaded multiplayer snake game server** in C.
The server will accept connections from multiple clients over TCP sockets, manage a shared
game board, and broadcast the game state to all connected players. The goal of this homework
is to familiarize yourself with POSIX threads (`pthreads`), socket programming, mutual
exclusion (`mutex`), and the challenges of concurrent programming in C.

> :nerd_face: Reference for pthreads: [https://man7.org/linux/man-pages/man7/pthreads.7.html](https://man7.org/linux/man-pages/man7/pthreads.7.html).
> Reference for sockets: [https://beej.us/guide/bgnet/html/](https://beej.us/guide/bgnet/html/).

A pre-built Python client is provided so you can visually test your server. **You are only
responsible for writing the C server.** The client is for your testing convenience and will
not be graded.

For all assignments in this course, you **MUST NOT** put any of the functions
that you write into the `main.c` file.  The file `main.c` **MUST ONLY** contain
`#include`s, local `#define`s and the `main` function.  The reason for this
restriction has to do with our use of the Criterion library to test your code
(this is discussed further at the end of this document).
Beyond this, you may have as many or as few additional `.c` files in the `src`
directory as you wish.  Also, you may declare as many or as few headers as you wish.
Note, however, that header and `.c` files distributed with the assignment base code
often contain a comment at the beginning which states that they are not to be
modified.  **PLEASE** take note of these comments and do not modify any such files,
as they will be replaced by the original versions during grading.

For all the assignments in this course, you are expected to write your own code;
you are not permitted to submit source code that you did not write yourself,
except for any code that is distributed as part of the basecode repository,
or code for which written permission has explicitly been given.
You are also not permitted to use library functions other than those provided
as part of the official Linux Mint software environment for the course.
In addition, for this particular assignment, there are some further restrictions
on what libraries you are allowed to use.

- Allowed:
  - `glibc` (GNU standard C library), including dynamic memory allocation
     (`malloc`, `calloc`, `free`) and standard I/O library functions.
  - `pthreads` (POSIX Threads library) for thread creation and synchronization.
  - POSIX sockets API (`socket`, `bind`, `listen`, `accept`, `send`, `recv`, etc.).
  - Standard POSIX functions (`close`, `getopt`, etc.).

- Disallowed:
  - Third-party networking libraries (e.g., `libevent`, `libev`, `libuv`).
  - Third-party threading libraries other than `pthreads`.
  - `select()`, `poll()`, or `epoll()` — you must use a thread-per-client model.

Submitting or using code that is not allowed will quite likely result in your
receiving a zero for this assignment and could potentially expose you to
charges of Academic Dishonesty.

> :scream:  This is a significantly more challenging project than Homework 1.
> It combines systems-level C programming with concurrent programming and networking,
> both of which introduce non-deterministic behavior and subtle bugs (race conditions,
> deadlocks) that are extremely difficult to debug after the fact.
> **Start early.** Build and test incrementally — get the game logic working first
> (Part 2), then add networking (Part 3), and finally add threading (Part 4).
> Do not attempt to write everything at once.


# Getting Started

This semester we will be using codegrade to submit and grade your projects.
If you are reading this document then you should have successfully used codegrade to
create your own git repo of my template linked to codegrade. Codegrade will rerun the
basic test scripts every time you do a git push.

> :scream: The tests that will be used to determine your grade are NOT the same
> as the tests you will see in this template repository or in your view of codegrade.
> I provide these tests to help you check basic functionality, ensure that your
> protocol formatting matches mine, and show you if your codegrade submission compiles.
> The real test cases will be run after the submission deadline.
> I STRONGLY encourage you to write your own additional tests to ensure that your code handles
> all of the required functionality.

Here is the structure of the base code:

<pre>
.
├── include
│   ├── game_board.h
│   ├── snake.h
│   ├── protocol.h
│   ├── server.h
│   ├── debug.h
│   └── global.h
├── Makefile
├── src
│   ├── main.c
│   ├── game_board.c
│   ├── snake.c
│   ├── protocol.c
│   └── server.c
├── tests
│   ├── test_snake.c
│   ├── test_board.c
│   ├── test_protocol.c
│   ├── test_integration.py
│   └── test_concurrency.py
└── client
    └── snake_client.py
</pre>

- The `.gitignore` file is a file that tells `git` to ignore files with names
matching specified patterns, so that they don't accidentally end up getting
committed to the repository.

> :scream:  The CI testing is for your own information; it does not directly have
> anything to do with assignment grading.
> The purpose of the tests are to alert you to possible problems with your code;
> if you see that testing has failed it is worth investigating why that has happened.

- The `Makefile` is a configuration file for the `make` build utility, which is what
you should use to compile your code.  In brief, `make` or `make all` will compile
anything that needs to be, `make debug` does the same except that it compiles the code
with options suitable for debugging, `make clean` removes files that resulted from
a previous compilation, and `make test` compiles the Criterion unit tests.
These "targets" can be combined; for example, you would use
`make clean debug` to ensure a complete clean and rebuild of everything for debugging.

- The `include` directory contains C header files (with extension `.h`) that are used
by the code.  Note that these files often contain `DO NOT MODIFY` instructions at the beginning.
You should observe these notices carefully where they appear.

- The `src` directory contains C source files (with extension `.c`).

- The `tests` directory contains C source code (and sometimes Python scripts)
that are used by the Criterion tests and integration tests.

- The `client` directory contains the provided Python game client for visual testing.

## A Note about Program Output

What a program does and does not print is VERY important.
In the UNIX world stringing together programs with piping and scripting is
commonplace. Although combining programs in this way is extremely powerful, it
means that each program must not print extraneous output.
Your server must follow the specifications for the binary protocol described below.
One part of our grading of this assignment will be to check whether your server
produces EXACTLY the specified binary output over its sockets. If your server
produces output that deviates from the specifications, even in a minor way,
or if it produces extraneous output on its sockets, it will adversely impact your grade
in a significant way, so pay close attention.

> :scream: Use the debug macro `debug` for any other program output or messages you may need
> while coding (e.g. debugging output). The `debug` macro prints to `stderr` and is
> compiled out in non-debug builds, so it will not interfere with grading.


# Part 1: Program Operation and Argument Validation

Your program will be a command-line server which when invoked will start listening
for incoming TCP connections on a specified port. The server will manage a snake
game on a shared board, with each connected client controlling one snake.

The specific usage scenarios for your program are as follows:

<pre>
Usage: bin/snake_server [options]
Options:
  -p port        Port number to listen on (required)
  -b board_size  Board size NxN (default: 20, min: 10, max: 50)
  -s seed        Random seed for apple placement (default: time-based)
  -m max_players Maximum concurrent players (default: 4, min: 1, max: 8)
  -h             Print this help message
</pre>

> :scream: A `PRINT_USAGE` macro has already been provided for you in the `global.h`
> header file.  You should use this macro to print the help message.

In case of successful startup, the server should print to `stdout`:
```
Snake server started on port <port> (board: <size>x<size>, max players: <max>)
```
and begin accepting connections. The server runs indefinitely until killed with
`Ctrl+C` (SIGINT).

In case of any error during startup (invalid arguments, port already in use, etc.),
the server should print a one-line message describing the error to `stderr`
and terminate with exit status `EXIT_FAILURE` (1).

The `main` function in `main.c` implements the argument processing using `getopt`.
I have provided macros in `global.h` for all output formatting to ensure consistency.


# Part 2: Game Logic

This part of the assignment involves implementing the core snake game mechanics.
These functions are **completely independent** of networking and threading — they
operate on in-memory data structures only. This means you can (and should) test
them thoroughly with Criterion before moving on to Parts 3 and 4.

> :relieved: This is the first point in this assignment where if for some reason
> you are unable to go on and complete the full assignment, you could stop and still collect
> a significant fraction of the grading score. We will unit-test your game logic
> functions extensively. So if you do a reasonable job of implementing `snake_move`,
> `board_place_apple`, collision detection, etc., then you will likely get most of
> the points from these unit tests.

## Game Board Coordinate System

The game board is an `N x N` grid where `N` is the board size specified at startup.
Coordinates use a zero-indexed system where `(0, 0)` is the **top-left** corner:

```
     col 0   col 1   col 2  ...  col N-1
row 0  (0,0)   (1,0)   (2,0)       (N-1,0)
row 1  (0,1)   (1,1)   (2,1)       (N-1,1)
row 2  (0,2)   (1,2)   (2,2)       (N-1,2)
 ...
row N-1(0,N-1) (1,N-1) (2,N-1)     (N-1,N-1)
```

A position on the board is represented as `(x, y)` where `x` is the column (increases
rightward) and `y` is the row (increases downward). This matches the convention used
by most terminal and graphics systems.

## Game Board Data Structures

The definitions of the core types are given in the header files in the `include` directory.
These structures are **DO NOT MODIFY** — they define the interface your code must implement.

### `position_t` (defined in `snake.h`)

```c
typedef struct {
    int x;  // Column (0 = left)
    int y;  // Row (0 = top)
} position_t;
```

### `direction_t` (defined in `snake.h`)

```c
typedef enum {
    DIR_UP    = 0,
    DIR_DOWN  = 1,
    DIR_LEFT  = 2,
    DIR_RIGHT = 3
} direction_t;
```

### `snake_t` (defined in `snake.h`)

```c
#define MAX_SNAKE_LENGTH 256

typedef struct {
    int          id;                        // Unique player ID (0-indexed)
    position_t   body[MAX_SNAKE_LENGTH];    // body[0] = head
    int          length;                    // Current length
    direction_t  direction;                 // Current movement direction
    direction_t  next_direction;            // Buffered direction from client input
    int          alive;                     // 1 if alive, 0 if dead
} snake_t;
```

The snake's body is stored as an array of positions where `body[0]` is always the head.
When the snake moves, every segment follows the one in front of it. The `direction` field
is the direction the snake is *currently* moving. The `next_direction` field is the direction
the client has *requested* — it is applied at the start of the next game tick. This two-field
approach prevents the classic bug where a snake moving right can instantly reverse into itself
by pressing left.

### `cell_t` (defined in `game_board.h`)

```c
typedef enum {
    CELL_EMPTY   = 0,
    CELL_WALL    = 1,
    CELL_APPLE   = 2,
    CELL_SNAKE_0 = 10,  // Snake player 0
    CELL_SNAKE_1 = 11,  // Snake player 1
    CELL_SNAKE_2 = 12,  // Snake player 2
    CELL_SNAKE_3 = 13,  // Snake player 3
    CELL_SNAKE_4 = 14,  // Snake player 4
    CELL_SNAKE_5 = 15,  // Snake player 5
    CELL_SNAKE_6 = 16,  // Snake player 6
    CELL_SNAKE_7 = 17   // Snake player 7
} cell_t;
```

A snake's cell value is `CELL_SNAKE_0 + snake_id`. This allows the board to distinguish
which player occupies each cell.

### `game_board_t` (defined in `game_board.h`)

```c
#define MAX_PLAYERS 8

typedef struct {
    int       size;                     // Board dimension (N x N)
    cell_t   *cells;                    // Flattened 2D array: cells[y * size + x]
    snake_t   snakes[MAX_PLAYERS];      // Array of all snakes
    int       num_snakes;               // Number of snakes currently in the game
    int       max_snakes;               // Maximum allowed snakes
    position_t apple;                   // Current apple position
    unsigned int rng_state;             // Random number generator state (for seeded apple placement)
} game_board_t;
```

The board's cells are stored as a **flattened 2D array** in row-major order.
To access the cell at position `(x, y)`, use: `board->cells[y * board->size + x]`.

The `rng_state` field is used for deterministic apple placement. You must use
the provided `board_random()` function (see below) for all random number generation
so that tests can produce reproducible results using `--seed`.


## Game Board Functions (`game_board.c`)

### `int board_init(game_board_t *board, int size, int max_snakes, unsigned int seed)`

Initializes a game board. Allocates the `cells` array (`size * size` cells),
sets all cells to `CELL_EMPTY`, initializes all snakes to a default state
(alive = 0, length = 0), sets `num_snakes = 0`, and stores the random seed
in `rng_state`. Places walls around the border of the board (all cells where
`x == 0`, `x == size-1`, `y == 0`, or `y == size-1` are set to `CELL_WALL`).
After initialization, calls `board_place_apple()` to place the first apple.

**Parameters:**
- `board`: Pointer to the game board to initialize
- `size`: Board dimension (N×N). Must be between 10 and 50 inclusive.
- `max_snakes`: Maximum number of concurrent players (1-8)
- `seed`: Random seed for reproducible apple placement

**Returns:**
- `0` on success
- `-1` on error (NULL pointer, invalid size, allocation failure)

### `void board_free(game_board_t *board)`

Frees the dynamically allocated `cells` array and sets the pointer to NULL.
Safely handles NULL pointers.

### `unsigned int board_random(game_board_t *board)`

Generates the next pseudorandom number using the board's `rng_state`.
You **must** use this implementation for all random number generation:

```c
unsigned int board_random(game_board_t *board) {
    board->rng_state = board->rng_state * 1103515245 + 12345;
    return (board->rng_state / 65536) % 32768;
}
```

This is a simple linear congruential generator. Using this specific formula
ensures that given the same seed, apple placement will be identical across
all submissions, which is critical for automated testing.

**Parameters:**
- `board`: Pointer to the game board

**Returns:**
- The next pseudorandom number

### `int board_place_apple(game_board_t *board)`

Places an apple on a random empty cell. The algorithm is:

1. Count the total number of `CELL_EMPTY` cells on the board.
2. If there are no empty cells, return `-1`.
3. Call `board_random(board)` to get a random number.
4. Compute the target index: `random_value % empty_count`.
5. Iterate through the cells array in order (row by row, left to right).
   Count empty cells until you reach the target index.
6. Place `CELL_APPLE` at that cell and store the position in `board->apple`.

**Parameters:**
- `board`: Pointer to the game board

**Returns:**
- `0` on success
- `-1` if no empty cells are available or on error

### `int board_add_snake(game_board_t *board, int *out_id)`

Adds a new snake to the board. Finds the first available snake slot
(where `alive == 0`), places the snake at a valid starting position,
and marks it as alive.

The starting position algorithm is:
1. Find the first snake slot where `alive == 0`. If none available, return `-1`.
2. Compute starting position based on the snake's ID:
   - The starting positions cycle through 4 quadrants of the board interior:
     - ID % 4 == 0: top-left quadrant at `(size/4, size/4)`
     - ID % 4 == 1: top-right quadrant at `(3*size/4, size/4)`
     - ID % 4 == 2: bottom-left quadrant at `(size/4, 3*size/4)`
     - ID % 4 == 3: bottom-right quadrant at `(3*size/4, 3*size/4)`
3. If the computed starting cell is not `CELL_EMPTY`, scan rightward and
   then downward (wrapping within the interior) until an empty cell is found.
   If no empty cell exists, return `-1`.
4. Initialize the snake: set `id`, `body[0]` to the starting position,
   `length = 1`, `direction = DIR_RIGHT`, `next_direction = DIR_RIGHT`,
   `alive = 1`.
5. Set the cell at the starting position to `CELL_SNAKE_0 + id`.
6. Store the assigned ID in `*out_id` and increment `board->num_snakes`.

**Parameters:**
- `board`: Pointer to the game board
- `out_id`: Pointer to integer where the assigned snake ID will be written

**Returns:**
- `0` on success
- `-1` on error (board full, no empty cells, NULL pointers)

### `int board_remove_snake(game_board_t *board, int snake_id)`

Removes a snake from the board. Sets all cells occupied by the snake to
`CELL_EMPTY`, sets `alive = 0`, `length = 0`, and decrements `board->num_snakes`.

**Parameters:**
- `board`: Pointer to the game board
- `snake_id`: ID of the snake to remove (0-7)

**Returns:**
- `0` on success
- `-1` on error (invalid ID, snake not alive)


## Snake Movement Functions (`snake.c`)

### `int snake_set_direction(snake_t *snake, direction_t dir)`

Sets the snake's `next_direction` to the requested direction, **but only if
the new direction is not directly opposite to the current `direction`**.
A snake moving `UP` cannot immediately go `DOWN`, and a snake moving `LEFT`
cannot immediately go `RIGHT`. If the requested direction is opposite, the
function does nothing and returns `0` (this is not an error — the input is
simply ignored).

**Parameters:**
- `snake`: Pointer to the snake
- `dir`: Requested direction

**Returns:**
- `0` on success
- `-1` on error (NULL pointer, invalid direction value)

### `int snake_advance(game_board_t *board, int snake_id)`

Advances a snake by one step. This is the core movement function called once
per game tick for each alive snake. The algorithm is:

1. Copy `next_direction` into `direction` (apply the buffered input).
2. Compute the new head position by moving one cell in the current `direction`:
   - `DIR_UP`:    `new_y = head_y - 1`
   - `DIR_DOWN`:  `new_y = head_y + 1`
   - `DIR_LEFT`:  `new_x = head_x - 1`
   - `DIR_RIGHT`: `new_x = head_x + 1`
3. Check what is at the new head position:
   - **`CELL_WALL`** or **`CELL_SNAKE_*`** (any snake, including itself): The snake dies.
     Call `board_remove_snake()` and return `1` to indicate death.
   - **`CELL_APPLE`**: The snake grows. Do NOT remove the tail. Move the body
     forward (shift all segments down by one index in the `body` array),
     place the new head at `body[0]`, set the cell to the snake's cell value,
     increment `length` (up to `MAX_SNAKE_LENGTH`), and call `board_place_apple()`
     to spawn a new apple. Return `2` to indicate an apple was eaten.
   - **`CELL_EMPTY`**: Normal movement. Clear the tail cell to `CELL_EMPTY`,
     shift all body segments down by one, place the new head at `body[0]`,
     and set the new head cell to the snake's cell value. Return `0`.

**Important**: When shifting the body, you must iterate from the tail toward the head:
`for (i = length - 1; i > 0; i--) body[i] = body[i-1];`
Then set `body[0] = new_head_position`.

**Parameters:**
- `board`: Pointer to the game board
- `snake_id`: ID of the snake to advance

**Returns:**
- `0` on normal movement
- `1` on death (collision)
- `2` on apple eaten
- `-1` on error


## Game Tick Function (`game_board.c`)

### `int board_tick(game_board_t *board)`

Advances the game by one tick. This function is called by the game loop thread
at regular intervals. It iterates through all snakes in order of ID (0 through
`MAX_PLAYERS - 1`), and for each alive snake, calls `snake_advance()`.

**Parameters:**
- `board`: Pointer to the game board

**Returns:**
- `0` on success
- `-1` on error


# Part 3: Network Protocol

This section defines the binary protocol used for communication between the
server and clients. The protocol is simple and fixed-size to make parsing
straightforward. **All multi-byte integers are in network byte order (big-endian).**

> :nerd_face: You should use `htons()`, `htonl()`, `ntohs()`, and `ntohl()` to convert
> between host and network byte order. See `man byteorder` for details.

## Message Types

### Client → Server Messages

All client messages are exactly **2 bytes**:

| Byte 0 (type) | Byte 1 (payload) | Description |
|:-:|:-:|:--|
| `0x01` | `0x00` | **JOIN** — Request to join the game |
| `0x02` | direction | **DIRECTION** — Change snake direction (0=UP, 1=DOWN, 2=LEFT, 3=RIGHT) |
| `0x03` | `0x00` | **LEAVE** — Disconnect gracefully |

### Server → Client Messages

Server messages have a **1-byte type** header followed by a variable-length payload:

**WELCOME (type `0x10`)** — Sent to a client after a successful JOIN. Total: 4 bytes.

| Offset | Size | Field |
|:-:|:-:|:--|
| 0 | 1 | Type: `0x10` |
| 1 | 1 | Assigned player ID (0-7) |
| 2 | 1 | Board size (N) |
| 3 | 1 | Max players |

**GAME_STATE (type `0x20`)** — Broadcast to all clients every game tick. Variable length.

| Offset | Size | Field |
|:-:|:-:|:--|
| 0 | 1 | Type: `0x20` |
| 1 | 1 | Number of alive snakes |
| 2 | 2 | Apple X position (big-endian uint16) |
| 4 | 2 | Apple Y position (big-endian uint16) |
| 6 | ... | Snake data (repeated for each alive snake) |

Each alive snake is encoded as:

| Offset | Size | Field |
|:-:|:-:|:--|
| 0 | 1 | Snake ID |
| 1 | 2 | Snake length (big-endian uint16) |
| 3 | 1 | Direction (0-3) |
| 4 | ... | Body segments: `length` pairs of (uint16 x, uint16 y), head first |

So each alive snake contributes `4 + (length * 4)` bytes.

**PLAYER_DEAD (type `0x30`)** — Sent to a client when their snake dies. Total: 2 bytes.

| Offset | Size | Field |
|:-:|:-:|:--|
| 0 | 1 | Type: `0x30` |
| 1 | 1 | Player ID that died |

**GAME_OVER (type `0x40`)** — Sent when the server shuts down or the game ends. Total: 2 bytes.

| Offset | Size | Field |
|:-:|:-:|:--|
| 0 | 1 | Type: `0x40` |
| 1 | 1 | Winner player ID (or `0xFF` if no winner) |

**ERROR (type `0xF0`)** — Sent when the server rejects a request. Total: 2 bytes.

| Offset | Size | Field |
|:-:|:-:|:--|
| 0 | 1 | Type: `0xF0` |
| 1 | 1 | Error code: `0x01` = game full, `0x02` = already joined, `0x03` = invalid message |


## Protocol Functions (`protocol.c`)

### `int protocol_serialize_welcome(uint8_t *buf, size_t buf_len, int player_id, int board_size, int max_players)`

Serializes a WELCOME message into the provided buffer.

**Parameters:**
- `buf`: Output buffer (must be at least 4 bytes)
- `buf_len`: Size of the output buffer
- `player_id`: Assigned player ID
- `board_size`: Board dimension
- `max_players`: Maximum concurrent players

**Returns:**
- Number of bytes written (4) on success
- `-1` on error (NULL buffer, buffer too small, invalid values)

### `int protocol_serialize_game_state(uint8_t *buf, size_t buf_len, const game_board_t *board)`

Serializes a GAME_STATE message from the current board state.
Iterates through all snakes, includes only alive snakes, and encodes
their full body positions in big-endian format.

**Parameters:**
- `buf`: Output buffer (must be large enough for the full state)
- `buf_len`: Size of the output buffer
- `board`: Pointer to the game board

**Returns:**
- Number of bytes written on success
- `-1` on error

### `int protocol_serialize_dead(uint8_t *buf, size_t buf_len, int player_id)`

Serializes a PLAYER_DEAD message.

**Returns:**
- Number of bytes written (2) on success
- `-1` on error

### `int protocol_serialize_error(uint8_t *buf, size_t buf_len, uint8_t error_code)`

Serializes an ERROR message.

**Returns:**
- Number of bytes written (2) on success
- `-1` on error

### `int protocol_deserialize_client_msg(const uint8_t *buf, size_t buf_len, uint8_t *out_type, uint8_t *out_payload)`

Parses a client message from a 2-byte buffer.

**Parameters:**
- `buf`: Input buffer (must be at least 2 bytes)
- `buf_len`: Size of the input buffer
- `out_type`: Pointer to store the message type
- `out_payload`: Pointer to store the payload byte

**Returns:**
- `0` on success
- `-1` on error (NULL pointers, buffer too small, unknown message type)

> :relieved: This is another point where you can collect significant partial credit.
> The protocol serialization and deserialization functions are self-contained and
> can be fully tested with Criterion unit tests. If your game logic (Part 2) and
> protocol (Part 3) are solid, you will earn a majority of the points even if
> your networking code has issues.


# Part 4: Server Architecture and Threading

This is where everything comes together. You will implement a multi-threaded
server that uses POSIX threads and mutexes to safely manage concurrent access
to the shared game board.

## Architecture Overview

Your server must have the following thread structure:

```
┌─────────────────────────────────────────────────────┐
│                    Main Thread                       │
│  - Parses arguments                                  │
│  - Initializes game board                            │
│  - Creates listening socket on specified port         │
│  - Spawns the Game Loop Thread                       │
│  - Enters accept() loop:                             │
│      For each new connection, spawns a Client Thread  │
└─────────────────────────────────────────────────────┘
         │                              │
         ▼                              ▼
┌─────────────────┐     ┌─────────────────────────────┐
│ Game Loop Thread │     │    Client Thread (1 per      │
│  (1 total)       │     │    connected player)         │
│                  │     │                              │
│  while(running): │     │  - Reads 2-byte messages     │
│    sleep(TICK_MS)│     │    from client socket         │
│    update game   │     │  - Handles JOIN/DIRECTION/   │
│    state         │     │    LEAVE client messages     │
│    broadcast     │     │                              │
│      game state  │     │                              │
└─────────────────┘     └─────────────────────────────┘
```

## Shared State and Synchronization

The `game_board_t` structure is shared between threads. **Your implementation must use a mutex to ensure safe concurrent access to shared game state.**

> :scream: **Race conditions are the #1 source of bugs in this assignment.**
> If you do not synchronize access to shared state, your server may appear to
> work most of the time but will fail unpredictably under load.
> We will compile your code with `-fsanitize=thread` during grading to detect
> data races automatically. If ThreadSanitizer reports a race, you will lose
> points on the concurrency tests even if the output appears correct.

## Server Functions (`server.c`)

### `int server_init(server_t *server, int port, int board_size, int max_players, unsigned int seed)`

Initializes the server state. Creates the TCP listening socket, binds to the
specified port (with `SO_REUSEADDR`), and calls `listen()`. Initializes the
game board via `board_init()`. Initializes the mutex via `pthread_mutex_init()`.

The `server_t` structure is defined in `server.h`:

```c
typedef struct {
    int               listen_fd;                    // Listening socket file descriptor
    game_board_t      board;                        // The shared game board
    pthread_mutex_t   board_mutex;                  // Mutex protecting the board
    int               client_fds[MAX_PLAYERS];      // Connected client socket fds (-1 if empty)
    int               client_snake_ids[MAX_PLAYERS]; // Maps client slot to snake ID (-1 if empty)
    int               running;                      // Server running flag (1 = running, 0 = shutting down)
} server_t;
```

All entries in `client_fds` and `client_snake_ids` should be initialized to `-1`.

**Parameters:**
- `server`: Pointer to server state
- `port`: TCP port number to bind to
- `board_size`: Board dimension
- `max_players`: Maximum concurrent players
- `seed`: Random seed for the game board

**Returns:**
- `0` on success
- `-1` on error

### `void *server_game_loop(void *arg)`

The game loop thread function. This function runs in its own thread and is
responsible for advancing the game state and broadcasting updates.

The loop:
1. Sleep for `TICK_INTERVAL_MS` milliseconds (defined in `global.h`, default 200ms).
   Use `usleep(TICK_INTERVAL_MS * 1000)`.
2. Call `board_tick()` to advance all snakes.
3. Check for dead snakes — for any snake that just died (was alive before tick
   but is now dead), send a `PLAYER_DEAD` message to that snake's client.
4. Serialize the game state with `protocol_serialize_game_state()`.
5. Broadcast the serialized game state to all connected clients by iterating
   through `client_fds` and calling `send()` on each valid fd.

The function should exit when `server->running` is set to `0`.

**Parameters:**
- `arg`: Pointer to `server_t` (cast from `void*`)

**Returns:**
- `NULL`

### `void *server_client_handler(void *arg)`

The client handler thread function. One instance runs per connected client.
You will need to define a struct to pass the server pointer and client fd
to this function (since `pthread_create` only takes one `void*` argument).

The handler:
1. Wait for a JOIN message from the client.
   - If the first message is not JOIN, send an ERROR (`0x03`) and close.
2. Add the player's snake to the game board.
   - If the board is full, send an ERROR (`0x01`) and close.
3. Send a WELCOME message with the assigned player ID, board size, and max players.
4. Register the client fd and snake id in the `client_fds` and `client_snake_ids` arrays.
5. Enter a loop reading 2-byte messages from the client socket:
   - `DIRECTION`: Update the player's direction.
   - `LEAVE`: Break out of the loop.
   - Any invalid message: Send ERROR (`0x03`).
   - If `recv()` returns 0 or error: Client disconnected, break out of loop.
6. On exit (leave or disconnect): Remove the player's snake from the game board.
7. Clear the `client_fds` and `client_snake_ids` entries to `-1`.
8. Close the client socket.
9. Free the argument struct.

**Parameters:**
- `arg`: Pointer to a struct containing the server pointer and client fd

**Returns:**
- `NULL`

### `int server_start(server_t *server)`

The main server loop. This function:
1. Creates the game loop thread (calls `pthread_create` with `server_game_loop`).
2. Enters an `accept()` loop:
   - Accepts a new client connection.
   - Allocates an argument struct with the server pointer and the new client fd.
   - Creates a new detached thread (calls `pthread_create` with `server_client_handler`).
   - The thread should be created detached (`pthread_detach`) so its resources
     are automatically freed when it exits.
3. When `server->running` becomes `0`, breaks out of the loop.

**Parameters:**
- `server`: Pointer to server state

**Returns:**
- `0` on success
- `-1` on error

### `void server_cleanup(server_t *server)`

Cleans up all server resources. Sets `running = 0`, closes all client sockets,
closes the listening socket, frees the board, and releases any synchronization resources you created.

> :nerd_face: A good way to handle `Ctrl+C` (SIGINT) is to set up a signal handler
> that sets `server->running = 0` and then closes the listening socket. This will
> cause the `accept()` call to return with an error, breaking the main loop.


# Part 5: Building and Running

## Compilation

Running `make` will produce the server executable at `bin/snake_server`.
Running `make test` will produce the Criterion test executable at `bin/snake_test`.
Running `make debug` will compile with debugging symbols and the `debug` macro enabled.

To compile with ThreadSanitizer (for checking race conditions yourself):
```
make clean
CFLAGS="-fsanitize=thread -g" make
```

## Running the Server

```bash
# Start server on port 8080 with default settings
bin/snake_server -p 8080

# Start with a specific seed (for reproducible testing)
bin/snake_server -p 8080 -s 1234

# Start with a larger board and more players
bin/snake_server -p 8080 -b 30 -m 6 -s 42
```

## Running the Provided Client

The provided Python client requires `pygame`. Install it with:
```bash
pip3 install pygame
```

Then connect to your running server:
```bash
python3 client/snake_client.py --host 127.0.0.1 --port 8080
```

You can open multiple terminal windows and run multiple clients to test
multiplayer functionality.

## Running Tests

```bash
# Run Criterion unit tests (game logic + protocol)
bin/snake_test --verbose=0

# Run integration test (starts your server, connects a bot)
python3 tests/test_integration.py

# Run concurrency stress test
python3 tests/test_concurrency.py
```


# Part 6: Implementation Notes

You will need to use `malloc` and `free` for the board cells array and for
the per-thread argument structs. All functions that allocate memory document
their memory management requirements in their comments.

For socket programming, you should use `send()` and `recv()` with appropriate
error checking. Note that `recv()` may return fewer bytes than requested —
you **must** handle partial reads by calling `recv()` in a loop until you have
received the expected number of bytes. A helper function for this is strongly
recommended:

```c
// Read exactly `len` bytes from `fd` into `buf`. Returns 0 on success, -1 on error/disconnect.
int recv_exact(int fd, uint8_t *buf, size_t len);
```

Similarly, `send()` may not send all bytes at once. You should handle partial
writes as well.

> :scream: **Deadlock warning**: Be very careful about the order in which you
> acquire locks. In this assignment there is only one mutex, so deadlock from
> lock ordering is not a concern. However, you CAN deadlock if you call a
> blocking function while holding a mutex. Avoid holding a mutex across any operation that may block for an unbounded amount of time.

> :nerd_face: **Tip**: Start by implementing and testing Part 2 (game logic) completely.
> Write your own Criterion tests. Only move to Part 3 (protocol) once your game
> logic is solid. Only move to Part 4 (server) once your protocol functions work.
> This incremental approach will save you many hours of debugging.


## Unit Testing

Unit testing is a part of the development process in which small testable
sections of a program (units) are tested individually to ensure that they are
all functioning properly. This is a very common practice in industry and is
often a requested skill by companies hiring graduates.

> :nerd_face: Some developers consider testing to be so important that they use a
> work flow called test driven development. In TDD, requirements are turned into
> failing unit tests. The goal is then to write code to make these tests pass.

This semester, we will be using a C unit testing framework called
[Criterion](https://github.com/Snaipe/Criterion), which will give you some
exposure to unit testing. We have provided a basic set of test cases for this
assignment.

## Compiling and Running Tests

When you compile your program with `make test`, a `snake_test` executable will be
created in your `bin` directory alongside the normal `snake_server` executable. Running this
executable from the `hw5` directory with the command `bin/snake_test` will run
the unit tests described above and print the test outputs to `stdout`. To obtain
more information about each test run, you can use the verbose print option:
`bin/snake_test --verbose=0`.

The tests we have provided are very minimal and are meant as a starting point
for you to learn about Criterion, not to fully test your homework. You may write
your own additional tests. However, this is not required
for this assignment. Criterion documentation for writing your own tests can be
found [here](http://criterion.readthedocs.io/en/master/).


## Grading Breakdown

Your grade for this assignment is split across three phases:

| Phase | Weight | What is Tested |
|:--|:-:|:--|
| Phase 1: Unit Tests | 30% | Game logic (`snake.c`, `game_board.c`) and protocol (`protocol.c`) tested via Criterion. No networking involved. |
| Phase 2: Integration Tests | 35% | Python bot clients connect to your server over `localhost`, send commands, and verify the received game state matches expected values. Tests single-player and multi-player scenarios. |
| Phase 3: Concurrency Tests | 35% | Your server is compiled with `-fsanitize=thread`. 50 concurrent bot clients stress-test your server. Any data race detected by ThreadSanitizer results in failing the concurrency tests. |

> :scream: **Timeouts**: Because socket programming often leads to deadlocks or infinite
> blocking (e.g., a thread calling `recv()` and never waking up), all integration and
> concurrency tests have strict timeouts. If your server deadlocks, the test will be
> killed and marked as a failure rather than hanging the grading queue.


# Hand-in instructions
**TEST YOUR PROGRAM VIGOROUSLY!**

Make sure that you have implemented all the required functions specified in the
header files.

Make sure that you have adhered to the restrictions (no calls to prohibited
library functions, no modifications to files that say "DO NOT MODIFY" at the beginning,
no functions other than `main()` in `main.c`) set out in this assignment document.

Make sure your directory tree looks basically like it did when you started
(there could possibly be additional files that you added, but the original organization
should be maintained) and that your homework compiles (you should be sure to try compiling
with both `make clean all` and `make clean debug` because there are certain errors that can
occur one way but not the other).

> :nerd_face: When writing your program try to comment as much as possible. Try to
> stay consistent with your formatting. It is much easier for your TA and the
> professor to help you if we can figure out what your code does quickly!
