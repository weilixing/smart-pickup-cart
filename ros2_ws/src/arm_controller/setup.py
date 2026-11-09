from setuptools import find_packages, setup

setup(
    name="arm_controller",
    version="0.1.0",
    packages=find_packages(exclude=["test"]),
    data_files=[
        ("share/ament_index/resource_index/packages", ["resource/arm_controller"]),
        ("share/arm_controller", ["package.xml"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="weilixing",
    maintainer_email="weilixing@example.com",
    description="四自由度舵机臂控制",
    license="MIT",
    entry_points={
        "console_scripts": [
            "arm_controller_node = arm_controller.arm_controller_node:main",
        ],
    },
)
