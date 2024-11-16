import numpy as np
import win32gui, win32ui, win32con
import cv2 as cv
import pytesseract
import PIL.Image
import pyautogui
import re
import serial
from time import time
from pytesseract import Output


class WindowCapture:
    # properties
    w = 0
    h = 0
    hwnd = None
    crop_x = 0
    crop_y = 0

    # constructor
    def __init__(self, window_name):
        # name of window to capture
        self.hwnd = win32gui.FindWindow(None, window_name)
        if not self.hwnd:
            raise Exception('Window not found: {}'.format(window_name))

        # Get screen size
        window_rect = win32gui.GetWindowRect(self.hwnd)
        self.w = window_rect[2] - window_rect[0]
        self.h = window_rect[3] - window_rect[1]



        # Focus on the health bar only
        side_pixels = 10
        border_pixels = 40

        self.w = self.w - (border_pixels * 2)
        self.h = self.h - border_pixels - side_pixels 
         
        self.crop_x = side_pixels
        self.crop_y = border_pixels 

    def get_screenshot(self):

        wDC = win32gui.GetWindowDC(self.hwnd)
        dcObj = win32ui.CreateDCFromHandle(wDC)
        cDC = dcObj.CreateCompatibleDC()
        dataBitMap = win32ui.CreateBitmap()
        dataBitMap.CreateCompatibleBitmap(dcObj, self.w, self.h)
        cDC.SelectObject(dataBitMap)
        cDC.BitBlt((0, 0), (self.w, self.h), dcObj, (self.crop_x, self.crop_y), win32con.SRCCOPY)

        signedIntsArray = dataBitMap.GetBitmapBits(True)
        img = np.fromstring(signedIntsArray, dtype='uint8')
        img.shape = (self.h, self.w, 4)

        # Free Resources
        dcObj.DeleteDC()
        cDC.DeleteDC()
        win32gui.ReleaseDC(self.hwnd, wDC)
        win32gui.DeleteObject(dataBitMap.GetHandle())

        # drop the alpha channel, or cv.matchTemplate() will throw an error 
        img = img[...,:3]

        # make image C_CONTIGUOUS to avoid errors
        img = np.ascontiguousarray(img)

        # crop the image to focus on the health bar

        # for full screen
        img = img[0:540, self.w - 300:self.w]
        img = img[0:40, 0:280]
        
        # for small windowed screen
        '''
        img = img[0:530, self.w - 922:self.w]
        img = img[0:35]
        '''

        return img



