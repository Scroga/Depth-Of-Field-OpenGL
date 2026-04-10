cmake_minimum_required(VERSION 3.10)

project(DepthOfField)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find OpenGL package
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

if(WIN32)
	# Attempt to find the GLFW root directory
	find_path(GLFW_ROOT_DIR NAMES include/GLFW/glfw3.h
		PATHS
			"${THIRD_PARTY_DIR}/glfw-3.4.bin.WIN64"
			"${THIRD_PARTY_DIR}/glfw-3.4"
			"${THIRD_PARTY_DIR}/../glfw-3.4.bin.WIN64"
			"${THIRD_PARTY_DIR}/../glfw-3.4"
		NO_DEFAULT_PATH) # Avoid searching in default paths

	if(GLFW_ROOT_DIR)
		# Determine the appropriate library directory based on the Visual Studio version
		if(MSVC_VERSION GREATER_EQUAL 1930 AND MSVC_VERSION LESS 1950)
			set(GLFW_LIB_DIR "${GLFW_ROOT_DIR}/lib-vc2022")
		elseif(MSVC_VERSION GREATER_EQUAL 1920 AND MSVC_VERSION LESS 1930)
			set(GLFW_LIB_DIR "${GLFW_ROOT_DIR}/lib-vc2019")
		elseif(MSVC_VERSION GREATER_EQUAL 1910 AND MSVC_VERSION LESS 1920)
			set(GLFW_LIB_DIR "${GLFW_ROOT_DIR}/lib-vc2017")
			# Add more conditions for other Visual Studio versions as needed
		else()
			message(FATAL_ERROR "Unsupported Visual Studio version for GLFW.")
		endif()

		add_library(glfw INTERFACE)

		# Set the include directory for the glfw target
		target_include_directories(glfw INTERFACE "${GLFW_ROOT_DIR}/include")

		# Assuming the GLFW library filename; adjust as necessary
		find_library(GLFW_LIBRARY NAMES glfw3 PATHS "${GLFW_LIB_DIR}" NO_DEFAULT_PATH)

		if(NOT GLFW_LIBRARY)
			message(FATAL_ERROR "GLFW library not found.")
		endif()

		# Link the library with the glfw target
		target_link_libraries(glfw INTERFACE "${GLFW_LIBRARY}")

		# Specify the include directory
		set(GLFW_INCLUDE_DIR "${GLFW_ROOT_DIR}/include")
	else()
		message(FATAL_ERROR "GLFW root directory not found.")
	endif()

	find_path(GLM_ROOT_DIR NAMES glm/glm.hpp
		PATHS
			"${THIRD_PARTY_DIR}/glm-1.0.1"
			"${THIRD_PARTY_DIR}/../glm-1.0.1"
		NO_DEFAULT_PATH)

	if (NOT GLM_ROOT_DIR)
		message(FATAL_ERROR "GLM root directory not found.")
	endif()

	add_library(glm INTERFACE)
	target_include_directories(glm INTERFACE "${GLM_ROOT_DIR}")
	target_compile_definitions(glm INTERFACE GLM_ENABLE_EXPERIMENTAL GLM_FORCE_SWIZZLE)
	add_library(glm::glm ALIAS glm)
else()
	find_package(glfw3 3.3 REQUIRED)
	find_package(glm REQUIRED)
	add_compile_definitions(GLM_FORCE_SWIZZLE GLM_ENABLE_EXPERIMENTAL)
endif()

add_library(utils
	utils/ogl_material_factory.cpp
	utils/ogl_geometry_factory.cpp
	utils/ogl_geometry_construction.cpp
	utils/obj_file_loading.cpp
	)
target_link_libraries(utils glm::glm glfw OpenGL::GL)
target_include_directories(utils PUBLIC
	${THIRD_PARTY_DIR}/glad/include
	${THIRD_PARTY_DIR}
)

set(LABS_OUT_DIR "${CMAKE_BINARY_DIR}")

# Setup resources
function(add_resources TARGET_NAME RESOURCE_DIR)
	# Make RESOURCE_DIR absolute
	if (NOT IS_ABSOLUTE "${RESOURCE_DIR}")
        set(ABS_RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE_DIR}")
    else()
        set(ABS_RESOURCE_DIR "${RESOURCE_DIR}")
    endif()

    if(NOT EXISTS "${ABS_RESOURCE_DIR}")
        message(WARNING "Resource directory '${ABS_RESOURCE_DIR}' does not exist (target: ${TARGET_NAME})")
        return()
    endif()

	# Ensure the target's runtime output goes there
    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

	# Name of the resource folder itself
    get_filename_component(RES_NAME "${ABS_RESOURCE_DIR}" NAME)

	# Copy directory after building the target
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${ABS_RESOURCE_DIR}"
                "${CMAKE_BINARY_DIR}/${RES_NAME}"
        COMMENT "Copying ${RES_NAME} for ${TARGET_NAME} to ${CMAKE_BINARY_DIR}/${RES_NAME}"
    )

endfunction()

add_executable(depth_of_field 
src/main.cpp
utils/error_handling.hpp
utils/ogl_resource.hpp
utils/shader.hpp
utils/window.hpp)

target_sources(depth_of_field PRIVATE
    ${THIRD_PARTY_DIR}/glad/src/glad.c
)

target_link_libraries(depth_of_field PRIVATE utils glm::glm glfw OpenGL::GL)

target_include_directories(depth_of_field PRIVATE
    ${THIRD_PARTY_DIR}/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_resources(depth_of_field "resources")
add_resources(depth_of_field "shaders")