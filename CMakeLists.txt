# cmake_minimum_required(VERSION <specify CMake version here>)
cmake_minimum_required(VERSION 3.17)
project(broker_yijinjing_ctp)

SET(CMAKE_BUILD_TYPE "Debug")  # Release Debug
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall")
add_compile_options(-std=c++17 -Wall -O2)

SET(ROOT_DIR "/home/work/sys/lib")

macro(SUBDIRLIST result curdir)
    file(GLOB children RELATIVE ${curdir} ${curdir}/*)
    set(dirlist "")
    foreach(child ${children})
        if(IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        endif()
    endforeach()
    set(${result} ${dirlist})
endmacro()

SUBDIRLIST(all_sub_index ${ROOT_DIR})
foreach(child ${all_sub_index})
    list(APPEND all_include ${ROOT_DIR}/${child}/include)
    list(APPEND all_lib ${ROOT_DIR}/${child}/lib)
endforeach()

SET(CTP_VERSION "libctp-6.3.15")

include_directories(${CMAKE_CURRENT_LIST_DIR}/lib/${CTP_VERSION}/include)
include_directories(${all_include})
link_directories(${CMAKE_CURRENT_LIST_DIR}/lib/${CTP_VERSION}/lib ${all_lib})

aux_source_directory (./src/libbroker_ctp LIB_LIST)

SET(BROKER_LIBRARY "broker_ctp")
add_library(${BROKER_LIBRARY} ${LIB_LIST})

SET(BROKER "ctp_broker")
SET(BROKER_TEST "test")

#add_executable(${BROKER} src/ctp_broker/main.cc)
#target_link_libraries(${BROKER}
#        ${BROKER_LIBRARY} thosttraderapi_se LinuxDataCollect broker coral swordfish x stdc++fs yaml-cpp  clickhouse-cpp-lib-static boost_date_time boost_filesystem boost_regex boost_system  boost_chrono boost_log boost_program_options boost_thread boost_iostreams z protobuf protobuf-lite sodium zmq ssl crypto iconv pthread dl)
#
#add_executable(${BROKER_TEST} src/test_broker/test.cc)
#target_link_libraries(${BROKER_TEST}
#        ${BROKER_LIBRARY} thosttraderapi_se LinuxDataCollect broker coral swordfish x stdc++fs yaml-cpp  clickhouse-cpp-lib-static boost_date_time boost_filesystem boost_regex boost_system  boost_chrono boost_log boost_program_options boost_thread boost_iostreams z protobuf protobuf-lite sodium zmq ssl crypto iconv pthread dl)

