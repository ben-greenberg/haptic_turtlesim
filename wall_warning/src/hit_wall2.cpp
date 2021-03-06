#include "ros/ros.h"
#include "rosgraph_msgs/Log.h"
#include "std_msgs/Byte.h"

void HitWallCallback(const rosgraph_msgs::Log::ConstPtr& msgin)
{
  ros::NodeHandle n;
  //ROS_INFO("I sent: [%i]", msgin->level);
  ros::Publisher feedback_pub = n.advertise<std_msgs::Byte>("feedback", 1000);
  ros::Rate loop_rate(10);

  while (ros::ok())
  {
    std_msgs::Byte msgout;		
    msgout.data = msgin->level;
    feedback_pub.publish(msgout);
    //ROS_INFO("I sent: [%i]", msgout.data);

    ros::spinOnce();

    loop_rate.sleep();
  }
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "hit_wall");
  ros::NodeHandle nh;
  ros::Subscriber sub = nh.subscribe("rosout_agg", 1000, HitWallCallback);
  ros::spin();

  return 0;
}

