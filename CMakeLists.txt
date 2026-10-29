cmake_minimum_required(VERSION 3.25)

project(graphics_engine VERSION 0.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE graphics_engine_headers CONFIGURE_DEPENDS src/*.hpp)
file(GLOB_RECURSE graphics_engine_sources CONFIGURE_DEPENDS src/*.cpp)

add_library(graphics_engine STATIC ${graphics_engine_sources} ${graphics_engine_headers})

# --- Dependencies -------------------------------------------------------------

find_package(PNG REQUIRED)
find_package(Freetype REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)

find_package(PkgConfig REQUIRED)
# pkg-config results land in the cache, so a prefix of its own keeps them from
# overwriting a consumer's search for the same module.
pkg_check_modules(GRAPHICS_ENGINE_EPOXY REQUIRED IMPORTED_TARGET epoxy)
pkg_check_modules(GRAPHICS_ENGINE_ICU REQUIRED IMPORTED_TARGET icu-uc)

# --- Submodules ----------------------------------------------

set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET3 OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_ENET OFF CACHE BOOL "" FORCE)
set(BUILD_CLSOCKET OFF CACHE BOOL "" FORCE)
set(BUILD_PYBULLET OFF CACHE BOOL "" FORCE)
set(USE_GRAPHICAL_BENCHMARK OFF CACHE BOOL "" FORCE)
set(ENABLE_VHACD OFF CACHE BOOL "" FORCE)
set(INSTALL_CMAKE_FILES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

add_subdirectory(external/bullet3 EXCLUDE_FROM_ALL)

set_target_properties(BulletCollision LinearMath PROPERTIES SYSTEM TRUE)

# --- Library ------------------------------------------------------------------

target_compile_features(graphics_engine PUBLIC cxx_std_26)
set_target_properties(graphics_engine PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(graphics_engine PUBLIC src)

target_include_directories(graphics_engine SYSTEM PUBLIC external/bullet3/src)

target_link_libraries(graphics_engine PUBLIC
	PNG::PNG
	Freetype::Freetype
	glm::glm
	Microsoft.GSL::GSL
	BulletCollision
	LinearMath
	PkgConfig::GRAPHICS_ENGINE_EPOXY
	PkgConfig::GRAPHICS_ENGINE_ICU
)

target_compile_definitions(graphics_engine PUBLIC GLM_ENABLE_EXPERIMENTAL)

# --- Compiler -----------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(graphics_engine PRIVATE
		-Wall
		-Wextra
		-Wpedantic
		-Wshadow
		-Wnon-virtual-dtor
		-Woverloaded-virtual
		-Wold-style-cast
		-Wcast-align
		-Wnull-dereference
		-Wdouble-promotion
		-Wformat=2
		-Wmisleading-indentation
		-Wimplicit-fallthrough
		-Wconversion
		-Wsign-conversion
		-Wzero-as-null-pointer-constant
		-Wundef
		-Wfloat-conversion
		-Werror
	)

	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_compile_options(graphics_engine PRIVATE
			-Wlogical-op
			-Wduplicated-cond
			-Wduplicated-branches
			-Wuseless-cast
		)
	endif()

	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		target_compile_options(graphics_engine PRIVATE
			-Wunreachable-code
			-Wextra-semi
			# Clang grades #embed as an extension; AGENTS.md of decade explains the exception.
			-Wno-c23-extensions
		)
	endif()
endif()

# A consumer that adds this directory brings its own tests and gates; the
# target names would collide with its own.
if(NOT PROJECT_IS_TOP_LEVEL)
	return()
endif()

enable_testing()
add_subdirectory(tests)

# --- Gates --------------------------------------------------------------------

add_custom_target(sanitize-address
	COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/san-address -G ${CMAKE_GENERATOR}
		-DCMAKE_BUILD_TYPE=RelWithDebInfo
		"-DCMAKE_CXX_FLAGS=-D_GLIBCXX_ASSERTIONS -fsanitize=address,leak,undefined,float-cast-overflow,float-divide-by-zero -fno-sanitize-recover=undefined -fno-omit-frame-pointer"
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/san-address
	COMMAND ${CMAKE_COMMAND} -E env UBSAN_OPTIONS=print_stacktrace=1
		${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/san-address --output-on-failure
	COMMENT "Sanitizer gate: address, leak, undefined, float checks, container assertions"
	VERBATIM USES_TERMINAL)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# The driver has to match the compiler that configured this directory; the
	# unversioned name points at the distribution's default LLVM major.
	string(REGEX MATCH "^[0-9]+" GRAPHICS_ENGINE_CLANG_MAJOR ${CMAKE_CXX_COMPILER_VERSION})
	find_program(GRAPHICS_ENGINE_RUN_CLANG_TIDY
		NAMES run-clang-tidy-${GRAPHICS_ENGINE_CLANG_MAJOR} run-clang-tidy
		REQUIRED)

	add_custom_target(clang-tidy
		COMMAND ${GRAPHICS_ENGINE_RUN_CLANG_TIDY} -p ${CMAKE_BINARY_DIR}
			-warnings-as-errors=*
			${CMAKE_CURRENT_SOURCE_DIR}/src/
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "clang-tidy gate"
		VERBATIM USES_TERMINAL)
endif()
