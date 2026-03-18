#include "rclcpp/rclcpp.hpp"
#include <cmath>
#include <algorithm>

#include "std_msgs/msg/float64_multi_array.hpp"
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "geometry_msgs/msg/point_stamped.hpp"
#include "actionlib_msgs/msg/goal_status.hpp"

#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include "tf2/exceptions.h"

class HeadBrigdeNode : public rclcpp::Node
{
public:
  HeadBrigdeNode() : Node("head_control_node"), tf_buffer_(this->get_clock()), tf_listener_(tf_buffer_)
  {

    // Declare parameters
    this->declare_parameter<std::string>( "head_goal_topic",          "/hardware/head/goal_pose");
    this->declare_parameter<std::string>( "head_current_pose_topic",  "/hardware/head/current_pose");
    //this->declare_parameter<std::string>( "head_goal_reached_topic",  "/hardware/head/goal_reached");

    this->declare_parameter<std::string>("camera_frame",                "head_rgbd_sensor_link");
    this->declare_parameter<std::string>("base_frame",                  "base_link");
    //this->declare_parameter<std::string>("neck_frame",                  "torso_lift_link");

    this->declare_parameter<double>("pan_min", -1.57);
    this->declare_parameter<double>("pan_max", 1.57);
    this->declare_parameter<double>("tilt_min", -0.9);
    this->declare_parameter<double>("tilt_max", 0.47);
    this->declare_parameter<double>("dead_zone_threshold", 0.05);

    head_goal_topic_ = this->get_parameter("head_goal_topic").as_string();
    head_cp_topic_ = this->get_parameter("head_current_pose_topic").as_string();

    camera_frame_ = this->get_parameter("camera_frame").as_string();
    base_frame_ = this->get_parameter("base_frame").as_string();
    //neck_frame_ = this->get_parameter("neck_frame").as_string();

    pan_min = static_cast<float>(this->get_parameter("pan_min").as_double());
    pan_max = static_cast<float>(this->get_parameter("pan_max").as_double());
    tilt_min = static_cast<float>(this->get_parameter("tilt_min").as_double());
    tilt_max = static_cast<float>(this->get_parameter("tilt_max").as_double());
    dead_zone_threshold = static_cast<float>(this->get_parameter("dead_zone_threshold").as_double());
    // Service Client
    
    // Subscribers
    head_cp_subscriber_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
        head_cp_topic_, 10,
        std::bind(&HeadBrigdeNode::headCurrentPoseCallback, this, std::placeholders::_1));
    
    look_at_subscriber_ = this->create_subscription<geometry_msgs::msg::PointStamped>(
        "/hardware/head/look_at", 1,
        std::bind(&HeadBrigdeNode::lookAtCallbackFixed, this, std::placeholders::_1));

    nav_status_subscriber_ = this->create_subscription<actionlib_msgs::msg::GoalStatus>(
        "/navigation/status", 1,
        std::bind(&HeadBrigdeNode::navigationStatusCallback, this, std::placeholders::_1));

     // Publishers

    // Publishers
    head_goal_publisher_ = this->create_publisher<std_msgs::msg::Float64MultiArray>(
        head_goal_topic_, 10);

    RCLCPP_INFO(this->get_logger(), "HEAD CONTROL NODE BRIDGE BY RUSANROD HAS BEEN STARTED.");
}
private:
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr head_cp_subscriber_;
    rclcpp::Subscription<geometry_msgs::msg::PointStamped>::SharedPtr look_at_subscriber_;
    rclcpp::Subscription<actionlib_msgs::msg::GoalStatus>::SharedPtr nav_status_subscriber_;
    rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr head_goal_publisher_;

    tf2_ros::Buffer tf_buffer_;
    tf2_ros::TransformListener tf_listener_;

    // Parameters
    std::string head_goal_topic_;
    std::string head_cp_topic_;

    std::string camera_frame_;
    std::string base_frame_;
    //std::string neck_frame_;
    
    // System variables
    float pan_min, pan_max, tilt_min, tilt_max, dead_zone_threshold;

    float current_pan = 0.0;
    float current_tilt = 0.0;
    float alpha = 0.5; // Smoothing factor for head movement

    // Utility functions

    std::vector<float> clampHeadPose(float goalPan, float goalTilt)
    {
        // Pan limits
        float clampedPan = std::clamp(goalPan, pan_min, pan_max);
        float clampedTilt = std::clamp(goalTilt, tilt_min, tilt_max);

        return {clampedPan, clampedTilt};
    }

    // Callbacks
    void headCurrentPoseCallback(const std_msgs::msg::Float64MultiArray::SharedPtr msg)
    {
        if (msg->data.size() < 2) {
            RCLCPP_WARN(this->get_logger(), "Current pose message has less than 2 elements.");
            return;
        }

        current_pan = static_cast<float>(msg->data[0]);
        current_tilt = static_cast<float>(msg->data[1]);
    }
