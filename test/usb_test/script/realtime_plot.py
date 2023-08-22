#!/usr/bin/env python3
# -*- coding:utf-8 -*-
###
# Filename: /home/dennis/develop/LRA/test/usb_test/script/realtime_plot.py
# Path: /home/dennis/develop/LRA/test/usb_test/script
# Created Date: Monday, August 21st 2023, 10:09:45 am
# Author: Dennis Liu
#
# Copyright (c) 2023 None
###

import os
import sys
import signal
import numpy as np
import matplotlib.pyplot as plt
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
from matplotlib.animation import FuncAnimation
from functools import partial

if len(sys.argv) < 2:
    print("Pipe name is required as an argument.")
    sys.exit(1)

TIME_WINDOW = 2.0
PIPE_NAME = sys.argv[1]
pwm_data = {'t': [], 'x_cmd': [], 'x_freq': [],
            'y_cmd': [], 'y_freq': [], 'z_cmd': [], 'z_freq': []}
acc_data = {'t': [], 'x': [], 'y': [], 'z': []}


def read_pwm_line(line):
    t, x_cmd, x_freq, y_cmd, y_freq, z_cmd, z_freq = map(
        float, line.strip().split(','))
    pwm_data['t'].append(t)
    pwm_data['x_cmd'].append(x_cmd)
    pwm_data['x_freq'].append(x_freq)
    pwm_data['y_cmd'].append(y_cmd)
    pwm_data['y_freq'].append(y_freq)
    pwm_data['z_cmd'].append(z_cmd)
    pwm_data['z_freq'].append(z_freq)

    while pwm_data['t'][-1] - pwm_data['t'][0] > TIME_WINDOW:
        for key in pwm_data:
            pwm_data[key].pop(0)


def read_acc_line(line):
    t, x, y, z = map(float, line.strip().split(','))
    acc_data['t'].append(t)
    acc_data['x'].append(x)
    acc_data['y'].append(y)
    acc_data['z'].append(z)

    while acc_data['t'][-1] - acc_data['t'][0] > TIME_WINDOW:
        for key in acc_data:
            acc_data[key].pop(0)


def plot_data(frame, ax1, ax2):
    ax1.clear()
    ax2.clear()

    # Plot pwm data
    ax1.plot(pwm_data['t'], pwm_data['x_cmd'], label='X_CMD')
    ax1.plot(pwm_data['t'], pwm_data['y_cmd'], label='Y_CMD')
    ax1.plot(pwm_data['t'], pwm_data['z_cmd'], label='Z_CMD')
    ax1.legend()
    ax1.set_title('PWM Data')

    # Plot acc data
    ax2.plot(acc_data['t'], acc_data['x'], label='X_ACC')
    ax2.plot(acc_data['t'], acc_data['y'], label='Y_ACC')
    ax2.plot(acc_data['t'], acc_data['z'], label='Z_ACC')
    ax2.legend()
    ax2.set_title('ACC Data')


def display_all_data(pwm_file, acc_file):
    global pwm_data, acc_data
    pwm_data = {'t': [], 'x_cmd': [], 'x_freq': [],
                'y_cmd': [], 'y_freq': [], 'z_cmd': [], 'z_freq': []}
    acc_data = {'t': [], 'x': [], 'y': [], 'z': []}

    with open(pwm_file, 'r') as f:
        for line in f:
            t, x_cmd, x_freq, y_cmd, y_freq, z_cmd, z_freq = map(
                float, line.strip().split(','))
            pwm_data['t'].append(t)
            pwm_data['x_cmd'].append(x_cmd)
            pwm_data['x_freq'].append(x_freq)
            pwm_data['y_cmd'].append(y_cmd)
            pwm_data['y_freq'].append(y_freq)
            pwm_data['z_cmd'].append(z_cmd)
            pwm_data['z_freq'].append(z_freq)

    with open(acc_file, 'r') as f:
        for line in f:
            t, x, y, z = map(float, line.strip().split(','))
            acc_data['t'].append(t)
            acc_data['x'].append(x)
            acc_data['y'].append(y)
            acc_data['z'].append(z)

    # Create a new figure and two subplots
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 6))

    # Call the updated plot_data function
    plot_data(None, ax1, ax2)
    plt.show()


class FileChangeHandler(FileSystemEventHandler):
    def __init__(self, filename, read_func):
        self.filename = filename
        self.read_func = read_func
        self.last_read_line = 0

    def on_modified(self, event):
        if event.src_path == self.filename:
            with open(self.filename, 'r') as f:
                lines = f.readlines()
                new_lines = lines[self.last_read_line:]
                for line in new_lines:
                    self.read_func(line)
                self.last_read_line = len(lines)


def monitor_pipe():
    global pwm_file, acc_file

    if not os.path.exists(PIPE_NAME):
        print("Pipe does not exist. Exiting.")
        sys.exit(1)

    with open(PIPE_NAME, 'r') as pipe:
        while True:
            message = pipe.readline().strip()
            if message == "eof":
                print('Get EOF')
                display_all_data(pwm_file, acc_file)
                os.unlink(PIPE_NAME)  # Remove the named pipe
                break
            else:
                pwm_file, acc_file = message.split(',')

                print(f"Get files information: {pwm_file}\n {acc_file}\n")

                # Set up watchdog for the files
                observer = Observer()
                pwm_handler = FileChangeHandler(pwm_file, read_pwm_line)
                acc_handler = FileChangeHandler(acc_file, read_acc_line)
                observer.schedule(pwm_handler, path=pwm_file, recursive=False)
                observer.schedule(acc_handler, path=acc_file, recursive=False)
                observer.start()

                # Start the animation for plotting
                print("Set FuncAnimation")
                fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 6))
                ani = FuncAnimation(
                    fig, plot_data, fargs=(ax1, ax2), interval=500)
                plt.show()


if __name__ == "__main__":
    def signal_handler(sig, frame):
        print("Ctrl+C pressed. Displaying all data...")
        display_all_data(pwm_file, acc_file)
        os.unlink(PIPE_NAME)  # Remove the named pipe
        sys.exit(0)

    print = partial(print, flush=True)

    print("python background start")

    signal.signal(signal.SIGINT, signal_handler)
    monitor_pipe()
