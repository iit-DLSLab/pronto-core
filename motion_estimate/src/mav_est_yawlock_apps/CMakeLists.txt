#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wno-deprecated-declarations")

################
add_executable(se-yawlock yawlock_app.cpp )
target_link_libraries(se-yawlock ${catkin_LIBRARIES} mav-est-yawlock)

################
add_executable(se-quick-lock quick_lock.cpp )
target_link_libraries(se-quick-lock ${catkin_LIBRARIES} mav-est-yawlock)