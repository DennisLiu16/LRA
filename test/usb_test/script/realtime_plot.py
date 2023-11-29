#!/usr/bin/env python3
# -*- coding:utf-8 -*-
###
# Filename: /home/dennis/develop/LRA/test/usb_test/script/realtime_plot_v2.py
# Path: /home/dennis/develop/LRA/test/usb_test/script
# Created Date: Monday, August 28th 2023, 4:04:51 pm
# Author: Dennis Liu
#
# Copyright (c) 2023 None
###

# import #

import sys
import os
import threading
import select
import time
import pyqtgraph as pg
import numpy as np
from PyQt5 import QtWidgets
from PyQt5.QtCore import QTimer
from functools import partial
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

# constant #

# class #


class PipeMonitor:
    def __init__(self, pipe_name):
        self.pipe_name = pipe_name
        self.stop_event = threading.Event()
        self.start_plot_event = threading.Event()
        self.stop_plot_event = threading.Event()
        self.pwm_file_name = None
        self.acc_file_name = None

    def __handle_message(self, message):
        if message == "eof":
            print('Get EOF -> should close pipe monitor thread')
            self.stop_event.set()
            self.stop_plot_event.set()
            print(f'Close pipe: {self.pipe_name}')
            os.unlink(self.pipe_name)
        elif ',' in message:
            self.pwm_file_name, self.acc_file_name = message.split(',')
            print(
                f"Get files information: {self.pwm_file_name}\n {self.acc_file_name}\n")
            self.start_plot_event.set()

    def __read_pipe(self):
        if not os.path.exists(self.pipe_name):
            print(
                f"Pipe: {self.pipe_name} not exists. Pipe monitor thread is exiting unexpectly")

            # close python process
            os._exit(1)
        try:
            with open(self.pipe_name, 'r') as pipe:
                while not self.stop_event.is_set():
                    ready, _, _ = select.select([pipe], [], [], 1)
                    if ready:
                        message = pipe.readline().strip()
                        self.__handle_message(message)
        except Exception as e:
            print(f"Error while reading the pipe: {e}")
        finally:
            print("Pipe monitor thread is exiting normally")

    def start(self):
        self.monitor_thread = threading.Thread(target=self.__read_pipe)
        print('Pipe monitor thread is starting')
        self.monitor_thread.start()

    def stop(self):
        self.stop_event.set()


class FileObserver:
    def __init__(self, file_name, max_freq, user_callback=None, fargs=()):
        self.file_name = file_name
        self.min_interval = 1.0 / max_freq
        self.line = 0
        self.user_callback = user_callback
        self.fargs = fargs

        # Initialize the observer
        self.observer = Observer()
        event_handler = LimitedFrequencyHandler(
            self._internal_callback, self.min_interval)
        self.observer.schedule(event_handler, self.file_name, recursive=False)

    def _internal_callback(self, event):
        with open(self.file_name, 'r') as f:
            lines = f.readlines()
            new_lines = lines[self.line:]
            try:
                for new_line in new_lines:
                    if self.user_callback:
                        self.user_callback(new_line, *self.fargs)
                        self.line += 1
            except Exception as e:
                print(f'user callback @{self.line}')
                print(f'Exception: {e}')

    def start(self):
        self.observer.start()

    def stop(self):
        self.observer.stop()
        self.observer.join()


class LimitedFrequencyHandler(FileSystemEventHandler):
    def __init__(self, callback, min_interval):
        super().__init__()
        self.callback = callback
        self.min_interval = min_interval
        self.last_triggered = 0

    def on_modified(self, event):
        current_time = time.time()
        if current_time - self.last_triggered > self.min_interval:
            self.callback(event)
            self.last_triggered = current_time

###
# size: should be a tuple like (1000, 600)


