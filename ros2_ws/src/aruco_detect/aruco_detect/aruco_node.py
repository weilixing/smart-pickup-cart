import json

import cv2
import rclpy
from cv_bridge import CvBridge
from rclpy.node import Node
from sensor_msgs.msg import Image
from std_msgs.msg import String


class ArucoDetect(Node):
    """识别 ArUco 标记（DICT_4X4_50），以 JSON 发布 /detected_markers。

    v1 输出 id + 像素中心，供站点确认；二期接入相机内参后扩展 6DOF 位姿估计。
    """

    def __init__(self) -> None:
        super().__init__("aruco_detect")
        self.declare_parameter("image_topic", "/camera/image_raw")
        self.declare_parameter("dictionary", "DICT_4X4_50")

        dict_name = self.get_parameter("dictionary").value
        aruco_dict = cv2.aruco.getPredefinedDictionary(getattr(cv2.aruco, dict_name))
        self._detector = cv2.aruco.ArucoDetector(
            aruco_dict, cv2.aruco.DetectorParameters()
        )
        self._bridge = CvBridge()

        topic = self.get_parameter("image_topic").value
        self.create_subscription(Image, topic, self._on_image, 10)
        self._pub = self.create_publisher(String, "detected_markers", 10)

    def _on_image(self, msg: Image) -> None:
        frame = self._bridge.imgmsg_to_cv2(msg, desired_encoding="mono8")
        corners, ids, _ = self._detector.detectMarkers(frame)
        if ids is None:
            return

        markers = []
        for marker_corners, marker_id in zip(corners, ids.flatten()):
            pts = marker_corners[0]
            markers.append(
                {
                    "id": int(marker_id),
                    "cx": float(pts[:, 0].mean()),
                    "cy": float(pts[:, 1].mean()),
                }
            )
        self._pub.publish(String(data=json.dumps(markers)))


def main(args=None) -> None:
    rclpy.init(args=args)
    node = ArucoDetect()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
