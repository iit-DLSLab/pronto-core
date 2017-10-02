set(LIBRARY_NAME leg_estimate)
add_library(${LIBRARY_NAME} leg_estimate.cpp 
  foot_contact_classify.cpp FootContact.cpp 
  FootContactAlt.cpp)
add_dependencies(${LIBRARY_NAME} ${catkin_EXPORTED_TARGETS} filter_tools)
target_link_libraries(${LIBRARY_NAME} filter_tools kalman_filter_tools pronto_vis)
pods_use_pkg_config_packages(${LIBRARY_NAME} forward-kinematics kdl-parser bot2-param-client path-util 
  model-client bot2-frames)