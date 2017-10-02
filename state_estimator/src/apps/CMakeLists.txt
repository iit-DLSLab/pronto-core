add_executable(mav-state-estimator mav_state_estimator.cpp)
add_dependencies(mav-state-estimator ${catkin_EXPORTED_TARGETS})
pods_use_pkg_config_packages(mav-state-estimator bot2-core eigen3 bot2-vis
  bot2-frames 
  lcmtypes_occ-map
  lcmtypes_pronto octomap)
# octomap octomap-util

target_link_libraries(mav-state-estimator octomap mav-state-est gpf-laser-lib)
#  octomap octomath
