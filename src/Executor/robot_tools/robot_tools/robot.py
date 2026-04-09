#from rclpy.node import Node
from .hardware import Hardware
from .hri import HRI
from .vision import Vision
from .tasks import Tasks
from .navigation import Navigation

class Robot:
    def __init__(self, node):
        self.node = node

        # módulos
        self.hri = HRI(node)
        self.vision = Vision(node)
        self.tasks = Tasks(node)
        self.navigation = Navigation(node)
        self.hardware = Hardware(node)

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
    
