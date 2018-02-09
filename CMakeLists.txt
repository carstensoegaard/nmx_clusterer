cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE debug)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(NMX_CLUSTERER_SOURCES BoxAdministration.cpp NMXClusterManager.cpp Clusterer.cpp)
set(NMX_CLUSTERER_HEADERS NMXClustererSettings.h NMXClustererDefinitions.h BoxAdministration.h NMXClusterManager.h Clusterer.h)
add_library(libnmx ${NMX_CLUSTERER_SOURCES} ${NMX_CLUSTERER_HEADERS})

set(NMX_HELPER_SOURCES SpecialDataReader.cpp EventManager.cpp)
set(NMX_HELPER_HEADERS NMXClustererHelper.h SpecialDataReader.h EventManager.h)
add_library(libnmxhelper ${NMX_HELPER_SOURCES} ${NMX_HELPER_HEADERS})

add_executable(testnmxclusterfile testNMXClustererFile.cpp NMXTimeOrderedBuffer.cpp NMXTimeOrderedBuffer.h)
target_link_libraries(testnmxclusterfile libnmx libnmxhelper Threads::Threads)

add_executable(testnmxclustermanager testClusterManager.cpp NMXTimeOrderedBuffer.cpp NMXTimeOrderedBuffer.h)
target_link_libraries(testnmxclustermanager libnmx libnmxhelper)
#
#set(testfileSOURCE testNMXClustererFile.cpp NMXClustererSettings.h NMXClustererDefinitions.h BoxAdministration.h BoxAdministration.cpp Clusterer.h Clusterer.cpp)
#add_executable(testnmxclusterfile testNMXClustererFile.cpp NMXClustererSettings.h NMXClustererDefinitions.h SpecialDataReader.h SpecialDataReader.cpp EventManager.h EventManager.cpp BoxAdministration.h BoxAdministration.cpp Clusterer.h Clusterer.cpp NMXClustererHelper.h NMXClusterPairing.cpp NMXClusterPairing.h NMXClusterManager.cpp NMXClusterManager.h testClusterManager.cpp)
#target_link_libraries(testnmxclusterfile Threads::Threads)

#add_executable(testClusterManager testClusterManager.cpp NMXClustererSettings.h NMXClustererDefinitions.h NMXClusterManager.h NMXClusterManager.cpp)
#target_link_libraries(testClusterManager)

#enable_testing()
#find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIRS})
#set(SOURCES UnitTests/nmxClustererTest.cpp NMXClustererDefinitions.h BoxAdministration.h BoxAdministration.cpp Clusterer.h Clusterer.cpp)
##add_executable(nmxclusterertest ${SOURCES})
#target_link_libraries(nmxclusterertest ${GTEST_BOTH_LIBRARIES})
#add_test(AllTestsInFoo nmxclusterertest)

