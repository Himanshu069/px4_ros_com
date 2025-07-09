#!/bin/bash

echo "=== Starting QGroundControl ==="
gnome-terminal -- bash -c "cd ~ && ./QGroundControl-x86_64.AppImage; exec bash"
sleep 3

cd ~/PX4-Autopilot || exit 1

echo "=== Starting PX4 SITL Instance 1 ==="
gnome-terminal -- bash -c "PX4_SIM_INSTANCE=1 MAV_SYS_ID=2 PX4_GZ_MODEL_POSE='0,1,0' make px4_sitl gz_x500 | tee ~/px4_instance1.log; exec bash"

echo "=== Starting PX4 SITL Instance 2 ==="
gnome-terminal -- bash -c "PX4_GZ_STANDALONE=1 PX4_SIM_INSTANCE=2 MAV_SYS_ID=3 PX4_GZ_MODEL_POSE='0,2,0' ./build/px4_sitl_default/bin/px4 -i 2 | tee ~/px4_instance2.log; exec bash"

echo "=== Starting Micro XRCE-DDS Agent ==="
gnome-terminal -- bash -c "MicroXRCEAgent udp4 -p 8888; exec bash"
sleep 5

echo "=== Waiting for PX4 Instance 1 Ready for takeoff ==="
while ! grep -q "Ready for takeoff!" ~/px4_instance1.log; do sleep 1; done

echo "=== Waiting for PX4 Instance 2 Ready for takeoff ==="
while ! grep -q "Ready for takeoff!" ~/px4_instance2.log; do sleep 1; done



echo "=== Starting ROS2 offboard_control node for instance 1 ==="
gnome-terminal -- bash -c "source /opt/ros/jazzy/setup.bash && cd ~/cslam_ws && source install/setup.bash && ros2 run px4_ros_com offboard_control; exec bash"
sleep 5

echo "=== Starting ROS2 offboard_control node for instance 2 ==="
gnome-terminal -- bash -c "source /opt/ros/jazzy/setup.bash && cd ~/cslam_ws && source install/setup.bash && ros2 run px4_ros_com offboard_control --ros-args -p px4_instance:=2; exec bash"
