
import sensor, image, time,math #程序所用到的库函数
import ustruct
from pyb import UART,LED #调取OpenMV硬件库 串口

blue_threshold  = (0, 48, -128, 127, -128, -17)#设置蓝色阈值
green_threshold  = (0, 69, -128, -9, -128, 127)#设置绿色阈值
red_threshold = (15, 100, 31, 127, -128, 55)#设置红色阈值
yellow_threshold = (62, 100, -28, 127, 24, 127)#设置黄色阈值

sensor.reset() #初始化摄像头
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QQVGA) #QQVGA: 160x120
sensor.set_hmirror(True)#水平方向翻转
sensor.set_vflip(True)#垂直方向翻转
sensor.skip_frames(n=2000)
sensor.set_auto_gain(True) #自动增益
sensor.set_auto_whitebal(True)#自动白平衡
clock = time.clock() #追踪帧率

uart = UART(3,115200)   #设置串口波特率，与stm32一致 OpenMV有两个串口 串口1对应P9 P10 串口3对应P4 P5
                        #P4连接STM32的PA10串口 P5连接STM32的PA9串口
uart.init(115200, bits=8, parity=None, stop=1 )


def sending_data(color,cx,cy): #定义一个发送函数sending_data同时向STM32发送颜色与坐标信息
    global uart;
    data = ustruct.pack("<bbbbbb",
    0x2c,0x12,int(color),int(cx),int(cy),0x5b) #0x2c,0x12为包头，收到包头开始接受数据位（c，x，y）
                                               #收到包尾0x5b停止接收
    uart.write(data);
    for i in data:
        print("data的内容是：    ",hex(i))

while(True):


    img = sensor.snapshot().lens_corr(strength = 1.5, zoom = 1.8)#从感光芯片获得一张图像并且进行畸变校正
    blue_blobs = img.find_blobs([blue_threshold],x_stride=16, y_stride=16, pixels_threshold=200 )
    green_blobs = img.find_blobs([green_threshold], x_stride=16, y_stride=16, pixels_threshold=200 )
    red_blobs = img.find_blobs([red_threshold], x_stride=16, y_stride=16, pixels_threshold=200 )
    yellow_blobs = img.find_blobs([yellow_threshold], x_stride=16, y_stride=16, pixels_threshold=200 )

    if blue_blobs:
        color_status = ord('B')
        for r in blue_blobs:
            img.draw_rectangle((r[0],r[1],r[2],r[3]),color=(255,255,255))#画矩形
            img.draw_cross(r[5], r[6],size=2,color=(255,255,255))#画十字
            img.draw_string(r[0], (r[1]-10), "blue", color=(0,0,255))#写字
            print("中心X坐标",r[5],"中心Y坐标",r[6],"识别颜色类型","蓝色")
            sending_data(color_status,r[5],r[6])
    elif green_blobs:
        color_status = ord('G')
        for y in green_blobs:
            img.draw_rectangle((y[0],y[1],y[2],y[3]),color=(255,255,255))
            img.draw_cross(y[5], y[6],size=2,color=(255,255,255))
            img.draw_string(y[0], (y[1]-10), "green", color=(0,255,0))
            print("中心X坐标",y[5],"中心Y坐标",y[6],"识别颜色类型","绿色")
            sending_data(color_status,y[5],y[6])
    elif red_blobs:
        color_status = ord('R')
        for y in red_blobs:
            img.draw_rectangle((y[0],y[1],y[2],y[3]),color=(255,255,255))
            img.draw_cross(y[5], y[6],size=2,color=(255,255,255))
            img.draw_string(y[0], (y[1]-10), "red", color=(255,0,0))
            print("中心X坐标",y[5],"中心Y坐标",y[6],"识别颜色类型","红色")
            sending_data(color_status,y[5],y[6])
    elif yellow_blobs:
        color_status = ord('Y')
        for y in yellow_blobs:
            img.draw_rectangle((y[0],y[1],y[2],y[3]),color=(255,255,255))
            img.draw_cross(y[5], y[6],size=2,color=(255,255,255))
            img.draw_string(y[0], (y[1]-10), "yellow", color=(255,255,0))
            print("中心X坐标",y[5],"中心Y坐标",y[6],"识别颜色类型","黄色")
            sending_data(color_status,y[5],y[6])
    else:
        color_status = ord('A')
        sending_data(color_status,0,0)





