from std_msgs.msg import Float64, Float64MultiArray, Float32
from std_srvs.srv import Trigger

from .service_client_base import ServiceClientBase


class Hardware(ServiceClientBase):
    def __init__(self, node):
        super().__init__(node)

        # =========================
        # Publishers
        # =========================
        self.pub_goal_gripper = node.create_publisher(
            Float64,
            '/hardware/arms/goal_gripper',
            10
        )

        self.pub_head_angles = node.create_publisher(
            Float64MultiArray,
            '/hardware/head/goal_pose',
            10
        )

        self.pub_torso_target = node.create_publisher(
            Float32,
            '/hardware/torso/goal',
            10
        )

        # =========================
        # Service clients
        # =========================
        self.cli_check_door = node.create_client(
            Trigger,
            '/hardware/door_detector/check'
        )

        self.cli_pointing_motion_enable = node.create_client(
            Trigger,
            '/hardware/pointing/enable'
        )

        self.cli_pointing_motion_disable = node.create_client(
            Trigger,
            '/hardware/pointing/disable'
        )

    # =========================================================
    # Gripper
    # =========================================================
    def open_gripper(self):
        msg = Float64()
        msg.data = -0.5236
        self.pub_goal_gripper.publish(msg)

    def close_gripper(self):
        msg = Float64()
        msg.data = 0.2618
        self.pub_goal_gripper.publish(msg)

    def relax_gripper(self):
        msg = Float64()
        msg.data = 0.0
        self.pub_goal_gripper.publish(msg)

    # =========================================================
    # Head
    # =========================================================
    def move_head(self, pan: float, tilt: float):
        msg = Float64MultiArray()
        msg.data = [pan, tilt]
        self.pub_head_angles.publish(msg)

    # =========================================================
    # Torso
    # =========================================================
    def move_torso(self, target: float):
        msg = Float32()
        msg.data = float(target)
        self.pub_torso_target.publish(msg)

    # =========================================================
    # Door
    # =========================================================
    def check_door(self) -> bool:
        req = Trigger.Request()

        try:
            res = self.call_service(
                self.cli_check_door,
                req,
                '/hardware/door_detector/check',
                wait_timeout=0.5,
                call_timeout=0.5
            )
        except Exception as e:
            self.node.get_logger().warn(
                f'Excepción en DoorDetector: {e}'
            )
            return False

        if res is None:
            self.node.get_logger().warn('Timeout esperando DoorDetector')
            return False

        return bool(res.success)

    # =========================================================
    # Pointing motion
    # =========================================================
    def pointing_motion_enable(self) -> bool:
        req = Trigger.Request()

        try:
            res = self.call_service(
                self.cli_pointing_motion_enable,
                req,
                '/hardware/pointing/enable',
                wait_timeout=0.5,
                call_timeout=0.5
            )
        except Exception as e:
            self.node.get_logger().warn(
                f'Excepción en PointingMotion enable: {e}'
            )
            return False

        if res is None:
            self.node.get_logger().warn(
                'Timeout esperando PointingMotion enable'
            )
            return False

        return bool(res.success)

    def pointing_motion_disable(self) -> bool:
        req = Trigger.Request()

        try:
            res = self.call_service(
                self.cli_pointing_motion_disable,
                req,
                '/hardware/pointing/disable',
                wait_timeout=0.5,
                call_timeout=0.5
            )
        except Exception as e:
            self.node.get_logger().warn(
                f'Excepción en PointingMotion disable: {e}'
            )
            return False

        if res is None:
            self.node.get_logger().warn(
                'Timeout esperando PointingMotion disable'
            )
            return False

        return bool(res.success)