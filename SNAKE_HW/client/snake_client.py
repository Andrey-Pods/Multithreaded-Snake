#!/usr/bin/env python3.12
"""
Snake Game Client - CSE 320 HW2

A graphical client that connects to the student's snake server and
renders the game in real time using pygame.

Usage:
    python3 client/snake_client.py --host 127.0.0.1 --port 8080
    --OR--
    python3.12 client/snake_client.py --host 127.0.0.1 --port 8080

Requirements:
    pip3 install pygame
    --OR--
    python3.12 -m pip install pygame "setuptools>=68"

On Python 3.12, Debian/Ubuntu's system setuptools is often too old: it still uses
pkgutil.ImpImporter (removed in 3.12), which breaks pygame via pkg_resources.
Upgrading setuptools for 3.12 (as above) fixes that.

This file is provided for your testing convenience.
It is NOT graded and you should NOT modify it.
"""

import argparse
import socket
import struct
import sys
import threading
import time

try:
    import pygame
except ImportError:
    print("Error: pygame is required. Install with: python3 -m pip install pygame")
    sys.exit(1)
except AttributeError as e:
    if "ImpImporter" in str(e):
        print(
            "Error: old setuptools/pkg_resources is incompatible with Python 3.12 "
            "(pkgutil.ImpImporter was removed). Upgrade setuptools, then retry:\n"
            '    python3.12 -m pip install --upgrade "setuptools>=68"'
        )
        sys.exit(1)
    raise

# ---------------------------------------------------------------------------
# Protocol constants (must match global.h)
# ---------------------------------------------------------------------------
MSG_JOIN        = 0x01
MSG_DIRECTION   = 0x02
MSG_LEAVE       = 0x03

MSG_WELCOME     = 0x10
MSG_GAME_STATE  = 0x20
MSG_PLAYER_DEAD = 0x30
MSG_GAME_OVER   = 0x40
MSG_ERROR       = 0xF0

DIR_UP    = 0
DIR_DOWN  = 1
DIR_LEFT  = 2
DIR_RIGHT = 3

# ---------------------------------------------------------------------------
# Colors for up to 8 players
# ---------------------------------------------------------------------------
SNAKE_COLORS = [
    (0, 200, 0),     # Green
    (0, 100, 255),   # Blue
    (255, 50, 50),   # Red
    (255, 200, 0),   # Yellow
    (200, 0, 200),   # Purple
    (0, 200, 200),   # Cyan
    (255, 128, 0),   # Orange
    (255, 100, 200), # Pink
]

BG_COLOR    = (30, 30, 30)
WALL_COLOR  = (80, 80, 80)
APPLE_COLOR = (255, 0, 0)
GRID_COLOR  = (45, 45, 45)
TEXT_COLOR   = (220, 220, 220)

CELL_SIZE = 24


# ---------------------------------------------------------------------------
# Network helpers
# ---------------------------------------------------------------------------
def recv_exact(sock, n):
    data = b""
    while len(data) < n:
        chunk = sock.recv(n - len(data))
        if not chunk:
            raise ConnectionError("Server closed connection")
        data += chunk
    return data


def send_msg(sock, msg_type, payload=0x00):
    sock.sendall(bytes([msg_type, payload]))


