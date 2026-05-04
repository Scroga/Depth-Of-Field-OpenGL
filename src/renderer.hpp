#pragma once

#include <vector>
#include <algorithm>

#include "camera.hpp"
#include "spotlight.hpp"
#include "framebuffer.hpp"
#include "depth_texture_framebuffer.hpp"
#include "shadowmap_framebuffer.hpp"

#include "ogl_material_factory.hpp"
#include "ogl_geometry_factory.hpp"

class QuadRenderer {
public:
				QuadRenderer()
								: mQuad(generateQuadTex())
				{
				}

				void render(const OGLShaderProgram& aShaderProgram, MaterialParameterValues& aParameters) const {
								aShaderProgram.use();
								aShaderProgram.setMaterialParameters(aParameters, MaterialParameterValues());
								GL_CHECK(glBindVertexArray(mQuad.vao.get()));
								GL_CHECK(glDrawElements(mQuad.mode, mQuad.indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(0)));
				}
protected:

				IndexedBuffer mQuad;
};

inline std::vector<CADescription> getColorShadowAttachments() {
				return {
					{ GL_RGBA, GL_FLOAT, GL_RGBA32F },
					{ GL_RGBA, GL_FLOAT, GL_RGBA32F },
				};
}

inline std::vector<CADescription> getSingleColorAttachment() {
				return {
					{ GL_RGBA, GL_FLOAT, GL_RGBA32F },
				};
}

struct PostprocessingParameters {
public:
				const float distFactor = 7.0f;
				const float radiusFactor = 0.01f;
				const float smoothnessFactor = 0.01f;

				float distance;
				float radius;
				float smoothness;
				int blurRadius;
				float gaussianSigma;
				bool debug;
};

struct Postprocessing {
				Postprocessing(OGLMaterialFactory& aMaterialFactory)
				{
								depthOfFieldProgram = std::static_pointer_cast<OGLShaderProgram>(
												aMaterialFactory.getShaderProgram("depth_of_field"));
				}

				void init(int aWidth, int aHeight) {
								finalImage = std::make_shared<OGLTexture>(createColorTexture(aWidth, aHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT));
								inputImage = std::make_shared<OGLTexture>(createColorTexture(aWidth, aHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT));
				}
				std::shared_ptr<OGLTexture> inputImage;
				std::shared_ptr<OGLTexture> finalImage;

				std::shared_ptr<OGLShaderProgram> depthOfFieldProgram;
};

class Renderer {
public:
				Renderer(OGLMaterialFactory& aMaterialFactory)
								: mMaterialFactory(aMaterialFactory)
								, mPostprocessing(aMaterialFactory)
								, mShadowMapSize(1024, 1024)
				{
								mCompositingShader = std::static_pointer_cast<OGLShaderProgram>(
												mMaterialFactory.getShaderProgram("compositing"));
								mFinalOutputShader = std::static_pointer_cast<OGLShaderProgram>(
												mMaterialFactory.getShaderProgram("final_output"));
								mShadowMapShader = std::static_pointer_cast<OGLShaderProgram>(
												mMaterialFactory.getShaderProgram("shadowmap"));
				}

