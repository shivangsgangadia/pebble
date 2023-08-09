import socket
from socket import timeout
import time
import io
from PIL import Image, UnidentifiedImageError
import threading
import argparse
import re
import copy
import os

# Adding a bit of professional touch
os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = "hide"
import pygame
import structures
from constants import *

ui_elements = [None for _ in range(UI_ELEMENTS_COUNT)]
buttons = [None for _ in range(BUTTON_COUNT)]
button_commands = [False for _ in range(BUTTON_COUNT)]

QUIT_FLAG = False
quit_lock = threading.Lock()  # Lock on QUIT_FLAG

image_buffer = None
image_size = ()
packet_loss = 0
network_cycle_time = 0
ui_data_lock = threading.Lock()

key_down_function_map = {
    pygame.K_s: TRANSLATE_FORWARD,
    pygame.K_w: TRANSLATE_BACKWARD,
    pygame.K_a: TRANSLATE_WITH_LEFT,
    pygame.K_d: TRANSLATE_WITH_RIGHT,
    pygame.K_e: TURN_IN_PLACE_LEFT,
    pygame.K_q: TURN_IN_PLACE_RIGHT,
    pygame.K_COMMA: CAMERA_TURN_RIGHT,
    pygame.K_PERIOD: CAMERA_TURN_LEFT,

}

key_press_flag = {
    pygame.K_w: False,
    pygame.K_s: False,
    pygame.K_a: False,
    pygame.K_d: False,
    pygame.K_q: False,
    pygame.K_e: False,
    pygame.K_COMMA: False,
    pygame.K_PERIOD: False,
}


key_up_function_map = {
    pygame.K_w: TRANSLATE_NOT,
    pygame.K_s: TRANSLATE_NOT,
    pygame.K_a: TRANSLATE_STRAIGHT,
    pygame.K_d: TRANSLATE_STRAIGHT,
    pygame.K_q: TURN_IN_PLACE_DONT,
    pygame.K_e: TURN_IN_PLACE_DONT,
    pygame.K_COMMA: TURN_IN_PLACE_DONT,
    pygame.K_PERIOD: TURN_IN_PLACE_DONT,
}


class ImageStreamer(threading.Thread):
    """
    Since Image streaming deals with the display surface, this class will have all data that may be required to
    be displayed on screen
    """

    def __init__(self, server_addr):
        super().__init__()
        self.server_addr = server_addr

    def run(self) -> None:
        global QUIT_FLAG
        global quit_lock, ui_data_lock, image_buffer, image_size, packet_loss, network_cycle_time
        crashed = False
        print()
        message = b'I0000'

        total_packet_count = 0
        lost_packet_count = 0

        clock = pygame.time.Clock()

        while not crashed:
            # Check if we need to exit
            if quit_lock.acquire():
                if QUIT_FLAG:
                    crashed = True
                quit_lock.release()

            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(IMAGE_SERVER_TIMEOUT)
                sock.connect(self.server_addr)
                sock.send(message)
                total_packet_count += 1

                last_frame_time = time.time()
                image_data = []
                while True:
                    chunk = sock.recv(1024)
                    if not chunk:
                        break
                    image_data.append(chunk)

                sock.close()
                image_data_stream = io.BytesIO(b"".join(image_data))
                img = Image.open(image_data_stream)

                # Calculate time
                frame_time = (time.time() - last_frame_time) * 1000

                # Copy data to buffers
                if ui_data_lock.acquire():
                    image_buffer = copy.copy(img.tobytes("raw", "RGB"))
                    image_size = img.size
                    packet_loss = (lost_packet_count / total_packet_count) * 100
                    network_cycle_time = frame_time
                    ui_data_lock.release()

                # Reset to avoid time lost in large calculations
                if total_packet_count > 10000:
                    total_packet_count = 0
                    lost_packet_count = 0

            except UnidentifiedImageError as e:
                continue
            except UnicodeDecodeError and ValueError as f:
                continue
            except timeout as se:
                print("Timed out")
            except OSError as o:
                # print(o)
                lost_packet_count += 1
                continue
            except IndexError as ie:
                continue
            finally:
                clock.tick(25)

        print("Exiting Image loop")


