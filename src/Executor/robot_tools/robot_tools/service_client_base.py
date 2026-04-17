import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient


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
    
    def call_action(
        self,
        client: ActionClient,
        goal_msg,
        action_name: str,
        wait_timeout: float = 2.0,
    ):
        if not client.wait_for_server(timeout_sec=wait_timeout):
            self.node.get_logger().error(f'Acción no disponible: {action_name}')
            return None

        # 1. Enviar el goal
        self.node.get_logger().info(f'Enviando meta a acción: {action_name}')
        send_goal_future = client.send_goal_async(goal_msg)
        rclpy.spin_until_future_complete(self.node, send_goal_future)

        goal_handle = send_goal_future.result()
        if not goal_handle or not goal_handle.accepted:
            self.node.get_logger().error(f'Meta rechazada por: {action_name}')
            return None

        # 2. Obtener el resultado
        result_future = goal_handle.get_result_async()
        rclpy.spin_until_future_complete(self.node, result_future)

        return result_future.result()