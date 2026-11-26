#!/usr/bin/env python3
"""向 RA8 发送协议心跳，看是否回 ACK(0x90) —— 双板串口冒烟测试。

用法（香橙派上）：
  python3 scripts/uart_ping_test.py /dev/ttyUSB0
"""

from __future__ import annotations

import sys
import time

try:
    import serial
except ImportError:
    print("请先: sudo apt install -y python3-serial")
    sys.exit(1)


def crc8(data: bytes) -> int:
    c = 0
    for b in data:
        c ^= b
    return c


def pack(cmd: int, payload: bytes = b"") -> bytes:
    body = bytes([cmd, len(payload)]) + payload
    return b"\xaa\x55" + body + bytes([crc8(body)])


def main() -> None:
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
    ser = serial.Serial(port, 115200, timeout=0.3)
    print(f"打开 {port} @ 115200")
    hb = pack(0x03)
    for i in range(10):
        ser.write(hb)
        ser.flush()
        time.sleep(0.05)
        resp = ser.read(64)
        print(f"[{i}] tx={hb.hex()} rx={resp.hex() if resp else '(空)'}")
        if resp and resp[:4] == b"\xaa\x55\x90\x01":
            print("  -> 收到 ACK，双板串口打通")
        time.sleep(0.4)
    ser.close()


if __name__ == "__main__":
    main()
