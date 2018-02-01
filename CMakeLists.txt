cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE debug)

#set(testSOURCE testNMXClusterer.cpp NMXClustererDefinitions.h BoxAdministration.h BoxAdministration.cpp Clusterer.h Clusterer.cpp)
#message(${testSOURCE})
#add_executable(testnmxclusterer testNMXClusterer.cpp NMXClustererDefinitions.h BoxAdministration.h BoxAdministration.cpp Clusterer.h Clusterer.cpp)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
#
set(testfileSOURCE testNMXClustererFile.cpp NMXClustererSettings.h NMXClustererDefinitions.h BoxAdministration.h BoxAdministration.cpp Clusterer.h Clusterer.cpp)
add_executable(testnmxclusterfile testNMXClustererFile.cpp NMXClustererSettings.h NMXClustererDefinitions.h SpecialDataReader.h SpecialDataReader.cpp EventManager.h EventManager.cpp BoxAdministration.h BoxAdministration.cpp Clusterer.h Clusterer.cpp NMXClustererHelper.h PairClusters.cpp PairClusters.h)
target_link_libraries(testnmxclusterfile Threads::Threads)


#enable_testing()
#find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIRS})
#set(SOURCES UnitTests/nmxClustererTest.cpp NMXClustererDefinitions.h BoxAdministration.h BoxAdministration.cpp Clusterer.h Clusterer.cpp)
##add_executable(nmxclusterertest ${SOURCES})
#target_link_libraries(nmxclusterertest ${GTEST_BOTH_LIBRARIES})
#add_test(AllTestsInFoo nmxclusterertest)

