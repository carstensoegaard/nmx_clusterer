cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE debug)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wno-error=unused-function")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(NMX_CLUSTERER_SOURCES BoxAdministration.cpp NMXClusterManager.cpp clusterer/src/NMXClusterer.cpp
        NMXClusterPairing.cpp NMXPlaneClusterer.cpp NMXLocationFinder.cpp)
set(NMX_CLUSTERER_HEADERS clusterer/include/NMXClustererSettings.h clusterer/include/NMXClustererDefinitions.h BoxAdministration.h NMXClusterManager.h
        NMXPlaneClusterer.h NMXClusterPairing.h NMXLocationFinder.h NMXQmatrix.h )
add_library(nmx ${NMX_CLUSTERER_SOURCES} ${NMX_CLUSTERER_HEADERS} clusterer/src/NMXClusterer.cpp clusterer/include/NMXClusterer.h)

set(NMX_HELPER_SOURCES SpecialDataReader.cpp ClusterReader.cpp WriteVerificationToDisk.cpp VerifyClusters.cpp)
set(NMX_HELPER_HEADERS NMXClustererHelper.h SpecialDataReader.h ClusterReader.h WriteVerificationToDisk.h VerifyClusters.h)
add_library(nmxhelper ${NMX_HELPER_SOURCES} ${NMX_HELPER_HEADERS})

add_executable(testnmxclusterfile testNMXClustererFile.cpp)
target_link_libraries(testnmxclusterfile nmx nmxhelper Threads::Threads)

#add_executable(testnmxclustermanager testClusterManager.cpp)
#target_link_libraries(testnmxclustermanager libnmx libnmxhelper)

add_executable(verifyclusterer verifyClusterer.cpp)
target_link_libraries(verifyclusterer nmxhelper)
#target_link_libraries(verifyclusterer)

#add_executable(testnmxtimeorderedbuffer testnmxtimeorderedbuffer.cpp)
#target_link_libraries(testnmxtimeorderedbuffer libnmx libnmxhelper Threads::Threads)
#
#set(testfileSOURCE testNMXClustererFile.cpp NMXClustererSettings.h NMXClustererDefinitions.h BoxAdministration.h BoxAdministration.cpp NMXPlaneClusterer.h NMXPlaneClusterer.cpp)
#add_executable(testnmxclusterfile testNMXClustererFile.cpp NMXClustererSettings.h NMXClustererDefinitions.h SpecialDataReader.h SpecialDataReader.cpp EventManager.h EventManager.cpp BoxAdministration.h BoxAdministration.cpp NMXPlaneClusterer.h NMXPlaneClusterer.cpp NMXClustererHelper.h NMXClusterPairing.cpp NMXClusterPairing.h NMXClusterManager.cpp NMXClusterManager.h testClusterManager.cpp)
#target_link_libraries(testnmxclusterfile Threads::Threads)

#add_executable(testClusterManager testClusterManager.cpp NMXClustererSettings.h NMXClustererDefinitions.h NMXClusterManager.h NMXClusterManager.cpp)
#target_link_libraries(testClusterManager)

#enable_testing()
#find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIRS})
#set(SOURCES UnitTests/nmxClustererTest.cpp NMXClustererDefinitions.h BoxAdministration.h BoxAdministration.cpp NMXPlaneClusterer.h NMXPlaneClusterer.cpp)
##add_executable(nmxclusterertest ${SOURCES})
#target_link_libraries(nmxclusterertest ${GTEST_BOTH_LIBRARIES})
#add_test(AllTestsInFoo nmxclusterertest)

