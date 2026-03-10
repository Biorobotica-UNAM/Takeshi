#include "rclcpp/rclcpp.hpp"
#include <cmath>

#include "std_srvs/srv/trigger.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"
#include "std_msgs/msg/string.hpp"
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "geometry_msgs/msg/point_stamped.hpp"

#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include "tf2/exceptions.h"

class HeadBrigdeNode : public rclcpp::Node
{
public:
  HeadBrigdeNode() : Node("head_control_node"), tf_buffer_(this->get_clock()), tf_listener_(tf_buffer_)
  {

    // Declare parameters
    this->declare_parameter<std::string>( "head_goal_topic",          "head/goal_pose");
    this->declare_parameter<std::string>( "head_current_topic",       "head/current_pose");
    this->declare_parameter<std::string>( "head_goal_reached_topic",  "head/goal_reached");

    this->declare_parameter<std::string>("camera_frame",                "camera_link");
    this->declare_parameter<std::string>("base_frame",                  "base_link");

    this->declare_parameter<double>("pan_min", -1.57);
    this->declare_parameter<double>("pan_max", 1.57);
    this->declare_parameter<double>("tilt_min", -0.9);
    this->declare_parameter<double>("tilt_max", 0.47);
    this->declare_parameter<double>("dead_zone_threshold", 0.05);

    head_goal_topic_ = this->get_parameter("head_goal_topic").as_string();
    head_cp_topic_ = this->get_parameter("head_current_pose_topic").as_string();

    camera_frame_ = this->get_parameter("camera_frame").as_string();
    base_frame_ = this->get_parameter("base_frame").as_string();

    pan_min = this->get_parameter("pan_min").as_double();
    pan_max = this->get_parameter("pan_max").as_double();
    tilt_min = this->get_parameter("tilt_min").as_double();
    tilt_max = this->get_parameter("tilt_max").as_double();
    dead_zone_threshold = this->get_parameter("dead_zone_threshold").as_double();

    // Service Client
    
    // Subscribers
    head_cp_subscriber_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
        head_cp_topic_, 10,
        std::bind(&HeadBrigdeNode::headCurrentPoseCallback, this, std::placeholders::_1));
    
    look_at_subscriber_ = this->create_subscription<geometry_msgs::msg::PointStamped>(
        "head/look_at_point", 10,
        std::bind(&HeadBrigdeNode::lookAtCallback, this, std::placeholders::_1));

    head_tracking_ = this->create_subscription<std_msgs::msg::String>(
        "head/tracking_target_frame", 10,
        std::bind(&HeadBrigdeNode::headTrackingCallback, this, std::placeholders::_1));    
        // Publishers
    head_goal_publisher_ = this->create_publisher<std_msgs::msg::Float64MultiArray>(
        head_goal_topic_, 10);

    // Timer

    tracking_timer_ = this->create_wall_timer(
        std::chrono::milliseconds(100),
        std::bind(&HeadBrigdeNode::onTrackingTick, this));
    tracking_timer_->cancel();


