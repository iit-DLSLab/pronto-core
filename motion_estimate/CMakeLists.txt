cmake_minimum_required(VERSION 2.8.3)
# needed for string arrays
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wno-deprecated-declarations")

if (NOT COMMAND pods_use_pkg_config_packages)
  include(cmake/pods.cmake)
endif()

project(motion_estimate)

find_package(catkin REQUIRED COMPONENTS
  filter_tools
  kalman_filter_tools
  estimate_tools
  pronto_vis
  pronto_math
  pcl_ros
  state_estimator
  pronto_conversions
  backlash_filter_tools
)


catkin_package(
  INCLUDE_DIRS
    include
  CATKIN_DEPENDS filter_tools estimate_tools 
    kalman_filter_tools pronto_vis pronto_math pcl_ros state_estimator
#  DEPENDS system_lib
)

###########
## Build ##
###########

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

###############################
set(LIBRARY_NAME leg_estimate)
add_library(${LIBRARY_NAME} src/leg_estimate/leg_estimate.cpp 
  src/leg_estimate/foot_contact_classify.cpp src/leg_estimate/FootContact.cpp 
  src/leg_estimate/FootContactAlt.cpp)
add_dependencies(${LIBRARY_NAME} ${catkin_EXPORTED_TARGETS} filter_tools pronto_math)
target_link_libraries(${LIBRARY_NAME} filter_tools kalman_filter_tools pronto_vis)
pods_use_pkg_config_packages(${LIBRARY_NAME} forward-kinematics kdl-parser bot2-param-client path-util 
  model-client bot2-frames)

#add_subdirectory(src/leg_estimate)
add_subdirectory(src/leg_estimate_apps)


###############################
set(LIBRARY_NAME mav-est-fovis)
add_library(${LIBRARY_NAME} src/mav_est_fovis/rbis_fovis_update.cpp)
add_dependencies(${LIBRARY_NAME} ${catkin_EXPORTED_TARGETS} )
pods_use_pkg_config_packages(${LIBRARY_NAME} bot2-vis)

###############################
set(LIBRARY_NAME mav-est-legodo)
add_library(${LIBRARY_NAME} src/mav_est_legodo/rbis_legodo_update.cpp 
  src/mav_est_legodo/rbis_legodo_common.cpp)
add_dependencies(${LIBRARY_NAME} ${catkin_EXPORTED_TARGETS} 
  leg_estimate pronto_math)

###############################
set(LIBRARY_NAME mav-est-pose-init)
add_library(${LIBRARY_NAME} src/pose_meas/pose_meas.cpp)

###############################
set(LIBRARY_NAME mav-est-yawlock)
add_library(${LIBRARY_NAME} src/mav_est_yawlock/rbis_yawlock_update.cpp 
  src/mav_est_yawlock/yawlock.cpp)
add_dependencies(${LIBRARY_NAME} ${catkin_EXPORTED_TARGETS} )
target_link_libraries(${LIBRARY_NAME} mav-state-est pronto_math)
pods_use_pkg_config_packages(${LIBRARY_NAME} forward-kinematics kdl-parser model-client 
  bot2-param-client bot2-frames lcmtypes_pronto eigen-utils)

add_subdirectory(src/mav_est_yawlock_apps)

add_subdirectory(src/fusion)


#############
## Install ##
#############

install(TARGETS ${TARGETS}
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)


# Mark cpp header files for installation
install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})
