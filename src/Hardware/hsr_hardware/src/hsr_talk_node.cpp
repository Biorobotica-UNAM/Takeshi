#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <sstream>
#include <iterator>
#include <algorithm>

#include "rclcpp/rclcpp.hpp"
#include "talker_interfaces/srv/talk.hpp"
#include "tmc_voice_msgs/msg/voice.hpp"

namespace
{
std::string trim(const std::string & s)
{
  const auto begin = s.find_first_not_of(" \t\n\r");
  if (begin == std::string::npos)
    return "";

  const auto end = s.find_last_not_of(" \t\n\r");
  return s.substr(begin, end - begin + 1);
}

size_t count_words(const std::string & text)
{
  std::istringstream iss(text);
  return std::distance(std::istream_iterator<std::string>(iss),
                       std::istream_iterator<std::string>());
}

size_t count_char(const std::string & text, char c)
{
  return std::count(text.begin(), text.end(), c);
}

}

class HsrTalkNode : public rclcpp::Node
{
public:
  HsrTalkNode()
  : Node("hsr_talk_node")
  {
    this->declare_parameter<std::string>("service_name", "talker/speech");
    this->declare_parameter<std::string>("topic_name", "/talk_request");

    const auto service_name = this->get_parameter("service_name").as_string();
    const auto topic_name   = this->get_parameter("topic_name").as_string();

    pub_ = this->create_publisher<tmc_voice_msgs::msg::Voice>(topic_name, 10);

    srv_ = this->create_service<talker_interfaces::srv::Talk>(
      service_name,
      std::bind(&HsrTalkNode::handle_talk, this,
                std::placeholders::_1, std::placeholders::_2));

    RCLCPP_INFO(this->get_logger(),
      "Listo: servicio /%s -> topic %s",
      service_name.c_str(), topic_name.c_str());
  }

private:
    rclcpp::Publisher<tmc_voice_msgs::msg::Voice>::SharedPtr pub_;
    rclcpp::Service<talker_interfaces::srv::Talk>::SharedPtr srv_;

    double sec_per_word = 0.32;
    double pause_comma = 0.10;
    double pause_period = 0.25;
    double pause_question = 0.10;

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

        const size_t words   = count_words(text);
        const size_t commas  = count_char(text, ',');
        const size_t periods = count_char(text, '.');

        double estimated_time =
            (words * sec_per_word) +
            (commas * pause_comma) +
            (periods * pause_period);

        tmc_voice_msgs::msg::Voice voice_msg;
        voice_msg.interrupting = true;
        voice_msg.queueing = false;
        voice_msg.language = tmc_voice_msgs::msg::Voice::ENGLISH;
        voice_msg.sentence = text;
        pub_->publish(voice_msg);
           
        std::this_thread::sleep_for(std::chrono::duration<double>(estimated_time));
        RCLCPP_INFO(this->get_logger(), "Publicado: '%s'", text.c_str());

        response->success = true;
        response->message = "Texto publicado en el tópico";
    }

};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<HsrTalkNode>());
  rclcpp::shutdown();
  return 0;
}