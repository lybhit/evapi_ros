#include "evarobot_teleop/ps3.h"
#include <signal.h>
IMJoystick::IMJoystick()
{
	this->max_lin = 1.0;
	this->max_ang = 3.0;
	this->deadman = false;
	
	this->max_x = 0.2;
	this->max_z = 0.5;
}

void IMJoystick::init()
{
	this->deadman_button = 10;
	this->axr_leftwards = 2;
	this->axr_upwards = 3;
	this->axl_leftwards = 0;
	this->axl_upwards = 1;

	this->triangle_button = 12;
	this->cross_button = 14;
	this->square_button = 15;
	this->circle_button = 13;
	
	this->vel_msg.linear.x = 0.0;
	this->vel_msg.angular.z = 0.0;
	
	this->client = this->n.serviceClient<im_msgs::SetRGB>("evarobot_rgb/SetRGB");
		
	this->vel_pub = this->n.advertise<geometry_msgs::Twist>("cmd_vel", 1);
	this->joy_sub_ = this->n.subscribe("joy", 10, &IMJoystick::CallbackJoy, this);
	
	this->n.param("evarobot_ps3/timeout", this->d_timeout, 1.0);
	ROS_INFO("timeout: %f", this->d_timeout);
		
	this->last_time = ros::Time::now();	
}

void IMJoystick::CallbackJoy(const sensor_msgs::Joy::ConstPtr& joy_msg)
{
	deadman = (((unsigned int)deadman_button < joy_msg->buttons.size()) && joy_msg->buttons[deadman_button]);

	this->last_time = ros::Time::now();	

	ROS_DEBUG("DEADMAN %d", joy_msg->buttons[deadman_button]);
	
	if (!deadman)
	{
		ROS_DEBUG("deadman is false");
		this->vel_msg.linear.x = 0.0;
		this->vel_msg.angular.z = 0.0;
		return;
	}

	if(((unsigned int)triangle_button < joy_msg->buttons.size()) && joy_msg->buttons[triangle_button])
		this->max_x += 0.1;
	
	if(((unsigned int)cross_button < joy_msg->buttons.size()) && joy_msg->buttons[cross_button])
					this->max_x -= 0.1;

	if(this->max_x < 0.0)
		max_x = 0.1;

	if(this->max_x > this->max_lin)
		max_x = this->max_lin;


	if(((unsigned int)square_button < joy_msg->buttons.size()) && joy_msg->buttons[square_button])
		this->max_z += 0.1;

	if(((unsigned int)circle_button < joy_msg->buttons.size()) && joy_msg->buttons[circle_button])
		this->max_z -= 0.1;

	if(this->max_z < 0.0)
		max_z = 0.1;

	if(this->max_z > this->max_ang)
		max_z = this->max_ang;



	ROS_DEBUG("linear %f <-- %d", joy_msg->axes[axr_upwards], axr_upwards);
	ROS_DEBUG("angular %f <-- %d", joy_msg->axes[axl_leftwards], axl_leftwards);

	this->vel_msg.linear.x = this->max_x * joy_msg->axes[axr_upwards];
	this->vel_msg.angular.z = this->max_z * joy_msg->axes[axl_leftwards];
}

void IMJoystick::PublishVel()
{
	this->CheckTimeout();
	this->vel_pub.publish(this->vel_msg);
}

void IMJoystick::CallRGBService()
{
	im_msgs::SetRGB srv;

	srv.request.times = -1;
	srv.request.mode = 3;
	srv.request.frequency = -1;
	srv.request.color = 0;

	if(this->client.call(srv) == 0)
	{
		ROS_ERROR("Failed to call service evarobot_rgb/SetRGB");
	}
}

void IMJoystick::CheckTimeout()
{
	ros::Duration duration = ros::Time::now() - this->last_time;
	if(duration.toSec() > d_timeout)
	{
		this->vel_msg.linear.x = 0.0;
		this->vel_msg.angular.z = 0.0;
		ROS_ERROR("Timeout Error [%f]", duration.toSec());
	}
}

void IMJoystick::TurnOffRGB()
{
	im_msgs::SetRGB srv;

	srv.request.times = -1;
	srv.request.mode = 0;
	srv.request.frequency = 1;
	srv.request.color = 0;
	if(this->client.call(srv) == 0)
	{
		ROS_ERROR("Failed to call service evarobot_rgb/SetRGB");
	}
}

bool g_b_terminate = false;

void sighandler(int sig)
{
	g_b_terminate = true;
}

int main(int argc, char **argv)
{
	ros::init(argc, argv, "evarobot_ps3");
	
	IMJoystick ps3;
  ps3.init();
	ps3.CallRGBService();
	
	ros::Rate loop_rate(10.0);
	
	signal(SIGINT, &sighandler);
	
  while (!g_b_terminate)
  {
		ps3.PublishVel();
		ros::spinOnce();
    loop_rate.sleep();
  }
  ps3.TurnOffRGB();  
	return 0;
}
