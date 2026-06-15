cmake_minimum_required(VERSION 3.8)
project(hsr_hardware)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(control_msgs REQUIRED)
find_package(trajectory_msgs REQUIRED)
find_package(actionlib_msgs REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(tmc_control_msgs REQUIRED)
find_package(tmc_voice_msgs REQUIRED)
find_package(talker_interfaces REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(head_interfaces REQUIRED)

find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

# Executable
add_executable(hsr_bringup src/hsr_bringup.cpp)
add_executable(hsr_arm_node src/hsr_arm_node.cpp)
add_executable(hsr_head_node src/hsr_head_node.cpp)
#add_executable(hsr_gripper_node src/hsr_gripper_node.cpp)
add_executable(hsr_talk_node src/hsr_talk_node.cpp)
add_executable(hsr_bumper_node src/hsr_bumper_node.cpp)
add_executable(hsr_teleop_node src/hsr_teleop.cpp)


ament_target_dependencies(hsr_bringup
  rclcpp
  std_srvs
  std_msgs
)
ament_target_dependencies(hsr_arm_node
  rclcpp
  std_msgs
  trajectory_msgs
  control_msgs
  actionlib_msgs
)
ament_target_dependencies(hsr_head_node
  rclcpp
  std_msgs
  trajectory_msgs
  control_msgs
  head_interfaces
)

#ament_target_dependencies(hsr_gripper_node
#  rclcpp
#  trajectory_msgs
#  rclcpp_action
#  tmc_control_msgs
#)

ament_target_dependencies(hsr_talk_node
  rclcpp
  talker_interfaces
  tmc_voice_msgs
)

ament_target_dependencies(hsr_bumper_node
  rclcpp
  std_msgs
  std_srvs
  geometry_msgs
)

ament_target_dependencies(hsr_teleop_node
  rclcpp
  std_msgs
  geometry_msgs
)


# Install
install(TARGETS 
  hsr_bringup
  hsr_arm_node
  hsr_head_node
  #hsr_gripper_node
  hsr_talk_node
  hsr_bumper_node
  hsr_teleop_node
  DESTINATION lib/${PROJECT_NAME}
)


ament_package()
