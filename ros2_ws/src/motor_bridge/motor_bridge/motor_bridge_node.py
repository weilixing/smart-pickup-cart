import math
import threading

import rclpy
from geometry_msgs.msg import Quaternion, Twist
from nav_msgs.msg import Odometry
from rclpy.node import Node
from std_msgs.msg import Bool, Float32, Int32, String, UInt8

from . import protocol

try:
    import serial
except ImportError:  # pragma: no cover
    serial = None


def yaw_to_quat(yaw: float) -> Quaternion:
    quat = Quaternion()
    quat.z = math.sin(yaw / 2.0)
    quat.w = math.cos(yaw / 2.0)
    return quat


class MotorBridge(Node):
    """串口桥：cmd_vel / 模式 / 臂 / 语音下行；里程计、循迹、避障、臂状态上行。"""

    def __init__(self) -> None:
        super().__init__("motor_bridge")
        self.declare_parameter("port", "/dev/ttyUSB0")
        self.declare_parameter("baud", 115200)
        self.declare_parameter("cmd_rate", 20.0)

        if serial is None:
            raise RuntimeError("缺少 pyserial：sudo apt install python3-serial")

        port = self.get_parameter("port").value
        baud = self.get_parameter("baud").value
        self._ser = serial.Serial(port, baud, timeout=0.1)
        self._write_lock = threading.Lock()
        self._parser = protocol.FrameParser()
        self._last_cmd = Twist()
        self._mode = 1  # 默认遥控，任务时切到 2

        self._odom_pub = self.create_publisher(Odometry, "odom", 10)
        self._line_err_pub = self.create_publisher(Float32, "line/error_mm", 10)
        self._on_line_pub = self.create_publisher(Bool, "line/on_line", 10)
        self._station_pub = self.create_publisher(UInt8, "line/station", 10)
        self._obstacle_pub = self.create_publisher(String, "obstacle/status", 10)
        self._arm_busy_pub = self.create_publisher(Bool, "arm/busy", 10)
        self._arm_ok_pub = self.create_publisher(Bool, "arm/ok", 10)
        self._voice_done_pub = self.create_publisher(UInt8, "voice/done", 10)

        self.create_subscription(Twist, "cmd_vel", self._on_cmd_vel, 10)
        self.create_subscription(UInt8, "mcu/mode", self._on_mode, 10)
        self.create_subscription(UInt8, "arm/seq", self._on_arm_seq, 10)
        self.create_subscription(UInt8, "voice/play", self._on_voice, 10)

        rate = self.get_parameter("cmd_rate").value
        self.create_timer(1.0 / rate, self._send_velocity)
        self.create_timer(1.0, self._send_heartbeat)

        self._running = True
        self._reader = threading.Thread(target=self._read_loop, daemon=True)
        self._reader.start()
        self.get_logger().info(f"串口已打开 {port} @ {baud}")

    def destroy_node(self):
        self._running = False
        try:
            self._write(protocol.pack_set_mode(0))
            self._write(protocol.pack_set_vel(0, 0))
        except Exception:
            pass
        super().destroy_node()

    def _on_cmd_vel(self, msg: Twist) -> None:
        self._last_cmd = msg

    def _on_mode(self, msg: UInt8) -> None:
        self._mode = int(msg.data)
        self._write(protocol.pack_set_mode(self._mode))

    def _on_arm_seq(self, msg: UInt8) -> None:
        self._write(protocol.pack_arm_seq(int(msg.data)))

    def _on_voice(self, msg: UInt8) -> None:
        self._write(protocol.pack_voice_index(int(msg.data)))

    def _send_velocity(self) -> None:
        if self._mode != 1:
            return
        linear_mm_s = int(self._last_cmd.linear.x * 1000.0)
        angular_mrad_s = int(self._last_cmd.angular.z * 1000.0)
        self._write(protocol.pack_set_vel(linear_mm_s, angular_mrad_s))

    def _send_heartbeat(self) -> None:
        self._write(protocol.pack_heartbeat())

    def _write(self, frame: bytes) -> None:
        with self._write_lock:
            self._ser.write(frame)

    def _read_loop(self) -> None:
        while self._running:
            data = self._ser.read(256)
            if not data:
                continue
            for cmd, payload in self._parser.feed(data):
                self._dispatch(cmd, payload)

    def _dispatch(self, cmd: int, payload: bytes) -> None:
        if cmd == protocol.CMD_ODOM and len(payload) == protocol.ODOM_STRUCT.size:
            self._publish_odom(payload)
        elif cmd == protocol.CMD_LINE and len(payload) == protocol.LINE_STRUCT.size:
            err, on_line, station = protocol.LINE_STRUCT.unpack(payload)
            self._line_err_pub.publish(Float32(data=float(err)))
            self._on_line_pub.publish(Bool(data=bool(on_line)))
            self._station_pub.publish(UInt8(data=station))
        elif cmd == protocol.CMD_OBSTACLE and len(payload) == protocol.OBSTACLE_STRUCT.size:
            front, left, right, state = protocol.OBSTACLE_STRUCT.unpack(payload)
            msg = String()
            msg.data = f"front={front},left={left},right={right},state={state}"
            self._obstacle_pub.publish(msg)
        elif cmd == protocol.CMD_ARM_STATE and len(payload) == protocol.ARM_STATE_STRUCT.size:
            busy, _last, ok = protocol.ARM_STATE_STRUCT.unpack(payload)
            self._arm_busy_pub.publish(Bool(data=bool(busy)))
            self._arm_ok_pub.publish(Bool(data=bool(ok)))
        elif cmd == protocol.CMD_VOICE_DONE and len(payload) >= 1:
            self._voice_done_pub.publish(UInt8(data=payload[0]))

    def _publish_odom(self, payload: bytes) -> None:
        x_mm, y_mm, theta_mrad, v_mm_s, w_mrad_s = protocol.ODOM_STRUCT.unpack(payload)
        msg = Odometry()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = "odom"
        msg.child_frame_id = "base_footprint"
        msg.pose.pose.position.x = x_mm / 1000.0
        msg.pose.pose.position.y = y_mm / 1000.0
        msg.pose.pose.orientation = yaw_to_quat(theta_mrad / 1000.0)
        msg.twist.twist.linear.x = v_mm_s / 1000.0
        msg.twist.twist.angular.z = w_mrad_s / 1000.0
        self._odom_pub.publish(msg)


def main(args=None) -> None:
    rclpy.init(args=args)
    node = MotorBridge()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
