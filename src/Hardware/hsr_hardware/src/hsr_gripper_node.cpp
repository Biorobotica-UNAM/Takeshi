#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "trajectory_msgs/msg/joint_trajectory.hpp"
#include "trajectory_msgs/msg/joint_trajectory_point.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "tmc_control_msgs/action/gripper_apply_effort.hpp" 
#include <cmath>
#include <chrono>

using namespace std::chrono_literals;

class GripperNode : public rclcpp::Node
{
public:
    using GripperApplyEffort = tmc_control_msgs::action::GripperApplyEffort;
    using GoalHandleGripper = rclcpp_action::ClientGoalHandle<GripperApplyEffort>;

    GripperNode()
    : Node("gripper_controller")
    {
        // Subscriber gripper command (corregido con std::bind)
        griper_cmd_pub_ = this-> create_publisher<trajectory_msgs::msg::JointTrajectory>(
            "/hardware/gripper/command", rclcpp::QoS(10).reliable());
        
        gripper_cmd_force_ = this->create_publisher<tmc_control_msgs::msg::GripperApplyEffort>(
            "/hardware/gripper/force_command", rclcpp::QoS(10).reliable());

        // Parámetros
        this->declare_parameter<double>("open_angle_deg", 90.0);
        this->declare_parameter<double>("half_open_angle_deg", 45.0);
        this->declare_parameter<double>("hand_close_force", 5.0);

        open_angle_ = degrees_to_radians(this->get_parameter("open_angle_deg").as_double());
        half_open_angle_ = degrees_to_radians(this->get_parameter("half_open_angle_deg").as_double());
        hand_close_force_ = this->get_parameter("hand_close_force").as_double();
    }

    void callback(const std_msgs::msg::String::SharedPtr msg)
    {
        if (msg->data == "open") {
            open(open_angle_);
        } else if (msg->data == "half_open") {
            open(half_open_angle_);
        } else if (msg->data == "close") {
            close(apply_force_client_, hand_close_force_);
        } else {
            RCLCPP_WARN(this->get_logger(), "Comando desconocido: '%s'", msg->data.c_str());
        }
    }

    // Función para cerrar con fuerza usando action client
    void close(rclcpp_action::Client<GripperApplyEffort>::SharedPtr client, double effort)
    {
        if (!client->wait_for_action_server(2s)) {
            RCLCPP_WARN(this->get_logger(), "Action server no disponible");
            return;
        }

        auto goal_msg = GripperApplyEffort::Goal();
        goal_msg.effort = effort;
        goal_msg.do_control_stop = true;

        client->async_send_goal(goal_msg);
    }

    // Función para abrir la mano
    void open(double position)
    {
        trajectory_msgs::msg::JointTrajectoryPoint point;
        point.positions.push_back(position);
        point.time_from_start.sec = 1;  // duración para abrir
        point.time_from_start.nanosec = 0;

        trajectory_msgs::msg::JointTrajectory traj;
        traj.header.stamp = this->get_clock()->now();
        traj.joint_names.push_back("hand_motor_joint");
        traj.points.push_back(point);

        gripper_trajectory_pub_->publish(traj);
    }

    // Función auxiliar
    static double degrees_to_radians(double deg)
    {
        return deg * M_PI / 180.0;
    }

private:
    rclcpp::Publisher<trajectory_msgs::msg::JointTrajectory>::SharedPtr gripper_trajectory_pub_;
    rclcpp_action::Client<GripperApplyEffort>::SharedPtr apply_force_client_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr gripper_command_sub_;

    double open_angle_;
    double half_open_angle_;
    double hand_close_force_;
};

// --- Main ---
int main(int argc, char ** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<GripperNode>();

    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}