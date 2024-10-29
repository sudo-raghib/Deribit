cmake_minimum_required(VERSION 3.10)
project(DeribitTradingClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
    /usr/local/include
)

# Find packages
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

include_directories(${CURL_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${nlohmann_json_INCLUDE_DIR})

# Source files
set(SOURCE_FILES
    main.cpp
    deribit_client.cpp
)

add_executable(DeribitTradingClient ${SOURCE_FILES})

target_link_libraries(DeribitTradingClient
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    nlohmann_json::nlohmann_json
)
