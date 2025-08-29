add_executable(pg20_co_await main.cpp)

target_compile_features(pg20_co_await PRIVATE cxx_std_20)

target_include_directories(pg20_co_await PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/include
)
