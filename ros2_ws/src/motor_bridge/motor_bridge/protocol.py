"""OrangePi 与 RA8 的串口帧编解码，协议定义见 docs/serial_protocol.md。"""

import struct

HEADER = b"\xaa\x55"

CMD_SET_VEL = 0x01
CMD_LIFT_CMD = 0x02
CMD_HEARTBEAT = 0x03

CMD_ODOM = 0x81
CMD_LIFT_STATE = 0x82
CMD_RFID = 0x84
CMD_IMU = 0x85

ODOM_STRUCT = struct.Struct("<iii hh")
IMU_STRUCT = struct.Struct("<hhhh")


def _crc8(data: bytes) -> int:
    crc = 0
    for byte in data:
        crc ^= byte
    return crc


def pack_frame(cmd: int, payload: bytes = b"") -> bytes:
    body = bytes([cmd, len(payload)]) + payload
    return HEADER + body + bytes([_crc8(body)])


def pack_set_vel(linear_mm_s: int, angular_mrad_s: int) -> bytes:
    return pack_frame(CMD_SET_VEL, struct.pack("<hh", linear_mm_s, angular_mrad_s))


def pack_lift_cmd(up: bool) -> bytes:
    return pack_frame(CMD_LIFT_CMD, bytes([1 if up else 0]))


class FrameParser:
    """增量式帧解析器：feed() 任意字节流，返回 (cmd, payload) 列表。"""

    MIN_FRAME = 5  # 2 头 + CMD + LEN + CRC

    def __init__(self) -> None:
        self._buf = bytearray()

    def feed(self, data: bytes):
        self._buf.extend(data)
        frames = []
        while True:
            frame = self._next_frame()
            if frame is None:
                return frames
            frames.append(frame)

    def _next_frame(self):
        start = self._buf.find(HEADER)
        if start < 0:
            # 保留可能的半个帧头（末尾出现 0xAA）
            keep = 1 if self._buf.endswith(b"\xaa") else 0
            del self._buf[: len(self._buf) - keep]
            return None
        if start > 0:
            del self._buf[:start]
        if len(self._buf) < 4:
            return None
        length = self._buf[3]
        total = 4 + length + 1
        if len(self._buf) < total:
            return None
        body = bytes(self._buf[2 : 2 + 2 + length])
        crc = self._buf[total - 1]
        del self._buf[:total]
        if _crc8(body) != crc:
            return None  # CRC 错：丢弃帧头，重新同步
        return body[0], body[2:]
