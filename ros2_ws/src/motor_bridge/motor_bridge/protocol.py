"""OrangePi 与 RA8 的串口帧编解码，见 docs/serial_protocol.md。"""

import struct

HEADER = b"\xaa\x55"

CMD_SET_MODE = 0x01
CMD_SET_VEL = 0x02
CMD_HEARTBEAT = 0x03
CMD_ARM_SEQ = 0x10
CMD_ARM_JOINT = 0x11
CMD_VOICE = 0x20

CMD_ODOM = 0x81
CMD_LINE = 0x82
CMD_OBSTACLE = 0x83
CMD_ARM_STATE = 0x84
CMD_VOICE_DONE = 0x85
CMD_IMU = 0x86

ODOM_STRUCT = struct.Struct("<iii hh")
LINE_STRUCT = struct.Struct("<hBB")
OBSTACLE_STRUCT = struct.Struct("<HHHB")
ARM_STATE_STRUCT = struct.Struct("<BBB")


def _crc8(data: bytes) -> int:
    crc = 0
    for byte in data:
        crc ^= byte
    return crc


def pack_frame(cmd: int, payload: bytes = b"") -> bytes:
    body = bytes([cmd, len(payload)]) + payload
    return HEADER + body + bytes([_crc8(body)])


def pack_set_mode(mode: int) -> bytes:
    return pack_frame(CMD_SET_MODE, bytes([mode & 0xFF]))


def pack_set_vel(linear_mm_s: int, angular_mrad_s: int) -> bytes:
    return pack_frame(CMD_SET_VEL, struct.pack("<hh", linear_mm_s, angular_mrad_s))


def pack_heartbeat() -> bytes:
    return pack_frame(CMD_HEARTBEAT)


def pack_arm_seq(seq_id: int) -> bytes:
    return pack_frame(CMD_ARM_SEQ, bytes([seq_id & 0xFF]))


def pack_voice_index(index: int) -> bytes:
    return pack_frame(CMD_VOICE, bytes([index & 0xFF]))


class FrameParser:
    """增量式帧解析器。"""

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
            return None
        return body[0], body[2:]
