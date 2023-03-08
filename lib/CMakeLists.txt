
# Add GLAD
add_subdirectory(glad)

# Add glfw
set(GLFW_INSTALL OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_USE_HYBRID_HPG OFF)
add_subdirectory(glfw)

# Add glm
add_subdirectory(glm)