project(MMOClient)

set(CMAKE_CXX_STANDARD 17)

set(CLIENT_HEADERS
      ../include/olcPixelGameEngine.h
      ../include/olcPGEX_TransformedView.h)

set(CLIENT_SOURCES 
      MMOClient.cpp)

add_executable(MMOClient
                MMOClient.cpp)

target_include_directories(MMOClient PUBLIC
                            ../include)
