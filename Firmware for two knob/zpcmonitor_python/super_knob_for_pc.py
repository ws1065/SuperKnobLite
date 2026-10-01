'''
Copyright: 
file name: File name
Data: Do not edit
LastEditor: 
LastData: 
Describe: 
'''
#!/usr/bin/env python

import os
import time
import sys
import atexit
import psutil
import serial
import serial.tools.list_ports

# 该简易应用  适用于Super Knob Lite旋钮，用于PC性能监控交互！！！ 需安装 import psutil

time.sleep(3)

def getmemory_cpu():
  phymem = psutil.virtual_memory()
  phycpu = psutil.cpu_percent(interval=1)
  line = "{\"cpu\":%s,\"memory\":%s}\r\n"%(
      str(phycpu),
      phymem.percent
      )
  return line



try:
  # 获取所有串口设备实例。
  # 如果没找到串口设备，则输出：“无串口设备。”
  # 如果找到串口设备，则依次输出每个设备对应的串口号和描述信息。
  ports_list = list(serial.tools.list_ports.comports())
  if len(ports_list) <= 0:
    print("无串口设备。")
  else:
    print("可用的串口设备如下：")
    for comport in ports_list:
      print(list(comport)[0], list(comport)[1])
  ser = serial.Serial("COM8", 115200)  # 打开COM17，将波特率配置为115200，其余参数使用默认值
  if ser.isOpen():  # 判断串口是否成功打开
    print("打开串口成功。")
    print(ser.name)  # 输出串口号
  else:
    print("打开串口失败。")

  while 1:
    time.sleep(1)
    printf_json = getmemory_cpu()
    print(printf_json)
    write_len = ser.write(printf_json.encode('utf-8'))
    # com_input = ser.read(4)
    # if com_input:  # 如果读取结果非空，则输出
    #   print(com_input)

except (KeyboardInterrupt,SystemExit):
  pass






