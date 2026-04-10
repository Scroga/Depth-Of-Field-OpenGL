#include <iostream>
#include <cassert>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <array>

#include "ogl_resource.hpp"
#include "error_handling.hpp"
#include "window.hpp"
#include "shader.hpp"
#include "first_person_camera.hpp"

#include "scene_definition.hpp"
#include "renderer.hpp"

#include "ogl_geometry_factory.hpp"
#include "ogl_material_factory.hpp"

#include <glm/gtx/string_cast.hpp>

void toggle(const std::string& aToggleName, bool& aToggleValue) {

				aToggleValue = !aToggleValue;
				std::cout << aToggleName << ": " << (aToggleValue ? "ON\n" : "OFF\n");
}

struct Config {
				int currentSceneIdx = 0;
				bool showSolid = true;
				bool showWireframe = false;
				bool showNormals = false;
};

int main() {
				// Initialize GLFW
				if (!glfwInit()) {
								std::cerr << "Failed to initialize GLFW" << std::endl;
								return -1;
				}

				const glm::vec3 cameraPosition{ 0.0f, 0.0f, 3.0f };
				const float cameraSpeed{ 0.05f };

				try {
								auto window = Window(1080, 720, "Depth of field");
								MouseTracking mouseTracking;
								Config config;
								FirstPersonCamera camera(window.aspectRatio(), cameraPosition, glm::vec3(0.0f, 1.0f, 0.0f));
								camera.setSpeed(cameraSpeed);
				
								window.lockCursor();
								window.onResize([&camera, &window](int width, int height) {
												camera.setAspectRatio(window.aspectRatio());
												});

								window.onCheckInput([&camera, &mouseTracking](GLFWwindow* aWin) {
												mouseTracking.update(aWin);

												if (glfwGetKey(aWin, GLFW_KEY_W) == GLFW_PRESS)
																camera.processKeyboard(FORWARD);
												if (glfwGetKey(aWin, GLFW_KEY_S) == GLFW_PRESS)
																camera.processKeyboard(BACKWARD);
												if (glfwGetKey(aWin, GLFW_KEY_A) == GLFW_PRESS)
																camera.processKeyboard(LEFT);
												if (glfwGetKey(aWin, GLFW_KEY_D) == GLFW_PRESS)
																camera.processKeyboard(RIGHT);
												if (glfwGetKey(aWin, GLFW_KEY_Q) == GLFW_PRESS)
																camera.processKeyboard(DOWN);
												if (glfwGetKey(aWin, GLFW_KEY_E) == GLFW_PRESS)
																camera.processKeyboard(UP);

												auto off = mouseTracking.offset();
												camera.processMouseMovement(off.x, off.y);
								});

								window.setKeyCallback([&config, &camera, &cameraPosition](GLFWwindow* aWin, int key, int scancode, int action, int mods) {
												if (action == GLFW_PRESS) {
																switch (key) {
																case GLFW_KEY_R:
																				camera.setPosition(cameraPosition);
																				camera.resetRotation();  
																				break;
																case GLFW_KEY_1:
																				config.currentSceneIdx = 0;
																				break;
																case GLFW_KEY_2:
																				config.currentSceneIdx = 1;
																				break;
																case GLFW_KEY_3:
																				config.currentSceneIdx = 2;
																				break;
																case GLFW_KEY_Z:
																				toggle("Show wireframe", config.showWireframe);
																				break;
																case GLFW_KEY_C:
																				toggle("Show normals", config.showNormals);
																				break;
																case GLFW_KEY_X:
																				toggle("Show solid", config.showSolid);
																				break;
																}
												}
												});

								OGLMaterialFactory materialFactory;
								materialFactory.loadShadersFromDir("./shaders/");
								materialFactory.loadTexturesFromDir("./resources/textures/");

								OGLGeometryFactory geometryFactory;

								std::array<SimpleScene, 3> scenes{
									createCubeScene(materialFactory, geometryFactory),
									createInstancedCubesScene(materialFactory, geometryFactory),
									createMonkeyScene(materialFactory, geometryFactory),
								};

								Renderer renderer(materialFactory);

								renderer.initialize();
								window.runLoop([&] {
												renderer.clear();
												if (config.showSolid) {
																GL_CHECK(glDisable(GL_POLYGON_OFFSET_LINE));
																GL_CHECK(glPolygonOffset(0.0f, 0.0f));
																GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
																renderer.renderScene(scenes[config.currentSceneIdx], camera, RenderOptions{ "solid" });
												}
												if (config.showWireframe) {
																GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
																GL_CHECK(glEnable(GL_POLYGON_OFFSET_LINE));
																GL_CHECK(glPolygonOffset(-1.0f, -1.0f));
																renderer.renderScene(scenes[config.currentSceneIdx], camera, RenderOptions{ "wireframe" });
												}
												if (config.showNormals) {
																GL_CHECK(glEnable(GL_POLYGON_OFFSET_LINE));
																GL_CHECK(glPolygonOffset(-1.0f, -1.0f));
																renderer.renderSceneNormals(scenes[config.currentSceneIdx], camera, RenderOptions{ "solid" });
												}
												});
				}
				catch (ShaderCompilationError& exc) {
								std::cerr
												<< "Shader compilation error!\n"
												<< "Shader type: " << exc.shaderTypeName()
												<< "\nError: " << exc.what() << "\n";
								return -3;
				}
				catch (OpenGLError& exc) {
								std::cerr << "OpenGL error: " << exc.what() << "\n";
								return -2;
				}
				catch (std::exception& exc) {
								std::cerr << "Error: " << exc.what() << "\n";
								return -1;
				}

				glfwTerminate();
				return 0;
}
