set(LIBRARY_NAME gpf-laser-lib)

add_library(${LIBRARY_NAME} SHARED laser_gpf_lib.cpp
  gpf.cpp
  LaserLikelihoodInterface.cpp
  rbis_gpf_update.cpp)

#add_dependencies(${LIBRARY_NAME} ${catkin_EXPORTED_TARGETS})
#target_link_libraries(${LIBRARY_NAME} octomap)
pods_use_pkg_config_packages(${LIBRARY_NAME} bot2-core eigen3   
  gtk+-2.0 lcmtypes_pronto laser-util bot2-lcmgl-client
  eigen-utils  path-util octomap-util octomap)
# octomap-util octomap occ-map