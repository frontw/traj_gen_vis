#include "auto_chaser/Chaser.h"

Chaser::Chaser():is_complete_chasing_path(false){}

void Chaser::init(ros::NodeHandle nh){

    preplanner.init(nh);
    smooth_planner.init(nh);
    pub_control_mav = nh.advertise<PoseStamped>("mav_control_pose",1);
}

bool Chaser::chase_update(GridField* global_edf_ptr,vector<Point> target_pnts,Point chaser_x0,Twist chaser_v0,Twist chaser_a0,TimeSeries knots){
    
    bool result = false;
    


    // phase 1 pre planning 
    preplanner.preplan(global_edf_ptr,target_pnts,chaser_x0);
    ROS_INFO("[Chaser] preplanning completed.");

    nav_msgs::Path waypoints = preplanner.get_preplanned_waypoints();

    if(waypoints.poses.size()){
        // phase 2 smooth planning     
        smooth_planner.traj_gen(knots,waypoints,chaser_v0,chaser_a0);
        if (smooth_planner.planner.is_spline_valid())
            {ROS_INFO("[Chaser] smooth path completed."); is_complete_chasing_path = true; return true;}
        else 
            ROS_WARN("[Chaser] smooth path incompleted.");
    }else
        ROS_WARN("[Chaser] preplanning failed");

    return false;
}

void Chaser::session(double t){
    preplanner.publish(); // markers     
    smooth_planner.publish();  // markers 
    if (is_complete_chasing_path){
        publish_control(t);
    }

}


Point Chaser::eval_point(double t_eval){    
    return smooth_planner.planner.point_eval_spline(t_eval);        
}


Twist Chaser::eval_velocity(double t_eval){
    return smooth_planner.planner.vel_eval_spline(t_eval);        
}

Twist Chaser::eval_acceleration(double t_eval){
    return smooth_planner.planner.accel_eval_spline(t_eval);        
}


void Chaser::publish_control(double t_eval){
    pose_control_mav.header.frame_id = smooth_planner.world_frame_id;
    pose_control_mav.pose.position = smooth_planner.planner.point_eval_spline(t_eval); 
    pub_control_mav.publish(pose_control_mav);
}