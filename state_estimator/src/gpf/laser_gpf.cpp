#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <deque>

#include <bot_core/bot_core.h>
#include <bot_frames/bot_frames.h>
#include <bot_param/param_client.h>

#include <lcm/lcm.h>
#include <bot_lcmgl_client/lcmgl.h>

#include <lcmtypes/bot_core/planar_lidar_t.hpp>
#include <lcmtypes/pronto/filter_state_t.hpp>
#include <lcmtypes/pronto/indexed_measurement_t.hpp>
#include <lcmtypes/bot_core/utime_t.hpp>
#include <lcmtypes/pronto/behavior_t.hpp>
#include <lcmtypes/pronto/controller_status_t.hpp>

#include <Eigen/Dense>
#include <eigen_utils/eigen_utils.hpp>
#include <Eigen/StdVector>

#include <mav_state_est/rbis.hpp>
#include <mav_state_est/gpf/laser_gpf_lib.hpp>

#include "rbis_gpf_update.hpp"

#include <ConciseArgs>

using namespace std;
using namespace Eigen;
using namespace eigen_utils;
using namespace MavStateEst;

class app_t {
public:
  app_t(int argc, char ** argv)
  {

    ConciseArgs opt(argc, argv);
    opt.parse();

    lcm_front = new MavStateEst::LCMFrontEnd("");
    counter = 0;
    downsample_factor = bot_param_get_int_or_fail(lcm_front->param, "state_estimator.laser_gpf.downsample_factor");

    laser_handler = new LaserGPFHandler(lcm_front->lcm_pub->getUnderlyingLCM(), lcm_front->param, lcm_front->frames);

    bot_gauss_rand_init(time(NULL)); //randomize for particles

    laser_queue = new deque<bot_core::planar_lidar_t *>();
    pointcloud_queue = new deque<bot_core::pointcloud_t *>();
    filter_state_queue = new deque<pronto::filter_state_t *>();

    gpf = laser_handler->gpf;

    //setup threads
    lcm_data_mutex = g_mutex_new();
    lcm_data_cond = g_cond_new();
    processor_thread = g_thread_create(processing_func, (void *) this, false, NULL);

    //----------------------------------------------------------------------------
    if (gpf->sensor_mode == LaserGPF::sensor_input_laser){
      std::cout << "LaserGPF out of process expects planar lidar on " << laser_handler->laser_channel.c_str() << std::endl;
      lcm_front->lcm_recv->subscribe(laser_handler->laser_channel.c_str(), &app_t::laser_message_handler, this);
    }else{
      std::cout << "LaserGPF out of process expects pointcloud/velodyne on " << laser_handler->laser_channel.c_str() << std::endl;
      lcm_front->lcm_recv->subscribe(laser_handler->laser_channel.c_str(), &app_t::pointcloud_message_handler, this);
    }
    lcm_front->lcm_recv->subscribe(lcm_front->filter_state_channel.c_str(), &app_t::filter_state_handler, this);

    // drc integration:
    lcm_front->lcm_recv->subscribe("STATE_EST_LASER_DISABLE", &app_t::laser_disable_handler, this);
    lcm_front->lcm_recv->subscribe("STATE_EST_LASER_ENABLE", &app_t::laser_enable_handler, this);
    //
    lcm_front->lcm_recv->subscribe("ATLAS_STATUS", &app_t::atlas_status_handler, this); // from BDI
    lcm_front->lcm_recv->subscribe("CONTROLLER_STATUS", &app_t::controller_status_handler, this); // from MIT controller
    lcm_front->lcm_recv->subscribe("STATE_EST_USE_NEW_MAP", &app_t::use_new_map_handler, this);
    behavior_prev = pronto::behavior_t::BEHAVIOR_NONE;
    utime_standing_trans = 0;

  }

  //--------------------LCM stuff-----------------------
  MavStateEst::LCMFrontEnd * lcm_front;

  LaserGPFHandler * laser_handler;

  //lcm reading thread stuff
  GThread * processor_thread;
  GMutex * lcm_data_mutex; //should be held when reading/writing message queues
  GCond * lcm_data_cond; //signals new lcm data
  deque<bot_core::planar_lidar_t *> * laser_queue;
  deque<bot_core::pointcloud_t *> * pointcloud_queue;
  deque<pronto::filter_state_t *> * filter_state_queue;
  int noDrop;
  //------------------------------------------------------

  int counter;
  int downsample_factor;

  LaserGPF *gpf;

  // Modules to Bit flip the laser on or off, added by mfallon
  int behavior_prev ;
  int64_t utime_standing_trans;

  void laser_disable_handler(const lcm::ReceiveBuffer* rbuf, const std::string& channel,
      const bot_core::utime_t * msg)
  {
    gpf->laser_enabled = false;
    //fprintf(stderr, "D\n");
  }

