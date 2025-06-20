cmake_minimum_required(VERSION 3.28)

project(
	edit
	VERSION 0.1.0
	LANGUAGES C
)

add_executable(
	edit

	"src/main.c"
)

target_compile_features(edit PRIVATE c_std_99)
target_compile_options(edit PRIVATE -Wall -Wextra -pedantic)

find_package(Curses REQUIRED)

target_include_directories(
	edit
	PRIVATE

	${PROJECT_SOURCE_DIR}/inc
	${CURSES_INCLUDE_DIRS}
)

target_link_libraries(edit PRIVATE ${CURSES_LIBRARIES})
