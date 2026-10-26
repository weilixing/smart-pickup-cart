from setuptools import find_packages, setup

setup(
    name="lift_controller",
    version="0.1.0",
    packages=find_packages(exclude=["test"]),
    data_files=[
        ("share/ament_index/resource_index/packages", ["resource/lift_controller"]),
        ("share/lift_controller", ["package.xml"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="lixing",
    maintainer_email="lixing@example.com",
    description="顶升平台控制",
    license="MIT",
    entry_points={
        "console_scripts": [
            "lift_controller_node = lift_controller.lift_controller_node:main",
        ],
    },
)
