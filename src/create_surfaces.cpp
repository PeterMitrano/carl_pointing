//adds interactive (clickable) markers and set those locations as navigation goals
#include <ros/ros.h>
#include <interactive_markers/interactive_marker_server.h>
#include <visualization_msgs/Marker.h>
#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>
#include <wpi_jaco_msgs/CartesianCommand.h>
#include <move_base_msgs/MoveBaseGoal.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/Point.h>
#include <geometry_msgs/Quaternion.h>
#include <std_msgs/Header.h>
#include <urdf/model.h>



typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction>* Move_Base_Client_ptr;
Move_Base_Client_ptr move_base_client;

void onParkingClick(const visualization_msgs::InteractiveMarkerFeedbackConstPtr &f){
	if (f->event_type == visualization_msgs:InteractiveMarkerFeedback::MOUSE_UP){

		ROS_INFO("parking spot clicked!");

	//copy header and pose from marker to new action goal
    //rotate 90 deg around z axis
	//for pose use the position of the mouse click
    geometry_msgs::Pose new_pose = f->pose;
    geometry_msgs::Point mouse_point = f->mouse_point;
    new_pose.orientation.x=mouse_point.x;
    new_pose.orientation.y=mouse_point.y;
    new_pose.orientation.z=mouse_point.z;
    new_pose.orientation.w=1;

    //create pose
    geometry_msgs::PoseStamped target_pose;
    target_pose.header = f->header;
    target_pose.pose = new_pose;

    //create goal
    move_base_msgs::MoveBaseGoal goal;
    goal.target_pose = target_pose;

    //send action goal
    client_ptr->sendGoal(goal);
	}
}

void onPointingClick(const visualization_msgs::InteractiveMarkerFeedbackConstPtr &f){
	if (f->event_type == visualization_msgs:InteractiveMarkerFeedback::MOUSE_UP){

		ROS_INFO("parking spot clicked!");

	//copy header and pose from marker to new action goal
    //rotate 90 deg around z axis
    geometry_msgs::Pose new_pose = f->pose;
    new_pose.orientation.x=0;
    new_pose.orientation.y=0;
    new_pose.orientation.z=1;
    new_pose.orientation.w=1;

    //create pose
    geometry_msgs::PoseStamped target_pose;
    target_pose.header = f->header;
    target_pose.pose = new_pose;

    //create goal
    move_base_msgs::MoveBaseGoal goal;
    goal.target_pose = target_pose;

    //TODO: SET ARM TO POSITION
	}
}

//return a clickable marker at the origin of the given link
visualization_msgs::InteractiveMarker createParkingSpot(std::string frame_id){
	visualization_msgs::InteractiveMarker marker;

	marker.header.frame_id = frame_id;
	marker.scale = 1;
	marker.name = frame_id + "_parking_spot";

	visualization_msgs::InteractiveMarkerControl control;
	control.interaction_mode = visualization_msgs::InteractiveMarkerControl::BUTTON;
	control.name = "parking spot";

	visualization_msgs::Marker box;
	box.type = visualization_msgs::Marker::CUBE;
	box.scale.x=0.15;
	box.scale.y=0.15;
	box.scale.z=0.05;
	box.color.r=0;
	box.color.g=0.5;
	box.color.b=0.25;
	box.color.a=1;

	control.markers.push_back(box);
	control.always_visible = true;
	marker.controls.push_back(control);

	return marker;
}

//returns a clickable marker at the origin of the given link and the same size as the surface
//eventuall this should use a mesh instead of a box
visualization_msgs::InteractiveMarker createSurface(std::string frame_id){
	visualization_msgs::InteractiveMarker marker;

	marker.header.frame_id = frame_id;
	marker.scale = 1;
	marker.name = frame_id + "_surface";

	visualization_msgs::InteractiveMarkerControl control;
	control.interaction_mode = visualization_msgs::InteractiveMarkerControl::BUTTON;
	control.name = "surface"

	visualization_msgs::Marker surface;
	surface.type = visualization_msgs::CUBE;
	surface.scale.x = 0.35;
	surface.scale.y = 0.35;
	surface.scale.z = 0.05;
	surface.color.r = .5;
	surface.color.g = .0;
	surface.color.b = .25;
	surface.color.a = 1;

	control.markers.push_back(surface);
	control.always_visible = true;
	marker.controls.push_back(control);

	return marker;
}

//returns true if the link name ends in "nav_goal_link"
bool isNavGoal(std::string link_name){
	std::string s = "nav_goal_link"
	return link_name.find(s, link_name.length()-s.length()) != std::string::npos;
}

//returns true if the link name ends in "surface_link"
bool isPointingGoal(std::string link_name){
	std::string s = "surface_link"
	return link_name.find(s, link_name.length()-s.length()) != std::string::npos;
}

int main(int argc, const char* argv[]){

	ros::init(argc,argv,"create_surfaces");
	ros::NodeHandle n;
	ros::Rate rate(10.0);

	interactive_markers::InteractiveMarkerServer parking_server("parking_markers");
	interactive_markers::InteractiveMarkerServer pointing_server("surfaces_markers");

	move_base_client = new Client("move_base", true);
	
	move_base_client->waitForServer();

	ROS_INFO("connected to move_base and arm_pose");

	urdf::Model ilab;

	//load the urdf with all the furniture off the param server
	if (!ilab.initParam("/ilab_description")){
		ROS_INFO("Couldn't find /ilab_description on the param server!");
		return 1;
	}

	std::map<std::string, bost::shared_ptr<urdf::Link> > links = ilab.links_;
	std::map<std::string, boost::shared_ptr<urdf::Link> >::iterator itr;


	ROS_INFO("Creating parking spots and surfaces...");

	//go through all the links
	//add ones with nav_goal_link to the parking marker server
	//add ones with surface_goal_link to the surface marker server
	for (itr = links.begin(); itr != links.end(); itr++){
		std::string link_name = itr->first;

		if (isNavGoal(link_name)){
			parking_server.insert(createParkingSpot(link_name), &onParkingClick);
		}
		else if (isPointingGoal(link_name)){
			pointing_server.insert(createSurface(link), &onPointingClick);
		}

	}

	parking_server.applyChanges();
	pointing_server.applyChanges();

	ros::spin();

	return 0;
}

