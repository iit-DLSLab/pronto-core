#include "gpf/rbis_gpf_update.hpp"
#include <path_util/path_util.h>

namespace MavStateEst {

RBISLaserGPFMeasurement::RBISLaserGPFMeasurement(LaserGPF *gpf_, bot_core::planar_lidar_t * laser_msg_, int64_t utime,
    lcm_t * lcm_, const std::string & pub_channel_) :
    RBISUpdateInterface(laser, utime), gpf(gpf_), laser_msg(laser_msg_), lcm_pub(lcm_), pub_channel(pub_channel_)
{

}

RBISLaserGPFMeasurement::RBISLaserGPFMeasurement(LaserGPF *gpf_, bot_core::pointcloud_t * pointcloud_msg_, int64_t utime,
    lcm_t * lcm_, const std::string & pub_channel_) :
    RBISUpdateInterface(laser, utime), gpf(gpf_), pointcloud_msg(pointcloud_msg_), lcm_pub(lcm_), pub_channel(pub_channel_)
{

}
RBISLaserGPFMeasurement::~RBISLaserGPFMeasurement()
{
  if (gpf->sensor_mode == LaserGPF::sensor_input_laser){
    delete laser_msg;
  }else{
    delete pointcloud_msg;
  }
}

void RBISLaserGPFMeasurement::updateFilter(const RBIS & prior_state, const RBIM & prior_cov, double prior_loglikelihood)
{

//  eigen_dump(prior_state);
//  eigen_dump(prior_cov);

  Eigen::VectorXd z_effective;
  Eigen::MatrixXd R_effective;

  double current_likelihood = 0;

  bool valid = false;
  if (gpf->sensor_mode == LaserGPF::sensor_input_laser){
    valid = gpf->getMeasurement(prior_state, prior_cov, laser_msg, z_effective, R_effective);
  }else{
    valid = gpf->getMeasurement(prior_state, prior_cov, pointcloud_msg, z_effective, R_effective);
  }

  if (valid) {

    //publish the gpf result for logging
    pronto_indexed_measurement_t * gpf_msg = gpfCreateLCMmsg(gpf->laser_gpf_measurement_indices, z_effective, R_effective);
    gpf_msg->utime = utime;
    gpf_msg->state_utime = prior_state.utime;
    pronto_indexed_measurement_t_publish(lcm_pub, pub_channel.c_str(), gpf_msg);
    pronto_indexed_measurement_t_destroy(gpf_msg);

// apply the gpf update
    RBIS dstate;
    RBIM dcov;
    current_likelihood = indexedMeasurement(z_effective, R_effective, gpf->laser_gpf_measurement_indices, prior_state,
        prior_cov, dstate,
        dcov);
    rbisApplyDelta(prior_state, prior_cov, dstate, dcov, posterior_state, posterior_covariance);

//    eigen_dump(z_effective.transpose());
//    eigen_dump(R_effective);
//
//    eigen_dump(posterior_state);
//    eigen_dump(posterior_covariance);

  }
  else { //skip this update
    posterior_state = prior_state;
    posterior_covariance = prior_cov;
  }

  loglikelihood = prior_loglikelihood + current_likelihood;
}

LaserGPFHandler::LaserGPFHandler(lcm_t * lcm, BotParam * param, BotFrames * frames)
{
  lcm_pub = lcm;
  gpf = new LaserGPF(lcm, param, frames);

  char * pub_chan = bot_param_get_str_or_fail(param, "state_estimator.laser_gpf_out_of_process.channel");
  pub_channel = pub_chan;
  free(pub_chan);

  char * laser_chan = bot_param_get_str_or_fail(param, "state_estimator.laser_gpf.channel");
  laser_channel = laser_chan;
  free(laser_chan);
}

RBISUpdateInterface * LaserGPFHandler::processMessage(const bot_core::planar_lidar_t * msg, MavStateEstimator* state_estimator)
{
  bot_core::planar_lidar_t * msg_cpy = new bot_core::planar_lidar_t(*msg);
  return new RBISLaserGPFMeasurement(gpf, msg_cpy, msg->utime, lcm_pub, pub_channel);
}

RBISUpdateInterface * LaserGPFHandler::processMessagePointcloud(const bot_core::pointcloud_t * msg, MavStateEstimator* state_estimator)
{
  bot_core::pointcloud_t * msg_cpy = new bot_core::pointcloud_t(*msg);
  return new RBISLaserGPFMeasurement(gpf, msg_cpy, msg->utime, lcm_pub, pub_channel);
}

}
