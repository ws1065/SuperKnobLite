#!/usr/bin/env python3
"""Wrap esptool with paced serial writes to defeat CH343 CDC burst data loss."""
import serial, time, sys

SLICE = 64      # bytes per serial write
DELAY = 0.006   # seconds between writes (→ ~5.5 KB/s, safely under 115200 UART rate)

_orig_write = serial.Serial.write
def paced_write(self, data):
    data = bytes(data)
    for i in range(0, len(data), SLICE):
        _orig_write(self, data[i:i+SLICE])
        self.flush()
        time.sleep(DELAY)
serial.Serial.write = paced_write

import esptool
esptool.main(sys.argv[1:])
