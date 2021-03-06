//////////////////////////////////////////////////////////
// ROSfalcon Simple Joystick Controller.
//
// Steven Martin
// 22/07/10

#include <iostream>
#include <string>
#include <cmath>
#include <ros/ros.h>
#include <sensor_msgs/Joy.h>

#include "falcon/core/FalconDevice.h"
#include "falcon/firmware/FalconFirmwareNovintSDK.h"
#include "falcon/util/FalconCLIBase.h"
#include "falcon/util/FalconFirmwareBinaryNvent.h"
#include "falcon/kinematic/stamper/StamperUtils.h"
#include "falcon/kinematic/FalconKinematicStamper.h"
#include "falcon/core/FalconGeometry.h"
#include "falcon/gmtl/gmtl.h"
#include "falcon/util/FalconFirmwareBinaryNvent.h"
#include "falcon/grip/FalconGripFourButton.h"

#include "rosgraph_msgs/Log.h"
#include "std_msgs/String.h"
#include "std_msgs/Byte.h"
#include "turtlesim/Pose.h"
#include <ros/callback_queue.h>

using namespace libnifalcon;
using namespace std;
using namespace StamperKinematicImpl;
FalconDevice m_falconDevice;

//Global Variables
turtlesim::Pose CurPose;	

/**********************************************
This function initialises the novint falcon controller

NoFalcons is the index of the falcon which you wish to initialise
Index 0 is first falcon.
**********************************************/
 
bool init_falcon(int NoFalcon) 

{
    cout << "Setting up LibUSB" << endl;
    m_falconDevice.setFalconFirmware<FalconFirmwareNovintSDK>(); //Set Firmware
    m_falconDevice.setFalconGrip<FalconGripFourButton>(); //Set Grip
    if(!m_falconDevice.open(NoFalcon)) //Open falcon @ index
    {
        cout << "Failed to find falcon" << endl;
        return false;
    }
    else
    {
        cout << "Falcon Found" << endl;
    }

    //There's only one kind of firmware right now, so automatically set that.
    m_falconDevice.setFalconFirmware<FalconFirmwareNovintSDK>();
    //Next load the firmware to the device

    bool skip_checksum = false;
    //See if we have firmware
    bool firmware_loaded = false;
    firmware_loaded = m_falconDevice.isFirmwareLoaded();
    if(!firmware_loaded)
    {
        cout << "Loading firmware" << endl;
        uint8_t* firmware_block;
        long firmware_size;
        {

            firmware_block = const_cast<uint8_t*>(NOVINT_FALCON_NVENT_FIRMWARE);
            firmware_size = NOVINT_FALCON_NVENT_FIRMWARE_SIZE;


            for(int i = 0; i < 20; ++i)
            {
                if(!m_falconDevice.getFalconFirmware()->loadFirmware(skip_checksum, NOVINT_FALCON_NVENT_FIRMWARE_SIZE, const_cast<uint8_t*>(NOVINT_FALCON_NVENT_FIRMWARE)))

                {
                    cout << "Firmware loading try failed" <<endl;
                }
                else
                {
                    firmware_loaded = true;
                    break;
                }
            }
        }
    }
    else if(!firmware_loaded)
    {
        cout << "No firmware loaded to device, and no firmware specified to load (--nvent_firmware, --test_firmware, etc...). Cannot continue" << endl;
        return false;
    }
    if(!firmware_loaded || !m_falconDevice.isFirmwareLoaded())
    {
        cout << "No firmware loaded to device, cannot continue" << endl;
        return false;
    }
    cout << "Firmware loaded" << endl;

    m_falconDevice.getFalconFirmware()->setHomingMode(true); //Set homing mode (keep track of encoders !needed!)
    cout << "Homing Set" << endl;
    std::array<int, 3> forces;
                    forces[0] = 0;
                    forces[1] = 0;
                    forces[2] = 0;
    m_falconDevice.getFalconFirmware()->setForces(forces);
    m_falconDevice.runIOLoop(); //read in data

    bool stop = false;
    bool homing = false;
    bool homing_reset = false;
    usleep(100000);
    int tryLoad = 0;
    while(!stop) //&& tryLoad < 100)
    {
        if(!m_falconDevice.runIOLoop()) continue;
        if(!m_falconDevice.getFalconFirmware()->isHomed())
        {
            if(!homing)
            {
                m_falconDevice.getFalconFirmware()->setLEDStatus(libnifalcon::FalconFirmware::RED_LED);
                cout << "Falcon not currently homed. Move control all the way out then push straight all the way in." << endl;

            }
            homing = true;
        }

        if(homing && m_falconDevice.getFalconFirmware()->isHomed())
        {
            m_falconDevice.getFalconFirmware()->setLEDStatus(libnifalcon::FalconFirmware::BLUE_LED);
            cout << "Falcon homed." << endl;
            homing_reset = true;
            stop = true;
        }
        tryLoad++;
    }
    /*if(tryLoad >= 100)
    {
        return false;
    }*/

    m_falconDevice.runIOLoop();
    return true;
}

class Pose
{
	public:
		float x;
		float y;
		float theta;
};


// callback to send new current Pose msgs
void CurPoseCallback(const turtlesim::Pose::ConstPtr& msg)			
{
	ROS_INFO("Tracking...");
	//CurPose.x = msg->x;
	//CurPose.y = msg->y;
	//CurPose.theta = msg->theta;	// copy msg to varible to use elsewhere
  	//if(CurPose.x == 0 || CurPose.x == 11 || CurPose.y == 0 || CurPose.y == 11)
	if(msg->x == 0 || msg->x == 11 || msg->y == 0 || msg->y == 11)
		{
			ROS_INFO("Hit wall");
		}
		
	return;
}


