from std_srvs.srv import SetBool
from talker_interfaces.srv import Talk
from listener_interfaces.srv import Listen

from .service_client_base import ServiceClientBase


class HRI(ServiceClientBase):
    def __init__(self, node, group):
        super().__init__(node)

        self.talk_cli = node.create_client(Talk, '/talk', callback_group=group)
        self.listen_cli = node.create_client(Listen, '/listen', callback_group=group)
        self.follow_cli = node.create_client(SetBool, '/follow_person', callback_group=group)

    def say(self, text: str) -> bool:
        req = Talk.Request()
        req.text = text

        res = self.call_service(self.talk_cli, req, '/talk')
        return res.success if res else False

    def listen(self, grammar: str = '') -> str:
        req = Listen.Request()
        req.grammar = grammar

        res = self.call_service(self.listen_cli, req, '/listen')
        return res.text if res else ''

    def follow_person(self, enable: bool) -> bool:
        req = SetBool.Request()
        req.data = enable

        res = self.call_service(self.follow_cli, req, '/follow_person')
        return res.success if res else False