    RCLCPP_INFO(this->get_logger(), "HEAD CONTROL NODE BRIDGE BY RUSANROD HAS BEEN STARTED.");
}
private:
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr head_cp_subscriber_;
    rclcpp::Subscription<geometry_msgs::msg::PointStamped>::SharedPtr look_at_subscriber_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr head_tracking_;
    rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr head_goal_publisher_;

    tf2_ros::Buffer tf_buffer_;
    tf2_ros::TransformListener tf_listener_;

    // Parameters
    std::string head_goal_topic_;
    std::string head_cp_topic_;

    std::string camera_frame_;
    std::string base_frame_;
    
    // System variables
    double pan_min, pan_max, tilt_min, tilt_max, dead_zone_threshold;

    float current_pan = 0.0;
    float current_tilt = 0.0;
    bool tracking_enabled_ = false;
    float tracking_gain_ = 0.5;

    std::string target_frame_to_track_;
    rclcpp::TimerBase::SharedPtr tracking_timer_;

    // Utility functions

    std::vector<float> limitHeadPose(float goalPan, float goalTilt)
    {
        // Pan limits
        if(goalPan > pan_max) goalPan = pan_max;
        if(goalPan < pan_min) goalPan = pan_min;
        
        // Tilt limits
        if(goalTilt > tilt_max) goalTilt = tilt_max;
        if(goalTilt < tilt_min) goalTilt = tilt_min;

        return {goalPan, goalTilt};
    }

    /*
    std::vector<float> xyzToPanTilt(float dx, float dy, float dz)
    {
        float pan = atan2(dy, dx);
        float dist_xy = sqrt(dx * dx + dy * dy);
        // dz depends on the robot's z axis orientation (+, -)
        float tilt = atan2(dz, dist_xy);
        return {pan, tilt};
    }
    */

    // Funcion corregida (para testear)
    std::vector<float> xyzToPanTilt(float x, float y, float z){
        float pan_error = atan2(x, z);

        float dist_xz = sqrt(x * x + z * z);
        float tilt_error = atan2(-y, dist_xz);
        return {pan_error, tilt_error};
    }

    void pointHeadTo(float x, float y, float z, bool track_mode = false)
    {
        auto error = xyzToPanTilt(x,y,z);

        if (!track_mode) tracking_gain_ = 1.0;

        if (std::abs(error[0]) < dead_zone_threshold) error[0] = 0.0;
        if (std::abs(error[1]) < dead_zone_threshold) error[1] = 0.0;

        float target_pan = current_pan + (error[0] * tracking_gain_);
        float target_tilt = current_tilt + (error[1] * tracking_gain_);
        auto limited = limitHeadPose(target_pan, target_tilt);

        // Create and publish message
        std_msgs::msg::Float64MultiArray head_goal_msg;
        head_goal_msg.data = {limited[0], limited[1]};
        head_goal_publisher_->publish(head_goal_msg);
    }

    void onTrackingTick(){
        if(tracking_enabled_) {
            try {
                // Get the current transform from base to target frame
                geometry_msgs::msg::TransformStamped transform = tf_buffer_.lookupTransform(
                    camera_frame_, target_frame_to_track_, tf2::TimePointZero);

                // Extract the translation components
                float dx = transform.transform.translation.x;
                float dy = transform.transform.translation.y;
                float dz = transform.transform.translation.z;

                // Point the head towards the target
                pointHeadTo(dx, dy, dz, true);

            } catch (tf2::TransformException &ex) {
                RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000,
                     "Tracking transform failed: %s", ex.what());
            }
        }
    }

    // Callbacks
    void headCurrentPoseCallback(const std_msgs::msg::Float64MultiArray::SharedPtr msg)
    {
        auto current = msg->data;
        current_pan = current[0];
        current_tilt = current[1];
    }

    void lookAtCallback(const geometry_msgs::msg::PointStamped::SharedPtr msg)
    {
        try {
        // Transform the point to the camera frame
        geometry_msgs::msg::PointStamped p_in_camera_frame;
        tf_buffer_.transform(*msg, p_in_camera_frame, camera_frame_);

        // Compute pan and tilt angles
        pointHeadTo(p_in_camera_frame.point.x,
                p_in_camera_frame.point.y,
                p_in_camera_frame.point.z);

        } catch (tf2::TransformException &ex) {
        RCLCPP_WARN(this->get_logger(), "Transform failed: %s", ex.what());
        }
    }

    void headTrackingCallback(const std_msgs::msg::String::SharedPtr msg)
    {
        if(msg->data.empty()) {
            tracking_enabled_ = false;
            tracking_timer_->cancel();
            RCLCPP_INFO(this->get_logger(), "Tracking detenido.");
        } else {
            target_frame_to_track_ = msg->data;
            tracking_enabled_ = true;
            tracking_timer_->reset(); // Reinicia el timer existente
            RCLCPP_INFO(this->get_logger(), "Tracking frame: %s", target_frame_to_track_.c_str());
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