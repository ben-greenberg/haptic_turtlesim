#include "ros/ros.h"
#include "rosgraph_msgs/Log.h"
#include "std_msgs/String.h"

void HitWallCallback(const rosgraph_msgs::Log::ConstPtr& msg)
{
  ROS_INFO("I heard: [%i]", msg->level);
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "hit_wall");
  ros::NodeHandle n;
  ros::Subscriber sub = n.subscribe("rosout_agg", 1000, HitWallCallback);
  ros::Rate loop_rate(10);
  ros::spin();

  return 0;
}
