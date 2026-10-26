from setuptools import find_packages, setup

setup(
    name="motor_bridge",
    version="0.1.0",
    packages=find_packages(exclude=["test"]),
    data_files=[
        ("share/ament_index/resource_index/packages", ["resource/motor_bridge"]),
        ("share/motor_bridge", ["package.xml"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="lixing",
    maintainer_email="lixing@example.com",
    description="OrangePi 与 RA8 之间的串口桥",
    license="MIT",
    entry_points={
        "console_scripts": [
            "motor_bridge_node = motor_bridge.motor_bridge_node:main",
        ],
    },
)