class RcwsRealtimePlotter:
    def __init__(self, stop_event, size=(1000, 600), hz=60):
        if not size:
            size = (1000, 600)

        self.width, self.height = size
        self.stop_event = stop_event

        # PyQt settings
        pg.setConfigOptions(antialias=True)

        # PyQt instance
        self.app = pg.mkQApp()
        self.mainWin = QtWidgets.QMainWindow()
        self.layoutManager = QtWidgets.QGridLayout(self.mainWin)
        # glw: GraphicsLayoutWidget
        # self.glw = pg.GraphicsLayoutWidget(title="Realtime data plotting")
        # self.glw.resize(self.width, self.height)
        # self.glw.setWindowTitle('Realtime plotting with PyQtGraph')

        # self.pwm_plot = self.glw.addPlot(title="PWM Data")
        # self.acc_plot = self.glw.addPlot(title="ACC Data")

        self.pwm_plot = pg.PlotWidget()
        self.acc_plot = pg.PlotWidget()

        self.pwm_data_lock = threading.Lock()
        self.acc_data_lock = threading.Lock()

        # TODO: replace to numpy
        self.pwm_data = {
            't': [],
            'x_cmd': [],
            'y_cmd': [],
            'z_cmd': [],
            'x_freq': [],
            'y_freq': [],
            'z_freq': [],
        }
        self.acc_data = {
            't': [],
            'x': [],
            'y': [],
            'z': []
        }
        self.pwm_curves = {
            'x_cmd': self.pwm_plot.plot(pen=pg.mkPen({'color': '#0072BD', 'width': 3}), name='X_CMD'),
            'y_cmd': self.pwm_plot.plot(pen=pg.mkPen({'color': '#D95319', 'width': 3}), name='Y_CMD'),
            'z_cmd': self.pwm_plot.plot(pen=pg.mkPen({'color': '#EDB120', 'width': 3}), name='Z_CMD')
        }
        self.acc_curves = {
            'x': self.acc_plot.plot(pen=pg.mkPen({'color': '#0072BD', 'width': 3}), name='X_ACC'),
            'y': self.acc_plot.plot(pen=pg.mkPen({'color': '#D95319', 'width': 3}), name='Y_ACC'),
            'z': self.acc_plot.plot(pen=pg.mkPen({'color': '#EDB120', 'width': 3}), name='Z_ACC')
        }

        self.fpsLabel = QtWidgets.QLabel()
        self.layoutManager.addWidget(self.glw, 0, 1)
        self.layoutManager.addWidget(self.fpsLabel, 0, 2)

        self.lastUpdate = time.perf_counter()
        self.avgFPS = 0.0

        self.timer = QTimer()
        self.timer.timeout.connect(self.update)
        integer_msec = round(1000 / hz)
        self.timer.start(integer_msec)

        self.mainWin.show()

    def update(self):
        # TODO: make sure has same length

        with self.pwm_data_lock:
            for key in self.pwm_curves:
                if key != 't':
                    self.pwm_curves[key].setData(
                        self.pwm_data['t'], self.pwm_data[key])

        with self.acc_data_lock:
            for key in self.acc_curves:
                if key != 't':
                    self.acc_curves[key].setData(
                        self.acc_data['t'], self.acc_data[key])

        # perfermance estimation
        now = time.perf_counter()
        fps = 1.0 / (now - self.lastUpdate)
        self.avgFPS = self.avgFPS * 0.8 + fps * 0.2
        self.fpsLabel.setText(f'fps: {self.avgFPS}')

        # state check (stop, close)
        if (self.stop_event):
            # close the glw and label
            pass

        # pause or not

    def run(self):
        print('QT plotter thread is starting')
        self.app.exec_()


# callbacks


TIME_WINDOW = 2.0


'''
new_line: a str, given by FileObserver::_internal_callback
data: a dict, each key contains an array or numpy array
'''


def pwm_file_callback(new_line, data, lock):
    _expected_key_num = len(data)

    with lock:

        values = new_line.strip().split(',')

        if len(values) != _expected_key_num:
            raise ValueError(f"Invalid data length: {new_line}")

        try:
            t, x_cmd, x_freq, y_cmd, y_freq, z_cmd, z_freq = map(float, values)
        except ValueError:
            raise ValueError(f"Invalid data value: {new_line}")

        data['t'].append(t)
        data['x_cmd'].append(x_cmd)
        data['x_freq'].append(x_freq)
        data['y_cmd'].append(y_cmd)
        data['y_freq'].append(y_freq)
        data['z_cmd'].append(z_cmd)
        data['z_freq'].append(z_freq)

        # remove out range data
        while (data['t'][-1] - data['t'][0]) > TIME_WINDOW:
            for key in data:
                data[key].pop(0)


def acc_file_callback(new_line, data, lock):

    _expected_key_num = len(data)

    with lock:
        values = new_line.strip().split(',')
        if len(values) != _expected_key_num:
            raise ValueError(f"Invalid data length: {new_line}")

        try:
            t, x, y, z = map(float, values)
        except ValueError:
            raise ValueError(f"Invalid data value: {new_line}")

        data['t'].append(t)
        data['x'].append(x)
        data['y'].append(y)
        data['z'].append(z)

        while (data['t'][-1] - data['t'][0]) > TIME_WINDOW:
            for key in data:
                data[key].pop(0)

# main #


if __name__ == "__main__":
    # print flush setting
    print = partial(print, flush=True)

    # arg process
    if len(sys.argv) < 2:
        print("Pipe name is required as an argument.")
        sys.exit(1)

    # get arg
    pipe_name = sys.argv[1]

    # create PipeMonitor
    pipe_monitor = PipeMonitor(pipe_name=pipe_name)
    pipe_monitor.start()

    # wait for plotting signal
    pipe_monitor.start_plot_event.wait()

    # realtime plotter create
    pg.setConfigOption('background', 'w')
    pg.setConfigOption('foreground', 'k')

    realtime_plotter = RcwsRealtimePlotter(
        pipe_monitor.stop_plot_event, (1200, 900), hz=60)

    # create file observers
    max_freq = 100
    pwm_file_observer = FileObserver(
        pipe_monitor.pwm_file_name, max_freq, pwm_file_callback, (realtime_plotter.pwm_data, realtime_plotter.pwm_data_lock))

    acc_file_observer = FileObserver(
        pipe_monitor.acc_file_name, max_freq, acc_file_callback, (realtime_plotter.acc_data, realtime_plotter.acc_data_lock))

    pwm_file_observer.start()
    acc_file_observer.start()

    # realtime_plotter start (blocking)
    realtime_plotter.run()

    # close file observer
    pipe_monitor.stop_plot_event.wait()
    print('Close file observers')
    pwm_file_observer.stop()
    acc_file_observer.stop()

    print("Exit realtime_plot.py")

    # plot overall curve
