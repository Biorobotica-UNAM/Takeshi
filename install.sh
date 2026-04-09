

sudo apt update
sudo apt install -y ros-humble-rmw-cyclonedds-cpp
export ROS_DOMAIN_ID=1 >> ~/.bashrc
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp >> ~/.bashrc
export CYCLONEDDS_URI=/cyclonedds_profile.xml >> ~/.bashrc
source /opt/ros/humble/setup.bash >> ~/.bashrc
source ~/.bashrc
