#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "sensor_msgs/point_cloud2_iterator.hpp"

#include <cstring>

class FloorFilterNode : public rclcpp::Node
{
public:

    FloorFilterNode() : Node("floor_filter_node")
    {
        this->declare_parameter<std::string>("input_cloud", "camera/points");
        this->declare_parameter<std::string>("output_cloud", "camera/points_filtered");
        this->declare_parameter<double>("z_min", 0.08);

        input_topic_  = this->get_parameter("input_cloud").as_string();
        output_topic_ = this->get_parameter("output_cloud").as_string();
        z_min_        = this->get_parameter("z_min").as_double();

        sub_cloud_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
            input_topic_,
            rclcpp::SensorDataQoS(),
            std::bind(&FloorFilterNode::cloudCallback, this, std::placeholders::_1));

        pub_cloud_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(
            output_topic_,
            rclcpp::SensorDataQoS());

        RCLCPP_INFO(this->get_logger(),
            "Fast floor filter started. Removing points with z < %.3f", z_min_);
    }

private:

    rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr sub_cloud_;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pub_cloud_;

    std::string input_topic_;
    std::string output_topic_;

    double z_min_;

    void cloudCallback(const sensor_msgs::msg::PointCloud2::SharedPtr msg)
    {
        const size_t total_points = msg->width * msg->height;
        const size_t point_step   = msg->point_step;

        sensor_msgs::msg::PointCloud2 output;

        output.header = msg->header;
        output.height = 1;
        output.fields = msg->fields;
        output.is_bigendian = msg->is_bigendian;
        output.point_step = point_step;
        output.is_dense = false;

        // Preallocate worst-case size
        output.data.resize(msg->data.size());

        uint8_t* out_ptr = output.data.data();
        const uint8_t* in_ptr = msg->data.data();

        size_t valid_points = 0;

        sensor_msgs::PointCloud2ConstIterator<float> iter_z(*msg, "z");

        for (size_t i = 0; i < total_points; ++i, ++iter_z)
        {
            if (*iter_z > z_min_)
            {
                const uint8_t* src = in_ptr + i * point_step;
                std::memcpy(out_ptr + valid_points * point_step, src, point_step);
                valid_points++;
            }
        }

        output.width = valid_points;
        output.row_step = valid_points * point_step;

        // shrink to actual size
        output.data.resize(valid_points * point_step);

        pub_cloud_->publish(output);
    }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<FloorFilterNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}