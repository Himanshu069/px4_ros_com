#!/bin/bash
echo "=== Cleaning up old processes ==="
pkill -f px4
pkill -f gz
pkill -f ros2
pkill -f MicroXRCEAgent
sleep 2  

echo "=== Clearing old PX4 logs ==="
> ~/px4_instance1.log
> ~/px4_instance2.log


cd ~/PX4-Autopilot || exit 1

echo "=== Starting Gazebo World  ==="
gnome-terminal -- bash -c "gz sim -v 4 -r /home/himanshu/PX4-Autopilot/Tools/simulation/gz/worlds/baylands.sdf; exec bash"

sleep 15


# echo "=== Starting Gazebo World  ==="
# gnome-terminal -- bash -c '
#   cd ~/aws-robomaker-small-warehouse-world || exit 1
#   export GZ_SIM_RESOURCE_PATH=$(pwd)/models:${GZ_SIM_RESOURCE_PATH}
#   echo "Starting gz sim with GZ_SIM_RESOURCE_PATH=$GZ_SIM_RESOURCE_PATH"
#   gz sim worlds/small_warehouse.world -v 4
#   exec bash
# '

echo "=== Starting PX4 SITL Instance 1 ==="
gnome-terminal -- bash -c "PX4_SIM_MODEL=gz_x500_depth ./build/px4_sitl_default/bin/px4 -i 1 | tee ~/px4_instance1.log; exec bash"

sleep 5

echo "=== Starting PX4 SITL Instance 2 ==="
gnome-terminal -- bash -c "PX4_GZ_MODEL_POSE="0,2" PX4_SIM_MODEL=gz_x500 ./build/px4_sitl_default/bin/px4 -i 9 | tee ~/px4_instance2.log; exec bash"

sleep 3

# echo "=== Starting QGroundControl ==="
# gnome-terminal -- bash -c "cd ~ && ./QGroundControl-x86_64.AppImage; exec bash"

# sleep 10

echo "=== Starting Micro XRCE-DDS Agent ==="
gnome-terminal -- bash -c "MicroXRCEAgent udp4 -p 8888; exec bash"
sleep 3

(
    echo "=== Waiting for PX4 Instance 1 Ready for takeoff ==="
    while true; do
        if grep -q "successfully created" ~/px4_instance1.log; then
            sleep 10
            echo "=== PX4 Instance 1 is ready ==="
            gnome-terminal -- bash -c "source /opt/ros/jazzy/setup.bash && cd ~/cslam_ws && source install/setup.bash && ros2 run px4_ros_com offboard_control --ros-args -p px4_instance:=1; exec bash"
            break
        fi
        sleep 1
    done
)&

(
    echo "=== Waiting for PX4 Instance 2 Ready for takeoff ==="
    while true; do
        if grep -q "successfully created" ~/px4_instance2.log; then
            sleep 5
            echo "=== PX4 Instance 2 is ready ==="
            gnome-terminal -- bash -c "source /opt/ros/jazzy/setup.bash && cd ~/cslam_ws && source install/setup.bash && ros2 run px4_ros_com offboard_control --ros-args -p px4_instance:=9; exec bash"
            break
        fi
        sleep 1
    done
)&

