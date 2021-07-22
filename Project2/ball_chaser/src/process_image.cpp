#include "ros/ros.h"
#include "ball_chaser/DriveToTarget.h"
#include <sensor_msgs/Image.h>

// Define a global client that can request services
ros::ServiceClient client;

// Define global variable that keeps track of ball, so we can avoid repeatedly making calls to drive bot if the ball remains in the center of the image for a while
char old_ball_pos = 'U'; // can be L(eft), M(iddle), R(ight) or U(nseen)

// This function calls the command_robot service to drive the robot in the specified direction
void drive_robot(float lin_x, float ang_z)
{
    // Request a service and pass the velocities to it to drive the robot
	ball_chaser::DriveToTarget srv;
	srv.request.linear_x = lin_x;
	srv.request.angular_z = ang_z;

	if (!client.call(srv))
		ROS_ERROR("Failed to call service command_robot");
	
}

// This callback function continuously executes and reads the image data
void process_image_callback(const sensor_msgs::Image img)
{

    int white_pixel = 255;


	/* it's an (img.height, img.width)=(800,800) image
	 each row has 800 rgb pixels thus we have 800*3 = 2400 = img.step
	 In total there are 800 * 800 = 640000 pixels
	 each of these pixels have rgb values so 640000*3 = 1920000 = img.data.size()
	 The image is flattened, even the rgb vals are flattened out which is why the size of the img.data array is 1920000 */

	char ball_pos = 'U';

	// loop through flattened image, use step of 3 to go to start of different rgb pixel each iteration
	for(int i=0; i < img.data.size(); i+=3){

		// check if pixel is a white pixel
		if((img.data[i],img.data[i+1],img.data[i+2]) == (white_pixel, white_pixel, white_pixel)){
			// check which third of image the pixel is in
			if((i%img.step) < img.step / 3){
				ball_pos = 'L';
			} else if((i%img.step) > img.step * (2.0/3.0)){
				ball_pos = 'R';
			}else {
				ball_pos = 'M';
			}
			break; //we only need one pixel (the one pixel we get is the first white pixel, so if the ball was on the boundary between two positions then that pixel would be at the top of the ball and hence be on the center line of the ball)
		}

	}

	if (ball_pos != old_ball_pos){
		old_ball_pos = ball_pos;
		if (ball_pos == 'U'){
			drive_robot(0, 0);
			ROS_INFO_STREAM("NO BALL");
		} else if (ball_pos == 'L'){
			drive_robot(0, 0.1);
			ROS_INFO_STREAM("BALL ON LEFT");
		} else if (ball_pos == 'M'){
			drive_robot(2.0, 0);
			ROS_INFO_STREAM("BALL IN MIDDLE");
		} else if (ball_pos == 'R'){
			drive_robot(0, -0.1);
			ROS_INFO_STREAM("BALL ON RIGHT");
		}
	}


}

int main(int argc, char** argv)
{
    // Initialize the process_image node and create a handle to it
    ros::init(argc, argv, "process_image");
    ros::NodeHandle n;

    // Define a client service capable of requesting services from command_robot
    client = n.serviceClient<ball_chaser::DriveToTarget>("/ball_chaser/command_robot");

    // Subscribe to /camera/rgb/image_raw topic to read the image data inside the process_image_callback function
    ros::Subscriber sub1 = n.subscribe("/camera/rgb/image_raw", 10, process_image_callback);

    // Handle ROS communication events
    ros::spin();

    return 0;
}

