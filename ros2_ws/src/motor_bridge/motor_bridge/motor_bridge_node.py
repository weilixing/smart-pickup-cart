import math
import threading

import rclpy
from geometry_msgs.msg import Quaternion, Twist
from nav_msgs.msg import Odometry
from rclpy.node import Node
from sensor_msgs.msg import Imu
from std_msgs.msg import Bool, String

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
    """cmd_vel -> RA8；RA8 上报 -> /odom、/imu/data_raw、/rfid_id、/mcu/lift_state。"""

    def __init__(self) -> None:
        super().__init__("motor_bridge")
        self.declare_parameter("port", "/dev/ttyUSB0")
        self.declare_parameter("baud", 115200)
        self.declare_parameter("cmd_rate", 20.0)
        self.declare_parameter("odom_frame", "odom")
        self.declare_parameter("base_frame", "base_footprint")

        if serial is None:
            raise RuntimeError("缺少 pyserial：sudo apt install python3-serial")

        port = self.get_parameter("port").value
        baud = self.get_parameter("baud").value
        self._ser = serial.Serial(port, baud, timeout=0.1)
        self._write_lock = threading.Lock()
        self._parser = protocol.FrameParser()

        self._odom_frame = self.get_parameter("odom_frame").value
        self._base_frame = self.get_parameter("base_frame").value
        self._last_cmd = Twist()

        self._odom_pub = self.create_publisher(Odometry, "odom", 10)
        self._imu_pub = self.create_publisher(Imu, "imu/data_raw", 10)
        self._rfid_pub = self.create_publisher(String, "rfid_id", 10)
        self._lift_state_pub = self.create_publisher(Bool, "mcu/lift_state", 10)

        self.create_subscription(Twist, "cmd_vel", self._on_cmd_vel, 10)
        self.create_subscription(Bool, "mcu/lift_cmd", self._on_lift_cmd, 10)

        rate = self.get_parameter("cmd_rate").value
        self.create_timer(1.0 / rate, self._send_velocity)

        self._running = True
        self._reader = threading.Thread(target=self._read_loop, daemon=True)
        self._reader.start()
        self.get_logger().info(f"串口已打开 {port} @ {baud}")

    def destroy_node(self):
        self._running = False
        self._send_stop()
        super().destroy_node()

    def _on_cmd_vel(self, msg: Twist) -> None:
        self._last_cmd = msg

    def _on_lift_cmd(self, msg: Bool) -> None:
        self._write(protocol.pack_lift_cmd(msg.data))

    def _send_velocity(self) -> None:
        linear_mm_s = int(self._last_cmd.linear.x * 1000.0)
        angular_mrad_s = int(self._last_cmd.angular.z * 1000.0)
        self._write(protocol.pack_set_vel(linear_mm_s, angular_mrad_s))

    def _send_stop(self) -> None:
        try:
            self._write(protocol.pack_set_vel(0, 0))
        except Exception:
            pass

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
        elif cmd == protocol.CMD_IMU and len(payload) == protocol.IMU_STRUCT.size:
            self._publish_imu(payload)
        elif cmd == protocol.CMD_RFID:
            uid = payload.split(b"\x00")[0].decode("ascii", errors="ignore")
            if uid:
                self._rfid_pub.publish(String(data=uid))
        elif cmd == protocol.CMD_LIFT_STATE and len(payload) >= 1:
            self._lift_state_pub.publish(Bool(data=payload[0] == 1))

    def _publish_odom(self, payload: bytes) -> None:
        x_mm, y_mm, theta_mrad, v_mm_s, w_mrad_s = protocol.ODOM_STRUCT.unpack(payload)
        msg = Odometry()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = self._odom_frame
        msg.child_frame_id = self._base_frame
        msg.pose.pose.position.x = x_mm / 1000.0
        msg.pose.pose.position.y = y_mm / 1000.0
        msg.pose.pose.orientation = yaw_to_quat(theta_mrad / 1000.0)
        msg.twist.twist.linear.x = v_mm_s / 1000.0
        msg.twist.twist.angular.z = w_mrad_s / 1000.0
        msg.pose.covariance[0] = 0.01
        msg.pose.covariance[7] = 0.01
        msg.pose.covariance[35] = 0.03
        self._odom_pub.publish(msg)

    def _publish_imu(self, payload: bytes) -> None:
        ax_mg, ay_mg, az_mg, gz_cdps = protocol.IMU_STRUCT.unpack(payload)
        msg = Imu()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = "imu_link"
        msg.linear_acceleration.x = ax_mg * 0.00980665
        msg.linear_acceleration.y = ay_mg * 0.00980665
        msg.linear_acceleration.z = az_mg * 0.00980665
        msg.angular_velocity.z = math.radians(gz_cdps / 100.0)
        msg.orientation_covariance[0] = -1.0  # 磁力计无，姿态不可用
        self._imu_pub.publish(msg)


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
