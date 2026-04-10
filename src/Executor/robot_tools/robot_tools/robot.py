#from rclpy.node import Node
from rclpy.callback_groups import ReentrantCallbackGroup
from .hardware import Hardware
from .hri import HRI
from .vision import Vision
from .tasks import Tasks
from .navigation import Navigation


class Robot:
    def __init__(self, node):
        self.node = node
        self.group = ReentrantCallbackGroup()

        # módulos
        self.hri = HRI(node, self.group)
        self.vision = Vision(node, self.group)
        self.tasks = Tasks(node, self.group)
        self.navigation = Navigation(node, self.group)
        self.hardware = Hardware(node, self.group)

    # =========================
    # Shortcuts (opcional)
    # =========================
    def say(self, text: str):
        return self.hri.say(text)

    def listen(self, grammar: str = ''):
        return self.hri.listen(grammar)

    def follow_person(self, enable: bool):
        return self.hri.follow_person(enable)
    
    def find_object(self, class_name: str):
        return self.tasks.find_object(class_name)
    
    def go_to(self, location_name: str):
        return self.navigation.go_to(location_name)
    
    def check_door_opened(self):
        return self.hardware.check_door()
    
