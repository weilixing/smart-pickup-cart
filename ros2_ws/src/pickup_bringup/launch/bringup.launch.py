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

    ekf_config = PathJoinSubstitution(
        [FindPackageShare("pickup_bringup"), "config", "ekf.yaml"]
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
            Node(
                package="robot_localization",
                executable="ekf_node",
                name="ekf_filter_node",
                parameters=[ekf_config],
            ),
            Node(
                package="lift_controller",
                executable="lift_controller_node",
            ),
            # 雷达单独启动：ros2 launch ydlidar_ros2_driver ydlidar_launch.py
            # 建图：ros2 launch slam_toolbox online_async_launch.py params_file:=config/slam_toolbox.yaml
            # 导航：ros2 launch nav2_bringup navigation_launch.py
        ]
    )
