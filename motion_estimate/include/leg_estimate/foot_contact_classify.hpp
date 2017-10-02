#ifndef FOOT_CONTACT_CLASSIFY_HPP_
#define FOOT_CONTACT_CLASSIFY_HPP_
// 1. I think I saw drc-foot-state publish that both feet were in contact when it was actually in the air
// 2. Obviously the foot will not make perfectly flat contact with the ground - esp with BDI's walking mode
//    Thus integrating JK or even just using constraints between consequtive JK measurements seems quite flawed

#include <iostream>
#include <sstream>      // std::stringstream
#include <Eigen/Dense>
#include <Eigen/StdVector>

#include <boost/shared_ptr.hpp>
#include <lcm/lcm-cpp.hpp>
#include <pronto_math/pronto_math.hpp>
#include <pronto_vis/pronto_vis.hpp>
#include <filter_tools/Filter.hpp>
#include <filter_tools/SignalTap.hpp> // SchmittTrigger
#include "lcmtypes/pronto/foot_contact_estimate_t.hpp"

//#include <foot_contact/TwoLegsEstimate_types.h>
#define LEFTFOOT  0
#define RIGHTFOOT 1

class FootSensing { 
public:
  FootSensing(double force_z, double torque_x, double torque_y):
    force_z (force_z), torque_x (torque_x), torque_y(torque_y){};
  float force_z;
  float torque_x;
  float torque_y;
};


enum walkmode { 
  UNKNOWN = -1,
  LEFT_PRIME_RIGHT_STAND = 0, //0 both in contact, left has been for longer 
  LEFT_PRIME_RIGHT_BREAK = 1, //1 .... 
  LEFT_PRIME_RIGHT_SWING = 2, //2 left in contact, right raised  
  LEFT_PRIME_RIGHT_STRIKE= 3, //3 
  LEFT_STAND_RIGHT_PRIME = 4, //4
  LEFT_BREAK_RIGHT_PRIME = 5, //5
  LEFT_SWING_RIGHT_PRIME = 6, //6
  LEFT_STRIKE_RIGHT_PRIME= 7, //7
  
};

class foot_contact_classify {
  public:
    foot_contact_classify ( boost::shared_ptr<lcm::LCM> &lcm_publish_ , bool publish_diagnostics_);

    // Set the classifier from the 
//    void setFootForces(float left_force_in, float right_force_in  ){
//      left_force_ = left_force_in;
//      right_force_ = right_force_in;
//    }
    void setFootSensing(FootSensing lfoot_sensing_in, 
                        FootSensing rfoot_sensing_in ){
      lfoot_sensing_ = lfoot_sensing_in; 
      rfoot_sensing_ = rfoot_sensing_in;
    }
    
    walkmode getMode(){ return mode_; }
    walkmode getPreviousMode(){ return previous_mode_; }
    
    
    // update foot classification.
    // returns: odometry_status
    // 0 -> 1 float
    // 0 is very accurate
    // 1 very inaccuracy
    // -1 unuseable/invalid
    float update (int64_t utime, Eigen::Isometry3d primary_foot, Eigen::Isometry3d secondary_foot,
                  int standing_foot);
    
    
    // TODO: return variable is not functioning currently [feb 2014]
    // returns: 
    // -2 logic in error 
    // -1 not initialized yet
    // 0 transitioning onto left foot now. switch leg odom to left
    // 1 transitioning onto right foot now. switch leg odom to right
    // 2 continuing to have left as primary foot [system an initialize from here]
    // 3 continuing to have right as primary foot
    int updateWalkingPhase (int64_t utime, bool left_contact, bool right_contact, 
                      bool left_contact_break, bool right_contact_break    );
    
    
    // Determine which points are in contact with the ground
    // Currently only a stub which determines distance of points off of plane of standing foot
    void determineContactPoints(int64_t utime, Eigen::Isometry3d primary_foot, Eigen::Isometry3d secondary_foot);
    
    void determineCenterOfPressure(int64_t utime, Eigen::Isometry3d primary_foot, Eigen::Isometry3d secondary_foot,
                                   int standing_foot);
    
    pronto::PointCloud* getContactPoints(){ return contact_points_; }
    


  private:
    boost::shared_ptr<lcm::LCM> lcm_publish_;
    pronto_vis* pc_vis_;
    
    // the schmitt trigger detector for force-based contact classication:
    
    FootSensing lfoot_sensing_, rfoot_sensing_; // un filtered
    
    LowPassFilter lpfilter_lfoot_, lpfilter_rfoot_; // low pass filters for the feet contact forces
    SchmittTrigger* left_contact_state_weak_;
    SchmittTrigger* right_contact_state_weak_;
    SchmittTrigger* left_contact_state_strong_;
    SchmittTrigger* right_contact_state_strong_;
    
    //pcl::PointCloud<pcl::PointXYZRGB>::Ptr contact_points_;
    pronto::PointCloud* contact_points_;
    // Transform from foot frame origin to a point on the sole below the feet
    double foot_to_sole_z_;
    Eigen::Isometry3d foot_to_sole_;
    
    //pcl::PointCloud<pcl::PointXYZRGB>::Ptr cp_moving_prev_;
    pronto::PointCloud* cp_moving_prev_;
    
    
    // initialization condition is both feet in contact with the ground
    bool initialized_;
    // Publish Debug Data e.g. kinematic velocities and foot contacts
    bool publish_diagnostics_;  
    
    walkmode mode_; 
    // the mode in the most previous iteration.
    // NOT: the mode that we came from some time ago
    walkmode previous_mode_;
    
    // time when the last mode break or strike transition occurred
    int64_t last_strike_utime_;
    int64_t last_break_utime_; 
    // Parameters:
    int64_t strike_blackout_duration_; // amount of time to black out when a foot contacts the ground
    int64_t break_blackout_duration_;  // amount of time to black out when a foot breaks the ground
    
    // level of verbosity:
    // 0 - say nothing except pre-initialization
    // 1 - say when the primary foot is switched and pre-initialization
    // 2 - say when a mode transition
    // 3 - say something each iteration
    int verbose_;
    

    
};


#endif
