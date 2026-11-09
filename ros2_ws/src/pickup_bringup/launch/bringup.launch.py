from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    port_arg = DeclareLaunchArgument(
        "serial_port",
        default_value="/dev/ttyUSB0",
        description="RA8 串口设备",
    )

    description = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution(
                [FindPackageShare("pickup_description"), "launch", "description.launch.py"]
            )
        )
    )

    return LaunchDescription(
        [
            port_arg,
            description,
            Node(
                package="motor_bridge",
                executable="motor_bridge_node",
                parameters=[{"port": LaunchConfiguration("serial_port")}],
            ),
            Node(package="arm_controller", executable="arm_controller_node"),
            Node(package="voice_announce", executable="voice_node"),
            Node(package="task_orchestrator", executable="orchestrator_node"),
        ]
    )
