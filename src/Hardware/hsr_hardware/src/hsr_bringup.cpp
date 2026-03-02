#include <rclcpp/rclcpp.hpp>
#include <std_srvs/srv/empty.hpp>

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  auto node = rclcpp::Node::make_shared("hsr_bringup");

  auto client =
    node->create_client<std_srvs::srv::Empty>("/stop_sending_localized_pose");

  if (!client->wait_for_service(std::chrono::seconds(5))) {
    RCLCPP_ERROR(node->get_logger(), "Service not available.");
    rclcpp::shutdown();
    return 1;
  }

  auto request = std::make_shared<std_srvs::srv::Empty::Request>();

  auto future = client->async_send_request(request);

  if (rclcpp::spin_until_future_complete(node, future) ==
      rclcpp::FutureReturnCode::SUCCESS)
  {
    auto response = future.get();
      RCLCPP_INFO(node->get_logger(),
                  "Successfully sent stop signal.");
  } else {
    RCLCPP_ERROR(node->get_logger(), "Service call failed.");
  }

  rclcpp::shutdown();
  return 0;
}