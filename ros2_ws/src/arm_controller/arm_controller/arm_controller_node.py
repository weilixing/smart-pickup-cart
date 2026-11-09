import threading

import rclpy
from rclpy.node import Node
from std_msgs.msg import Bool, UInt8
from std_srvs.srv import Trigger


class ArmController(Node):
    """抓取 / 投放 / 回零：Trigger 服务 → /arm/seq，等待 /arm/busy 落下且 /arm/ok。"""

    SEQ_HOME = 0
    SEQ_PICK = 1
    SEQ_PLACE = 2

    def __init__(self) -> None:
        super().__init__("arm_controller")
        self.declare_parameter("timeout", 15.0)

        self._seq_pub = self.create_publisher(UInt8, "arm/seq", 10)
        self.create_subscription(Bool, "arm/busy", self._on_busy, 10)
        self.create_subscription(Bool, "arm/ok", self._on_ok, 10)

        self.create_service(Trigger, "arm/pick", self._make_handler(self.SEQ_PICK, "抓取"))
        self.create_service(Trigger, "arm/place", self._make_handler(self.SEQ_PLACE, "投放"))
        self.create_service(Trigger, "arm/home", self._make_handler(self.SEQ_HOME, "回零"))

        self._busy = False
        self._ok = False
        self._done = threading.Event()

    def _on_busy(self, msg: Bool) -> None:
        self._busy = msg.data
        if not msg.data:
            self._done.set()

    def _on_ok(self, msg: Bool) -> None:
        self._ok = msg.data

    def _make_handler(self, seq_id: int, label: str):
        def _handler(_request, response):
            self._done.clear()
            self._ok = False
            self._seq_pub.publish(UInt8(data=seq_id))
            timeout = self.get_parameter("timeout").value
            if not self._done.wait(timeout):
                response.success = False
                response.message = f"{label}超时，请检查舵机与 RA8"
                return response
            response.success = bool(self._ok)
            response.message = f"{label}完成" if self._ok else f"{label}失败"
            return response

        return _handler


def main(args=None) -> None:
    rclpy.init(args=args)
    node = ArmController()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
