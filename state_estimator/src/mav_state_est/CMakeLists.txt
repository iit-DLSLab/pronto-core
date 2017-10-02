set(LIBRARY_NAME mav-state-est)

add_library(${LIBRARY_NAME} lcm_front_end.cpp
  mav_state_est.cpp
  rbis.cpp
  rbis_initializer.cpp
  rbis_update_interface.cpp
  sensor_handlers.cpp
  update_history.cpp)

add_dependencies(${LIBRARY_NAME} ${catkin_EXPORTED_TARGETS} estimate_tools)
target_link_libraries(${LIBRARY_NAME} estimate_tools)
pods_use_pkg_config_packages(${LIBRARY_NAME} bot2-core gtk+-2.0 eigen3
  bot2-frames)