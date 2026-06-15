import rclpy
from rclpy import node
from rclpy.node import Node
from enum import Enum, auto

from robot_tools.robot import Robot
from rclpy.executors import MultiThreadedExecutor


# =========================================================
# SUB-FSM: TIDY UP
# =========================================================
class TidyUpManager:

    class State(Enum):
        PERCEIVING = auto()
        PICKING = auto()
        PLACING = auto()
        CHECK_NEXT = auto()
        FINISHED = auto()

    def __init__(self, robot: Robot):
        self.robot = robot
        self.state = self.State.PERCEIVING
        self.objects_count = 0

    def run_step(self) -> bool:
        if self.state == self.State.PERCEIVING:
            self.robot.node.get_logger().info("[TidyUp] Escaneando mesa...")
            detected_obj = self.robot.find_object("all")
            self.state = self.State.PICKING

        elif self.state == self.State.PICKING:
            self.robot.node.get_logger().info("[TidyUp] Intentando agarre...")
            self.state = self.State.PLACING

        elif self.state == self.State.PLACING:
            self.robot.node.get_logger().info("[TidyUp] Objeto colocado.")
            self.state = self.State.CHECK_NEXT

        elif self.state == self.State.CHECK_NEXT:
            self.objects_count += 1
            if self.objects_count >= 2:
                self.state = self.State.FINISHED
            else:
                self.state = self.State.PERCEIVING

        elif self.state == self.State.FINISHED:
            return True

        return False


# =========================================================
# SUB-FSM: SERVE BREAKFAST
# =========================================================
class ServeBreakfastManager:

    class State(Enum):
        FIND_CEREAL = auto()
        POURING = auto()
        SERVING = auto()
        FINISHED = auto()

    def __init__(self, robot: Robot):
        self.robot = robot
        self.state = self.State.FIND_CEREAL

    def run_step(self) -> bool:
        if self.state == self.State.FIND_CEREAL:
            self.robot.say("Looking for the cereal box...")
            self.robot.node.get_logger().info("[Breakfast] Buscando cereal...")
            self.state = self.State.POURING

        elif self.state == self.State.POURING:
            self.robot.node.get_logger().info("[Breakfast] Sirviendo...")
            self.state = self.State.SERVING

        elif self.state == self.State.SERVING:
            self.robot.node.get_logger().info("[Breakfast] Listo.")
            self.state = self.State.FINISHED

        elif self.state == self.State.FINISHED:
            return True

        return False


# =========================================================
# FSM PRINCIPAL
# =========================================================
class GlobalState(Enum):
    INIT = auto()
    WAITING_DOOR = auto()
    NAV_KITCHEN = auto()
    TIDY_UP = auto()
    BREAKFAST = auto()
    BYE = auto()


class PickAndPlaceFSM(Node):

    def __init__(self):
        super().__init__('pick_and_place_fsm')

        self.state = GlobalState.INIT

        self.robot = None
        self.tidy_manager = None
        self.breakfast_manager = None

        self.timer = self.create_timer(0.5, self.control_loop)

    # -----------------------------------------------------
    def control_loop(self):

        if self.state == GlobalState.INIT:
            self.get_logger().info("Iniciando FSM...")

            self.robot = Robot(self)

            self.tidy_manager = TidyUpManager(self.robot)
            self.breakfast_manager = ServeBreakfastManager(self.robot)

            self.state = GlobalState.WAITING_DOOR

        elif self.state == GlobalState.WAITING_DOOR:

            self.robot.say("Waiting for the door to be opened...")
            if self.robot.check_door_opened():
                self.get_logger().info("¡Puerta abierta!")
                self.state = GlobalState.NAV_KITCHEN

        elif self.state == GlobalState.NAV_KITCHEN:
            self.robot.say("Navigating to the kitchen...")
            self.get_logger().info("Navegando a la cocina...")
            self.robot.navigation.go_to("kitchen")

            self.state = GlobalState.TIDY_UP

        elif self.state == GlobalState.TIDY_UP:
            self.robot.say("Tidying up the table...")

            if self.tidy_manager.run_step():
                self.get_logger().info(">> TidyUp completado")
                self.state = GlobalState.BREAKFAST

        elif self.state == GlobalState.BREAKFAST:
            self.robot.say("Serving breakfast...")

            if self.breakfast_manager.run_step():
                self.get_logger().info(">> Desayuno servido")
                self.state = GlobalState.BYE

        elif self.state == GlobalState.BYE:
            self.robot.say("Challenge completed")
            self.get_logger().info("Reto finalizado")

            self.timer.cancel()


# =========================================================
# MAIN
# =========================================================
def main():
    rclpy.init()
    node = PickAndPlaceFSM()
    
    executor = MultiThreadedExecutor()
    executor.add_node(node)
    try:
        executor.spin()
    finally:
        node.destroy_node()
        rclpy.shutdown()