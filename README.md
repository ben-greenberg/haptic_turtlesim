# Description
This package allows ROS to interface with Novint Falcon and expose the device parameters via rostopics.

It has been modified to enable haptic feedback to the Falcon joystick while running the turtlesim node. This package is primary the work of the following people.

## Author
Steven Martin:

Adnan Munawar: amunawar@wpi.edu

## Dependencies
Tested on ROS Hydro and Indigo.

Download and install libnifalcon

 https://github.com/libnifalcon/libnifalcon

Make sure udev permissions are set correctly. In ubunut 14.04 

  **roscd ros_falcon**
  
  **sudo cp udev_rules/99-udev-novint.rules /etc/udev/rules.d**
  
  **unplug - replug falcon**

### Usage
Run any bin from the pacakge and use rostopic list to see the exposed topics

