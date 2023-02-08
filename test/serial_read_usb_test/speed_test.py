# This file is copied from https://blog.csdn.net/Naisu_kun/article/details/118192032

from typing import Counter
import serial
import serial.tools.list_ports
import time
import datetime

print('正在搜尋串口……')
port_list = list(serial.tools.list_ports.comports())
print('串口列表：')
for i in range(0, len(port_list)):
    print(port_list[i])
print('')

port = input('請輸入串口編號 e.g. WIN: COMX, Linux: /dev/ttyXXXY')
print('')

ser = serial.Serial(port, 9600, timeout=5)

run = True
count = 0
starttime = int(round(time.time() * 1000))

print(datetime.datetime.now().strftime('%H:%M:%S.%f') + ': 開始測試 STM32 -> PC 傳送速率...')
ser.write('S'.encode('utf-8'))
while (run):
    ser.read(2048) # 接收来自单片机的数据
    count += 1
    currenttime = int(round(time.time() * 1000))
    run = False if (currenttime - starttime) >= 1000 else True

ser.write('E'.encode('utf-8'))
print(datetime.datetime.now().strftime('%H:%M:%S.%f') + ': 結束測試, 速率約為' + str(count * 2048 / 1000) + 'K Byte/s\n')

sendbuf = bytes(2048)
run = True
count = 0
starttime = int(round(time.time() * 1000))

print(datetime.datetime.now().strftime('%H:%M:%S.%f') + ': 開始測試 STM32 <- PC 接收速率...')
while (run):
    count += ser.write(sendbuf) # 向单片机发送数据
    currenttime = int(round(time.time() * 1000))
    run = False if (currenttime - starttime) >= 1000 else True

print(datetime.datetime.now().strftime('%H:%M:%S.%f') + ': 結束測試, 速率約為' + str(count / 1000) + 'K Byte/s\n')

ser.close()
exit()