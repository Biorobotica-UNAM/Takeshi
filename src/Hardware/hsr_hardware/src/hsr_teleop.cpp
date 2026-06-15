#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>

#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <algorithm>
#include <chrono>

class KeyboardTeleopNode : public rclcpp::Node
{
public:
  KeyboardTeleopNode()
  : Node("keyboard_teleop_node"),
    running_(true),
    target_linear_(0.0),
    target_angular_(0.0),
    head_pan_(0.0),
    head_tilt_(0.0),
    head_pose_initialized_(false)
  {
    this->declare_parameter<std::string>("cmd_vel_topic", "/hardware/base/cmd_vel");
    this->declare_parameter<std::string>("head_goal_topic", "/hardware/head/goal_pose");
    this->declare_parameter<std::string>("head_current_pose_topic", "/hardware/head/current_pose");

    this->declare_parameter<double>("publish_rate", 20.0);

    this->declare_parameter<double>("linear_step", 0.05);
    this->declare_parameter<double>("angular_step", 0.15);
    this->declare_parameter<double>("max_linear_speed", 0.5);
    this->declare_parameter<double>("max_angular_speed", 1.5);

    this->declare_parameter<double>("head_step", 0.1);
    this->declare_parameter<double>("head_pan_min", -1.57);
    this->declare_parameter<double>("head_pan_max",  1.57);
    this->declare_parameter<double>("head_tilt_min", -0.9);
    this->declare_parameter<double>("head_tilt_max",  0.47);  // mismo que control node

    cmd_vel_topic_         = this->get_parameter("cmd_vel_topic").as_string();
    head_goal_topic_       = this->get_parameter("head_goal_topic").as_string();
    head_current_pose_topic_ = this->get_parameter("head_current_pose_topic").as_string();
    publish_rate_          = this->get_parameter("publish_rate").as_double();

    linear_step_           = this->get_parameter("linear_step").as_double();
    angular_step_          = this->get_parameter("angular_step").as_double();
    max_linear_speed_      = this->get_parameter("max_linear_speed").as_double();
    max_angular_speed_     = this->get_parameter("max_angular_speed").as_double();

    head_step_             = this->get_parameter("head_step").as_double();
    head_pan_min_          = this->get_parameter("head_pan_min").as_double();
    head_pan_max_          = this->get_parameter("head_pan_max").as_double();
    head_tilt_min_         = this->get_parameter("head_tilt_min").as_double();
    head_tilt_max_         = this->get_parameter("head_tilt_max").as_double();

  
    auto qos = rclcpp::QoS(rclcpp::KeepLast(10)); // Mantiene los últimos 10 mensajes
    qos.transient_local();  // <--- Esto arregla el error de DURABILITY

    // 2. Crea el publicador usando ese perfil
    cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>(
        cmd_vel_topic_, 
        qos
    );
    head_pub_    = this->create_publisher<std_msgs::msg::Float64MultiArray>(head_goal_topic_, 10);

    head_current_pose_sub_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
      head_current_pose_topic_, 10,
      std::bind(&KeyboardTeleopNode::headCurrentPoseCallback, this, std::placeholders::_1));

    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(static_cast<int>(1000.0 / publish_rate_)),
      std::bind(&KeyboardTeleopNode::publishBaseCmd, this));

    printHelp();

    keyboard_thread_ = std::thread(&KeyboardTeleopNode::keyboardLoop, this);
  }

  ~KeyboardTeleopNode() override
  {
    running_ = false;
    if (keyboard_thread_.joinable()) {
      keyboard_thread_.join();
    }
    stopRobot();
  }

