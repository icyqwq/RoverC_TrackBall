import sensor
import image
import lcd
import time
import utime
from machine import UART
from Maix import GPIO
from fpioa_manager import *

Px = -0.4

fm.register(34,fm.fpioa.UART1_TX)
fm.register(35,fm.fpioa.UART1_RX)
uart_out = UART(UART.UART1, 115200, 8, None, 1, timeout=1000, read_buf_len=4096)

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.run(1)

#target_lab_threshold = (12, 71, 30, 70, 2, 50)   #R
target_lab_threshold = (35, 80, -45, -15, -3, 40)   #G
prev_area = 0
while True:
    img = sensor.snapshot()

    blobs = img.find_blobs([target_lab_threshold], x_stride = 2, y_stride = 2, pixels_threshold = 100, merge = True, margin = 20)
    if blobs:
        max_area = 0
        target = blobs[0]
        for b in blobs:
            if b.area() > max_area:
                max_area = b.area()
                target = b

        xerr = 160 - target[5]
        ux = int(Px * xerr)

        if ux > 100:
            ux = 100
        elif ux < -100:
            ux = -100

        if abs(xerr) < 20:
            ux = 0
        #print(ux)

        if uart_out.read(4096):
            area = target.area()
            hexlist = [ux & 0xFF, (area >> 16) & 0xFF, (area >> 8) & 0xFF, area & 0xFF]
            a = uart_out.write(bytes(hexlist))

        #print(abs(prev_area - target.area()))
        prev_area = target.area()
        tmp=img.draw_rectangle(target[0:4])
        tmp=img.draw_cross(target[5], target[6])
        # c=img.get_pixel(target[5], target[6])
    else:
        if uart_out.read(1):
            hexlist = [0x00, 0x00, 0x00, 0x00]
            a = uart_out.write(bytes(hexlist))
