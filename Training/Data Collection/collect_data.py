from datetime import datetime
import sys
import csv
import serial
import pygame
import pygame.locals
from pygame.locals import *

ser = serial.Serial(sys.argv[1], 115200, timeout=0.1)
ser.write(b"E")

MAX_DISTANCE = 400

PIXEL_SIZE = 64
WIDTH = 8
SCREEN_X = PIXEL_SIZE * WIDTH
SCREEN_Y = PIXEL_SIZE * WIDTH

SAVE_RECORD_COUNT = 8

pygame.init()
screen = pygame.display.set_mode((SCREEN_X, SCREEN_Y))
square = pygame.Surface((PIXEL_SIZE, PIXEL_SIZE))


history = []
rows = []

prev = None
delta = [0] * 64

new_sample_count = 0

save_symbol = False
save_count = 0

with open(
    f"data/{datetime.now().isoformat(timespec='seconds').replace(':', '-')}.csv",
    "w",
    newline="",
) as f:
    csvwriter = csv.writer(f, delimiter=",", quoting=csv.QUOTE_MINIMAL)
    csvwriter.writerow(
        ["symbol", "time"]
        + [f"dist({x},{y})" for y in range(8) for x in range(8)]
        + [f"sigma({x},{y})" for y in range(8) for x in range(8)]
    )

    font = pygame.font.Font(pygame.font.get_default_font(), 12)
    while True:
        if b"\r\n" in (line := ser.readline()):
            if b"Inference: " in line:
                # print(line)
                pass
            elif len(line.strip()) > 0:
                vals = list(map(float, line.split(b",")[:-1]))
                distances = vals[0:64]
                deltas = vals[64:128]

                if len(distances) == 64:
                    new_row = [datetime.now().isoformat()] + distances + deltas
                    history.append(new_row)
                    del history[:-SAVE_RECORD_COUNT]

                    if save_count > 0:
                        rows.append([save_symbol] + new_row)
                        save_count -= 1
                        if save_count == 0:
                            print("done")
                        else:
                            print(f"Saved {save_symbol} {len(rows)}")

                    new_sample_count += 1

                    c = 0
                    for y in range(0, SCREEN_Y, PIXEL_SIZE):
                        for x in range(0, SCREEN_X, PIXEL_SIZE):
                            color = 255 * min(distances[c], MAX_DISTANCE) / MAX_DISTANCE
                            square.fill((color, color, color))
                            draw_me = pygame.Rect(x, y, PIXEL_SIZE, PIXEL_SIZE)
                            screen.blit(square, draw_me)

                            text = font.render(f"{distances[c]}", True, (0, 0, 0))
                            text_pos = text.get_rect(
                                center=(x + PIXEL_SIZE / 2, y + PIXEL_SIZE * 0.3)
                            )
                            screen.blit(text, text_pos)

                            text = font.render(f"{deltas[c]}", True, (0, 0, 0))
                            text_pos = text.get_rect(
                                center=(x + PIXEL_SIZE / 2, y + PIXEL_SIZE * 0.6)
                            )
                            screen.blit(text, text_pos)

                            c += 1

        keypad = [
            pygame.locals.K_KP0,
            pygame.locals.K_KP1,
            pygame.locals.K_KP2,
            pygame.locals.K_KP3,
            pygame.locals.K_KP4,
            pygame.locals.K_KP5,
            pygame.locals.K_KP6,
            pygame.locals.K_KP7,
            pygame.locals.K_KP8,
            pygame.locals.K_KP9,
        ]
        for event in pygame.event.get():
            if event.type == pygame.KEYDOWN:
                if (
                    event.key >= ord("0") and event.key <= ord("9")
                ) or event.key in keypad:
                    if new_sample_count >= SAVE_RECORD_COUNT:
                        if event.key >= ord("0") and event.key <= ord("9"):
                            symbol = event.key - ord("0")
                        else:
                            symbol = keypad.index(event.key)

                        save_symbol = symbol
                        save_count = SAVE_RECORD_COUNT
                        # for i in range(SAVE_RECORD_COUNT):
                        #     rows.append([symbol] + history[-(i + 1)])

                        # print(f"saved {symbol=} {len(rows)=}")
                        # new_sample_count = 0
                    else:
                        print("not enough new samples")
                elif event.key == pygame.K_d:
                    del rows[-SAVE_RECORD_COUNT:]
                    print(f"deleted, {len(rows)=}")
                elif event.key == pygame.K_s:
                    csvwriter.writerows(rows)
                    print(f"saved {len(rows)} rows")
                    rows = []
            elif event.type == QUIT:
                csvwriter.writerows(rows)
                print(f"saved {len(rows)} rows")
                rows = []
                pygame.quit()
                sys.exit()
        pygame.display.update()
