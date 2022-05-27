cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

set (CMAKE_AUTOMOC ON)

project(video-generator)

set(MITK_SUPERBUILD_DIR "$ENV{MITK_BUILD_PATH}")
set(QT_DIR "$ENV{QT_ROOT_PATH}")

set(CMAKE_PREFIX_PATH
		"${MITK_SUPERBUILD_DIR}/MITK-build"
		"${MITK_SUPERBUILD_DIR}/ep/src/VTK/CMake/"
		"${QT_DIR}/gcc_64/lib/cmake/Qt5"
		)

find_package(MITK CONFIG REQUIRED)
find_package(OpenMP REQUIRED)

add_subdirectory(modules/MovieMaker)

set(cpp_files
  VideoGenerator.cpp
  VideoGeneratorMain.cpp
  QmitkFFmpegWriter2.cpp
	)

set(header_files
  VideoGenerator.h
  QmitkFFmpegWriter2.h
	)

add_executable(${PROJECT_NAME} ${cpp_files})
target_link_libraries(  ${PROJECT_NAME} PUBLIC
                        MitkCore  
						MitkQtWidgetsExt
						MovieMaker
						MitkSceneSerializationBase
						)


target_include_directories(${PROJECT_NAME} PUBLIC
		"${CMAKE_SOURCE_DIR}/modules/MovieMaker"
		)
