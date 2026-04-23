#pragma once

#include <memory>
#include <vector>
#include <ranges>
#include <random>

#include "scene_object.hpp"
#include "cube.hpp"

#include "material_factory.hpp"
#include "geometry_factory.hpp"
#include "simple_scene.hpp"

std::vector<glm::vec3> generateOakPositions() {
				std::vector<glm::vec3> seeds = {
					glm::vec3(0.0f, 0.0f, -40.0f),
					glm::vec3(-50.0f, 0.0f, -10.0f),
					glm::vec3(35.0f, 0.0f, 5.0f),
				};

				float maxRadius = 30;
				int count = 13;
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_real_distribution<float> dist(-maxRadius, maxRadius);

				std::vector<glm::vec3> points;
				for (auto& seed : seeds) {
								int generated = 0;
								while (generated < count) {
												float x = dist(gen);
												float y = dist(gen);
												if ((x * x + y * y) < (maxRadius * maxRadius)) {
																glm::vec3 point = glm::vec3(seed.x + x, seed.y, seed.z + y);
																points.push_back(point);
																++generated;
												}
								}
				}
				return points;
}


inline SimpleScene createCottageScene(MaterialFactory& aMaterialFactory, GeometryFactory& aGeometryFactory) {
				SimpleScene scene;
				{
								auto cottage = std::make_shared<LoadedMeshObject>("./resources/geometry/cottage.obj");
								cottage->addMaterial(
												"solid",
												MaterialParameters(
																"material_deffered",
																RenderStyle::Solid,
																{
																	{ "u_diffuseTexture", TextureInfo("cottage/cottageDif.jpg") },
																}
																)
								);
								cottage->prepareRenderData(aMaterialFactory, aGeometryFactory);
								scene.addObject(cottage);
				}
				{
								auto ground = std::make_shared<LoadedMeshObject>("./resources/geometry/ground.obj");
								ground->addMaterial(
												"solid",
												MaterialParameters(
																"material_deffered",
																RenderStyle::Solid,
																{
																	{ "u_diffuseTexture", TextureInfo("cottage/groundDif.png") },
																}
																)
								);
								ground->prepareRenderData(aMaterialFactory, aGeometryFactory);
								scene.addObject(ground);
				}

				for (auto position : generateOakPositions()) {
								auto oak = std::make_shared<LoadedMeshObject>("./resources/geometry/oak.obj");
								oak->setPosition(position);
								oak->setScale(glm::vec3(1.0f));
								oak->addMaterial(
												"solid",
												MaterialParameters(
																"material_deffered",
																RenderStyle::Solid,
																{
																	{ "u_diffuseTexture", TextureInfo("cottage/OakDif.png") },
																}
																)
								);
								oak->prepareRenderData(aMaterialFactory, aGeometryFactory);
								scene.addObject(oak);
				}



				return scene;
}