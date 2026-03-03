#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/bool.hpp>
#include <std_msgs/msg/empty.hpp>
#include <std_srvs/srv/empty.hpp>
#include <geometry_msgs/msg/twist.hpp>

class HSRBumperNode : public rclcpp::Node
{
public:
    HSRBumperNode() : Node("hsr_bumper_node")
    {
        
        cli_bumper_reset_ = this->create_client<std_srvs::srv::Empty>("/bumper/reset");
        
        sub_front_bumper_ = this->create_subscription<std_msgs::msg::Bool>(
            "/base_f_bumper_sensor", 10,
            std::bind(&HSRBumperNode::f_bumper_callback, this, std::placeholders::_1));
            
        sub_back_bumper_ = this->create_subscription<std_msgs::msg::Bool>(
            "/base_b_bumper_sensor", 10,
            std::bind(&HSRBumperNode::b_bumper_callback, this, std::placeholders::_1));

        pub_nav_stop_ = this->create_publisher<std_msgs::msg::Empty>(
            "/navigation/stop", 10);

        pub_general_stop_ = this->create_publisher<std_msgs::msg::Empty>(
            "/stop", 10);
                
        pub_cmd_vel_ = this->create_publisher<geometry_msgs::msg::Twist>(
            "/hardware/base/cmd_vel", rclcpp::QoS(rclcpp::KeepLast(10))
            .reliability(rclcpp::ReliabilityPolicy::Reliable)
            .durability(rclcpp::DurabilityPolicy::TransientLocal));

        RCLCPP_INFO(this->get_logger(), "HSR Bumper Node has been started.");            
    }

private:
    rclcpp::Client<std_srvs::srv::Empty>::SharedPtr cli_bumper_reset_;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr sub_front_bumper_;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr sub_back_bumper_;
    rclcpp::Publisher<std_msgs::msg::Empty>::SharedPtr pub_nav_stop_;
    rclcpp::Publisher<std_msgs::msg::Empty>::SharedPtr pub_general_stop_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr pub_cmd_vel_;

    void f_bumper_callback(const std_msgs::msg::Bool::SharedPtr msg)
    {
        if (msg->data)
        {
            RCLCPP_INFO(this->get_logger(), "Front bumper activated!");
            stop_robot();

            // Iniciar retroceso usando un temporizador
            move_robot(-0.4f, 0.0f, 0.0f); // start moving backward

            // Crear un temporizador de 500ms para detener el robot y resetear bumper
            auto timer = std::make_shared<rclcpp::TimerBase::SharedPtr>();
            *timer = this->create_wall_timer(
                std::chrono::milliseconds(500),
                [this, timer]() {
                    //stop_robot();
                    send_bumper_reset_request();
                    (*timer)->cancel(); // cancelar el timer después de ejecutarse
                }
            );
        }
    }

    void b_bumper_callback(const std_msgs::msg::Bool::SharedPtr msg)
    {
        if (msg->data)
        {
            RCLCPP_INFO(this->get_logger(), "Back bumper activated!");
            stop_robot();
            send_bumper_reset_request();
        }
    }

    void move_robot(float linear_x,
                    float linear_y,
                    float angular_z)
    {  
        geometry_msgs::msg::Twist cmd_msg;
        cmd_msg.linear.x = linear_x;
        cmd_msg.linear.y = linear_y;
        cmd_msg.angular.z = angular_z;
        pub_cmd_vel_->publish(cmd_msg);
    }

    void stop_robot()
    {
        move_robot(0.0f, 0.0f, 0.0f);
        std_msgs::msg::Empty empty_msg;
        pub_nav_stop_->publish(empty_msg);
        pub_general_stop_->publish(empty_msg);
        move_robot(0.0f, 0.0f, 0.0f); // Ensure the robot stops by sending zero velocity commands
    }

    void send_bumper_reset_request()
    {
        auto request = std::make_shared<std_srvs::srv::Empty::Request>();
        cli_bumper_reset_->async_send_request(request);
    }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HSRBumperNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}