class CommandAndUIHandler:
    """
    Handles all events in the pygame event queue. Does not deal with the UI elements at all
    """

    def __init__(self, command_server):
        self.command_server = command_server
        self.command_bytes = bytearray(2)

    def run(self) -> None:
        global QUIT_FLAG, key_down_function_map, key_up_function_map, key_press_flag
        global quit_lock, ui_data_lock, ui_elements, image_buffer, image_size, packet_loss, network_cycle_time, buttons, button_commands
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        ui_elements[IP_TEXT_DISPLAY_INDEX].render_text(self.command_server[0])
        keep_running = True

        # Render interactive elements once
        for button in buttons:
            button.render()

        while keep_running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    if quit_lock.acquire(timeout=0.05):
                        QUIT_FLAG = True
                        quit_lock.release()
                        keep_running = False
                elif event.type == pygame.KEYDOWN:
                    try:
                        key_press_flag[event.key] = True
                        # self.command_bytes[0] |= key_down_function_map[event.key]
                    except KeyError:
                        pass

                elif event.type == pygame.KEYUP:
                    try:
                        key_press_flag[event.key] = False
                        # self.command_bytes[0] |= key_up_function_map[event.key]
                    except KeyError:
                        pass

                elif event.type == pygame.MOUSEBUTTONDOWN:
                    for button in buttons:
                        button.check_pressed()

                elif event.type == pygame.MOUSEBUTTONUP:
                    for button in buttons:
                        button.check_released()

            for i in range(BUTTON_COUNT):
                if button_commands[i]:
                    button_commands[i] = False
                    self.command_bytes[1] |= buttons[i].command_mask
                    # print("pressed ", buttons[i].command_mask)

            # print("loop")
            for key in key_down_function_map:
                if key_press_flag[key]:
                    self.command_bytes[0] |= key_down_function_map[key]

            sock.sendto(self.command_bytes, self.command_server)
            # print(self.command_bytes[0])
            # print("x axis state : " + str(X_AXIS_STATE))
            self.command_bytes[0] = 0
            self.command_bytes[1] = 0

            # Handle UI
            if ui_data_lock.acquire(timeout=0.05):
                if image_buffer is not None:
                    ui_elements[IMAGE_DISPLAY_INDEX].render(pygame.image.frombytes(image_buffer, image_size, "RGB"))
                ui_elements[PACKET_LOSS_DISPLAY_INDEX].render_text("Packet Loss {:.1f} %".format(packet_loss))
                ui_elements[CYCLE_TIME_DISPLAY_INDEX].render_text("Image time {:.1f} ms".format(network_cycle_time))
                ui_data_lock.release()

            pygame.display.flip()

        print("Exiting event loop")


def is_valid_ip(inp):
    regex = '''^(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\.( 
            25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\.( 
            25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\.( 
            25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)$'''
    if re.search(regex, inp):
        return True
    else:
        return False


def get_ip_from_args():
    args = argparse.ArgumentParser()
    args.add_argument("--serverip", dest="ip", help="Enter the IP address of the server", type=str)

    return args.parse_args().ip


def on_click(button_index):
    global button_commands
    button_commands[button_index] = True


