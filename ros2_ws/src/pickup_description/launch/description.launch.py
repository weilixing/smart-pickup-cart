from launch import LaunchDescription
from launch.substitutions import Command, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    urdf = PathJoinSubstitution(
        [FindPackageShare("pickup_description"), "urdf", "pickup_bot.urdf.xacro"]
    )

    return LaunchDescription(
        [
            Node(
                package="robot_state_publisher",
                executable="robot_state_publisher",
                parameters=[{"robot_description": Command(["xacro ", urdf])}],
            ),
        ]
    )
