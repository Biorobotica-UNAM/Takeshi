from std_msgs.msg import Float64, Float64MultiArray, Float32
from std_srvs.srv import Trigger
from rclpy.action import ActionClient
from tmc_control_msgs.action import GripperApplyEffort
import time

from .service_client_base import ServiceClientBase


class Hardware(ServiceClientBase):
    def __init__(self, node, callback_group=None):
        super().__init__(node)

        # =========================
        # Publishers
        # =========================

        self.pub_head_angles = node.create_publisher(
            Float64MultiArray,
            '/hardware/head/goal_pose',
            10, callback_group=callback_group
        )

        self.pub_torso_target = node.create_publisher(
            Float32,
            '/hardware/torso/goal_pose',
            10, callback_group=callback_group
        )

        # =========================
        # Service clients
        # =========================
        self.cli_check_door = node.create_client(
            Trigger,
            '/hardware/door_detector/check', callback_group=callback_group
        )

        self.cli_pointing_motion_enable = node.create_client(
            Trigger,
            '/hardware/pointing/enable', callback_group=callback_group
        )

        self.cli_pointing_motion_disable = node.create_client(
            Trigger,
            '/hardware/pointing/disable', callback_group=callback_group
        )

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
    


class Gripper(ServiceClientBase):
    def __init__(self, node, callback_group=None):
        super().__init__(node)
        self._action_client = ActionClient(
            self.node, 
            GripperApplyEffort, 
            '/gripper_controller/grasp', callback_group=callback_group
        )

    def _manage_gripper(self, effort: float):
        time.sleep(1.0)
        
        goal = GripperApplyEffort.Goal()
        goal.effort = effort
        goal.do_control_stop = True

        # Usamos el nuevo placeholder de la base
        result = self.call_action(self._action_client, goal, "GripperGrasp")
        
        if result and result.status == 4: # STATUS_SUCCEEDED
            return True
        return False

    def open(self):
        return self._manage_gripper(-0.1)

    def close(self):
        return self._manage_gripper(0.1)