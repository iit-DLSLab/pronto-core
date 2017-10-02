cmake_minimum_required(VERSION 2.8.3)
# needed for string arrays
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wno-deprecated-declarations")

#if (NOT COMMAND pods_use_pkg_config_packages)
#  include(cmake/pods.cmake)
#endif()

project(filter_tools)

#find_package(catkin REQUIRED COMPONENTS
#)

catkin_package(
  INCLUDE_DIRS
    include
)

###########
## Build ##
###########

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

######################################################

add_library(${PROJECT_NAME} src/Filter.cpp src/HeavyLowPassFilter.cpp
  src/SignalTap.cpp)

pods_use_pkg_config_packages(${PROJECT_NAME} bot2-frames eigen3)


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