# ---------------------------------------------------------------------------
# Client class
# ---------------------------------------------------------------------------
class SnakeClient:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = None
        self.player_id = -1
        self.board_size = 20
        self.max_players = 4
        self.game_state = None
        self.alive = True
        self.connected = False
        self.lock = threading.Lock()
        self.error_msg = None

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(5)
        try:
            self.sock.connect((self.host, self.port))
        except (ConnectionRefusedError, socket.timeout) as e:
            self.error_msg = f"Could not connect to {self.host}:{self.port} — {e}"
            return False

        # Send JOIN
        send_msg(self.sock, MSG_JOIN)

        # Receive welcome
        try:
            data = recv_exact(self.sock, 4)
        except Exception as e:
            self.error_msg = f"Failed to receive welcome: {e}"
            return False

        if data[0] == MSG_ERROR:
            error_names = {0x01: "Game Full", 0x02: "Already Joined", 0x03: "Invalid Message"}
            self.error_msg = f"Server error: {error_names.get(data[1], f'0x{data[1]:02x}')}"
            return False

        if data[0] != MSG_WELCOME:
            self.error_msg = f"Unexpected response: 0x{data[0]:02x}"
            return False

        self.player_id = data[1]
        self.board_size = data[2]
        self.max_players = data[3]
        self.connected = True
        self.sock.settimeout(2)
        return True

    def send_direction(self, direction):
        if self.connected and self.sock:
            try:
                send_msg(self.sock, MSG_DIRECTION, direction)
            except Exception:
                pass

    def receive_loop(self):
        """Background thread: continuously read server messages."""
        while self.connected:
            try:
                msg_type_byte = recv_exact(self.sock, 1)
                msg_type = msg_type_byte[0]

                if msg_type == MSG_GAME_STATE:
                    # Read rest of header
                    hdr = recv_exact(self.sock, 5)
                    num_snakes = hdr[0]
                    apple_x = struct.unpack("!H", hdr[1:3])[0]
                    apple_y = struct.unpack("!H", hdr[3:5])[0]

                    snakes = []
                    for _ in range(num_snakes):
                        shdr = recv_exact(self.sock, 4)
                        sid = shdr[0]
                        slen = struct.unpack("!H", shdr[1:3])[0]
                        sdir = shdr[3]
                        body_data = recv_exact(self.sock, slen * 4)
                        body = []
                        for i in range(slen):
                            bx = struct.unpack("!H", body_data[i*4:i*4+2])[0]
                            by = struct.unpack("!H", body_data[i*4+2:i*4+4])[0]
                            body.append((bx, by))
                        snakes.append({"id": sid, "length": slen,
                                       "direction": sdir, "body": body})

                    with self.lock:
                        self.game_state = {
                            "num_snakes": num_snakes,
                            "apple": (apple_x, apple_y),
                            "snakes": snakes,
                        }

                elif msg_type == MSG_PLAYER_DEAD:
                    data = recv_exact(self.sock, 1)
                    dead_id = data[0]
                    if dead_id == self.player_id:
                        with self.lock:
                            self.alive = False

                elif msg_type == MSG_GAME_OVER:
                    recv_exact(self.sock, 1)
                    self.connected = False

                elif msg_type == MSG_ERROR:
                    recv_exact(self.sock, 1)

            except socket.timeout:
                continue
            except Exception:
                self.connected = False
                break

    def disconnect(self):
        if self.sock:
            try:
                send_msg(self.sock, MSG_LEAVE)
            except Exception:
                pass
            try:
                self.sock.close()
            except Exception:
                pass
        self.connected = False


