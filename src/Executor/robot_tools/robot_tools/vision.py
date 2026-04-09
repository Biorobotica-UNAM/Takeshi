from std_srvs.srv import SetBool

from .service_client_base import ServiceClientBase

class Vision(ServiceClientBase):
    def __init__(self, node):
        super().__init__(node)

        self.find_object_cli = node.create_client(SetBool, '/find_object')

    def find_object(self, class_name: str, model: str) -> bool:
        req = SetBool.Request()
        req.data = True  # Aquí podrías usar class_name y model para personalizar la búsqueda

        res = self.call_service(self.find_object_cli, req, '/find_object')
        return res.success if res else False