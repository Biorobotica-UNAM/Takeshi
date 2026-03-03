#include <cctype>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "talker_interfaces/srv/talk.hpp"

class HsrTalkNode : public rclcpp::Node
{
public:
  HsrTalkNode()
  : Node("hsr_talk_node")
  {
    this->declare_parameter<std::string>("service_name", "talker/speech");
    this->declare_parameter<std::string>("topic_name", "talker/speech");

    const auto service_name = this->get_parameter("service_name").as_string();
    const auto topic_name   = this->get_parameter("topic_name").as_string();

    pub_ = this->create_publisher<std_msgs::msg::String>(topic_name, 10);

    srv_ = this->create_service<talker_interfaces::srv::Talk>(
      service_name,
      std::bind(&HsrTalkNode::handle_talk, this,
                std::placeholders::_1, std::placeholders::_2));

    RCLCPP_INFO(this->get_logger(),
      "Listo: servicio /%s -> topic /%s",
      service_name.c_str(), topic_name.c_str());
  }

private:
  static std::string trim(std::string s)
  {
    auto is_space = [](unsigned char c){ return std::isspace(c); };
    while (!s.empty() && is_space(s.front())) s.erase(s.begin());
    while (!s.empty() && is_space(s.back()))  s.pop_back();
    return s;
  }

  void handle_talk(
    const std::shared_ptr<talker_interfaces::srv::Talk::Request> request,
    std::shared_ptr<talker_interfaces::srv::Talk::Response> response)
  {
    const std::string text = trim(request->text);

    if (text.empty()) {
      RCLCPP_WARN(this->get_logger(), "Texto vacío recibido");
      response->success = false;
      response->message = "Texto vacío";
      return;
    }

    std_msgs::msg::String msg;
    msg.data = text;
    pub_->publish(msg);

    RCLCPP_INFO(this->get_logger(), "Publicado: '%s'", text.c_str());

    response->success = true;
    response->message = "Texto publicado en el tópico";
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_;
  rclcpp::Service<talker_interfaces::srv::Talk>::SharedPtr srv_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<HsrTalkNode>());
  rclcpp::shutdown();
  return 0;
}