def init_ui(display):
    global ui_elements
    font = pygame.font.Font("freesansbold.ttf", FONT_SIZE)
    ui_elements[PACKET_LOSS_DISPLAY_INDEX] = structures.UI_element(
        display, PACKET_LOSS_DISPLAY_LOCATION,
        font, TEXT_DISPLAY_COLOR
    )
    ui_elements[IP_TEXT_DISPLAY_INDEX] = structures.UI_element(
        display, IP_TEXT_DISPLAY_LOCATION,
        font, TEXT_DISPLAY_COLOR
    )
    ui_elements[CYCLE_TIME_DISPLAY_INDEX] = structures.UI_element(
        display, CYCLE_TIME_DISPLAY_LOCATION,
        font, TEXT_DISPLAY_COLOR
    )
    ui_elements[IMAGE_DISPLAY_INDEX] = structures.UI_element(
        display, IMAGE_DISPLAY_LOCATION,
    )

    buttons[OPEN_BUTTON_INDEX] = structures.Button(
        display, "Open Bot", OPEN_BUTTON_LOCATION, on_click, OPEN_BUTTON_INDEX, OPEN_PEBBLE
    )

    buttons[MOVE_BUTTON_INDEX] = structures.Button(
        display, "Move mode", MOVE_BUTTON_LOCATION, on_click, MOVE_BUTTON_INDEX, MOVE_PEBBLE
    )

    buttons[CLOSE_BUTTON_INDEX] = structures.Button(
        display, "Close Bot", CLOSE_BUTTON_LOCATION, on_click, CLOSE_BUTTON_INDEX, CLOSE_PEBBLE
    )

    buttons[INCREMENT_STRIDE_LENGTH_INDEX] = structures.Button(
        display, "Step Length +", (INCREMENT_BUTTONS_LOCATION_X, INCREMENT_BUTTONS_LOCATION_Y), on_click,
        INCREMENT_STRIDE_LENGTH_INDEX, INCREMENT_STRIDE_LENGTH
    )

    buttons[DECREMENT_STRIDE_LENGTH_INDEX] = structures.Button(
        display, "Step Length -", (INCREMENT_BUTTONS_LOCATION_X + 150, INCREMENT_BUTTONS_LOCATION_Y),
        on_click, DECREMENT_STRIDE_LENGTH_INDEX, DECREMENT_STRIDE_LENGTH
    )

    buttons[INCREMENT_STRIDE_HEIGHT_INDEX] = structures.Button(
        display, "Step Height +", (INCREMENT_BUTTONS_LOCATION_X, INCREMENT_BUTTONS_LOCATION_Y + 50), on_click,
        INCREMENT_STRIDE_HEIGHT_INDEX, INCREMENT_STRIDE_HEIGHT
    )

    buttons[DECREMENT_STRIDE_HEIGHT_INDEX] = structures.Button(
        display, "Step Height -", (INCREMENT_BUTTONS_LOCATION_X + 150, INCREMENT_BUTTONS_LOCATION_Y + 50),
        on_click, DECREMENT_STRIDE_HEIGHT_INDEX, DECREMENT_STRIDE_HEIGHT
    )

    buttons[INCREMENT_INCLINE_INDEX] = structures.Button(
        display, "Incline +", (INCREMENT_BUTTONS_LOCATION_X, INCREMENT_BUTTONS_LOCATION_Y + 100),
        on_click, INCREMENT_INCLINE_INDEX,
        INCREMENT_INCLINE
    )

    buttons[DECREMENT_INCLINE_INDEX] = structures.Button(
        display, "Incline -", (INCREMENT_BUTTONS_LOCATION_X + 150, INCREMENT_BUTTONS_LOCATION_Y + 100),
        on_click, DECREMENT_INCLINE_INDEX, DECREMENT_INCLINE
    )


if __name__ == '__main__':
    ip = get_ip_from_args()
    if ip is None or not is_valid_ip(ip):
        print("No valid IP addr supplied")
        quit()

    image_server_socket = (str(ip), IMAGE_SERVER_PORT)
    command_server_socket = (str(ip), COMMAND_SERVER_PORT)

    pygame.init()
    game_display = pygame.display.set_mode((DISPLAY_WIDTH, DISPLAY_HEIGHT))
    socket.setdefaulttimeout(0.5)

    init_ui(game_display)

    image_streamer = ImageStreamer(image_server_socket)
    image_streamer.start()

    command_and_ui_thread = CommandAndUIHandler(command_server_socket)
    command_and_ui_thread.run()  # Must run on main Thread to ensure compatibility with Windows

    image_streamer.join()

    pygame.quit()
    quit()