				void initialize(int aWidth, int aHeight) {
								mWidth = aWidth;
								mHeight = aHeight;
								GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));

								mPostprocessing.init(aWidth, aHeight);
								mFramebuffer = std::make_unique<DepthTextureFramebuffer>(aWidth, aHeight, getColorShadowAttachments());
								mShadowmapFramebuffer = std::make_unique<Framebuffer>(mShadowMapSize.x, mShadowMapSize.y, getSingleColorAttachment());
								mCompositingParameters = {};
								mFinalOutputParameters = { { "u_diffuse", TextureInfo("diffuse", mPostprocessing.finalImage)} };
				}

				template<typename TScene, typename TLight>
				void shadowMapPass(const TScene& aScene, const TLight& aLight) {
								GL_CHECK(glEnable(GL_DEPTH_TEST));

								mShadowmapFramebuffer->bind();

								GL_CHECK(glViewport(0, 0, mShadowMapSize.x, mShadowMapSize.y));
								GL_CHECK(glClearColor(1.0f, 1.0f, 1.0f, 1.0f));
								GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

								mShadowmapFramebuffer->setDrawBuffers();
								auto projection = aLight.getProjectionMatrix();
								auto view = aLight.getViewMatrix();

								MaterialParameterValues fallbackParameters;
								fallbackParameters["u_projMat"] = projection;
								fallbackParameters["u_viewMat"] = view;
								fallbackParameters["u_near"] = aLight.near();
								fallbackParameters["u_far"] = aLight.far();

								std::vector<RenderData> renderData;
								RenderOptions renderOptions = { "solid" };
								for (const auto& object : aScene.getObjects()) {
												auto data = object.getRenderData(renderOptions);
												if (data) {
																renderData.push_back(data.value());
												}
								}

								mShadowMapShader->use();
								for (const auto& data : renderData) {
												const glm::mat4 modelMat = data.modelMat;
												const OGLGeometry& geometry = static_cast<const OGLGeometry&>(data.mGeometry);

												fallbackParameters["u_modelMat"] = modelMat;

												mShadowMapShader->setMaterialParameters(fallbackParameters, {});

												geometry.bind();
												geometry.draw();
								}

								mShadowmapFramebuffer->unbind();
				}

				void clear() {
								mFramebuffer->bind();
								GL_CHECK(glClearColor(0.4f, 0.6f, 0.8f, 1.0f));
								GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
				}

				template<typename TScene, typename TCamera, typename TLight>
				void geometryPass(const TScene& aScene, const TCamera& aCamera, const TLight& aLight, RenderOptions aRenderOptions) {
								GL_CHECK(glEnable(GL_DEPTH_TEST));
								GL_CHECK(glViewport(0, 0, mWidth, mHeight));

								mFramebuffer->bind();
								mFramebuffer->setDrawBuffers();

								auto projection = aCamera.getProjectionMatrix();
								auto view = aCamera.getViewMatrix();

								std::vector<RenderData> renderData;
								for (const auto& object : aScene.getObjects()) {
												auto data = object.getRenderData(aRenderOptions);
												if (data) {
																renderData.push_back(data.value());
												}
								}

								MaterialParameterValues fallbackParameters;

								fallbackParameters["u_projMat"] = projection;
								fallbackParameters["u_viewMat"] = view;

								fallbackParameters["u_lightMat"] = aLight.getViewMatrix();
								fallbackParameters["u_lightProjMat"] = aLight.getProjectionMatrix();
								fallbackParameters["u_shadowMap"] = TextureInfo("shadowMap", mShadowmapFramebuffer->getColorAttachment(0));

								for (const auto& data : renderData) {
												const glm::mat4 modelMat = data.modelMat;
												const MaterialParameters& params = data.mMaterialParams;
												const OGLShaderProgram& shaderProgram = static_cast<const OGLShaderProgram&>(data.mShaderProgram);
												const OGLGeometry& geometry = static_cast<const OGLGeometry&>(data.mGeometry);

												fallbackParameters["u_modelMat"] = modelMat;
												fallbackParameters["u_normalMat"] = glm::transpose(glm::inverse(glm::mat3(modelMat)));

												shaderProgram.use();
												shaderProgram.setMaterialParameters(params.mParameterValues, fallbackParameters);

												geometry.bind();
												geometry.draw();
								}
								mFramebuffer->unbind();
				}

				template<typename TLight>
				void compositingPass(const TLight& aLight) {
								mCompositingShader->use();
								mCompositingShader->setMaterialParameters(mCompositingParameters);
								
								// u_outputImage
								GL_CHECK(glBindImageTexture(0, mPostprocessing.inputImage->texture.get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F));
								// u_diffuse
								GL_CHECK(glBindImageTexture(1, mFramebuffer->getColorAttachment(0)->texture.get(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F));
								// u_shadows
								GL_CHECK(glBindImageTexture(2, mFramebuffer->getColorAttachment(1)->texture.get(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F));

								GL_CHECK(glDispatchCompute((mWidth + 15) / 16, (mHeight + 15) / 16, 1));
								GL_CHECK(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
				}

				template<typename TCamera>
				void postprocessingPass(const TCamera& aCamera, const PostprocessingParameters& parameters) {
								mPostprocessing.depthOfFieldProgram->use();
								GL_CHECK(glBindImageTexture(0, mPostprocessing.inputImage->texture.get(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F));
								GL_CHECK(glBindImageTexture(1, mPostprocessing.finalImage->texture.get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F));

								MaterialParameterValues depthOfFieldParameters;
								depthOfFieldParameters["u_depthTexture"] = TextureInfo("depthTexture", mFramebuffer->getDepthAttachment());
								depthOfFieldParameters["u_distance"] = parameters.distance;
								depthOfFieldParameters["u_radius"] = parameters.radius;
								depthOfFieldParameters["u_smoothness"] = parameters.smoothness;
								depthOfFieldParameters["u_near"] = aCamera.near();
								depthOfFieldParameters["u_far"] = aCamera.far();
								depthOfFieldParameters["u_blurRadius"] = parameters.blurRadius;
								depthOfFieldParameters["u_gaussianSigma"] = parameters.gaussianSigma;
								depthOfFieldParameters["u_debug"] = parameters.debug;

								mPostprocessing.depthOfFieldProgram->setMaterialParameters(depthOfFieldParameters);

								GL_CHECK(glDispatchCompute((mWidth + 15) / 16, (mHeight + 15) / 16, 1));
								GL_CHECK(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));

								GL_CHECK(glDisable(GL_DEPTH_TEST));
								GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
								mQuadRenderer.render(*mFinalOutputShader, mFinalOutputParameters);
				}

protected:
				int mWidth = 100;
				int mHeight = 100;
				glm::ivec2 mShadowMapSize;
				std::unique_ptr<DepthTextureFramebuffer> mFramebuffer;
				std::unique_ptr<Framebuffer> mShadowmapFramebuffer;
				MaterialParameterValues mCompositingParameters;
				MaterialParameterValues mFinalOutputParameters;
				QuadRenderer mQuadRenderer;
				std::shared_ptr<OGLShaderProgram> mCompositingShader;
				std::shared_ptr<OGLShaderProgram> mFinalOutputShader;
				std::shared_ptr<OGLShaderProgram> mShadowMapShader;
				OGLMaterialFactory& mMaterialFactory;
				Postprocessing mPostprocessing;
};