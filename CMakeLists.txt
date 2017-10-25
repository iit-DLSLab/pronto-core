cmake_minimum_required(VERSION 2.8.3)
# needed for string arrays
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wno-deprecated-declarations")

project(state_estimator)

find_package(catkin)

find_package(PkgConfig REQUIRED)
pkg_check_modules(bot2-core bot2-core)
include_directories(${bot2-core_INCLUDE_DIRS})
list(APPEND LEGACY_LIBRARIES ${bot2-core_LIBRARIES})

find_package(GTK2)
include_directories(${GTK2_INCLUDE_DIRS})
list(APPEND LEGACY_LIBRARIES ${GTK2_LIBRARIES})


pkg_check_modules(eigen-utils eigen-utils)
include_directories(${eigen-utils_INCLUDE_DIRS})
list(APPEND LEGACY_LIBRARIES ${eigen-utils_LIBRARIES})
###########
pkg_check_modules(lcmtypes_pronto lcmtypes_pronto)
include_directories(${lcmtypes_pronto_INCLUDE_DIRS})
list(APPEND LEGACY_LIBRARIES ${lcmtypes_pronto_LIBRARIES})
## Build ##
###########

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

######################################################
set(LIBRARY_NAME mav-state-est)

add_library(${LIBRARY_NAME} src/lcm_front_end.cpp
  src/mav_state_est.cpp
  src/rbis.cpp
  src/rbis_initializer.cpp
  src/rbis_update_interface.cpp
  src/sensor_handlers.cpp
  src/update_history.cpp)

catkin_package(
  INCLUDE_DIRS include ${bot2-core_INCLUDE_DIRS}
  LIBRARIES ${LIBRARY_NAME}
)

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
