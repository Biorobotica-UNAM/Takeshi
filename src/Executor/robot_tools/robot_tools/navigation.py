from known_locations_interfaces.srv import LocationsGet

from .service_client_base import ServiceClientBase


class Navigation(ServiceClientBase):
    def __init__(self, node):
        super().__init__(node)

        self.go_to_cli = node.create_client(
            LocationsGet,
            '/nav_controller/go_to_location'
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