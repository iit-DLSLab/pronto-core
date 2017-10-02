#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wno-deprecated-declarations")

################
add_executable(se-fusion fusion.cpp )
target_link_libraries(se-fusion ${catkin_LIBRARIES} mav-state-est mav-est-legodo mav-est-legodo 
  mav-est-fovis mav-est-pose-init mav-est-yawlock estimate_tools backlash_filter_tools
  pronto_math leg_estimate path-util lcmtypes_occ-map)# gpf-laser-lib)