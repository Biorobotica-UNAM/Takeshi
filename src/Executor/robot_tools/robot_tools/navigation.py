from known_locations_interfaces.srv import LocationsGet, LocationsServer

from .service_client_base import ServiceClientBase


class Navigation(ServiceClientBase):
    def __init__(self, node, callback_group=None):
        super().__init__(node)

        self.go_to_cli = node.create_client(
            LocationsGet,
            '/nav_controller/go_to_location', callback_group=callback_group
        )

    # ---------------------------------------------------------
    # GoTo
    # ---------------------------------------------------------
    def go_to(self, location_name: str) -> bool:
        req = LocationsGet.Request()
        req.location_name = location_name

        res = self.call_service(
            self.go_to_cli,
            req,
            '/nav_controller/go_to_location',
            wait_timeout=0.5,
            call_timeout=120.0
        )

        if res is None:
            self.node.get_logger().warn(
                f"Timeout o error en /nav_controller/go_to_location ({location_name})"
            )
            return False

        return bool(res.success)
    
    def set_location(self, location_name: str) -> bool :
        req = LocationsServer.Request()
        req.location_name = location_name

        res = self.call_service(
            self.go_to_cli,
            req,
            '/nav_controller/set_location',
            wait_timeout=0.5,
            call_timeout=5.0
        )

        if res is None:
            self.node.get_logger().warn(
                f"Timeout o error en /nav_controller/set_location ({location_name})"
            )
            return False

        return bool(res.success)