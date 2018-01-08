PROJECT(skynet)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

ADD_DEFINITIONS(-DZLOG_MESSAGE)

SET(CMAKE_C_FLAGS_DEBUG "-O0 -Wall -g -ggdb")
SET(CMAKE_C_FLAGS_RELEASE "-O3 -Wall")

SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall")

SET(CMAKE_BUILD_TYPE Debug)
SET(EXECUTABLE_OUTPUT_PATH bin)
SET(LIBRARY_OUTPUT_PATH lib)

INCLUDE_DIRECTORIES(base/)
INCLUDE_DIRECTORIES(thirdparty/include)

# LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/thirdparty/lib)


###############################################################################
#############################   library   #####################################
###############################################################################

AUX_SOURCE_DIRECTORY(base/raft RAFT)
ADD_LIBRARY(raft ${RAFT})

AUX_SOURCE_DIRECTORY(base/box BOX)
ADD_LIBRARY(box ${BOX})



###############################################################################
##############################   test   #######################################
###############################################################################

# ADD_EXECUTABLE(ae_server test/ae/ae_server.c)
# TARGET_LINK_LIBRARIES(ae_server ae)




###############################################################################
###########################   application  ####################################
###############################################################################

AUX_SOURCE_DIRECTORY(skynet SKYNET)
ADD_EXECUTABLE(skynet ${SKYNET})
TARGET_LINK_LIBRARIES(skynet zlog event raft box jansson)