  void laser_enable_handler(const lcm::ReceiveBuffer* rbuf, const std::string& channel,
      const bot_core::utime_t * msg)
  {
    gpf->laser_enabled = true;
    //fprintf(stderr, "E\n");

    // this will allow the laser to function for 2 seconds:
    fprintf(stderr, "Forcing enabling of laser from viewer\n");
    utime_standing_trans = msg->utime;
  }

  void use_new_map_handler(const lcm::ReceiveBuffer* rbuf, const std::string& channel,
      const bot_core::utime_t * msg){
    std::cout << "Deleting current gpf and restarting\n";
    delete laser_handler;

    laser_handler = new LaserGPFHandler(lcm_front->lcm_pub->getUnderlyingLCM(), lcm_front->param, lcm_front->frames);


  }

  void atlas_status_handler(const lcm::ReceiveBuffer* rbuf, const std::string& channel,
      const pronto::behavior_t * msg){

    if (msg->behavior == pronto::behavior_t::BEHAVIOR_USER){
      // Don't change the behavior variable in user mode
      //fprintf(stderr, "\nIn BDI user-mode\n");
      behavior_prev = msg->behavior;
      return;
    }

    if ( (msg->behavior != pronto::behavior_t::BEHAVIOR_STAND) &&  (msg->behavior != pronto::behavior_t::BEHAVIOR_MANIPULATE) ){
      if ( !gpf->laser_enabled ){
        fprintf(stderr, "\nNot Standing or Manipulating - enabling  laser\n");
      }
      gpf->laser_enabled = true;
    }

    if ( (behavior_prev != pronto::behavior_t::BEHAVIOR_STAND) &&  (behavior_prev != pronto::behavior_t::BEHAVIOR_MANIPULATE) ){
      if ( (msg->behavior == pronto::behavior_t::BEHAVIOR_STAND) ||  (msg->behavior == pronto::behavior_t::BEHAVIOR_MANIPULATE) ){
        fprintf(stderr, "\nEntering stand\n");
        utime_standing_trans = msg->utime;
      }
    }

    if ( (msg->behavior == pronto::behavior_t::BEHAVIOR_STAND) ||  (msg->behavior == pronto::behavior_t::BEHAVIOR_MANIPULATE) ){
      if ( msg->utime - utime_standing_trans > 2E6){
        if (gpf->laser_enabled){
          fprintf(stderr, "\nBeen standing for some time %f - disabling laser\n", ( (double) (msg->utime - utime_standing_trans)*1E-6) );
        }
        gpf->laser_enabled = false;
      }
    }
    behavior_prev = msg->behavior;
  }


  void controller_status_handler(const lcm::ReceiveBuffer* rbuf, const std::string& channel,
      const pronto::controller_status_t * msg){

    if (behavior_prev != pronto::behavior_t::BEHAVIOR_USER){
      // Don't change the behavior variable when not in user mode
      //fprintf(stderr, "\nIn BDI user-mode\n");
      return;
    }

    // std::cout << (int) behavior_prev << " is bdi\n";
    bool laser_enabled_after = gpf->laser_enabled;

    if (msg->state == pronto::controller_status_t::DUMMY){
      //std::cout << msg->utime << " got " << (int) msg->state << " MIT dummy\n";
      // Dummy is usually very short, so don't do anything
      laser_enabled_after = false;
    }else if (msg->state == pronto::controller_status_t::UNKNOWN){
      //std::cout << msg->utime << " got " << (int) msg->state << " MIT unknown\n";
      // Unknown is usually very short (1 tic)
      laser_enabled_after = false;
    }else if (msg->state == pronto::controller_status_t::STANDING){
      //std::cout << msg->utime << " got " << (int) msg->state << " MIT standing\n";
      laser_enabled_after = false;
    }else if (msg->state == pronto::controller_status_t::WALKING){
      //std::cout << msg->utime << " got " << (int) msg->state << " MIT walking\n";
      laser_enabled_after = true;
    }else{
      // other modes?
      std::cout << msg->utime << " got " << (int) msg->state << " MIT other---------------------\n";
    }

    if (gpf->laser_enabled != laser_enabled_after){
      std::cout << "Changing Laser mode: " << (int) gpf->laser_enabled << " --> " << (int) laser_enabled_after << "\n";
    }

    gpf->laser_enabled = laser_enabled_after;

  }




  ////////////

  void filter_state_handler(const lcm::ReceiveBuffer* rbuf, const std::string& channel,
      const pronto::filter_state_t * msg)
  {
    pronto::filter_state_t * msg_copy = new pronto::filter_state_t(*msg);
    g_mutex_lock(lcm_data_mutex);
    filter_state_queue->push_back(msg_copy);
    g_mutex_unlock(lcm_data_mutex);
    g_cond_broadcast(lcm_data_cond);
  }

  void laser_message_handler(const lcm::ReceiveBuffer* rbuf, const std::string& channel,
      const bot_core::planar_lidar_t * msg)
  {
    if (counter++ % downsample_factor != 0)
      return;

    bot_core::planar_lidar_t * msg_copy = new bot_core::planar_lidar_t(*msg);
    g_mutex_lock(lcm_data_mutex);
    if (!noDrop) {
      //clear the old messages
      while (!laser_queue->empty()) {
        bot_core::planar_lidar_t * msg_queued = laser_queue->front();
        delete msg_queued;
        laser_queue->pop_front();
      }
    }
    laser_queue->push_back(msg_copy);
    g_mutex_unlock(lcm_data_mutex);
    g_cond_broadcast(lcm_data_cond);
  }

