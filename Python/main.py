import pytesseract
import PIL.Image
import cv2 as cv
import numpy as np
import pyautogui
import re
import serial
import win32gui, win32ui, win32con
from time import time
from pytesseract import Output
from windowcapture import WindowCapture

myconfig = r"--psm 7 --oem 3"
serial = serial.Serial(port='COM12', baudrate=115200)

# Get name of all open windows
def list_window_names():
    window_names = []

    def winEnumHandler(hwnd, ctx):
        if win32gui.IsWindowVisible(hwnd):
            name = win32gui.GetWindowText(hwnd)
            if name:
                window_names.append(name)
    
    win32gui.EnumWindows(winEnumHandler, None)
    return window_names

all_windows = list_window_names()

user_window = input("Enter the name of the window to capture: ")

# look for window matching partial window name from user. Do this because terraria changes its window name every time its opened. Eg: Terraria: The Grass is Greener on This Side, Terraria: Earthbound
matching_windows = [win for win in all_windows if user_window.lower() in win.lower()]

if matching_windows:
    # Use the first matching window name
    user_window = matching_windows[0]
    print(f"Found window: {user_window}")
    wincap = WindowCapture(user_window)
else:
    print("No window found with that name.")


loop_time = time()

# flag to check if default color has been printed already
printed_default = False

previous_color = 0

while(True):
    screenshot = wincap.get_screenshot()

    #debug the loop rate
    #print('FPS {}'.format(1/ (time()-loop_time)))

    loop_time = time()

    # Convert the image to grayscale
    screenshot = cv.cvtColor(screenshot, cv.COLOR_BGR2GRAY)

    # Initialize MSER
    mser = cv.MSER_create()

    # Detect regions in the screenshot
    regions, _ = mser.detectRegions(screenshot)

    # Create a blank mask with the same size as the screenshot
    mask = np.zeros_like(screenshot, dtype=np.uint8)

    # Iterate over each region and draw its convex hull on the mask
    for region in regions:
        hull = cv.convexHull(np.array(region, dtype=np.float32))  # Apply convex hull to each region
        hull = np.array(hull, dtype=np.int32)  # Convert hull to integer type
        cv.fillConvexPoly(mask, hull, 255)     # Fill the convex hull on the mask

    # Apply the mask to the screenshot
    result_img = cv.bitwise_and(screenshot, screenshot, mask=mask)
    cv.imshow('Computer Vision', result_img)

    # read text from screenshot 
    text = pytesseract.image_to_string(result_img,config=myconfig)
    
    # hsv threshold to only show the white text containing player health but MSER technique gave better results
    ''' 
    # Load image, convert to HSV, color threshold to get mask
    image = screenshot
    hsv = cv.cvtColor(image, cv.COLOR_BGR2HSV)
    lower = np.array([0, 0, 168])
    upper = np.array([172, 111, 255])
    mask = cv.inRange(hsv, lower, upper)

    # Invert image and OCR
    invert = 255 - mask
    text = pytesseract.image_to_string(invert, config=myconfig)
    cv.imshow('Computer Vision', invert)    
    print(text)
    '''
    
    # separate the text read into 2 numbers. eg. 500/500 -> ['500','500']
    health = re.findall(r'\d+', text)
    
    # if player health is read
    if len(health) >= 2:
        try:
            playerHealth = int(health[0])
            totalHealth = int(health[1])

            if totalHealth != 0:  # Prevent division by zero
                percentage = (playerHealth / totalHealth) * 100
            else:
                # print("Error getting health")
                # percentage = 0  
                pass

            if percentage >= 80:
                color = 3 # turn on all LEDs

            if percentage < 80 and percentage >= 50:
                color = 2 # turn on yellow and red LEDs

            if percentage < 50:
                color = 1 # turn on red LED

            # Print color only if the health has changed to avoid printing the same number over and over
            if color != previous_color:
                print(color) # print to terminal 
                serial.write(str(color).encode()) # print to the serial port

                previous_color = color  # Update previous color to new color 
                printed_default = False
      
        except:
            pass
    # if no player health is read then print the default color which is 0 so no LEDs turn on       
    else:
        if not printed_default:
            print(previous_color)
            serial.write(str(previous_color).encode())
            printed_default = True  # flag == True to only print once
    

    #press 'q' with the output window focused to exit
    #wait 1ms eveyr loop to process key presses
    if cv.waitKey(1) == ord('q'):
        cv.destroyAllWindows()
        break

print('Done')