private:
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr head_pub_;
  rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr head_current_pose_sub_;
  rclcpp::TimerBase::SharedPtr timer_;

  std::thread keyboard_thread_;
  std::mutex mutex_;
  std::atomic<bool> running_;

  std::string cmd_vel_topic_;
  std::string head_goal_topic_;
  std::string head_current_pose_topic_;

  double publish_rate_;

  double linear_step_;
  double angular_step_;
  double max_linear_speed_;
  double max_angular_speed_;

  double head_step_;
  double head_pan_min_;
  double head_pan_max_;
  double head_tilt_min_;
  double head_tilt_max_;

  double target_linear_;
  double target_angular_;

  double head_pan_;
  double head_tilt_;
  bool head_pose_initialized_;

  static double clamp(double value, double min_val, double max_val)
  {
    return std::max(min_val, std::min(value, max_val));
  }

  void printHelp()
  {
    RCLCPP_INFO(this->get_logger(), "Base: w/s/a/d, x stop");
    RCLCPP_INFO(this->get_logger(), "Head: i/k tilt, j/l pan, m center");
  }

  void headCurrentPoseCallback(const std_msgs::msg::Float64MultiArray::SharedPtr msg)
  {
    if (msg->data.size() < 2) {
      RCLCPP_WARN(this->get_logger(), "head current pose invalida");
      return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    head_pan_ = msg->data[0];
    head_tilt_ = msg->data[1];
    head_pose_initialized_ = true;
  }

  int getKey()
  {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    int ch = -1;
    int retval = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
    if (retval > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
      ch = getchar();
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
  }

  void keyboardLoop()
  {
    while (rclcpp::ok() && running_) {
      int key = getKey();
      if (key < 0) {
        continue;
      }

      bool publish_head_now = false;

      {
        std::lock_guard<std::mutex> lock(mutex_);

        switch (key) {
          case 'w':
            target_linear_ = clamp(target_linear_ + linear_step_, -max_linear_speed_, max_linear_speed_);
            break;
          case 's':
            target_linear_ = clamp(target_linear_ - linear_step_, -max_linear_speed_, max_linear_speed_);
            break;
          case 'a':
            target_angular_ = clamp(target_angular_ + angular_step_, -max_angular_speed_, max_angular_speed_);
            break;
          case 'd':
            target_angular_ = clamp(target_angular_ - angular_step_, -max_angular_speed_, max_angular_speed_);
            break;
          case 'x':
            target_linear_ = 0.0;
            target_angular_ = 0.0;
            break;

          case 'i':
            if (head_pose_initialized_) {
              head_tilt_ = clamp(head_tilt_ + head_step_, head_tilt_min_, head_tilt_max_);
              publish_head_now = true;
            }
            break;
          case 'k':
            if (head_pose_initialized_) {
              head_tilt_ = clamp(head_tilt_ - head_step_, head_tilt_min_, head_tilt_max_);
              publish_head_now = true;
            }
            break;
          case 'j':
            if (head_pose_initialized_) {
              head_pan_ = clamp(head_pan_ + head_step_, head_pan_min_, head_pan_max_);
              publish_head_now = true;
            }
            break;
          case 'l':
            if (head_pose_initialized_) {
              head_pan_ = clamp(head_pan_ - head_step_, head_pan_min_, head_pan_max_);
              publish_head_now = true;
            }
            break;
          case 'm':
            head_pan_ = 0.0;
            head_tilt_ = 0.0;
            publish_head_now = true;
            break;
          default:
            break;
        }
      }

      if (publish_head_now) {
        publishHeadCmd();
      }
    }
  }

  void publishBaseCmd()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    geometry_msgs::msg::Twist msg;
    msg.linear.x = target_linear_;
    msg.angular.z = target_angular_;
    cmd_vel_pub_->publish(msg);
  }

  void publishHeadCmd()
  {
    std_msgs::msg::Float64MultiArray msg;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      msg.data = {head_pan_, head_tilt_};
    }
    head_pub_->publish(msg);
  }

  void stopRobot()
  {
    geometry_msgs::msg::Twist base_msg;
    cmd_vel_pub_->publish(base_msg);
  }
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<KeyboardTeleopNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}