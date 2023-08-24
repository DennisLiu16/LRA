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

import atexit
from functools import partial
from matplotlib.animation import FuncAnimation
from watchdog.events import FileSystemEventHandler
from watchdog.observers import Observer
import matplotlib.pyplot as plt
import os
import sys
import signal
import threading
import select

start_plotting_event = threading.Event()

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

    # Ensure pwm_data's lengths are consistent
    min_len_pwm = min(len(pwm_data[key]) for key in pwm_data)
    for key in pwm_data:
        while len(pwm_data[key]) > min_len_pwm:
            pwm_data[key].pop(0)

    # Ensure acc_data's lengths are consistent
    min_len_acc = min(len(acc_data[key]) for key in acc_data)
    for key in acc_data:
        while len(acc_data[key]) > min_len_acc:
            acc_data[key].pop(0)

    # Plot pwm data
    # make sure len is same
    ax1.plot(pwm_data['t'], pwm_data['x_cmd'], label='X_CMD')
    ax1.plot(pwm_data['t'], pwm_data['y_cmd'], label='Y_CMD')
    ax1.plot(pwm_data['t'], pwm_data['z_cmd'], label='Z_CMD')
    ax1.legend()
    ax1.set_title('PWM Data')

    # Plot acc data
    # make sure len is same
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


def handle_close(evt):
    print("close figure callback")


def plot_thread(plot_stop_event):
    print("Set FuncAnimation")
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 6))
    ani = FuncAnimation(fig, plot_data, fargs=(ax1, ax2), interval=100)
    fig.canvas.mpl_connect('close_event', handle_close)
    plt.show()

    # Block the thread until it's signaled to stop
    plot_stop_event.wait()
    plt.close()

    display_all_data(pwm_file, acc_file)


def start_plotting(pwm_file, acc_file, plot_stop_event):
    # Set up watchdog for the files
    observer = Observer()
    pwm_handler = FileChangeHandler(pwm_file, read_pwm_line)
    acc_handler = FileChangeHandler(acc_file, read_acc_line)
    observer.schedule(pwm_handler, path=pwm_file, recursive=False)
    observer.schedule(acc_handler, path=acc_file, recursive=False)
    observer.start()

    # Use the main thread for GUI
    plot_thread(plot_stop_event)


def monitor_pipe(stop_event, plot_stop_event):
    global pwm_file, acc_file

    if not os.path.exists(PIPE_NAME):
        print("Pipe does not exist. Exiting.")
        sys.exit(1)

    with open(PIPE_NAME, 'r') as pipe:
        while not stop_event.is_set():
            # Use select for IO-based triggering
            ready, _, _ = select.select([pipe], [], [], 1)
            if ready:
                message = pipe.readline().strip()
                if message == "eof":
                    print('Get EOF')
                    plot_stop_event.set()  # Signal to stop plotting
                    os.unlink(PIPE_NAME)  # Remove the named pipe
                    break
                elif ',' in message:
                    pwm_file, acc_file = message.split(',')
                    print(f"Get files information: {pwm_file}\n {acc_file}\n")
                    start_plotting_event.set()  # Signal the main thread to start plotting


def ensure_pipe_deleted():
    """
    Ensure the named pipe is deleted.
    """
    if os.path.exists(PIPE_NAME):
        os.unlink(PIPE_NAME)
        print(f"{PIPE_NAME} deleted.")


if __name__ == "__main__":
    # Register the function to be called on normal termination
    atexit.register(ensure_pipe_deleted)

    # Handle SIGINT (Ctrl+C) and other common termination signals
    for sig in [signal.SIGTERM, signal.SIGINT, signal.SIGHUP, signal.SIGQUIT]:
        # At exit, the atexit-registered function will be called
        signal.signal(sig, lambda signum, frame: exit(1))

    print = partial(print, flush=True)

    print("python background start")

    # Start the monitor_pipe in a separate thread
    stop_event = threading.Event()
    plot_stop_event = threading.Event()
    monitor_thread = threading.Thread(
        target=monitor_pipe, args=(stop_event, plot_stop_event))
    monitor_thread.start()

    # In the main thread, check for the start_plotting_event
    while not stop_event.is_set():
        if start_plotting_event.is_set():
            start_plotting_event.clear()
            start_plotting(pwm_file, acc_file, plot_stop_event)

    print("Exit realtime_plot.py")
