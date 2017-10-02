#ifndef RBIS_LEGODO_LIB_UPDATE_HPP_
#define RBIS_LEGODO_LIB_UPDATE_HPP_

#include <lcm/lcm.h>
#include <lcm/lcm-cpp.hpp>
#include <bot_core/bot_core.h>
#include <bot_param/param_client.h>
#include <bot_frames/bot_frames.h>
#include <model-client/model-client.hpp>

#include <string>

#include <mav_est_legodo/rbis_legodo_common.hpp>

#include <leg_estimate/leg_estimate.hpp>
#include <backlash_filter_tools/torque_adjustment.hpp>

#include <lcmtypes/bot_core/joint_state_t.hpp>
#include <lcmtypes/pronto/controller_foot_contact_t.hpp>
#include <lcmtypes/bot_core/six_axis_force_torque_t.hpp>
#include <lcmtypes/bot_core/six_axis_force_torque_array_t.hpp>


namespace MavStateEst {
  
// Equivalent to bot_core_pose contents
struct PoseT { 
  int64_t utime;
  Eigen::Vector3d pos;
  Eigen::Vector3d vel;
  Eigen::Vector4d orientation;
  Eigen::Vector3d rotation_rate;
  Eigen::Vector3d accel;
};  

class LegOdoHandler {
public:

  LegOdoHandler(lcm::LCM* lcm_recv,  lcm::LCM* lcm_pub, 
      BotParam * param, ModelClient* model, BotFrames * frames);
  RBISUpdateInterface * processMessage(const bot_core::joint_state_t *msg, MavStateEstimator* state_estimator);


  // Classes:
  // Calculates the Pelvis Translation between measurements:
  leg_estimate* leg_est_;
  // Converts the Pelvis Translation into a RBIS measurement, which is then passed to the estimator
  LegOdoCommon* leg_odo_common_;

  // Ancillary handlers
  void poseBodyHandler(const lcm::ReceiveBuffer* rbuf, const std::string& channel, const  bot_core::pose_t* msg);  
  void viconHandler(const lcm::ReceiveBuffer* rbuf, const std::string& channel, const  bot_core::rigid_transform_t* msg);  
  void republishHandler (const lcm::ReceiveBuffer* rbuf, const std::string& channel);
  void controllerInputHandler(const lcm::ReceiveBuffer* rbuf, const std::string& channel, const  pronto::controller_foot_contact_t* msg);
  void forceTorqueHandler(const lcm::ReceiveBuffer* rbuf, const std::string& channel, const  bot_core::six_axis_force_torque_array_t* msg);

  void sendTransAsVelocityPose(BotTrans msgT, int64_t utime, int64_t prev_utime, std::string channel);
  
  // Utilities
  lcm::LCM* lcm_pub;
  lcm::LCM* lcm_recv;
  boost::shared_ptr<lcm::LCM> lcm_recv_boost;
  boost::shared_ptr<lcm::LCM> lcm_pub_boost;
  boost::shared_ptr<ModelClient> model_boost;
  BotFrames* frames;
  std::string channel_force_torque;
  
  // Settings 
  // Number of iterations to assume are zero velocity at start:
  int zero_initial_velocity;
  // Republish certain incoming messages, so as to produced output logs
  bool republish_incoming_poses_;
  // Publish Debug Data e.g. kinematic velocities and foot contacts
  bool publish_diagnostics_;
  // Republish core sensors - currently just Force/Torque
  bool republish_sensors_;
  int verbose_; 
  
  // Torque Adjustment:
  bool use_torque_adjustment_;
  EstimateTools::TorqueAdjustment* torque_adjustment_;

  
  // Vicon state (just used for republishing):
  Eigen::Isometry3d prev_worldvicon_to_body_vicon_;
  int64_t prev_vicon_utime_;

  PoseT world_to_body_full_;  // POSE_BODY NB: this is whats calculated by the

  bot_core::six_axis_force_torque_array_t force_torque_; // Most recent force torque messurement
  bool force_torque_init_; // Have we received a force torque message?
 
  // Contact points of the feet deemed to be in contact:
  int n_control_contacts_left_;
  int n_control_contacts_right_;

};


}
#endif /* RBIS_LEGODO_LIB_UPDATE_HPP_ */

