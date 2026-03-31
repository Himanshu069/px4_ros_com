#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <px4_msgs/msg/sensor_combined.hpp>
#include <px4_msgs/msg/vehicle_attitude.hpp>
#include <sensor_msgs/msg/imu.hpp>

class PX4IMUBridge : public rclcpp::Node
{
public:
    PX4IMUBridge() : Node("px4_imu_bridge")
    {
        // QoS Profiles
        auto sub_qos = rclcpp::QoS(rclcpp::KeepLast(10)).best_effort().transient_local();
        auto pub_qos = rclcpp::QoS(rclcpp::KeepLast(10)).reliable().durability_volatile();

        // Parameters
        this->declare_parameter<double>("gyro_noise", 0.0150);
        this->declare_parameter<double>("accel_noise", 0.3500);
        this->declare_parameter<std::string>("px4_ns", "");
        this->declare_parameter<std::string>("vehicle_ns", "x500_drone_0");

        double gyro_noise = this->get_parameter("gyro_noise").as_double();
        double accel_noise = this->get_parameter("accel_noise").as_double();
        gyro_var_ = gyro_noise * gyro_noise;
        accel_var_ = accel_noise * accel_noise;

        std::string px4_ns = this->get_parameter("px4_ns").as_string();
        std::string vehicle_ns = this->get_parameter("vehicle_ns").as_string();

        // Topic Setup
        std::string px4_topic = px4_ns.empty() ? "/fmu/out/sensor_combined" : px4_ns + "/fmu/out/sensor_combined";
        std::string imu_topic = "/" + vehicle_ns + "/imu/data_raw";
        frame_id_ = vehicle_ns + "/imu_sensor";

        // Subscriptions
        sensor_sub_ = this->create_subscription<px4_msgs::msg::SensorCombined>(
            px4_topic, sub_qos,
            std::bind(&PX4IMUBridge::sensor_callback, this, std::placeholders::_1));

        attitude_sub_ = this->create_subscription<px4_msgs::msg::VehicleAttitude>(
            "/fmu/out/vehicle_attitude", sub_qos,
            std::bind(&PX4IMUBridge::attitude_callback, this, std::placeholders::_1));

        // Publisher
        imu_pub_ = this->create_publisher<sensor_msgs::msg::Imu>(imu_topic, pub_qos);

        RCLCPP_INFO(this->get_logger(), "Bridge started. Gyro Var: %f, Accel Var: %f", gyro_var_, accel_var_);
    }

private:
    void attitude_callback(const px4_msgs::msg::VehicleAttitude::SharedPtr msg)
    {
        latest_attitude_ = msg;
    }

    void sensor_callback(const px4_msgs::msg::SensorCombined::SharedPtr msg)
    {
        sensor_msgs::msg::Imu imu_msg;

        imu_msg.header.stamp = this->now();
        imu_msg.header.frame_id = frame_id_;

        // Coordinate Mapping (Matching your specific Python logic)
        imu_msg.angular_velocity.x = msg->gyro_rad[1];
        imu_msg.angular_velocity.y = -msg->gyro_rad[0];
        imu_msg.angular_velocity.z = -msg->gyro_rad[2];

        imu_msg.linear_acceleration.x = msg->accelerometer_m_s2[1];
        imu_msg.linear_acceleration.y = -msg->accelerometer_m_s2[0];
        imu_msg.linear_acceleration.z = -msg->accelerometer_m_s2[2];

        // Fill Covariances
        for (int i = 0; i < 9; i++) {
            imu_msg.angular_velocity_covariance[i] = 0.0;
            imu_msg.linear_acceleration_covariance[i] = 0.0;
            imu_msg.orientation_covariance[i] = 0.0;
        }

        // Diagonal elements
        imu_msg.angular_velocity_covariance[0] = imu_msg.angular_velocity_covariance[4] = imu_msg.angular_velocity_covariance[8] = gyro_var_;
        imu_msg.linear_acceleration_covariance[0] = imu_msg.linear_acceleration_covariance[4] = imu_msg.linear_acceleration_covariance[8] = accel_var_;

        if (latest_attitude_) {
            imu_msg.orientation.w = latest_attitude_->q[0];
            imu_msg.orientation.x = latest_attitude_->q[1];
            imu_msg.orientation.y = latest_attitude_->q[2];
            imu_msg.orientation.z = latest_attitude_->q[3];

            double ov = 0.05;
            imu_msg.orientation_covariance[0] = imu_msg.orientation_covariance[4] = imu_msg.orientation_covariance[8] = ov;
        } else {
            imu_msg.orientation_covariance[0] = -1.0;
        }

        imu_pub_->publish(imu_msg);
    }

    double gyro_var_;
    double accel_var_;
    std::string frame_id_;
    px4_msgs::msg::VehicleAttitude::SharedPtr latest_attitude_{nullptr};

    rclcpp::Subscription<px4_msgs::msg::SensorCombined>::SharedPtr sensor_sub_;
    rclcpp::Subscription<px4_msgs::msg::VehicleAttitude>::SharedPtr attitude_sub_;
    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<PX4IMUBridge>());
    rclcpp::shutdown();
    return 0;
}