int main(int argc, char* argv[])
{

	ros::init(argc,argv, "FalconJoystick");				//Open a new node
    
    ros::NodeHandle node;								//Create a handle for this node
    int falcon_int;										//Declare a variable called falcon_int of the type int
    bool debug;
    //Button 4 is mimic-ing clutch.
    bool clutchPressed, coagPressed;					//Declare variables called clutchPressed and coagPressed of the type bool
    node.param<int>("falcon_number", falcon_int, 0);		//Not sure what this does - It seems to get a parameter for "falcon_number", and takes falcon_int as the parameter and maybe 0 is the default
    node.param<bool>("falcon_debug", debug, false);
    node.param<bool>("falcon_clutch", clutchPressed, true);
    node.param<bool>("falcon_coag", coagPressed, true);



    if(init_falcon(falcon_int))
    {
        cout << "Falcon Initialised Starting ROS Node" << endl;	//Print the line to screen

        m_falconDevice.setFalconKinematic<libnifalcon::FalconKinematicStamper>();	//Initialize something in the falcon

        //Start ROS Publisher
        ros::Publisher pub = node.advertise<sensor_msgs::Joy>("/falcon/joystick",10);	//Advertise a topic of type sensor_msgs/Joy called /falcon/joystick at a rate of 10 hz
				
		ros::Subscriber sub = node.subscribe("/turtle1/pose", 10, CurPoseCallback); 	//Subscribe to a topic called /turtle1/pose at a rate of 10 hz, when it receives a message perform the CurPoseCallback function       
		
		ros::Rate loop_rate(10);
	
        while(node.ok())
        {

            sensor_msgs::Joy Joystick;					//Declare a variable called Joystick of the type sensor_msgs/Joy
	//		turtlesim::Pose Turtle						//Declare a variable called Turtle of the type turtlesim/Pose
            std::array<double, 3> prevPos;				//Declare a variable called prevPos of the type std/array

            Joystick.buttons.resize(1);					//Set the size of the arrays within the Joystick variable
            Joystick.axes.resize(3);

            std::array<double,3> forces;				//Declare a variable called forces of the type std/array
            //Request the current encoder positions:
            std::array<double, 3> Pos;					//Declare a variable called Pos of the type std/array
            std::array<double, 3> newHome, prevHome;	//Declare variables called newHome and prevHome of the type std/array
            int buttons;								//Declare a variable called buttons of the type int
	
            if(m_falconDevice.runIOLoop())
            {
                /////////////////////////////////////////////
                Pos = m_falconDevice.getPosition();  //Read in cartesian position

                buttons = m_falconDevice.getFalconGrip()->getDigitalInputs(); //Read in buttons

                //Publish ROS values
                Joystick.buttons[0] = buttons;
                Joystick.axes[0] = Pos[0];
                Joystick.axes[1] = Pos[1];
                Joystick.axes[2] = Pos[2];
                pub.publish(Joystick);
                
                //TODO if Joystick can subscribe to twist message use those forces instead for haptic feedback
                //if
                float KpGainX = -200;
                float KpGainY = -200;
                float KpGainZ = -200;

                float KdGainX = -500;
                float KdGainY = -500;
                float KdGainZ = -500;
                

                // Check if button 4 is pressed, set the forces equal to 0.
                if(buttons == 4 || buttons == 2){
                    if(buttons == 4 && coagPressed == false){
                        ROS_INFO("Coag Pressed (Button 4)");
                        coagPressed = true;
                    }
                    else if(buttons == 2 && clutchPressed == false){
                        ROS_INFO("Clutch Pressed (Button 2)");
                        clutchPressed = true;
                    }
                    forces[0] = 0;	//left/right, positive pushes right
                    forces[1] = 0;	//up/down, positive pushes up
                    forces[2] = 3;	//in/out, positive pushes back	//Push the grip back so that it can only move easily in two dimensions
                }
                else{
                    if(coagPressed == true){
                        ROS_INFO("Coag Released (Button 4)");
                        coagPressed = false;
                        newHome = Pos;
                    }
                    else if(clutchPressed == true){
                        ROS_INFO("Clutch Released (Button 2)");
                        clutchPressed = false;
                        newHome = Pos;
                    }
                    //Simple PD controller
                    forces[0] = ((Pos[0] - newHome[0]) * KpGainX) + (Pos[0] - prevPos[0])*KdGainX;
                    forces[1] = ((Pos[1] - newHome[1]) * KpGainY) + (Pos[1] - prevPos[1])*KdGainY;
                    forces[2] = ((Pos[2] - newHome[2]) * KpGainZ) + (Pos[2] - prevPos[2])*KdGainZ;
                }
                m_falconDevice.setForce(forces); //Write falcon forces to driver (Forces updated on IO loop) Should run @ ~1kHz


                if(debug)
                {
                    cout << "Position= " << Pos[0] <<" " << Pos[1] << " " << Pos[2] <<  endl;
                    cout << "newHome  = " << newHome[0] <<" " << newHome[1] << " " << newHome[2] <<  endl;
                    cout << "Error   =" << Pos[0] - newHome[0] <<" " << Pos[1]-newHome[1] << " " << Pos[2] -newHome[2] <<  endl;
                    cout << "Force= " << forces[0] <<" " << forces[1] << " " << forces[2] <<  endl;
                }
                prevPos = Pos;
                prevHome = newHome;
            }
			ros::spinOnce(); 		//Callback is actually called here
            loop_rate.sleep();
        }
        m_falconDevice.close();
    }
    return 0;
}
