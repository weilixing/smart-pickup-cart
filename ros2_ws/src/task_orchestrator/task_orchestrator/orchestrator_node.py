from enum import Enum, auto

import rclpy
from rclpy.node import Node
from std_msgs.msg import Bool, String, UInt8
from std_srvs.srv import Trigger


class State(Enum):
    IDLE = auto()
    LINE_TO_PICKUP = auto()
    WAIT_ANNOUNCE = auto()
    PICK = auto()
    LINE_TO_DROP = auto()
    PLACE = auto()
    LINE_HOME = auto()
    DONE = auto()
    FAILED = auto()


class Orchestrator(Node):
    """闭环：循迹取货点 → 播报 → 抓取 → 投放点 → 投放 → 返航。

    /line/station：1=取货点，2=投放点，3=出发点。
    """

    STATION_PICKUP = 1
    STATION_DROP = 2
    STATION_HOME = 3

    def __init__(self) -> None:
        super().__init__("orchestrator")
        self._mode_pub = self.create_publisher(UInt8, "mcu/mode", 10)
        self._voice_pub = self.create_publisher(String, "voice/say", 10)
        self.create_subscription(Bool, "start_delivery", self._on_start, 10)
        self.create_subscription(UInt8, "line/station", self._on_station, 10)
        self.create_subscription(UInt8, "voice/done", self._on_voice_done, 10)

        self._arm_pick = self.create_client(Trigger, "arm/pick")
        self._arm_place = self.create_client(Trigger, "arm/place")

        self._state = State.IDLE
        self._station = 0
        self._pending_after_voice = None
        self._arm_future = None
        self.create_timer(0.2, self._tick)
        self.get_logger().info("就绪：向 /start_delivery 发 true 开始取货闭环")

    def _on_start(self, msg: Bool) -> None:
        if msg.data and self._state == State.IDLE:
            self.get_logger().info("出发：循迹前往取货点")
            self._station = 0
            self._mode_pub.publish(UInt8(data=2))
            self._state = State.LINE_TO_PICKUP

    def _on_station(self, msg: UInt8) -> None:
        self._station = int(msg.data)

    def _on_voice_done(self, _msg: UInt8) -> None:
        if self._state == State.WAIT_ANNOUNCE and self._pending_after_voice:
            nxt = self._pending_after_voice
            self._pending_after_voice = None
            nxt()

    def _say(self, key: str) -> None:
        self._voice_pub.publish(String(data=key))

    def _call_arm(self, client) -> None:
        if not client.wait_for_service(timeout_sec=2.0):
            self.get_logger().error("机械臂服务不可用")
            self._fail()
            return
        self._arm_future = client.call_async(Trigger.Request())

    def _arm_done(self) -> bool:
        if self._arm_future is None or not self._arm_future.done():
            return False
        result = self._arm_future.result()
        self._arm_future = None
        if not result.success:
            self.get_logger().error(result.message)
            self._fail()
            return False
        return True

    def _fail(self) -> None:
        self._mode_pub.publish(UInt8(data=3))
        self._state = State.FAILED

    def _start_pick(self) -> None:
        self.get_logger().info("开始抓取")
        self._call_arm(self._arm_pick)
        self._state = State.PICK

    def _start_place(self) -> None:
        self.get_logger().info("开始投放")
        self._call_arm(self._arm_place)
        self._state = State.PLACE

    def _tick(self) -> None:
        if self._state == State.LINE_TO_PICKUP and self._station == self.STATION_PICKUP:
            self._mode_pub.publish(UInt8(data=0))
            self._pending_after_voice = self._start_pick
            self._say("arrive_pickup")
            self._state = State.WAIT_ANNOUNCE

        elif self._state == State.PICK and self._arm_done():
            self._say("pick_done")
            self.get_logger().info("抓取完成，前往投放点")
            self._station = 0
            self._mode_pub.publish(UInt8(data=2))
            self._state = State.LINE_TO_DROP

        elif self._state == State.LINE_TO_DROP and self._station == self.STATION_DROP:
            self._mode_pub.publish(UInt8(data=0))
            self._pending_after_voice = self._start_place
            self._say("arrive_drop")
            self._state = State.WAIT_ANNOUNCE

        elif self._state == State.PLACE and self._arm_done():
            self._say("place_done")
            self._say("returning")
            self.get_logger().info("投放完成，返航")
            self._station = 0
            self._mode_pub.publish(UInt8(data=2))
            self._state = State.LINE_HOME

        elif self._state == State.LINE_HOME and self._station == self.STATION_HOME:
            self._mode_pub.publish(UInt8(data=0))
            self._say("home")
            self._state = State.DONE

        if self._state == State.DONE:
            self.get_logger().info("一次取货闭环完成")
            self._state = State.IDLE
        elif self._state == State.FAILED:
            self.get_logger().error("任务失败，已急停")
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
