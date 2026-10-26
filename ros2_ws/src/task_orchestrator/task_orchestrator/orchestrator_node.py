import math
from enum import Enum, auto

import rclpy
from geometry_msgs.msg import PoseStamped
from nav2_simple_commander.robot_navigator import BasicNavigator, TaskResult
from rclpy.node import Node
from std_msgs.msg import Bool, String
from std_srvs.srv import SetBool


class State(Enum):
    IDLE = auto()
    NAV_TO_PICKUP = auto()
    CONFIRM_RFID = auto()
    LIFT_UP = auto()
    NAV_TO_DELIVER = auto()
    LIFT_DOWN = auto()
    NAV_HOME = auto()
    DONE = auto()
    FAILED = auto()


class Orchestrator(Node):
    """完整闭环：/start_delivery 触发 → 取货点 → RFID 确认 → 顶升 → 投递点 → 放下 → 回家。

    三个站点坐标（map 系）通过参数配置：home_pose / pickup_pose / deliver_pose，
    格式 [x, y, yaw_deg]，建图后用 RViz 的 "2D Pose Estimate" 在目标点读取。
    """

    def __init__(self) -> None:
        super().__init__("orchestrator")
        self.declare_parameter("home_pose", [0.0, 0.0, 0.0])
        self.declare_parameter("pickup_pose", [1.0, 0.0, 0.0])
        self.declare_parameter("deliver_pose", [0.0, 1.0, 0.0])
        self.declare_parameter("expected_rfid", "")  # 空串 = 接受任意标签
        self.declare_parameter("rfid_timeout", 10.0)

        self._navigator = BasicNavigator()
        self._lift_client = self.create_client(SetBool, "lift/set")
        self.create_subscription(Bool, "start_delivery", self._on_start, 10)
        self.create_subscription(String, "rfid_id", self._on_rfid, 10)

        self._state = State.IDLE
        self._last_rfid = None
        self._lift_future = None
        self._nav_ready = False
        self.create_timer(0.2, self._tick)
        self.get_logger().info("就绪：向 /start_delivery 发 true 开始一次取送演示")

    def _make_pose(self, values) -> PoseStamped:
        pose = PoseStamped()
        pose.header.frame_id = "map"
        pose.header.stamp = self.get_clock().now().to_msg()
        pose.pose.position.x = values[0]
        pose.pose.position.y = values[1]
        half = math.radians(values[2]) / 2.0
        pose.pose.orientation.z = math.sin(half)
        pose.pose.orientation.w = math.cos(half)
        return pose

    def _on_start(self, msg: Bool) -> None:
        if msg.data and self._state == State.IDLE:
            self.get_logger().info("收到订单，出发去取货点")
            self._goto("pickup_pose", State.NAV_TO_PICKUP)

    def _on_rfid(self, msg: String) -> None:
        self._last_rfid = msg.data

    def _goto(self, param: str, state: State) -> None:
        if not self._nav_ready:
            self.get_logger().info("等待 Nav2 激活……")
            self._navigator.waitUntilNav2Active()
            self._nav_ready = True
        pose = self._make_pose(self.get_parameter(param).value)
        self._navigator.goToPose(pose)
        self._state = state

    def _nav_done(self) -> bool:
        if self._navigator.isTaskComplete():
            result = self._navigator.getResult()
            if result != TaskResult.SUCCEEDED:
                self.get_logger().error(f"导航失败：{result}")
                self._state = State.FAILED
            return True
        return False

    def _call_lift(self, up: bool) -> None:
        if not self._lift_client.wait_service(timeout_sec=2.0):
            self.get_logger().error("/lift/set 服务不可用")
            self._state = State.FAILED
            return
        self._lift_future = self._lift_client.call_async(SetBool.Request(data=up))

    def _lift_done(self) -> bool:
        if self._lift_future is None or not self._lift_future.done():
            return False
        if not self._lift_future.result().success:
            self.get_logger().error("顶升动作失败")
            self._state = State.FAILED
            return False
        self._lift_future = None
        return True

    def _tick(self) -> None:
        if self._state == State.NAV_TO_PICKUP and self._nav_done():
            if self._state == State.NAV_TO_PICKUP:
                self.get_logger().info("到达取货点，等待 RFID 确认")
                self._last_rfid = None
                self._state = State.CONFIRM_RFID

        elif self._state == State.CONFIRM_RFID:
            expected = self.get_parameter("expected_rfid").value
            if self._last_rfid and (not expected or self._last_rfid == expected):
                self.get_logger().info(f"确认货物 {self._last_rfid}，顶升取货")
                self._call_lift(up=True)
                self._state = State.LIFT_UP

        elif self._state == State.LIFT_UP and self._lift_done():
            self.get_logger().info("取货完成，前往投递点")
            self._goto("deliver_pose", State.NAV_TO_DELIVER)

        elif self._state == State.NAV_TO_DELIVER and self._nav_done():
            if self._state == State.NAV_TO_DELIVER:
                self.get_logger().info("到达投递点，放下货物")
                self._call_lift(up=False)
                self._state = State.LIFT_DOWN

        elif self._state == State.LIFT_DOWN and self._lift_done():
            self.get_logger().info("投递完成，返回出发点")
            self._goto("home_pose", State.NAV_HOME)

        elif self._state == State.NAV_HOME and self._nav_done():
            if self._state == State.NAV_HOME:
                self._state = State.DONE

        if self._state == State.DONE:
            self.get_logger().info("一次取送闭环完成")
            self._state = State.IDLE


def main(args=None) -> None:
    rclpy.init(args=args)
    node = Orchestrator()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
