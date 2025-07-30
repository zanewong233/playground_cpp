cmake_minimum_required(VERSION 3.14)

# 找到系统线程库（在 Unix 下是 pthread，在 Windows 下是 Win32 线程）
find_package(Threads REQUIRED)

# 自动把目录下所有 .cpp 都变成独立的 benchmark 可执行文件
file(GLOB BENCH_SRC CONFIGURE_DEPENDS "*.cpp")
foreach(src ${BENCH_SRC})
  # 去掉路径和扩展名，得到 target 名称
  get_filename_component(name ${src} NAME_WE)
  add_executable(${name} ${src})
  target_link_libraries(${name}
    PRIVATE
      benchmark::benchmark
      Threads::Threads
  )
  
  target_include_directories(${name}
	PRIVATE
	  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  )
endforeach()