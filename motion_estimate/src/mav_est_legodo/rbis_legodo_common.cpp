#include "mav_est_legodo/rbis_legodo_common.hpp"

namespace MavStateEst {

LegOdoCommon::LegOdoCommon(lcm::LCM* lcm_recv,  lcm::LCM* lcm_pub, BotParam * param):
  lcm_recv(lcm_recv), lcm_pub(lcm_pub){
  verbose = false;
  
  char* mode_str = bot_param_get_str_or_fail(param, "state_estimator.legodo.mode");
  if (strcmp(mode_str, "lin_rot_rate") == 0) {
    mode_ = MODE_LIN_AND_ROT_RATE;
    std::cout << "LegOdo will provide velocity and rotation rates." << std::endl;
  }else if (strcmp(mode_str, "lin_rate") == 0){
    mode_ = MODE_LIN_RATE;
    std::cout << "LegOdo will provide linear velocity rates but no rotation rates." << std::endl;
  }else if (strcmp(mode_str, "pos_and_lin_rate") == 0){
    mode_ = MODE_POSITION_AND_LIN_RATE;
    std::cout << "LegOdo will provide positions as well as linear velocity rates." << std::endl;
  }else{
    std::cout << "Legodo not understood! [LegOdoCommon]." << std::endl;
  }

  free(mode_str);
  
  R_legodo_xyz_ = bot_param_get_double_or_fail(param, "state_estimator.legodo.r_xyz");
  
  R_legodo_vxyz_ = bot_param_get_double_or_fail(param, "state_estimator.legodo.r_vxyz");
  R_legodo_vang_ = bot_param_get_double_or_fail(param, "state_estimator.legodo.r_vang");
  
  R_legodo_vxyz_uncertain_ = bot_param_get_double_or_fail(param, "state_estimator.legodo.r_vxyz_uncertain");
  R_legodo_vang_uncertain_ = bot_param_get_double_or_fail(param, "state_estimator.legodo.r_vang_uncertain");
}  
  
void LegOdoCommon::getCovariance(LegOdoCommonMode mode_current, bool delta_certain,
  Eigen::MatrixXd &cov_legodo, Eigen::VectorXi &z_indices){
  
  // Determine which velocity variance to use
  double R_legodo_vxyz_current = R_legodo_vxyz_;
  double R_legodo_vang_current = R_legodo_vang_;
  if (!delta_certain){
    R_legodo_vxyz_current = R_legodo_vxyz_uncertain_;
    R_legodo_vang_current = R_legodo_vang_uncertain_;
  }
  
  Eigen::VectorXd R_legodo;
  if (mode_current == MODE_LIN_AND_ROT_RATE) {
    z_indices.resize(6);
    R_legodo.resize(6);
  }else if (mode_current == MODE_LIN_RATE){
    z_indices.resize(3);
    R_legodo.resize(3);
  }else if (mode_current == MODE_POSITION_AND_LIN_RATE){
    z_indices.resize(6);
    R_legodo.resize(6);
  }

  // Initialize covariance matrix based on mode.
  if (mode_current == MODE_LIN_AND_ROT_RATE) {
    R_legodo(0) = bot_sq(R_legodo_vxyz_current);
    R_legodo(1) = bot_sq(R_legodo_vxyz_current);
    R_legodo(2) = bot_sq(R_legodo_vxyz_current);
    R_legodo(3) = bot_sq(R_legodo_vang_current);
    R_legodo(4) = bot_sq(R_legodo_vang_current);
    R_legodo(5) = bot_sq(R_legodo_vang_current);
    
    z_indices.head<3>() = eigen_utils::RigidBodyState::velocityInds();
    z_indices.tail<3>() = eigen_utils::RigidBodyState::angularVelocityInds();
  }else if (mode_current == MODE_LIN_RATE){
    R_legodo(0) = bot_sq(R_legodo_vxyz_current);
    R_legodo(1) = bot_sq(R_legodo_vxyz_current);
    R_legodo(2) = bot_sq(R_legodo_vxyz_current);
    
    z_indices.head<3>() = eigen_utils::RigidBodyState::velocityInds();
  }else if (mode_current == MODE_POSITION_AND_LIN_RATE){
    R_legodo(0) = bot_sq(R_legodo_xyz_);
    R_legodo(1) = bot_sq(R_legodo_xyz_);
    R_legodo(2) = bot_sq(R_legodo_xyz_);
    R_legodo(3) = bot_sq(R_legodo_vxyz_current);
    R_legodo(4) = bot_sq(R_legodo_vxyz_current);
    R_legodo(5) = bot_sq(R_legodo_vxyz_current);
    
    z_indices.head<3>() = eigen_utils::RigidBodyState::positionInds();
    z_indices.tail<3>() = eigen_utils::RigidBodyState::velocityInds();    
  }
  
  cov_legodo = R_legodo.asDiagonal();

}


void printTrans(BotTrans bt, std::string message){
  std::cout << message << ": "
      << bt.trans_vec[0] << ", " << bt.trans_vec[1] << ", " << bt.trans_vec[2] << " | "
      << bt.rot_quat[0] << ", " << bt.rot_quat[1] << ", " << bt.rot_quat[2] << ", " << bt.rot_quat[3] << "\n";
}


// Difference the transform and scale by elapsed time:
BotTrans LegOdoCommon::getTransAsVelocityTrans(BotTrans msgT, int64_t utime, int64_t prev_utime){

  Eigen::Isometry3d msgE = pronto::getBotTransAsEigen(msgT);
  Eigen::Isometry3d msgE_vel = pronto::getDeltaAsVelocity(msgE, (utime-prev_utime) );
  BotTrans msgT_vel;
  msgT_vel = pronto::getEigenAsBotTrans(msgE_vel);
 
  return msgT_vel;
}


RBISUpdateInterface * LegOdoCommon::createMeasurement(BotTrans &odo_positionT, BotTrans &odo_deltaT, 
                                                      int64_t utime, int64_t prev_utime, 
                                                      int odo_position_status, float odo_delta_status){
  BotTrans odo_velT = getTransAsVelocityTrans(odo_deltaT, utime, prev_utime);
  
  Eigen::MatrixXd cov_legodo_use;
  
  LegOdoCommonMode mode_current = mode_;
  if ( (mode_current == MODE_POSITION_AND_LIN_RATE) && (!odo_position_status) ){
    if (verbose) std::cout << "LegOdo Mode is MODE_POSITION_AND_LIN_RATE but position is not suitable\n";
    if (verbose) std::cout << "Falling back to lin rate only for this iteration\n";
    mode_current = MODE_LIN_RATE;
  }
  
  bool delta_certain = true;
  if (odo_delta_status < 0.5){ // low variance, high reliable
    delta_certain = true;
  }else{ // high variance, low reliable e.g. breaking contact
    delta_certain = false;
  }  
  
  Eigen::MatrixXd cov_legodo;
  Eigen::VectorXi z_indices;
  getCovariance(mode_current, delta_certain, cov_legodo, z_indices );  
  

  if (mode_current == MODE_LIN_AND_ROT_RATE) {
    // Apply a linear and rotation rate correction
    // This was finished in Feb 2015 but not tested on the robot
    Eigen::VectorXd z_meas(6);

    double rpy[3];
    bot_quat_to_roll_pitch_yaw(odo_deltaT.rot_quat,rpy);
    double elapsed_time = ( (double) utime -  prev_utime)/1000000;
    double rpy_rate[3];
    rpy_rate[0] = rpy[0]/elapsed_time;
    rpy_rate[1] = rpy[1]/elapsed_time;
    rpy_rate[2] = rpy[2]/elapsed_time;

    z_meas.head<3>() = Eigen::Map<const Eigen::Vector3d>(odo_velT.trans_vec);
    z_meas.tail<3>() = Eigen::Map<const Eigen::Vector3d>(rpy_rate);
    return new RBISIndexedMeasurement(z_indices, z_meas, cov_legodo, RBISUpdateInterface::legodo,
           utime);
  }else if (mode_current == MODE_LIN_RATE) {
    return new RBISIndexedMeasurement(z_indices,
           Eigen::Map<const Eigen::Vector3d>( odo_velT.trans_vec ), cov_legodo, RBISUpdateInterface::legodo,
           utime);
  }else if (mode_current == MODE_POSITION_AND_LIN_RATE) {
    // Newly added mode
    if (verbose) std::cout << "LegOdometry Mode update both xyz position and rate\n";
    Eigen::VectorXd z_meas(6);
    z_meas.head<3>() = Eigen::Map<const Eigen::Vector3d>(odo_positionT.trans_vec);
    z_meas.tail<3>() = Eigen::Map<const Eigen::Vector3d>(odo_velT.trans_vec);
    return new RBISIndexedMeasurement(z_indices, z_meas, cov_legodo, RBISUpdateInterface::legodo,
           utime);
  }else{
    std::cout << "LegOdometry Mode not supported, exiting\n";
    return NULL;
  }

}

}
