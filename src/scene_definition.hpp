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

inline SimpleScene createCottageScene(MaterialFactory& aMaterialFactory, GeometryFactory& aGeometryFactory) {
				SimpleScene scene;
				{
								auto cottage = std::make_shared<LoadedMeshObject>("./resources/geometry/cabins.obj");
								cottage->setPosition(glm::vec3(0.0f));
								cottage->addMaterial(
												"solid",
												MaterialParameters(
																"material_deffered",
																RenderStyle::Solid,
																{
																	{ "u_diffuseTexture", TextureInfo("cottageDif.jpg") },
																}
																)
								);
								cottage->prepareRenderData(aMaterialFactory, aGeometryFactory);
								scene.addObject(cottage);
				}
				{
								auto ground = std::make_shared<LoadedMeshObject>("./resources/geometry/terrain.obj");
								ground->addMaterial(
												"solid",
												MaterialParameters(
																"material_deffered",
																RenderStyle::Solid,
																{
																	{ "u_diffuseTexture", TextureInfo("groundDif.png") },
																}
																)
								);
								ground->prepareRenderData(aMaterialFactory, aGeometryFactory);
								scene.addObject(ground);
				}
				{
								auto oak = std::make_shared<LoadedMeshObject>("./resources/geometry/oaks.obj");
								oak->setPosition(glm::vec3(0.0f));
								oak->setScale(glm::vec3(1.0f));
								oak->addMaterial(
												"solid",
												MaterialParameters(
																"material_deffered",
																RenderStyle::Solid,
																{
																	{ "u_diffuseTexture", TextureInfo("OakDif.png") },
																}
																)
								);
								oak->prepareRenderData(aMaterialFactory, aGeometryFactory);
								scene.addObject(oak);
				}
				return scene;
}