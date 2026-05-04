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

void changeValue(const std::string& name, float& value, float factor) {
				value += factor;
				if (value < 0.0f) value = 0.0f;
				std::cout << name << ": " << value << '\n';
}

void printInfo() {
				std::cout
								<< "\n=== Controls ===\n"
								<< "Camera movement:\n"
								<< "  W        - move forward\n"
								<< "  S        - move backward\n"
								<< "  A        - move left\n"
								<< "  D        - move right\n"
								<< "  Q        - move down\n"
								<< "  E        - move up\n"
								<< "  Mouse    - rotate camera\n"
								<< "\n"
								<< "Camera:\n"
								<< "  R        - reset camera position and rotation\n"
								<< "\n"
								<< "Depth of field parameters:\n"
								<< "  Up       - increase focus distance\n"
								<< "  Down     - decrease focus distance\n"
								<< "  Right    - increase radius\n"
								<< "  Left     - decrease radius\n"
								<< "  >        - increase smoothness\n"
								<< "  <        - decrease smoothness\n"
								<< "\n"
								<< "Debug / info:\n"
								<< "  F        - toggle debug mode\n"
								<< "  I        - print this info\n"
								<< "================\n\n";
}

int main() {
				// Initialize GLFW
				if (!glfwInit()) {
								std::cerr << "Failed to initialize GLFW" << std::endl;
								return -1;
				}

				const float cameraNear = 0.1f;
				const float cameraFar = 500.0f;
				const glm::vec3 cameraPosition{ 0.0f, 10.0f, 50.0f };
				const float cameraSpeed{ 0.20f };

				try {
								PostprocessingParameters parameters{
												.distance = 60.0f,
												.radius = 0.01f,
												.smoothness = 0.1f,
												.blurRadius = 5,
												.gaussianSigma = 2.0f,
												.debug = false
								};

								auto window = Window(1080, 720, "Depth of field");
								MouseTracking mouseTracking;
								FirstPersonCamera camera(window.aspectRatio(), cameraPosition, cameraNear, cameraFar);
								camera.setSpeed(cameraSpeed);

								SpotLight light(90, 100.0f, 3000.0f);
								light.setPosition(glm::vec3(-600.0f, 700.0f, 600.0f));
								light.lookAt(glm::vec3());

								window.lockCursor();

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

								window.setKeyCallback([&camera, &cameraPosition, &parameters](GLFWwindow* aWin, int key, int scancode, int action, int mods) {												
												if (action == GLFW_PRESS) {
																switch (key) {
																case GLFW_KEY_R:
																				camera.setPosition(cameraPosition);
																				camera.resetRotation();
																				break;

																case GLFW_KEY_F:
																				toggle("debug", parameters.debug);
																				break;
																case GLFW_KEY_I:
																				printInfo();
																				break;
																case GLFW_KEY_UP:
																				changeValue("Distance", parameters.distance, parameters.distFactor);
																				break;
																case GLFW_KEY_DOWN:
																				changeValue("Distance", parameters.distance, -parameters.distFactor);
																				break;
																case GLFW_KEY_RIGHT:
																				changeValue("Radius", parameters.radius, parameters.radiusFactor);
																				break;
																case GLFW_KEY_LEFT:
																				changeValue("Radius", parameters.radius, -parameters.radiusFactor);
																				break;
																case GLFW_KEY_PERIOD: // This is '>' key
																				changeValue("Smoothness", parameters.smoothness, parameters.smoothnessFactor);
																				break;
																case GLFW_KEY_COMMA: // This is '<' key
																				changeValue("Smoothness", parameters.smoothness, -parameters.smoothnessFactor);
																				break;
																}
												}
												});

								OGLMaterialFactory materialFactory;
								materialFactory.loadShadersFromDir("./shaders/");
								materialFactory.loadTexturesFromDir("./resources/textures/");

								OGLGeometryFactory geometryFactory;

								std::array<SimpleScene, 1> scenes{
									createCottageScene(materialFactory, geometryFactory),
								};

								Renderer renderer(materialFactory);
								window.onResize([&camera, &window, &renderer](int width, int height) {
												camera.setAspectRatio(window.aspectRatio());
												renderer.initialize(width, height);
												});

								const int currentSceneIdx = 0;
								renderer.initialize(window.size()[0], window.size()[1]);
								window.runLoop([&] {
												renderer.shadowMapPass(scenes[currentSceneIdx], light);

												renderer.clear();
												renderer.geometryPass(scenes[currentSceneIdx], camera, light, RenderOptions{ "solid" });
												renderer.compositingPass(light);
												renderer.postprocessingPass(camera, parameters);
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