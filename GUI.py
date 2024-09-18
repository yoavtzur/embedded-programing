import PySimpleGUI as sg
import serial as ser
import sys, glob
import time
import serial.tools.list_ports
from tkinter import *
from tkinter.colorchooser import askcolor
import mouse
import os
from os import path
import threading
import binascii
import pyautogui


class Paint(object):
    DEFAULT_PEN_SIZE = 5.0
    DEFAULT_COLOR = 'black'

    def __init__(self):
        self.root = Tk()

        self.brush_button = Button(self.root, text='Back', command=self.close_painter, bg='lightblue',
                font=('Arial', 12, 'bold'))
        self.brush_button.grid(row=0, column=0)
        self.c = Canvas(self.root, bg='white', width=700, height=700)
        self.c.grid(row=1, columnspan=1)
        self.setup()
        self.root.mainloop()


    def setup(self):
        self.old_x = None
        self.old_y = None
        self.line_width = self.DEFAULT_PEN_SIZE
        self.color = self.DEFAULT_COLOR
        self.eraser_on = False
        self.active_button =None
        if self.c:
            self.c.bind('<Motion>', self.paint)
            self.c.bind('<ButtonRelease-1>', self.reset)

    def paint(self, event):
        global state
        if state == 0 and self.old_x and self.old_y:  # paint
            self.c.create_line(self.old_x, self.old_y, event.x, event.y,width=self.line_width, fill=self.color,
                               capstyle=ROUND, smooth=TRUE, splinesteps=36)

        elif state == 1 and self.old_x and self.old_y:  # erase = 'white'
            self.c.create_line(self.old_x, self.old_y, event.x, event.y,width=30, fill='white',
                               capstyle=ROUND, smooth=TRUE, splinesteps=36)
        elif state == 2:  # Neutral
            pass
        else:
            pass

        self.old_x = event.x
        self.old_y = event.y

    def reset(self, event):
        self.old_x, self.old_y = None, None

    def close_painter(self):
        global PaintActive
        PaintActive = 0
        self.root.destroy()

command_map = {
    "inc_lcd": "01",
    "dec_lcd": "02",
    "rra_lcd": "03",
    "set_delay": "04",
    "clear_lcd": "05",
    "stepper_deg": "06",
    "stepper_scan": "07",
    "sleep": "08"
}
def translate_to_machine_code(high_level_content):
    machine_codes = []
    lines = high_level_content.splitlines()
    for line in lines:
        parts = line.split()
        command = parts[0]
        if command in command_map:
            code = command_map[command]
            if len(parts) > 1:
                params = ''.join(parts[1:]).replace(',', '')
                # Check for multiple parameters which are separated by a comma
                if ',' in parts[1]:
                    # Multiple parameters, process each separately
                    param_list = parts[1].split(',')
                    hex_params = ''.join(f"{int(x):02X}" for x in param_list)
                else:
                    # Single parameter
                    hex_params = f"{int(params):02X}"
                full_command = f"{code}{hex_params}"
            else:
                full_command = code
        else:
            full_command = '00'  # Unknown command
        machine_codes.append(full_command)
    return '\n'.join(machine_codes)


def show_window(layout_num, window):
    for i in range(1, 7):
        if i == layout_num:
            window[f'COL{layout_num}'].update(visible=True)
        else:
            window[f'COL{i}'].update(visible=False)


def send_state(message=None, file_option=False):
    s.reset_output_buffer()
    if file_option:
        bytesMenu = message
    else:
        bytesMenu = bytes(message, 'ascii')
    s.write(bytesMenu)


def read_from_MSP(state, size):
    n = 4
    while True:
        while s.in_waiting > 0:  # while the input buffer isn't empty
            if state == 'Painter':
                message = s.read(size=size)
                message = binascii.hexlify(message).decode('utf-8')
                final_message = [message[i:i + n] for i in range(0, len(message), n)]
            elif state == 'script':
                try:
                    final_message = s.read(size=size).decode('utf-8')
                except UnicodeDecodeError:
                    print("Failed  decode as utf-8. Received data:", s.readline())
                    continue
            else:
                try:
                    final_message = s.readline().decode('utf-8')
                except UnicodeDecodeError:
                    print("Failed  decode as utf-8. Received data:", s.readline())
                    continue
            return final_message


