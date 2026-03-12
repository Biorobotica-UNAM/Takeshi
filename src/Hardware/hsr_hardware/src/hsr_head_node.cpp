#include <rclcpp/rclcpp.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>

#include <std_msgs/msg/float64_multi_array.hpp>
#include <std_msgs/msg/bool.hpp>

#include <trajectory_msgs/msg/joint_trajectory.hpp>
#include <trajectory_msgs/msg/joint_trajectory_point.hpp>
#include <control_msgs/msg/joint_trajectory_controller_state.hpp>

#include <string>
#include <cmath>

class HeadController : public rclcpp::Node
{
public:
  HeadController()
  : Node("head_controller")
  {
    //############
    // Declare parameters with default values
    this->declare_parameter<bool>(        "use_namespace",            false);
    this->declare_parameter<std::string>( "head_cmd_topic_",          "/head_trajectory_controller/joint_trajectory");
    this->declare_parameter<std::string>( "head_state_topic",         "/head_trajectory_controller/controller_state");
    this->declare_parameter<std::string>( "head_goal_topic",          "head/goal_pose");
    this->declare_parameter<std::string>( "head_current_topic",       "head/current_pose");
    this->declare_parameter<std::string>( "head_goal_reached_topic",  "head/goal_reached");
    this->declare_parameter<double>("pan_min", -3.14);
    this->declare_parameter<double>("pan_max", 1.74);
    this->declare_parameter<double>("tilt_min", -0.9);
    this->declare_parameter<double>("tilt_max", 0.47);


    // Initialize internal variables from declared parameters
    this->get_parameter("head_cmd_topic_",          head_cmd_topic_);
    this->get_parameter("head_state_topic",         head_state_topic_);
    this->get_parameter("head_goal_topic",          head_goal_topic_);
    this->get_parameter("head_current_topic",       head_current_topic_);
    this->get_parameter("head_goal_reached_topic",  head_goal_reached_topic_);
    this->get_parameter("pan_min", pan_min_);
    this->get_parameter("pan_max", pan_max_);
    this->get_parameter("tilt_min", tilt_min_);
    this->get_parameter("tilt_max", tilt_max_);


    // Publishers
    pub_head_goal_traj_ = this->create_publisher<trajectory_msgs::msg::JointTrajectory>(
      head_cmd_topic_, 
      rclcpp::QoS(10).reliable());
    pub_head_current_pose_ = this->create_publisher<std_msgs::msg::Float64MultiArray>(
      head_current_topic_, 
      rclcpp::QoS(10).reliable());
    pub_head_goal_reached_ = this->create_publisher<std_msgs::msg::Bool>(
      head_goal_reached_topic_, 
      rclcpp::QoS(10).reliable());

    // Subscribers
    sub_head_goal_pose_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
      head_goal_topic_, rclcpp::QoS(10).reliable(),
      std::bind(&HeadController::headGoalPoseCallback, this, std::placeholders::_1));

    sub_head_state_ = this->create_subscription<control_msgs::msg::JointTrajectoryControllerState>(
      head_state_topic_, rclcpp::QoS(10).reliable(),
      std::bind(&HeadController::headStateCallback, this, std::placeholders::_1));

    // Init
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(100),
      std::bind(&HeadController::checkGoalReached, this));

    RCLCPP_INFO(this->get_logger(), "HeadController.->Node has been started.");
  }

private:

  std::string head_cmd_topic_;
  std::string head_state_topic_;
  std::string head_goal_topic_;
  std::string head_current_topic_;
  std::string head_goal_reached_topic_;

  double pan_min_, pan_max_, tilt_min_, tilt_max_;

  rclcpp::Publisher<trajectory_msgs::msg::JointTrajectory>::SharedPtr  pub_head_goal_traj_;
  rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr       pub_head_current_pose_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr                    pub_head_goal_reached_;

  rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr    sub_head_goal_pose_;
  rclcpp::Subscription<control_msgs::msg::JointTrajectoryControllerState>::SharedPtr sub_head_state_;

  float current_pan_ = 0.0f;
  float current_tilt_ = 0.0f;
  double max_speed = 1.5; // rad/s
  
  float target_pan_;
  float target_tilt_;
  
  bool initialized_{false};
  bool goal_received_{false};
  double goal_reached_threshold_ = 0.05;

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::TimerBase::SharedPtr startup_timer_;

  void headStateCallback(
      const control_msgs::msg::JointTrajectoryControllerState::SharedPtr msg)
  {
    if(!initialized_ && msg->actual.positions[0] == 0.0 && msg->actual.positions[1] == 0.0){
      initialized_ = true;
      sendHeadGoalTrajectory(0.0, 0.0, 1.0);
    }

    // expected: positions.size() >= 2  (0:tilt, 1:pan)
    if (msg->actual.positions.size() >= 2) {
      current_tilt_ = static_cast<float>(msg->actual.positions[0]); // tilt
      current_pan_ = static_cast<float>(msg->actual.positions[1]); // pan

      std_msgs::msg::Float64MultiArray arr;
      arr.data = {current_pan_, current_tilt_};
      pub_head_current_pose_->publish(arr);
    }

  }

  void headGoalPoseCallback(const std_msgs::msg::Float64MultiArray::SharedPtr msg)
  {
    if (msg->data.size() != 2) {
      RCLCPP_ERROR(this->get_logger(),
                   "[head_controller] Head goal must be 2 values [pan, tilt] (got %zu)",
                   msg->data.size());
      return;
    }

    double pan  = msg->data[0];
    double tilt = msg->data[1];

    // clamp to limits
    target_pan_ = std::clamp(pan, pan_min_, pan_max_);
    target_tilt_ = std::clamp(tilt, tilt_min_, tilt_max_);

    double error = std::hypot(target_pan_ - current_pan_, target_tilt_ - current_tilt_);
    double time = std::max(0.5, error / max_speed);

    if (error < goal_reached_threshold_) return;
    

    goal_received_ = true;
    sendHeadGoalTrajectory(target_pan_, target_tilt_, time);
  }

  void checkGoalReached(){
    double error = std::hypot(target_pan_ - current_pan_, target_tilt_ - current_tilt_);
    
    std_msgs::msg::Bool msg;
    msg.data = (error < 0.05); // Margen de error de ~3 grados
    pub_head_goal_reached_->publish(msg);
  }

  void sendHeadGoalTrajectory(float pan, float tilt, double time)
  {
    trajectory_msgs::msg::JointTrajectory traj;
    traj.joint_names = {"head_pan_joint", "head_tilt_joint"}; 

    trajectory_msgs::msg::JointTrajectoryPoint p;

    p.positions = {pan, tilt};
    p.time_from_start = rclcpp::Duration::from_seconds(time);
    traj.points.push_back(p);

    pub_head_goal_traj_->publish(traj);
  }

};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<HeadController>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}