# ---------------------------------------------------------------------------
# Pygame rendering
# ---------------------------------------------------------------------------
def run_game(client):
    board_px = client.board_size * CELL_SIZE
    info_height = 60
    width = board_px
    height = board_px + info_height

    pygame.init()
    screen = pygame.display.set_mode((width, height))
    pygame.display.set_caption(f"Snake — Player {client.player_id}")
    clock = pygame.time.Clock()
    font = pygame.font.SysFont("monospace", 16)

    # Start receive thread
    recv_thread = threading.Thread(target=client.receive_loop, daemon=True)
    recv_thread.start()

    running = True
    while running and client.connected:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_UP or event.key == pygame.K_w:
                    client.send_direction(DIR_UP)
                elif event.key == pygame.K_DOWN or event.key == pygame.K_s:
                    client.send_direction(DIR_DOWN)
                elif event.key == pygame.K_LEFT or event.key == pygame.K_a:
                    client.send_direction(DIR_LEFT)
                elif event.key == pygame.K_RIGHT or event.key == pygame.K_d:
                    client.send_direction(DIR_RIGHT)
                elif event.key == pygame.K_ESCAPE or event.key == pygame.K_q:
                    running = False

        # --- Draw ---
        screen.fill(BG_COLOR)

        # Grid lines
        for x in range(0, board_px, CELL_SIZE):
            pygame.draw.line(screen, GRID_COLOR, (x, 0), (x, board_px))
        for y in range(0, board_px, CELL_SIZE):
            pygame.draw.line(screen, GRID_COLOR, (0, y), (board_px, y))

        # Walls (border)
        bs = client.board_size
        for x in range(bs):
            for y in [0, bs - 1]:
                rect = pygame.Rect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE)
                pygame.draw.rect(screen, WALL_COLOR, rect)
        for y in range(bs):
            for x in [0, bs - 1]:
                rect = pygame.Rect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE)
                pygame.draw.rect(screen, WALL_COLOR, rect)

        with client.lock:
            state = client.game_state

        if state:
            # Apple
            ax, ay = state["apple"]
            apple_rect = pygame.Rect(ax * CELL_SIZE + 2, ay * CELL_SIZE + 2,
                                     CELL_SIZE - 4, CELL_SIZE - 4)
            pygame.draw.rect(screen, APPLE_COLOR, apple_rect, border_radius=6)

            # Snakes
            for snake in state["snakes"]:
                color = SNAKE_COLORS[snake["id"] % len(SNAKE_COLORS)]
                for i, (bx, by) in enumerate(snake["body"]):
                    rect = pygame.Rect(bx * CELL_SIZE + 1, by * CELL_SIZE + 1,
                                       CELL_SIZE - 2, CELL_SIZE - 2)
                    pygame.draw.rect(screen, color, rect, border_radius=3)
                    if i == 0:
                        # Draw eyes on head
                        cx = bx * CELL_SIZE + CELL_SIZE // 2
                        cy = by * CELL_SIZE + CELL_SIZE // 2
                        pygame.draw.circle(screen, (255, 255, 255),
                                           (cx - 4, cy - 3), 3)
                        pygame.draw.circle(screen, (255, 255, 255),
                                           (cx + 4, cy - 3), 3)
                        pygame.draw.circle(screen, (0, 0, 0),
                                           (cx - 4, cy - 3), 1)
                        pygame.draw.circle(screen, (0, 0, 0),
                                           (cx + 4, cy - 3), 1)

        # Info bar
        info_y = board_px + 5
        my_color = SNAKE_COLORS[client.player_id % len(SNAKE_COLORS)]
        status = "ALIVE" if client.alive else "DEAD"
        status_color = (0, 255, 0) if client.alive else (255, 0, 0)

        id_surf = font.render(f"Player {client.player_id}", True, my_color)
        screen.blit(id_surf, (10, info_y))

        status_surf = font.render(status, True, status_color)
        screen.blit(status_surf, (10, info_y + 22))

        if state:
            score = 0
            for s in state["snakes"]:
                if s["id"] == client.player_id:
                    score = s["length"]
            score_surf = font.render(f"Length: {score}", True, TEXT_COLOR)
            screen.blit(score_surf, (150, info_y))

            players_surf = font.render(f"Players: {state['num_snakes']}", True, TEXT_COLOR)
            screen.blit(players_surf, (150, info_y + 22))

        pygame.display.flip()
        clock.tick(30)

    client.disconnect()
    pygame.quit()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description="Snake Game Client")
    parser.add_argument("--host", default="127.0.0.1", help="Server host")
    parser.add_argument("--port", type=int, required=True, help="Server port")
    args = parser.parse_args()

    client = SnakeClient(args.host, args.port)
    print(f"Connecting to {args.host}:{args.port}...")

    if not client.connect():
        print(f"Failed to connect: {client.error_msg}")
        sys.exit(1)

    print(f"Connected! Player ID: {client.player_id}, "
          f"Board: {client.board_size}x{client.board_size}, "
          f"Max players: {client.max_players}")
    print("Controls: Arrow keys or WASD to move, Q/Esc to quit")

    run_game(client)
    print("Disconnected.")


if __name__ == "__main__":
    main()