def getJoystickTelemetry():
    global state
    telem = read_from_MSP('Painter', 4)
    Vy = int((telem[0]), 16)
    Vx = int((telem[1]), 16)
    if Vx > 1024 or Vy > 1024:
        getJoystickTelemetry()
    print("Vx: " + str(Vx) + ", Vy: " + str(Vy) + ", state: " + str(state))
    return Vx, Vy

def start_painter(window):
    Paint()

def ButtonMain(name, key, color1='white', color2='black'):
    return [sg.Button(name, key=key, size=(35, 2), font=('Helvetica', 14), button_color=(color1, color2))]

def GUI():
    sg.theme('DarkAmber')  # theme

    layout_main = [
        [sg.Text("Final Project - Yoav and guy", size=(35, 2), justification='center',
                 font=('Helvetica', 18, 'bold'))],
        ButtonMain("Manual control of motor",'_ManualStepper_','white', 'blue'),
        ButtonMain("Joystick based PC painter",'_Painter_','white', 'green'),
        ButtonMain("Stepper Motor Calibration",'_Calib_','white', 'red'),
        ButtonMain("Script Mode",'_Script_','white', 'purple'),
    ]
    layout_main = [[sg.Column(layout_main, element_justification='center', justification='center')]]

    layout_manualstepper = [
        [sg.Text("Manual control of motor", size=(35, 2), justification='center', font=('Helvetica', 15, 'italic'))],
        [sg.Button("Rotate", key='_Rotation_', size=(18, 1), font=('Helvetica', 14),
                   button_color=('white', 'darkblue')),
        sg.Button("Stop", key='_Stop_', size=(10, 1), font=('Helvetica', 14), button_color=('white', 'darkred')),
        sg.Button("Joystick Control", key='_JoyStickCrtl_', size=(20, 1), font=('Helvetica', 14),
                   button_color=('white', 'teal'))],
        [sg.Button("Back", key='_BackMenu_', size=(5, 1), font=('Helvetica', 14), button_color=('black', 'orange'),
                   pad=(300, 180))]
    ]

    layout_painter = [
        [sg.Text("Joystick based PC painter", size=(35, 2), justification='center', font=('Helvetica', 18, 'bold'),
                 background_color='lightblue')],
        [sg.Canvas(size=(100, 100), background_color='lightblue', key='canvas')],
        [sg.Button("Back", key='_BackMenu_', size=(5, 1), font=('Helvetica', 14), button_color=('white', 'blue'),
                   pad=(300, 180))]
    ]

    layout_calib = [
        [sg.Text("Stepper Motor Calibration", size=(35, 2), justification='center', font=('Helvetica', 15, 'italic'))],
         [sg.Push(),sg.Button("Update", key='_update_', size=(30, 1), font=('Helvetica', 14),
                   button_color=('white', 'darkgreen'))],
        [sg.Push(),sg.Text("Counter: ", justification='left', font=('Helvetica', 14)),
                    sg.Text(" ", size=(35, 2), key="Counter", justification='center', font=('Helvetica', 14))],
        [sg.Push(),sg.Text("Phi: ", justification='left', font=('Helvetica', 14)),
                    sg.Text(" ", size=(35, 2), key="Phi", justification='center', font=('Helvetica', 14))],
        [sg.Text(' ', size=(35, 1))], # space
        [sg.Button("Back", key='_BackMenu_', size=(5, 1), font=('Helvetica', 14), button_color=('black', 'orange'))]
    ]

    file_viewer = [[sg.Text("File Folder", font='Helvetica 12 bold', size=(15, 1)),
                        sg.In(size=(20, 1), enable_events=True, key='_Folder_'),
                        sg.FolderBrowse(font='Helvetica 12 bold', button_color=('white', 'blue'))],
                   [sg.Listbox(values=[], enable_events=True, size=(40, 23), key="_FileList_")],
                   [sg.Button('Back', key='_BackMenu_', size=(5, 1), font='Helvetica 12 bold'),
                        sg.Button('Burn', key='_Burn_', size=(5, 1), font='Helvetica 12 bold')],
                   [sg.Text(' ', key='_ACK_', size=(35, 1), font='Helvetica 12 bold')]]

    file_description = [
        [sg.Text("File Description", size=(35, 1), justification='center', font='Helvetica 12 bold')],
        [sg.Text(size=(42, 1), key="_FileHeader_", font='Helvetica 12 bold')],
        [sg.Multiline(size=(42, 15), key="_FileContent_")],
        [sg.HSeparator()],
        [sg.Text("Executed List", size=(35, 1), justification='center', font='Helvetica 12 bold')],
        [sg.Listbox(values=[], enable_events=True, size=(42, 4), key="_ExecutedList_")],
        [sg.Button('Execute', key='_Execute_', size=(17, 1), font='Helvetica 12 bold')],
        [sg.Button('Clear', key='_Clear_', size=(17, 1), font='Helvetica 12 bold')]
    ]

    layout_calc = [
        [sg.Text(" ", key='_FileName_', size=(35, 2), justification='center', font='Verdana 15')],
        [sg.Text("Current Degree: ", justification='center', font='Verdana 15'),
         sg.Text(" ", size=(35, 2), key="Degree", justification='center', font='Verdana 15')],
        [sg.Button("Back", key='_BackScript_', size=(5, 1), font='Verdana 15')],
        [sg.Button('Run', key='_Run_', size=(38, 1), font='Verdana 15')]
    ]

    layout_script = [[sg.Column(file_viewer),sg.VSeparator(),sg.Column(file_description)]]

    layout = [[sg.Column(layout_main, key='COL1', ),
               sg.Column(layout_manualstepper, key='COL2', visible=False),
               sg.Column(layout_painter, key='COL3', visible=False),
               sg.Column(layout_calib, key='COL4', visible=False),
               sg.Column(layout_script, key='COL5', visible=False),
               sg.Column(layout_calc, key='COL6', visible=False)]]


    global window
    # Create the Window
    window = sg.Window(title='Control System of motor-based machine', element_justification='c', layout=layout,
                       size=(700, 700))

    canvas = window['canvas']
    execute_list = []
    empty_list = []
    file_name = ''
    file_size = ''
    file_path = ''
    count_calib=513
    global PaintActive
    while True:
        event, values = window.read()
        if event == "_ManualStepper_":
            send_state('m')  # manual stepper
            show_window(2, window)
            while "_BackMenu_" not in event:
                event, values = window.read()
                if event == "_Rotation_":
                    send_state('A')  # Auto Rotate
                elif event == "_Stop_":
                    send_state('M')  # Stop , was S
                elif event == "_JoyStickCrtl_":
                    send_state('J')  # JoyStick Control

        if event == "_Painter_":
            global state
            #   state = 0
            PaintActive = 1
            send_state('P')  # Painter
            threading.Thread(target=start_painter, args=(window,)).start()
            firstTime = 1
            while PaintActive:
                try:
                    Vx, Vy = getJoystickTelemetry()
                except:
                    continue
                if Vx == 1000 and Vy == 1000:
                    state = (state + 1) % 3 # change mode color
                elif firstTime:
                    x_init, y_init = Vx, Vy
                    firstTime = 0
                else:
                    curr_x, curr_y = mouse.get_position()  # read the cursor's current position
                    dx, dy = int(Vx) - int(x_init), int(Vy) - int(y_init)  # convert to int
                    mouse.move(curr_x - int(dx / 25), curr_y - int(dy / 25))  # move cursor to desired position
            show_window(1, window)


        if event == "_Calib_":
            s.reset_input_buffer()
            send_state('C')  # Calibration
            show_window(4, window)
            while "_BackMenu_" not in event:
                event, values = window.read()
                if "_update_" in event:
                    counter = read_from_MSP('calib', 4)
                    window["Counter"].update(value=counter)
                    counter = counter.split('\x00')[0]
                    if counter.isdigit():
                        count_calib = int(counter)
                        phi=count_calib/360
                        phi =1/phi
                        window["Phi"].update(value=str(round(phi, 4)) + "[deg]")
                        window.refresh()

        if event == "_Script_":
            burn_index = 0
            send_state('S')  # Script
            show_window(5, window)

        if event == '_Folder_':
            selected_folder = values['_Folder_']
            try:
                window["_FileContent_"].update('')
                window["_FileHeader_"].update('')
                file_path = ''
                file_list = os.listdir(selected_folder)
            except Exception as e:
                file_list = []
            fnames = [f for f in file_list if
                      os.path.isfile(os.path.join(selected_folder, f)) and f.lower().endswith(".txt")]
            window["_FileList_"].update(fnames)

        if event == '_FileList_':
            try:
                file_path = os.path.join(values["_Folder_"], values["_FileList_"][0])
                file_size = path.getsize(file_path)
                try:
                    with open(file_path, "rt", encoding='utf-8') as f:
                        file_content = f.read()
                except Exception as e:
                    print("Error: ", e)
                file_name = values["_FileList_"][0]
                window["_FileHeader_"].update(f"File name: {file_name} | Size: {file_size} Bytes")
                window["_FileContent_"].update(file_content)
            except Exception as e:
                print("Error: ", e)
                window["_FileContent_"].update('')

        if event == '_Burn_':
            send_state(f"{file_name}\n")
            execute_list.append(f"{file_name}")
            time.sleep(0.1)
            #    send_state(f"{file_size}\n")
            time.sleep(0.1)
            try:
                with open(file_path, "rt") as f:  # Open file in binary read mode
                    high_level_content = f.read()
                # Translate to machine language
                machine_language_content = translate_to_machine_code(high_level_content)
                # Convert the machine language text to binary data
                machine_language_bytes = bytes(machine_language_content + 'Z', 'utf-8')  # Append 'Z' end marker
                # Send the machine language as binary data
            except Exception as e:
                print("Error: ", e)
            send_state(machine_language_bytes, file_option=True)
            # message_send(file_content_b[start:finish], file=True)
            print(file_name)
            print(f"{file_size}")
            time.sleep(0.5)
            if (burn_index == 0):
                ptr_state = 'W'
            elif (burn_index == 1):
                ptr_state = 'X'
            elif (burn_index == 2):
                ptr_state = 'Y'
            burn_index += 1
            send_state(ptr_state)
            try:
                burn_ack = read_from_MSP('script', 3).rstrip('\x00')
            except:
                print("error")
            if burn_ack == "FIN":
                window['_ACK_'].update('"'+file_name+'"'+' burned successfully!')
                window.refresh()
                print(burn_ack)
            print("burn file index: " + ptr_state)
            time.sleep(0.3)
            window['_ExecutedList_'].update(execute_list)

        if event == '_ExecutedList_':
            file_name_to_execute = values["_ExecutedList_"][0]
            file_index = execute_list.index(file_name_to_execute)  # for send state - 0,1,2
            if (file_index == 0):
                exe_state = 'T'
            elif (file_index == 1):
                exe_state = 'U'
            elif (file_index == 2):
                exe_state = 'V'

        if event == '_Execute_':
            curr_phi = 0
            step_phi = 360 / count_calib
            show_window(6, window)
            window['_FileName_'].update('"' + file_name_to_execute + '"' + " execute window")
            while "_BackScript_" not in event:
                event, values = window.read()
                if event == '_Run_':
                    time.sleep(0.5)
                    send_state(exe_state)
                    print("execute file index: " + exe_state)
                    time.sleep(0.6)
                    s.reset_input_buffer()
                    s.reset_output_buffer()
                    curr_counter = read_from_MSP('script', 3).rstrip('\x00')
                    print(curr_counter)
                    while curr_counter != "FIN":
                        while 'F' not in curr_counter:
                            window.refresh()
                            curr_phi = int(curr_counter) * step_phi
                            window["Degree"].update(value=str(round(curr_phi, 4)) + "[deg]")
                            curr_counter = read_from_MSP('script', 3).rstrip('\x00')
                            print(curr_counter)
                        curr_counter = read_from_MSP('script', 3).rstrip('\x00')
                        print(curr_counter)
                        window.refresh()
                        if 'F' not in curr_counter:
                            curr_phi = int(curr_counter) * step_phi
                            window["Degree"].update(value=str(round(curr_phi, 4)) + "[deg]")
                    window.refresh()

        if event == '_Clear_':
            window['_ExecutedList_'].update(empty_list)
            window['_ACK_'].update(' ')

        if event == sg.WIN_CLOSED:
            break

        if event is not None and "_BackMenu_" in event:
            show_window(1, window)
        if event is not None and "_BackScript_" in event:
            show_window(5, window)
    window.close()


if __name__ == '__main__':

    s = ser.Serial('COM3', baudrate=9600, bytesize=ser.EIGHTBITS,
                   parity=ser.PARITY_NONE, stopbits=ser.STOPBITS_ONE,
                   timeout=1)  # timeout of 1  read and write operations are block

    enableTX = True
    s.set_buffer_size(1024, 1024)
    # clear buffers
    s.reset_input_buffer()
    s.reset_output_buffer()
    state = 2  # Start at neutral state
    PaintActive = 0
    GUI()
