import threading

import rclpy
from rclpy.node import Node
from std_msgs.msg import Bool
from std_srvs.srv import SetBool


class LiftController(Node):
    """顶升平台：SetBool 服务（true=升起 / false=降下），转发命令到 RA8 并等待到位反馈。

    v1 用服务即可；需要进度反馈和中途取消时再升级为 ROS2 action。
    """

    def __init__(self) -> None:
        super().__init__("lift_controller")
        self.declare_parameter("timeout", 8.0)

        self._cmd_pub = self.create_publisher(Bool, "mcu/lift_cmd", 10)
        self.create_subscription(Bool, "mcu/lift_state", self._on_state, 10)
        self.create_service(SetBool, "lift/set", self._on_set)

        self._state_event = threading.Event()
        self._target_state = False

    def _on_state(self, msg: Bool) -> None:
        if msg.data == self._target_state:
            self._state_event.set()

    def _on_set(self, request, response):
        self._target_state = request.data
        self._state_event.clear()
        self._cmd_pub.publish(Bool(data=request.data))

        timeout = self.get_parameter("timeout").value
        if self._state_event.wait(timeout):
            response.success = True
            response.message = "已升起" if request.data else "已降下"
        else:
            response.success = False
            response.message = "等待到位反馈超时，请检查限位开关与 RA8 连接"
        return response


def main(args=None) -> None:
    rclpy.init(args=args)
    node = LiftController()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
