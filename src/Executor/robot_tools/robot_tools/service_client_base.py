import rclpy
from rclpy.node import Node


class ServiceClientBase:
    def __init__(self, node: Node):
        self.node = node

    def call_service(
        self,
        client,
        request,
        service_name: str,
        wait_timeout: float = 2.0,
        call_timeout: float = 5.0,
    ):
        if not client.wait_for_service(timeout_sec=wait_timeout):
            self.node.get_logger().warn(
                f'Servicio no disponible: {service_name}'
            )
            return None

        future = client.call_async(request)
        rclpy.spin_until_future_complete(
            self.node,
            future,
            timeout_sec=call_timeout
        )

        if not future.done():
            self.node.get_logger().warn(
                f'Timeout al llamar servicio: {service_name}'
            )
            return None

        result = future.result()
        if result is None:
            self.node.get_logger().warn(
                f'La llamada falló: {service_name}'
            )
            return None

        return result