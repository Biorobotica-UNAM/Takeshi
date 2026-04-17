from human_interfaces.srv import DetectPointing
from std_srvs.srv import SetBool

from .service_client_base import ServiceClientBase

class Vision(ServiceClientBase):
    def __init__(self, node):
        super().__init__(node)

        self.find_waving_people_action = node.create_client(DetectPointing, '/tasks/find_waving_human')
        self.focus_action = node.create_client(SetBool, '/vision/focus')
    
    def find_people(self) -> bool:
        req = DetectPointing.Request()

        res = self.call_service(self.find_waving_people_action, req, '/tasks/find_waving_human')
        return res.success if res else False
    
    def focus(self) -> bool:
        req = SetBool.Request()
        req.data = True

        res = self.call_service(self.focus_action, req, '/vision/focus')
        return res.success if res else False
    
    def unfocus(self) -> bool:
        req = SetBool.Request()
        req.data = False

        res = self.call_service(self.focus_action, req, '/vision/focus')
        return res.success if res else False
    