/*
    void lookAtCallback(const geometry_msgs::msg::PointStamped::SharedPtr msg)
    {
        try {
        // Transform the point to the camera frame
        geometry_msgs::msg::PointStamped p_in_camera_frame;
        tf_buffer_.transform(*msg, p_in_camera_frame, camera_frame_);

        // Compute pan and tilt angles
        pointHeadTo(p_in_camera_frame.point.x,
                p_in_camera_frame.point.y,
                p_in_camera_frame.point.z, false);

        } catch (tf2::TransformException &ex) {
        RCLCPP_WARN(this->get_logger(), "Transform failed: %s", ex.what());
        }
    }*/

    void lookAtCallback(const geometry_msgs::msg::PointStamped::SharedPtr msg)
    {
        RCLCPP_INFO(this->get_logger(), "Received look_at_point command: [%.2f, %.2f, %.2f] in frame %s",
                    msg->point.x, msg->point.y, msg->point.z, msg->header.frame_id.c_str());
        try {
            // Transform to a fixed frame to ensure consistent calculations
            geometry_msgs::msg::PointStamped p_cam;
            //tf_buffer_.transform(*msg, p_cam, neck_frame_);

            double x = p_cam.point.x;
            double y = p_cam.point.y;
            double z = p_cam.point.z;

            double pan = std::atan2(y, x);
            double dist_xy = std::sqrt(x * x + y * y);
            double tilt = std::atan2(z, dist_xy);

            auto clamped_angles = clampHeadPose(static_cast<float>(pan), static_cast<float>(tilt));

            float pan_delta = std::abs(clamped_angles[0] - current_pan);
            float tilt_delta = std::abs(clamped_angles[1] - current_tilt);

            // Avoids minimal and unnecessary movements
            float pan_target = pan_delta > dead_zone_threshold ? clamped_angles[0] : current_pan;
            float tilt_target = tilt_delta > dead_zone_threshold ? clamped_angles[1] : current_tilt;
            
            pan_target = (alpha * pan_target) ;
            tilt_target = (alpha * tilt_target);

            std_msgs::msg::Float64MultiArray head_goal_msg;
            head_goal_msg.data = {pan_target, tilt_target};
            //head_goal_msg.data = {current_pan, current_tilt};
            head_goal_publisher_->publish(head_goal_msg);

        } catch (tf2::TransformException &ex) {
            RCLCPP_WARN(this->get_logger(), "TF Error: %s", ex.what());
        }
    }

    void lookAtCallbackFixed(const geometry_msgs::msg::PointStamped::SharedPtr msg)
    {
        RCLCPP_INFO(this->get_logger(), "Received look_at_point command: [%.2f, %.2f, %.2f] in frame %s",
                    msg->point.x, msg->point.y, msg->point.z, msg->header.frame_id.c_str());
        try {
            // Transform to a fixed frame to ensure consistent calculations
            geometry_msgs::msg::PointStamped target_in_base;
            tf_buffer_.transform(*msg, target_in_base, camera_frame_);
            
            geometry_msgs::msg::TransformStamped tf_base_to_cam;
            tf_base_to_cam = tf_buffer_.lookupTransform(base_frame_, camera_frame_, tf2::TimePointZero);

            double dx = target_in_base.point.x - tf_base_to_cam.transform.translation.x;
            double dy = target_in_base.point.y - tf_base_to_cam.transform.translation.y;
            double dz = target_in_base.point.z - tf_base_to_cam.transform.translation.z;
            
            double dist_xy = std::sqrt(dx * dx + dy * dy);
            
            if (dist_xy < 1e-4) return;

            double pan = std::atan2(dy, dx);
            double tilt = -std::atan2(dz, dist_xy);

            auto clamped = clampHeadPose(static_cast<float>(pan), static_cast<float>(tilt));

            float pan_delta  = std::abs(clamped[0] - current_pan);
            float tilt_delta = std::abs(clamped[1] - current_tilt);

            float pan_target  = pan_delta  > dead_zone_threshold ? clamped[0] : current_pan;
            float tilt_target = tilt_delta > dead_zone_threshold ? clamped[1] : current_tilt;

            std_msgs::msg::Float64MultiArray head_goal_msg;
            head_goal_msg.data = {clamped[0], clamped[1]};
            head_goal_publisher_->publish(head_goal_msg);

        } catch (tf2::TransformException &ex) {
            RCLCPP_WARN(this->get_logger(), "TF Error: %s", ex.what());
        }
    }

    void navigationStatusCallback(const actionlib_msgs::msg::GoalStatus::SharedPtr msg)
    {
        // Check if the navigation goal has been reached
        if (msg->status == actionlib_msgs::msg::GoalStatus::SUCCEEDED) {
            std_msgs::msg::Float64MultiArray head_goal_msg;
            head_goal_msg.data = {0.0, 0.0}; // Reset to default pose
            head_goal_publisher_->publish(head_goal_msg);
        }
    }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HeadBrigdeNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}