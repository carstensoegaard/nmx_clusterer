cmake_minimum_required(VERSION 3.5)

project(NMXClusterer CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE debug)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wno-error=unused-function")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(NMXCLUSTERER_INC
        clusterer/include/NMXClustererSettings.h
        clusterer/include/NMXClustererDefinitions.h
        clusterer/include/NMXClusterer.h
        clusterer/include/NMXPlaneClusterer.h
        clusterer/include/NMXBoxAdministration.h
        clusterer/include/NMXClusterManager.h
        clusterer/include/NMXClusterPairing.h
        clusterer/include/NMXQmatrix.h
        clusterer/include/NMXLocationFinder.h)

set(NMXCLUSTERER_SRC
        clusterer/src/NMXClusterer.cpp
        clusterer/src/NMXPlaneClusterer.cpp
        clusterer/src/NMXBoxAdministration.cpp
        clusterer/src/NMXClusterManager.cpp
        clusterer/src/NMXClusterPairing.cpp
        clusterer/src/NMXLocationFinder.cpp)

add_library(nmxbase STATIC ${NMXCLUSTERER_INC} ${NMXCLUSTERER_SRC})

set(NMXHELPER_INC
        helper/include/NMXClustererHelper.h
        helper/include/ClusterReader.h
        helper/include/NMXClustererVerification.h
        helper/include/SpecialDataReader.h
        helper/include/VerifyClusters.h
        helper/include/WriteVerificationToDisk.h)

set(NMXHELPER_SRC
        helper/src/ClusterReader.cpp
        helper/src/NMXClustererVerification.cpp
        helper/src/SpecialDataReader.cpp
        helper/src/VerifyClusters.cpp
        helper/src/WriteVerificationToDisk.cpp)

add_library(nmxhelper STATIC ${NMXHELPER_INC} ${NMXHELPER_SRC})

add_executable(testnmxclustererfile testNMXClustererFile.cpp)
target_link_libraries(testnmxclustererfile nmxbase nmxhelper Threads::Threads)

add_executable(verifyclusterer verifyClusterer.cpp)
target_link_libraries(verifyclusterer nmxhelper)
