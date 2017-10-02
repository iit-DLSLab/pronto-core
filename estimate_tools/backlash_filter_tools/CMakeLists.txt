cmake_minimum_required(VERSION 2.8.3)
# needed for string arrays
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wno-deprecated-declarations")

if (NOT COMMAND pods_use_pkg_config_packages)
  include(cmake/pods.cmake)
endif()

project(backlash_filter_tools)

find_package(catkin REQUIRED COMPONENTS
  filter_tools
  kalman_filter_tools
  estimate_tools
)

catkin_package(
  INCLUDE_DIRS
    include
  CATKIN_DEPENDS filter_tools  kalman_filter_tools  estimate_tools
)

###########
## Build ##
###########

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

######################################################
add_library(${PROJECT_NAME} src/backlash_filter.cpp src/torque_adjustment.cpp)
add_dependencies(${PROJECT_NAME} ${catkin_EXPORTED_TARGETS})
pods_use_pkg_config_packages(${PROJECT_NAME} lcmtypes_bot2-core)

######################################################
add_executable(test-kalman-filter src/kalman_filter_demo.cpp)
target_link_libraries(test-kalman-filter ${catkin_LIBRARIES} 
  kalman_filter_tools filter_tools backlash_filter_tools)
pods_use_pkg_config_packages(test-kalman-filter eigen3)

add_executable(test-filter src/filtering_demo.cpp)
target_link_libraries(test-filter ${catkin_LIBRARIES} backlash_filter_tools estimate_tools filter_tools)

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
