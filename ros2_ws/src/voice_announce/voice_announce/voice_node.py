import threading

import rclpy
from rclpy.node import Node
from std_msgs.msg import String, UInt8
from std_srvs.srv import Trigger

# 与 RA8 / SYN6288 预置语编号约定一致
PHRASES = {
    "arrive_pickup": 1,  # 已到达取货点
    "pick_done": 2,  # 抓取完成
    "arrive_drop": 3,  # 已到达投放点
    "place_done": 4,  # 投放完成
    "returning": 5,  # 开始返航
    "home": 6,  # 已返回出发点
}


class VoiceAnnounce(Node):
    def __init__(self) -> None:
        super().__init__("voice_announce")
        self.declare_parameter("timeout", 8.0)
        self._pub = self.create_publisher(UInt8, "voice/play", 10)
        self.create_subscription(UInt8, "voice/done", self._on_done, 10)
        self.create_subscription(String, "voice/say", self._on_say, 10)
        self.create_service(Trigger, "voice/test", self._on_test)
        self._expect = None
        self._done = threading.Event()

    def _on_done(self, msg: UInt8) -> None:
        if self._expect is None or msg.data == self._expect:
            self._done.set()

    def _on_say(self, msg: String) -> None:
        key = msg.data.strip()
        if key not in PHRASES:
            self.get_logger().warn(f"未知提示语键：{key}")
            return
        self.play(key)

    def _on_test(self, _req, response):
        ok = self.play("arrive_pickup")
        response.success = ok
        response.message = "播报测试完成" if ok else "播报超时"
        return response

    def play(self, key: str) -> bool:
        index = PHRASES[key]
        self._expect = index
        self._done.clear()
        self._pub.publish(UInt8(data=index))
        timeout = self.get_parameter("timeout").value
        return self._done.wait(timeout)


def main(args=None) -> None:
    rclpy.init(args=args)
    node = VoiceAnnounce()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
