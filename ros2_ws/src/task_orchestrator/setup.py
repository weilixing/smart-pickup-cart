from setuptools import find_packages, setup

setup(
    name="task_orchestrator",
    version="0.1.0",
    packages=find_packages(exclude=["test"]),
    data_files=[
        ("share/ament_index/resource_index/packages", ["resource/task_orchestrator"]),
        ("share/task_orchestrator", ["package.xml"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="lixing",
    maintainer_email="lixing@example.com",
    description="任务状态机：接单 → 取货 → 投递 → 返回",
    license="MIT",
    entry_points={
        "console_scripts": [
            "orchestrator_node = task_orchestrator.orchestrator_node:main",
        ],
    },
)
