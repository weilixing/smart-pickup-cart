from setuptools import find_packages, setup

setup(
    name="voice_announce",
    version="0.1.0",
    packages=find_packages(exclude=["test"]),
    data_files=[
        ("share/ament_index/resource_index/packages", ["resource/voice_announce"]),
        ("share/voice_announce", ["package.xml"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="weilixing",
    maintainer_email="weilixing@example.com",
    description="语音播报调度",
    license="MIT",
    entry_points={
        "console_scripts": [
            "voice_node = voice_announce.voice_node:main",
        ],
    },
)
