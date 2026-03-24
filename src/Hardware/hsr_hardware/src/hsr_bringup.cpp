#include <rclcpp/rclcpp.hpp>
#include <std_srvs/srv/empty.hpp>
#include <chrono>

int main(int argc, char * argv[])
{
  using namespace std::chrono_literals;
  rclcpp::init(argc, argv);

  auto node = rclcpp::Node::make_shared("hsr_bringup_client");

  // Quitamos el '/' inicial para mayor flexibilidad con namespaces
  auto client = node->create_client<std_srvs::srv::Empty>("/stop_sending_localized_pose");

  RCLCPP_INFO(node->get_logger(), "Esperando al servicio...");
  
  // Reintentar hasta que el servicio aparezca o ROS se apague
  while (!client->wait_for_service(1s)) {
    if (!rclcpp::ok()) {
      RCLCPP_ERROR(node->get_logger(), "Interrumpido mientras esperaba el servicio.");
      return 1;
    }
    RCLCPP_INFO(node->get_logger(), "Servicio no disponible, intentando de nuevo...");
  }

  auto request = std::make_shared<std_srvs::srv::Empty::Request>();
  auto result_future = client->async_send_request(request);

  RCLCPP_INFO(node->get_logger(), "Enviando petición...");

  // Esperar el resultado
  if (rclcpp::spin_until_future_complete(node, result_future, 5s) ==
      rclcpp::FutureReturnCode::SUCCESS)
  {
    RCLCPP_INFO(node->get_logger(), "Se envió la señal de parada exitosamente.");
  } else {
    RCLCPP_ERROR(node->get_logger(), "Error: El servicio no respondió a tiempo.");
  }

  rclcpp::shutdown();
  return 0;
}