  void pointcloud_message_handler(const lcm::ReceiveBuffer* rbuf, const std::string& channel,
      const bot_core::pointcloud_t * msg)
  {
    if (counter++ % downsample_factor != 0)
      return;

    bot_core::pointcloud_t * msg_copy = new bot_core::pointcloud_t(*msg);
    g_mutex_lock(lcm_data_mutex);
    if (!noDrop) {
      //clear the old messages
      while (!pointcloud_queue->empty()) {
        bot_core::pointcloud_t * msg_queued = pointcloud_queue->front();
        delete msg_queued;
        pointcloud_queue->pop_front();
      }
    }
    pointcloud_queue->push_back(msg_copy);
    g_mutex_unlock(lcm_data_mutex);
    g_cond_broadcast(lcm_data_cond);
  }

  static void * processing_func(void * user)
  {
    app_t * app = (app_t *) user;

    g_mutex_lock(app->lcm_data_mutex);
    while (1) {

      bot_core::planar_lidar_t * laser_msg;
      bot_core::pointcloud_t * pointcloud_msg;
      int64_t msg_utime;
      if (app->gpf->sensor_mode == LaserGPF::sensor_input_laser){
        if (app->laser_queue->empty() || app->filter_state_queue->empty()) {
          g_cond_wait(app->lcm_data_cond, app->lcm_data_mutex);
          continue;
        }
        laser_msg = app->laser_queue->front();
        msg_utime = laser_msg->utime;
        app->laser_queue->pop_front();
      }else{
        if (app->pointcloud_queue->empty() || app->filter_state_queue->empty()) {
          g_cond_wait(app->lcm_data_cond, app->lcm_data_mutex);
          continue;
        }
        pointcloud_msg = app->pointcloud_queue->front();
        msg_utime = pointcloud_msg->utime;
        app->pointcloud_queue->pop_front();
      }

      pronto::filter_state_t * fs_msg = NULL;
      //keep going until the queue is empty, or the front is actually after the
      while (!app->filter_state_queue->empty() && app->filter_state_queue->front()->utime <= msg_utime) {
        if (fs_msg != NULL) {
          delete fs_msg;
        }
        fs_msg = app->filter_state_queue->front();
        app->filter_state_queue->pop_front();
      }
      if (fs_msg == NULL) {
        fprintf(stderr, "WARNING: first filter state is after the laser message\n");
        continue;
      }

      g_mutex_unlock(app->lcm_data_mutex);

      Eigen::VectorXd z_effective;
      Eigen::MatrixXd R_effective;

      Map<const RBIM> cov(&fs_msg->cov[0]);
      Map<const RBIS::VectorNd> state_vec_map(&fs_msg->state[0]);
      Quaterniond quat;
      botDoubleToQuaternion(quat, fs_msg->quat);
      RBIS state(state_vec_map, quat);

      bool valid = false;
      if (app->gpf->sensor_mode == LaserGPF::sensor_input_laser){
        valid = app->gpf->getMeasurement(state, cov, laser_msg, z_effective, R_effective);
      }else{
        valid = app->gpf->getMeasurement(state, cov, pointcloud_msg, z_effective, R_effective);
      }


      if (valid) {
//      eigen_dump(state);
//      eigen_dump(cov);
//      eigen_dump(z_effective.transpose());
//      eigen_dump(R_effective);

        pronto::indexed_measurement_t * gpf_msg = gpfCreateLCMmsgCPP(app->gpf->laser_gpf_measurement_indices, z_effective,
            R_effective);
        gpf_msg->utime = msg_utime;
        gpf_msg->state_utime = fs_msg->utime;

        app->lcm_front->lcm_pub->publish(app->laser_handler->pub_channel.c_str(), gpf_msg);

        delete gpf_msg;
      }

      //destroy the local copies of the data
      if (app->gpf->sensor_mode == LaserGPF::sensor_input_laser){
        delete laser_msg;
      }else{
        delete pointcloud_msg;
      }
      delete fs_msg;

      //go back around loop, must hold lcm_data lock
      g_mutex_lock(app->lcm_data_mutex);
    }
    return NULL;
  }

};

int main(int argc, char **argv)
{

  app_t * app = new app_t(argc, argv);

  while (true) {
    int ret = app->lcm_front->lcm_recv->handle();
    if (ret != 0) {
      printf("log is done\n");
      break;
    }
  }

  //wait on the mutex until the queues are empty
  g_mutex_lock(app->lcm_data_mutex);
  g_cond_wait(app->lcm_data_cond, app->lcm_data_mutex);

  //todo: compute stats, cleanup, etc...
  printf("all_done!\n");
  return 0;
}
