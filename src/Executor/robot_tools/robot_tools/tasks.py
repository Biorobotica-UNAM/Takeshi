from std_srvs.srv import Trigger
from vision_interfaces.srv import FindObject
from faces_interfaces.srv import FaceRecog

from .service_client_base import ServiceClientBase


class Tasks(ServiceClientBase):
    def __init__(self, node, callback_group=None):
        super().__init__(node)

        self.find_object_cli = node.create_client(FindObject, '/tasks/find_object', callback_group=callback_group)
        self.find_person_cli = node.create_client(FaceRecog, '/tasks/find_person', callback_group=callback_group)
        self.find_seat_cli   = node.create_client(Trigger, '/tasks/find_seat', callback_group=callback_group)

    # ---------------------------------------------------------
    # FindObject
    # ---------------------------------------------------------
    def find_object(self, class_name: str):
        req = FindObject.Request()
        req.class_name = class_name

        res = self.call_service(self.find_object_cli, req, '/tasks/find_object', call_timeout=10.0)

        if res is None:
            self.node.get_logger().error("FindObject falló (timeout o error)")
            return None

        if res.success:
            self.node.get_logger().info(f"FindObject OK: {res.detected_class}")
        else:
            self.node.get_logger().error(f"FindObject ERROR: {res.message}")

        return res

    # ---------------------------------------------------------
    # FindPerson
    # ---------------------------------------------------------
    def find_person(self, name: str):
        req = FaceRecog.Request()
        req.name = name

        res = self.call_service(self.find_person_cli, req, '/tasks/find_person', call_timeout=10.0)

        if res is None:
            self.node.get_logger().error("FindPerson falló (timeout o error)")
            return None

        if res.success:
            self.node.get_logger().info(f"FindPerson OK: {res.name}")
        else:
            self.node.get_logger().error("FindPerson ERROR: person not found")

        return res

    # ---------------------------------------------------------
    # FindSeat
    # ---------------------------------------------------------
    def find_seat(self):
        req = Trigger.Request()

        res = self.call_service(self.find_seat_cli, req, '/tasks/find_seat', call_timeout=10.0)

        if res is None:
            self.node.get_logger().error("FindSeat falló (timeout o error)")
            return None

        if res.success:
            self.node.get_logger().info(f"FindSeat OK: {res.message}")
        else:
            self.node.get_logger().error(f"FindSeat ERROR: {res.message}")

        return res