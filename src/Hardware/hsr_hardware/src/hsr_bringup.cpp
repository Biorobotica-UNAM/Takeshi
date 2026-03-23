#include <rclcpp/rclcpp.hpp>
#include <std_srvs/srv/empty.hpp>
#include <chrono>

int main(int argc, char * argv[])
{
  using namespace std::chrono_literals;
  rclcpp::init(argc, argv);

  auto node = rclcpp::Node::make_shared("hsr_bringup");

  auto client =
    node->create_client<std_srvs::srv::Empty>("/stop_sending_localized_pose");

  while(!client->wait_for_service(3s)) {
    if (!rclcpp::ok()) {
        RCLCPP_ERROR(node->get_logger(), "Error while waiting for service. Exiting...");
        return 1;
    }
    RCLCPP_ERROR(node->get_logger(), "Service not available.");
    return 1;
  }

  auto request = std::make_shared<std_srvs::srv::Empty::Request>();

  auto result_future = client->async_send_request(request);

  if (rclcpp::spin_until_future_complete(node, result_future, 5s) ==
      rclcpp::FutureReturnCode::SUCCESS)
  {
    auto response = result_future.get();
      RCLCPP_INFO(node->get_logger(),
                  "Successfully sent stop signal.");
  } else {
    RCLCPP_ERROR(node->get_logger(), "Service call failed.");
  }

  rclcpp::shutdown();
  return 0;
}