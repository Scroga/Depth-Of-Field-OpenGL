#pragma once

#include <vector>

#include "camera.hpp"
#include "framebuffer.hpp"
#include "ogl_material_factory.hpp"
#include "ogl_geometry_factory.hpp"

class QuadRenderer {
public:
				QuadRenderer()
								: mQuad(generateQuadTex()) {
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

inline std::vector<CADescription> getColorNormalPositionAttachments() {
				return {
					{ GL_RGBA, GL_FLOAT, GL_RGBA32F },
					{ GL_RGBA, GL_FLOAT, GL_RGBA32F },
					{ GL_RGBA, GL_FLOAT, GL_RGBA32F },
					{ GL_RGBA, GL_FLOAT, GL_RGBA32F },
				};
}

struct Postprocessing {
				Postprocessing(OGLMaterialFactory& aMaterialFactory) {
								program = std::static_pointer_cast<OGLShaderProgram>(
												aMaterialFactory.getShaderProgram("convolution"));
				}

				void init(int aWidth, int aHeight) {
								inputImage = std::make_shared<OGLTexture>(createColorTexture(aWidth, aHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT));
								finalImage = std::make_shared<OGLTexture>(createColorTexture(aWidth, aHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT));
				}

				std::shared_ptr<OGLTexture> inputImage;
				std::shared_ptr<OGLTexture> finalImage;

				std::shared_ptr<OGLShaderProgram> program;
};


class Renderer {
public:

				Renderer(OGLMaterialFactory& aMaterialFactory)
								: mMaterialFactory(aMaterialFactory)
								, mPostprocessing(aMaterialFactory)
				{
								mCompositingShader = std::static_pointer_cast<OGLShaderProgram>(
												mMaterialFactory.getShaderProgram("compositing2"));

								mFinalOutputShader = std::static_pointer_cast<OGLShaderProgram>(
												mMaterialFactory.getShaderProgram("final_output"));
				}

				void initialize(int aWidth, int aHeight) {
								mWidth = aWidth;
								mHeight = aHeight;

								GL_CHECK(glEnable(GL_DEPTH_TEST));
								GL_CHECK(glClearColor(0.2f, 0.3f, 0.3f, 1.0f));

								mPostprocessing.init(aWidth, aHeight);
								mFramebuffer = std::make_unique<Framebuffer>(aWidth, aHeight, getColorNormalPositionAttachments());

								mCompositingParameters = {
												// 	{ "u_diffuse", TextureInfo("diffuse", mFramebuffer->getColorAttachment(0)) },
												// 	{ "u_normal", TextureInfo("diffuse", mFramebuffer->getColorAttachment(1)) },
												// 	{ "u_position", TextureInfo("diffuse", mFramebuffer->getColorAttachment(2)) },
												// 	// { "u_shadowMap", TextureInfo("shadowMap", mShadowmapFramebuffer->getDepthMap()) },
													// { "u_shadowMap", TextureInfo("shadowMap", mShadowmapFramebuffer->getColorAttachment(0)) },
								};
								mFinalOutputParameters = {
									{ "u_diffuse", TextureInfo("diffuse", mPostprocessing.finalImage) },
								};
				}

				void clear() {
								mFramebuffer->bind();
								GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
								GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
				}

				template<typename TScene, typename TCamera>
				void geometryPass(const TScene& aScene, const TCamera& aCamera, RenderOptions aRenderOptions) {
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
								fallbackParameters["u_solidColor"] = glm::vec4(0, 0, 0, 1);
								fallbackParameters["u_viewPos"] = aCamera.getPosition();
								//fallbackParameters["u_near"] = aCamera.near();
								//fallbackParameters["u_far"] = aCamera.far();
								for (const auto& data : renderData) {
												const glm::mat4& modelMat = data.modelMat;
												const MaterialParameters& params = data.mMaterialParams;
												const OGLShaderProgram& shaderProgram = static_cast<const OGLShaderProgram&>(data.mShaderProgram);
												const OGLGeometry& geometry = static_cast<const OGLGeometry&>(data.mGeometry);

												fallbackParameters["u_modelMat"] = modelMat;
												fallbackParameters["u_normalMat"] = glm::mat3(modelMat);

												shaderProgram.use();
												shaderProgram.setMaterialParameters(params.mParameterValues, fallbackParameters);
												geometry.bind();
												geometry.draw();
								}
								mFramebuffer->unbind();
				}

				void compositingPass() {
								mCompositingShader->use();

								// Add compositing parameters

								mCompositingShader->setMaterialParameters(mCompositingParameters);
								GL_CHECK(glBindImageTexture(0, mPostprocessing.inputImage->texture.get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F));
								GL_CHECK(glBindImageTexture(1, mFramebuffer->getColorAttachment(1)->texture.get(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F));
								GL_CHECK(glBindImageTexture(2, mFramebuffer->getColorAttachment(2)->texture.get(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F));
								GL_CHECK(glBindImageTexture(3, mFramebuffer->getColorAttachment(3)->texture.get(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F));
								GL_CHECK(glBindImageTexture(4, mFramebuffer->getColorAttachment(4)->texture.get(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F));

								GL_CHECK(glDispatchCompute((mWidth + 15) / 16, (mHeight + 15) / 16, 1));
								GL_CHECK(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));

								mPostprocessing.program->use();
								GL_CHECK(glBindImageTexture(0, mPostprocessing.inputImage->texture.get(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F));
								GL_CHECK(glBindImageTexture(1, mPostprocessing.finalImage->texture.get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F));

								MaterialParameterValues postprocessingParameters;
								
								// TODO: ADD paramters

								mPostprocessing.program->setMaterialParameters(postprocessingParameters);

								GL_CHECK(glDispatchCompute((mWidth + 15) / 16, (mHeight + 15) / 16, 1));
								GL_CHECK(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));

								GL_CHECK(glDisable(GL_DEPTH_TEST));
								GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));

								// mQuadRenderer.render(*mCompositingShader, mCompositingParameters);
								mQuadRenderer.render(*mFinalOutputShader, mFinalOutputParameters);
				}

protected:
				int mWidth = 100;
				int mHeight = 100;
				std::unique_ptr<Framebuffer> mFramebuffer;
				MaterialParameterValues mCompositingParameters;
				MaterialParameterValues mFinalOutputParameters;
				std::shared_ptr<OGLShaderProgram> mCompositingShader;
				std::shared_ptr<OGLShaderProgram> mFinalOutputShader;
				OGLMaterialFactory& mMaterialFactory;
				Postprocessing mPostprocessing;
				QuadRenderer mQuadRenderer;
};
