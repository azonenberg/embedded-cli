# CMake build script for embedded-cli.
# Intended to be integrated into a larger project, not built standalone.

add_library(embedded-cli STATIC

	# TODO: only for stm32 targets?
	../stm32-cpp/src/cli/UARTOutputStream.cpp

	CLIOutputStream.cpp
	CLISessionContext.cpp
	CLIToken.cpp
	)

# TODO: only for stm32 targets?
target_include_directories(embedded-cli
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	"$<TARGET_PROPERTY:stm32-cpp,INTERFACE_INCLUDE_DIRECTORIES>"
	)
