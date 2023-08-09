# Depends on pygame being pre-imported
import pygame
import constants

class UI_element:
    def __init__(self, display, location_tuple, font=None, text_color_tuple=None):
        self.display = display
        self.location_tuple = location_tuple
        self.font = font
        self.bg_color_tuple = (0, 0, 0)
        self.text_color_tuple = text_color_tuple

    def clear_area(self, size):
        pygame.draw.rect(
            self.display,
            self.bg_color_tuple,
            pygame.Rect(self.location_tuple, size)
        )

    def render_text(self, text):
        text_element = self.font.render(text, True, self.text_color_tuple)
        self.clear_area(text_element.get_size())
        self.display.blit(text_element, self.location_tuple)

    def render(self, data):
        self.display.blit(data, self.location_tuple)


class Button:
    def __init__(self, display, text, location, on_click, index, command_mask):
        self.display = display
        self.location = location
        self.font = pygame.font.Font("freesansbold.ttf", constants.FONT_SIZE_BUTTON)
        self.text = self.font.render(text, True, constants.TEXT_DISPLAY_COLOR_BUTTON_1)
        self.bg_color = constants.BG_COLOR_BUTTON_1
        self.padding = constants.PADDING_BUTTON * 2
        self.size = (self.text.get_size()[0] + self.padding, self.text.get_size()[1] + self.padding)
        self.surface_area = pygame.Rect(
            (self.location[0] - constants.PADDING_BUTTON, self.location[1] - constants.PADDING_BUTTON),
            self.size,
        )
        self.executed = False
        self.is_pressed = False
        self.toggle_enabled = False
        self.on_click = on_click
        self.index = index
        self.command_mask = command_mask

    def render(self):
        if self.is_pressed:
            self.bg_color = constants.BG_COLOR_BUTTON_1
        else:
            self.bg_color = constants.BG_COLOR_BUTTON_2

        # Clear background
        pygame.draw.rect(
            self.display,
            (0, 0, 0),
            self.surface_area
        )
        pygame.draw.rect(
            self.display,
            self.bg_color,
            self.surface_area
        )
        self.display.blit(self.text, self.location)

    def check_pressed(self):
        """ Called if mousebutton down """
        x, y = pygame.mouse.get_pos()
        if self.surface_area.collidepoint(x, y):
            if not self.executed:
                self.is_pressed = True
                self.on_click(self.index)
                self.executed = True
            # self.render()

    def check_released(self):
        """ Called if mousebutton up """
        x, y = pygame.mouse.get_pos()
        if self.executed:
            self.executed = False
            self.is_pressed = False
            # self.render()
