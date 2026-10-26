from setuptools import find_packages, setup

setup(
    name="aruco_detect",
    version="0.1.0",
    packages=find_packages(exclude=["test"]),
    data_files=[
        ("share/ament_index/resource_index/packages", ["resource/aruco_detect"]),
        ("share/aruco_detect", ["package.xml"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="lixing",
    maintainer_email="lixing@example.com",
    description="ArUco 标记识别",
    license="MIT",
    entry_points={
        "console_scripts": [
            "aruco_node = aruco_detect.aruco_node:main",
        ],
    },
)
