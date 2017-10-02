set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wno-deprecated-declarations")

################
add_executable(se-leg-odometry main.cpp LegOdoWrapper.cpp )
target_link_libraries(se-leg-odometry ${catkin_LIBRARIES} leg_estimate)

################
add_executable(se-convert-bdi-into-vicon-frame convert_bdi_into_vicon_frame.cpp )
target_link_libraries(se-convert-bdi-into-vicon-frame ${catkin_LIBRARIES} leg_estimate)

################
add_executable(se-init-from-bdi init_from_bdi.cpp )
target_link_libraries(se-init-from-bdi ${catkin_LIBRARIES} leg_estimate)