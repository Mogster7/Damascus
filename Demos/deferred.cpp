
#include "deferred.h"

#include "Window.h"
#include "glfw3.h"
#include "imgui_impl_glfw.h"
#include "Camera/Camera.h"
#include "glm/glm.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

constexpr glm::uvec2 FB_SIZE = { 1600, 900 }; 

class DeferredRenderingContext : public RenderingContext
{
public:
	const std::vector<TexVertex> meshVerts = {
		{ { -1.0, 1.0, 0.5 },{ 0.0f, 1.0f } },	// 0
		{ { -1.0, -1.0, 0.5 },{ 0.0f, 0.0f } },	    // 1
		{ { 1.0, -1.0, 0.5 },{ 1.0f, 0.0f } },    // 2
		{ { 1.0, 1.0, 0.5 },{ 1.0f, 1.0f } },   // 3

	};

	const std::vector<ColorVertex> meshVerts2 = {
		{ { -0.25, 0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } },	// 0
		{ { -0.25, -0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } },	    // 1
		{ { 0.25, -0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
		{ { 0.25, 0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } },   // 3

	};

	const std::vector<uint32_t> squareIndices = {
		0, 1, 2,
		2, 3, 0
	};


	const std::vector<Vertex> cubeVerts = {
		{ { -1.0, -1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f } , { 1.0f, 0.0f, 1.0f }},
		{ { 1.0f, -1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } , { 0.0f, 0.0f, 1.0f }},
                                                
		{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f } , { 0.0f, 1.0f, 1.0f } },
		{ { -1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f } , { 1.0f, 0.0f, 1.0f }},
                                                
		{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f  }, { 0.0f, 0.0f, 0.0f}},
		{ { 1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f  }},
                                                
		{ { 1.0f, 1.0f, -1.0f }, { 1.0f, 1.0f, 0.0f } , { 1.0f, 1.0f, 0.0f }},
		{ { -1.0f, 1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f  }},
	};


	const std::vector<uint32_t> cubeIndices =
	{
		// front
		0, 1, 2,
		2, 3, 0,
		// right
		1, 5, 6,
		6, 2, 1,
		// back
		7, 6, 5,
		5, 4, 7,
		// left
		4, 0, 3,
		3, 7, 4,
		// bottom
		4, 5, 1,
		1, 0, 4,
		// top
		3, 2, 6,
		6, 7, 3
	};

	const std::vector<uint32_t> meshIndices = {
		0, 1, 2,
		2, 3, 0
	};

	Texture testTex;

	Camera camera;

	bool copyDepth = true;
	int32_t debugDisplayTarget = 0;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
		glm::vec4 instancePos[3];
	} uboOffscreenVS;

	struct UboViewProjection
    {
        glm::mat4 projection;
        glm::mat4 view;
    } uboViewProjection;


	struct Light {
		glm::vec4 position = {};
		glm::vec3 color = {};
		float radius = {};
	};

	struct UboComposition {
		Light lights[6] = {};
		glm::vec4 viewPos = {};
		float globalLightStrength = 1.0f;
		int debugDisplayTarget = 0;
	} uboComposition;


	// One sampler for the frame buffer color attachments
	vk::Sampler colorSampler;

    std::vector<Mesh<Vertex>> objects;
    std::vector<Mesh<Vertex>> forwardObjects;

    // Descriptors
	Descriptors descriptors;
    vk::PushConstantRange pushRange;


    std::vector<Buffer> uniformBufferViewProjection;
    std::vector<Buffer> uniformBufferComposition;

	// Deferred Data

	struct GBuffer
	{
		uint32_t width, height;
		FrameBufferAttachment position, normal, albedo;
		FrameBufferAttachment depth;
		RenderPass renderPass;
		GraphicsPipeline pipeline;
		PipelineLayout pipelineLayout;

		std::vector<FrameBuffer> frameBuffers;
		std::vector<CommandBuffer> drawBuffers;
		std::vector<Semaphore> semaphores;
	} gBuffer;

	std::vector<CommandBuffer> depthCopyCmd1;
	std::vector<Semaphore> depthCopySem1;
	std::vector<CommandBuffer> depthCopyCmd2;
	std::vector<Semaphore> depthCopySem2;


	struct FSQ
	{
		RenderPass renderPass;
		GraphicsPipeline pipeline;
		PipelineLayout pipelineLayout;
		FrameBufferAttachment depth;

		std::vector<FrameBuffer> frameBuffers;
		std::vector<CommandBuffer> drawBuffers;
		std::vector<Semaphore> semaphores;
		Mesh<TexVertex> mesh;
	} fsq;

	struct DebugDraw
	{
		RenderPass renderPass;
		GraphicsPipeline pipeline;
		PipelineLayout pipelineLayout;
		FrameBufferAttachment depth;

		std::vector<FrameBuffer> frameBuffers;
		std::vector<CommandBuffer> drawBuffers;
		std::vector<Semaphore> semaphores;
		Mesh<PosVertex> mesh;
		float debugLineLength = 1.0f;
		float debugLineWidth = 1.0f;
	} debugDraw;

	std::array<float, 4> clearColor = {0.0f, 0.0f, 0.0f, 0.0f};

	bool cursorActive = false;

	void SetInputCallbacks()
	{
		auto win = ((Window*)window.lock().get())->GetHandle();
		static auto cursorPosCallback = [](GLFWwindow* window, double xPos, double yPos)
		{
			auto context = reinterpret_cast<DeferredRenderingContext*>(glfwGetWindowUserPointer(window));
			context->camera.ProcessMouseInput({ xPos, yPos });
		};
		static auto keyCallback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			auto context = reinterpret_cast<DeferredRenderingContext*>(glfwGetWindowUserPointer(window));
			context->camera.ProcessKeyboardInput(key, action);
		};
		glfwSetCursorPosCallback(win, cursorPosCallback);
		glfwSetKeyCallback(win, keyCallback);
	}



	void Initialize(std::weak_ptr<Window> winHandle, bool enableOverlay) override
	{
		RenderingContext::Initialize(winHandle, enableOverlay);
		auto win = ((Window*)window.lock().get())->GetHandle();
		// glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetWindowUserPointer(win, this);

		SetInputCallbacks();
		
		camera.flipY = false;
		camera.SetPerspective(45.0f, (float)FB_SIZE.x / FB_SIZE.y, 0.1f, 100.0f);
		camera.SetPosition({ 12, -6.0f, -2.0f });
		camera.pitch = -6.0f;
		camera.yaw = -4.0f;
		// uboViewProjection.projection = glm::perspective(glm::radians(45.0f), (float)extent.width / extent.height, 0.1f, 100.0f);
		uboViewProjection.projection = camera.matrices.perspective;
    
		// // Invert up direction so that pos y is up 
		// // its down by default in Vulkan
		uboViewProjection.projection[1][1] *= -1;
		std::vector<PosVertex> dummy{ PosVertex(glm::vec3(1.0f)) };
		debugDraw.mesh.CreateDynamic(dummy, device);
		
    
		for (int i = 0; i < 20; ++i)
		{
			forwardObjects.emplace_back().CreateStatic(cubeVerts, cubeIndices, device);
			auto& obj = forwardObjects.back();

			obj.model = glm::scale(obj.model, glm::vec3(utils::Random() * 3.0f));
			obj.model = glm::rotate(obj.model, utils::Random() * 2.0f * glm::pi<float>(),
									glm::vec3(utils::Random(), utils::Random(), utils::Random()));
			obj.model = glm::translate(obj.model, glm::vec3(
				utils::Random(-10.0f, 10.0f), utils::Random(-10.0f, 10.0f), utils::Random(-10.0f, 10.0f)));
		}
		 //objects.emplace_back().Create(device, meshVerts, squareIndices);
		 fsq.mesh.CreateStatic(meshVerts, meshIndices, device);


		InitializeAttachments();
		InitializeAssets();
		objects.emplace_back().CreateModel(std::string(ASSET_DIR) + "Models/viking_room.obj", false, device);
		objects.back().SetModel(glm::scale(objects.back().model, glm::vec3(5.0f, 5.0f, 5.0f)));
		InitializeUniformBuffers();
		InitializeDescriptorSets();
		InitializePipelines();
	}

	void Destroy() override
	{
		device.waitIdle();

		descriptors.Destroy();

		gBuffer.albedo.Destroy();
		gBuffer.position.Destroy();
		gBuffer.normal.Destroy();
		gBuffer.depth.Destroy();

		utils::VectorDestroyer(gBuffer.frameBuffers);
		utils::VectorDestroyer(gBuffer.semaphores);
		
		gBuffer.pipeline.Destroy();
		gBuffer.pipelineLayout.Destroy();
		gBuffer.renderPass.Destroy();
		
		utils::VectorDestroyer(fsq.frameBuffers);
		utils::VectorDestroyer(fsq.semaphores);

		fsq.pipeline.Destroy();
		fsq.pipelineLayout.Destroy();
		fsq.renderPass.Destroy();
		fsq.mesh.Destroy();

		utils::VectorDestroyer(debugDraw.frameBuffers);
		utils::VectorDestroyer(debugDraw.semaphores);
		debugDraw.depth.Destroy();
		debugDraw.pipeline.Destroy();
		debugDraw.pipelineLayout.Destroy();
		debugDraw.renderPass.Destroy();
		debugDraw.mesh.Destroy();
				
		device.commandPool.FreeCommandBuffers(
			gBuffer.drawBuffers, fsq.drawBuffers, debugDraw.drawBuffers
		);

		utils::VectorDestroyer(uniformBufferViewProjection);
		utils::VectorDestroyer(uniformBufferComposition);
		utils::VectorDestroyer(objects);
		utils::VectorDestroyer(forwardObjects);
		
		RenderingContext::Destroy();
	}

	
	void InitializeAttachments()
	{
		const size_t imageViewCount = device.swapchain.imageViews.size();
		// ----------------------
		// Deferred Render Pass
		// ----------------------
		{
			gBuffer.width = FB_SIZE.x;
			gBuffer.height = FB_SIZE.y;

			vk::Format RGBA = vk::Format::eR16G16B16A16Sfloat;
			auto colorAttach = vk::ImageUsageFlagBits::eColorAttachment;
			auto sampled = vk::ImageUsageFlagBits::eSampled;

			// COLOR SAMPLER FOR DEFERRED ATTACHMENTS
			vk::SamplerCreateInfo samplerInfo = {};
			samplerInfo.magFilter = vk::Filter::eNearest;
			samplerInfo.minFilter = vk::Filter::eNearest;
			samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
			samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
			samplerInfo.addressModeV = samplerInfo.addressModeU;
			samplerInfo.addressModeW = samplerInfo.addressModeU;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.maxAnisotropy = 1.0f;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 1.0f;
			samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;

			gBuffer.position.Create(RGBA, { gBuffer.width, gBuffer.height },
									colorAttach | sampled,
									vk::ImageAspectFlagBits::eColor,
									vk::ImageLayout::eColorAttachmentOptimal,
									samplerInfo,
									device);

			gBuffer.normal.Create(RGBA, { gBuffer.width, gBuffer.height },
								  colorAttach | sampled,
								  vk::ImageAspectFlagBits::eColor,
								  vk::ImageLayout::eColorAttachmentOptimal,
								  samplerInfo,
								  device);

			gBuffer.albedo.Create(vk::Format::eR8G8B8A8Unorm, { gBuffer.width, gBuffer.height },
								  colorAttach | sampled,
								  vk::ImageAspectFlagBits::eColor,
								  vk::ImageLayout::eColorAttachmentOptimal,
								  samplerInfo,
								  device);

			gBuffer.depth.Create(FrameBufferAttachment::GetDepthFormat(), { gBuffer.width, gBuffer.height },
								 vk::ImageUsageFlagBits::eDepthStencilAttachment | sampled | 
								 vk::ImageUsageFlagBits::eTransferSrc,
								 vk::ImageAspectFlagBits::eDepth,
								 vk::ImageLayout::eDepthStencilAttachmentOptimal,
								 // No sampler
								 device);

			// Set up separate renderpass with references to the color and depth attachments
			std::array<vk::AttachmentDescription, 4> attachmentDescs = {};

			// Init attachment properties
			for (uint32_t i = 0; i < 4; ++i)
			{
				attachmentDescs[i].samples = vk::SampleCountFlagBits::e1;
				attachmentDescs[i].loadOp = vk::AttachmentLoadOp::eClear;
				attachmentDescs[i].storeOp = vk::AttachmentStoreOp::eStore;
				attachmentDescs[i].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
				attachmentDescs[i].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
				if (i == 3)
				{
					//attachmentDescs[i].initialLayout = vk::ImageLayout::eTransferSrcOptimal;
					attachmentDescs[i].initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
					attachmentDescs[i].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				}
				else
				{
					attachmentDescs[i].initialLayout = vk::ImageLayout::eUndefined;
					attachmentDescs[i].finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				}
			}

			attachmentDescs[0].format = gBuffer.position.format;
			attachmentDescs[1].format = gBuffer.normal.format;
			attachmentDescs[2].format = gBuffer.albedo.format;
			attachmentDescs[3].format = gBuffer.depth.format;

			std::array<vk::AttachmentReference, 3> colorRefs =
			{
				vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal),
				vk::AttachmentReference(1, vk::ImageLayout::eColorAttachmentOptimal),
				{ 2, vk::ImageLayout::eColorAttachmentOptimal },

			};

			vk::AttachmentReference depthRef;
			depthRef.attachment = 3;
			depthRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

			vk::SubpassDescription subpass = {};
			subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
			subpass.pColorAttachments = colorRefs.data();
			subpass.colorAttachmentCount = colorRefs.size();
			subpass.pDepthStencilAttachment = &depthRef;

			std::array<vk::SubpassDependency, 2> dependencies;

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
			dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
			dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
			dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
			dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
			dependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
			dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

			vk::RenderPassCreateInfo rpInfo = {};
			rpInfo.pAttachments = attachmentDescs.data();
			rpInfo.attachmentCount = attachmentDescs.size();
			rpInfo.subpassCount = 1;
			rpInfo.pSubpasses = &subpass;
			rpInfo.dependencyCount = 2;
			rpInfo.pDependencies = dependencies.data();

			std::vector<vk::ClearValue> clearValues(4);
			clearValues[0].color = vk::ClearColorValue(clearColor);
			clearValues[1].color = vk::ClearColorValue(clearColor);
			clearValues[2].color = vk::ClearColorValue(clearColor);
			clearValues[3].depthStencil = vk::ClearDepthStencilValue(1.0f);
			
			gBuffer.renderPass.Create(rpInfo, device, vk::Extent2D(FB_SIZE.x, FB_SIZE.y), clearValues);
		}
		// Frame Buffers
		{
			std::array<vk::ImageView, 4> attachments;
			attachments[0] = gBuffer.position.imageView.Get();
			attachments[1] = gBuffer.normal.imageView.Get();
			attachments[2] = gBuffer.albedo.imageView.Get();
			attachments[3] = gBuffer.depth.imageView.Get();

			vk::FramebufferCreateInfo fbInfo = {};
			fbInfo.renderPass = gBuffer.renderPass;
			fbInfo.attachmentCount = attachments.size();
			fbInfo.pAttachments = attachments.data();
			fbInfo.width = gBuffer.width;
			fbInfo.height = gBuffer.height;
			fbInfo.layers = 1;

			gBuffer.frameBuffers.resize(imageViewCount);

			for (size_t i = 0; i < imageViewCount; ++i)
				FrameBuffer::Create(gBuffer.frameBuffers[i], fbInfo, device);
		}
		// Command Buffers
		{
			gBuffer.drawBuffers.resize(imageViewCount);
			depthCopyCmd1.resize(imageViewCount);
			depthCopyCmd2.resize(imageViewCount);

			vk::CommandBufferAllocateInfo cmdInfo = {};
			cmdInfo.commandPool = device.commandPool.Get();
			cmdInfo.level = vk::CommandBufferLevel::ePrimary;
			cmdInfo.commandBufferCount = static_cast<uint32_t>(imageViewCount);

			utils::CheckVkResult(
				device.allocateCommandBuffers(&cmdInfo, gBuffer.drawBuffers.data()),
				"Failed to allocate deferred command buffers"
			);

			utils::CheckVkResult(
				device.allocateCommandBuffers(&cmdInfo, depthCopyCmd1.data()),
				"Failed to allocate deferred command buffers"
			);
			utils::CheckVkResult(
				device.allocateCommandBuffers(&cmdInfo, depthCopyCmd2.data()),
				"Failed to allocate deferred command buffers"
			);
		}
		// Semaphores
		{
			gBuffer.semaphores.resize(MAX_FRAME_DRAWS);
			depthCopySem1.resize(MAX_FRAME_DRAWS);
			depthCopySem2.resize(MAX_FRAME_DRAWS);
			vk::SemaphoreCreateInfo semInfo = {};
			for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
			{
				Semaphore::Create(gBuffer.semaphores[i], semInfo, device);
				Semaphore::Create(depthCopySem1[i], semInfo, device);
				Semaphore::Create(depthCopySem2[i], semInfo, device);
			}
		}


		// ----------------------
		// FSQ Render Pass
		// ----------------------
		{
			// Render pass & subpass
			{

				// ATTACHMENTS
				vk::AttachmentDescription colorAttachment;
				colorAttachment.format = device.swapchain.imageFormat;
				colorAttachment.samples = vk::SampleCountFlagBits::e1;					// Number of samples to write for multisampling
				colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;				// Describes what to do with attachment before rendering
				colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;			// Describes what to do with attachment after rendering
				colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;	// Describes what to do with stencil before rendering
				colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;	// Describes what to do with stencil after rendering
				colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
				colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

				fsq.depth.Create(FrameBufferAttachment::GetDepthFormat(), { gBuffer.width, gBuffer.height },
								 vk::ImageUsageFlagBits::eDepthStencilAttachment,
								 vk::ImageAspectFlagBits::eDepth,
								 vk::ImageLayout::eDepthStencilAttachmentOptimal,
								 // No sampler
								 device);

				vk::AttachmentDescription depthAttachment;
				depthAttachment.format = FrameBufferAttachment::GetDepthFormat();
				depthAttachment.samples = vk::SampleCountFlagBits::e1;			
				depthAttachment.loadOp = vk::AttachmentLoadOp::eLoad;			
				depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;		
				depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
				depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;	
				depthAttachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;		

				std::array<vk::AttachmentDescription, 2> attachmentsDescFSQ = { colorAttachment, depthAttachment };

				// REFERENCES
				// Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
				vk::AttachmentReference colorAttachmentRef;
				colorAttachmentRef.attachment = 0;
				colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

				vk::AttachmentReference depthAttachmentRef;
				depthAttachmentRef.attachment = 1;
				depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

				vk::SubpassDescription subpass = {};
				subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
				subpass.colorAttachmentCount = 1;
				subpass.pColorAttachments = &colorAttachmentRef;
				subpass.pDepthStencilAttachment = &depthAttachmentRef;


				// ---------------------------
				// SUBPASS for converting between IMAGE_LAYOUT_UNDEFINED to IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				vk::SubpassDependency dependency = {};

				// Indices of the dependency for this subpass and the dependent subpass
				// VK_SUBPASS_EXTERNAL refers to the implicit subpass before/after this pass
				// Happens after
				dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
				// Pipeline Stage
				dependency.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
				// Stage access mask 
				dependency.srcAccessMask = vk::AccessFlagBits::eMemoryRead;

				// Happens before 
				dependency.dstSubpass = 0;
				dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
				dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;

				vk::RenderPassCreateInfo createInfo;
				createInfo.attachmentCount = static_cast<uint32_t>(attachmentsDescFSQ.size());
				createInfo.pAttachments = attachmentsDescFSQ.data();
				createInfo.subpassCount = 1;
				createInfo.pSubpasses = &subpass;
				createInfo.dependencyCount = 1;
				createInfo.pDependencies = &dependency;

				std::vector<vk::ClearValue> clearValues(2);
				clearValues[0].color = vk::ClearColorValue(clearColor);
				clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f);

				fsq.renderPass.Create(createInfo, device, device.swapchain.extent, clearValues);
			}

			// Frame buffers
			{
				const auto& imageViews = device.swapchain.imageViews;
				const auto& extent = device.swapchain.extent;
				size_t imageViewsSize = imageViews.size();
				fsq.frameBuffers.resize(imageViewsSize);

				vk::FramebufferCreateInfo createInfo;
				// FB width/height
				createInfo.width = extent.width;
				createInfo.height = extent.height;
				// FB layers
				createInfo.layers = 1;
				createInfo.renderPass = fsq.renderPass;
				createInfo.attachmentCount = 2;

				for (size_t i = 0; i < imageViewsSize; ++i)
				{
					std::array<vk::ImageView, 2> attachments = {
						imageViews[i].Get(),
						fsq.depth.imageView
					};

					// List of attachments 1 to 1 with render pass
					createInfo.pAttachments = attachments.data();

					FrameBuffer::Create(&fsq.frameBuffers[i], createInfo, device);
				}
			}

			// Command buffers
			{
				fsq.drawBuffers.resize(imageViewCount);

				vk::CommandBufferAllocateInfo cmdInfo = {};
				cmdInfo.commandPool = device.commandPool.Get();
				cmdInfo.level = vk::CommandBufferLevel::ePrimary;
				cmdInfo.commandBufferCount = static_cast<uint32_t>(imageViewCount);

				utils::CheckVkResult(
					device.allocateCommandBuffers(&cmdInfo, fsq.drawBuffers.data()),
					"Failed to allocate deferred command buffers"
				);
			}

			// Semaphores
			fsq.semaphores.resize(MAX_FRAME_DRAWS);
			vk::SemaphoreCreateInfo semInfo = {};
			for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
			{
				Semaphore::Create(fsq.semaphores[i], semInfo, device);
			}
		}

		// ----------------------
		// Debug Draw Render Pass
		// ----------------------
		{
			// Render pass & subpass
			{
				debugDraw.depth.Create(FrameBufferAttachment::GetDepthFormat(), device.swapchain.extent,
						 vk::ImageUsageFlagBits::eDepthStencilAttachment |
						 vk::ImageUsageFlagBits::eTransferDst,
						 vk::ImageAspectFlagBits::eDepth,
						 vk::ImageLayout::eDepthStencilAttachmentOptimal,
						 // No sampler
						 device);

				// ATTACHMENTS
				vk::AttachmentDescription colorAttachment;
				colorAttachment.format = device.swapchain.imageFormat;
				colorAttachment.samples = vk::SampleCountFlagBits::e1;					// Number of samples to write for multisampling
				colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;				// Describes what to do with attachment before rendering
				colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;			// Describes what to do with attachment after rendering
				colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;	// Describes what to do with stencil before rendering
				colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;	// Describes what to do with stencil after rendering

				// Framebuffer data will be stored as an image, but images can be given different data layouts
				// to give optimal use for certain operations
				colorAttachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;			// Image data layout before render pass starts
				colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;		// Image data layout after render pass (to change to)

				vk::AttachmentDescription depthAttachment = {};
				depthAttachment.format = debugDraw.depth.format;
				depthAttachment.samples = vk::SampleCountFlagBits::e1;
				depthAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
				depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
				depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;	
				depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;	
				depthAttachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

				std::array<vk::AttachmentDescription, 2> attachmentsDescFSQ = { colorAttachment, depthAttachment };

				// REFERENCES
				// Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
				vk::AttachmentReference colorAttachmentRef;
				colorAttachmentRef.attachment = 0;
				colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

				vk::AttachmentReference depthAttachmentRef;
				depthAttachmentRef.attachment = 1;
				depthAttachmentRef.layout = depthAttachment.finalLayout;

				vk::SubpassDescription subpass = {};
				subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
				subpass.colorAttachmentCount = 1;
				subpass.pColorAttachments = &colorAttachmentRef;
				subpass.pDepthStencilAttachment = &depthAttachmentRef;


				// ---------------------------
				// SUBPASS for converting between IMAGE_LAYOUT_UNDEFINED to IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				vk::SubpassDependency dependency = {};

				// Indices of the dependency for this subpass and the dependent subpass
				// VK_SUBPASS_EXTERNAL refers to the implicit subpass before/after this pass
				// Happens after
				dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
				// Pipeline Stage
				dependency.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
				// Stage access mask 
				dependency.srcAccessMask = vk::AccessFlagBits::eMemoryRead;

				// Happens before 
				dependency.dstSubpass = 0;
				dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
				dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;

				vk::RenderPassCreateInfo createInfo;
				createInfo.attachmentCount = static_cast<uint32_t>(attachmentsDescFSQ.size());
				createInfo.pAttachments = attachmentsDescFSQ.data();
				createInfo.subpassCount = 1;
				createInfo.pSubpasses = &subpass;
				createInfo.dependencyCount = 1;
				createInfo.pDependencies = &dependency;


				std::vector<vk::ClearValue> clearValues(2);
				clearValues[0].color = vk::ClearColorValue(clearColor);
				clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f);

				const auto& extent = device.swapchain.extent;

				debugDraw.renderPass.Create(createInfo, device, extent, clearValues);
			}

			// Frame buffers
			{
				const auto& imageViews = device.swapchain.imageViews;
				const auto& extent = device.swapchain.extent;
				size_t imageViewsSize = imageViews.size();
				debugDraw.frameBuffers.resize(imageViewsSize);

				vk::FramebufferCreateInfo createInfo;
				// FB width/height
				createInfo.width = extent.width;
				createInfo.height = extent.height;
				// FB layers
				createInfo.layers = 1;
				createInfo.renderPass = debugDraw.renderPass;
				createInfo.attachmentCount = 2;

				for (size_t i = 0; i < imageViewsSize; ++i)
				{
					std::array<vk::ImageView, 2> attachments = {
						imageViews[i].Get(),
						debugDraw.depth.imageView
					};

					// List of attachments 1 to 1 with render pass
					createInfo.pAttachments = attachments.data();

					FrameBuffer::Create(&debugDraw.frameBuffers[i], createInfo, device);
				}
			}

			// Command buffers
			{
				debugDraw.drawBuffers.resize(imageViewCount);

				vk::CommandBufferAllocateInfo cmdInfo = {};
				cmdInfo.commandPool = device.commandPool.Get();
				cmdInfo.level = vk::CommandBufferLevel::ePrimary;
				cmdInfo.commandBufferCount = static_cast<uint32_t>(imageViewCount);

				utils::CheckVkResult(
					device.allocateCommandBuffers(&cmdInfo, debugDraw.drawBuffers.data()),
					"Failed to allocate deferred command buffers"
				);
			}

			// Semaphores
			debugDraw.semaphores.resize(MAX_FRAME_DRAWS);
			vk::SemaphoreCreateInfo semInfo = {};
			for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
			{
				Semaphore::Create(debugDraw.semaphores[i], semInfo, device);
			}
		}

	}


	void InitializeAssets()
	{
		std::vector<std::vector<Mesh<Vertex>::MeshData>> meshData(20);

		auto loadSection = [&meshData](const std::string& sectionPath, std::vector<Mesh<Vertex>>& objects,
									   Device& device, int threadID)
		{
			std::ifstream sectionFile;
			sectionFile.open(sectionPath);
			std::string modelsPath = std::string(ASSET_DIR) + "Models/";
			std::string input;
			std::vector<Mesh<Vertex>::MeshData> section;
			while (sectionFile >> input)
			{
				std::string combinedPath = modelsPath + input;
				section.emplace_back(Mesh<Vertex>::LoadModel(combinedPath));
				input.clear();
			}
			meshData[threadID] = std::move(section);
		};

		std::vector<std::thread> loadGroup = {};
		for (int i = 1; i < 21; ++i)
		{
			loadGroup.emplace_back(loadSection,
								   std::string(ASSET_DIR) + "Models/Section" + std::to_string(i) + ".txt",
								   objects, device, i - 1);
		}

		for (auto& loader : loadGroup)
		{
			loader.join();
		}

		for (auto& vector : meshData)
		{
			for (auto& data : vector)
			{
				objects.emplace_back();
				objects.back().CreateStatic(data.vertices, data.indices, device);
			}
		}
	}

	void InitializeUniformBuffers()
	{
		vk::DeviceSize vpBufferSize = sizeof(UboViewProjection);

		const auto& images = device.swapchain.images;
		// One uniform buffer for each image
		uniformBufferViewProjection.resize(images.size());
		uniformBufferComposition.resize(images.size());

		vk::BufferCreateInfo vpCreateInfo = {};
		vpCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		vpCreateInfo.size = vpBufferSize;


		vk::BufferCreateInfo compCreateInfo = {};
		compCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		compCreateInfo.size = sizeof(UboComposition);


		VmaAllocationCreateInfo aCreateInfo = {};
		aCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		for (size_t i = 0; i < images.size(); ++i)
		{
			uniformBufferViewProjection[i].Create(vpCreateInfo, aCreateInfo, device);
			uniformBufferComposition[i].Create(compCreateInfo, aCreateInfo, device);
		}

		UpdateUniformBuffers(0.0f, device.imageIndex);
	}

	void InitializeDescriptorSets()
	{
		auto vpDescInfos = Buffer::AggregateDescriptorInfo(uniformBufferViewProjection);
		auto compositionDescInfos = Buffer::AggregateDescriptorInfo(uniformBufferComposition);
		std::array<DescriptorInfo, 5> descriptorInfos =
		{
			DescriptorInfo::CreateAsync(0,
				vk::ShaderStageFlagBits::eVertex,
				vk::DescriptorType::eUniformBuffer,
				vpDescInfos),
			DescriptorInfo::Create(1,
				vk::ShaderStageFlagBits::eFragment,
				vk::DescriptorType::eCombinedImageSampler,
				gBuffer.position.GetDescriptor(vk::ImageLayout::eShaderReadOnlyOptimal)),
			DescriptorInfo::Create(2,
				vk::ShaderStageFlagBits::eFragment,
				vk::DescriptorType::eCombinedImageSampler,
				gBuffer.normal.GetDescriptor(vk::ImageLayout::eShaderReadOnlyOptimal)),
			DescriptorInfo::Create(3,
				vk::ShaderStageFlagBits::eFragment,
				vk::DescriptorType::eCombinedImageSampler,
				gBuffer.albedo.GetDescriptor(vk::ImageLayout::eShaderReadOnlyOptimal)),
			DescriptorInfo::CreateAsync(4,
				vk::ShaderStageFlagBits::eFragment,
				vk::DescriptorType::eUniformBuffer,
				compositionDescInfos),
		};

		descriptors.Create(
			descriptorInfos,
			device.swapchain.images.size(),
			device
		);

		// PUSH CONSTANTS
		// NO create needed
		// Where push constant is located
		pushRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
		pushRange.offset = 0;
		pushRange.size = sizeof(glm::mat4);
	}

	void InitializePipelines()
	{
		// ----------------------
		// DEFERRED PIPELINE
		// ----------------------
		ShaderModule vertModule;
		auto vertInfo = ShaderModule::Load(
			vertModule,
			"fillBuffersVert.spv",
			vk::ShaderStageFlagBits::eVertex,
			device
		);
		ShaderModule fragModule;
		auto fragInfo = ShaderModule::Load(
			fragModule,
			"fillBuffersFrag.spv",
			vk::ShaderStageFlagBits::eFragment,
			device
		);

		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertInfo, fragInfo };

		// VERTEX BINDING DATA
		vk::VertexInputBindingDescription bindDesc = {};
		// Binding position (can bind multiple streams)
		bindDesc.binding = 0;
		bindDesc.stride = sizeof(Vertex);
		// Instancing option, draw one object at a time in this case
		bindDesc.inputRate = vk::VertexInputRate::eVertex;

		// VERTEX ATTRIB DATA
		std::array<vk::VertexInputAttributeDescription, Vertex::NUM_ATTRIBS> attribDesc;
		// Position attribute
		// Binding the data is at (should be same as above)
		attribDesc[0].binding = 0;
		// Location in shader where data is read from
		attribDesc[0].location = 0;
		// Format for the data being sent
		attribDesc[0].format = vk::Format::eR32G32B32Sfloat;
		// Where the attribute begins as an offset from the beginning
		// of the structures
		attribDesc[0].offset = offsetof(Vertex, pos);
		// Normal attribute
		attribDesc[1].binding = 0;
		attribDesc[1].location = 1;
		attribDesc[1].format = vk::Format::eR32G32B32Sfloat;
		attribDesc[1].offset = offsetof(Vertex, normal);
		// Color attribute
		attribDesc[2].binding = 0;
		attribDesc[2].location = 2;
		attribDesc[2].format = vk::Format::eR32G32B32Sfloat;
		attribDesc[2].offset = offsetof(Vertex, color);
		// Texture coord
		attribDesc[3].binding = 0;
		attribDesc[3].location = 3;
		attribDesc[3].format = vk::Format::eR32G32Sfloat;
		attribDesc[3].offset = offsetof(Vertex, texPos);


		// VERTEX INPUT
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindDesc;
		// Data spacing / stride info
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDesc.size());
		vertexInputInfo.pVertexAttributeDescriptions = attribDesc.data();

		// INPUT ASSEMBLY
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// VIEWPORT

		vk::Extent2D extent = { gBuffer.width, gBuffer.height };

		vk::Viewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)extent.width;
		viewport.height = (float)extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vk::Rect2D scissor;
		scissor.extent = extent;
		scissor.offset = vk::Offset2D(0, 0);

		vk::PipelineViewportStateCreateInfo viewportState = {};
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		vk::PipelineRasterizationStateCreateInfo rasterizeState = {};
		rasterizeState.depthClampEnable = VK_FALSE;			// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
		rasterizeState.rasterizerDiscardEnable = VK_FALSE;	// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
		rasterizeState.polygonMode = vk::PolygonMode::eFill;	// How to handle filling points between vertices
		rasterizeState.lineWidth = 1.0f;						// How thick lines should be when drawn
		rasterizeState.cullMode = vk::CullModeFlagBits::eNone;		// Which face of a triangle to cull
		rasterizeState.frontFace = vk::FrontFace::eCounterClockwise;	// Winding to determine which side is front
		rasterizeState.depthBiasEnable = VK_FALSE;			// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)

		vk::PipelineMultisampleStateCreateInfo multisampleState = {};
		// Whether or not to multi-sample
		multisampleState.sampleShadingEnable = VK_FALSE;
		// Number of samples per fragment
		multisampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;

		vk::PipelineColorBlendAttachmentState blendState = {};
		blendState.colorWriteMask = vk::ColorComponentFlags(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
		blendState.blendEnable = VK_FALSE;
		std::array<vk::PipelineColorBlendAttachmentState, 3> blendAttachmentStates = {
			blendState, blendState, blendState
		};
		vk::PipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.logicOpEnable = VK_FALSE;
		colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
		colorBlendState.pAttachments = blendAttachmentStates.data();

		vk::PipelineLayoutCreateInfo layout = {};
		layout.setLayoutCount = 1;
		layout.pSetLayouts = &descriptors.layout;
		layout.pushConstantRangeCount = 1;
		layout.pPushConstantRanges = &pushRange;

		PipelineLayout::Create(gBuffer.pipelineLayout, layout, device);

		// Depth testing
		vk::PipelineDepthStencilStateCreateInfo depthStencilState = {};
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = vk::CompareOp::eLess;
		// if this was true, you can fill out min/max bounds in this createinfo
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.stencilTestEnable = VK_FALSE;


		vk::GraphicsPipelineCreateInfo pipelineInfo;
		pipelineInfo.stageCount = 2;									// Number of shader stages
		pipelineInfo.pStages = shaderStages;							// List of shader stages
		pipelineInfo.pVertexInputState = &vertexInputInfo;		// All the fixed function pipeline states
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.pRasterizationState = &rasterizeState;
		pipelineInfo.pMultisampleState = &multisampleState;
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDepthStencilState = &depthStencilState;
		pipelineInfo.layout = (gBuffer.pipelineLayout).Get();							// Pipeline Layout pipeline should use
		pipelineInfo.renderPass = gBuffer.renderPass;							// Render pass description the pipeline is compatible with
		pipelineInfo.subpass = 0;										// Subpass of render pass to use with pipeline

		GraphicsPipeline::Create(gBuffer.pipeline, pipelineInfo, device, vk::PipelineCache(), 1);
		vertModule.Destroy();
		fragModule.Destroy();

		// ----------------------
		// FSQ PIPELINE
		// ----------------------
		vertInfo = ShaderModule::Load(
			vertModule,
			"fsqVert.spv",
			vk::ShaderStageFlagBits::eVertex,
			device
		);
		fragInfo = ShaderModule::Load(
			fragModule,
			"fsqFrag.spv",
			vk::ShaderStageFlagBits::eFragment,
			device
		);

		shaderStages[0] = vertInfo;
		shaderStages[1] = fragInfo;

		pipelineInfo.pStages = shaderStages;
		pipelineInfo.renderPass = fsq.renderPass;

		PipelineLayout::Create(fsq.pipelineLayout, layout, device);
		pipelineInfo.layout = fsq.pipelineLayout.Get();

		// Depth testing
		depthStencilState = vk::PipelineDepthStencilStateCreateInfo();
		depthStencilState.depthTestEnable = VK_FALSE;
		depthStencilState.depthWriteEnable = VK_FALSE;
		depthStencilState.depthCompareOp = vk::CompareOp::eLess;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.stencilTestEnable = VK_FALSE;


		// VERTEX BINDING DATA
		vk::VertexInputBindingDescription bindDescFSQ = vk::VertexInputBindingDescription();
		// Binding position (can bind multiple streams)
		bindDescFSQ.binding = 0;
		bindDescFSQ.stride = sizeof(TexVertex);
		// Instancing option, draw one object at a time in this case
		bindDescFSQ.inputRate = vk::VertexInputRate::eVertex;

		// VERTEX ATTRIB DATA
		std::array<vk::VertexInputAttributeDescription, TexVertex::NUM_ATTRIBS> attribDescFSQ;
		// Position attribute
		// Binding the data is at (should be same as above)
		attribDescFSQ[0].binding = 0;
		// Location in shader where data is read from
		attribDescFSQ[0].location = 0;
		// Format for the data being sent
		attribDescFSQ[0].format = vk::Format::eR32G32B32Sfloat;
		// Where the attribute begins as an offset from the beginning
		// of the structures
		attribDescFSQ[0].offset = offsetof(TexVertex, pos);
		// Normal attribute
		attribDescFSQ[1].binding = 0;
		attribDescFSQ[1].location = 1;
		attribDescFSQ[1].format = vk::Format::eR32G32Sfloat;
		attribDescFSQ[1].offset = offsetof(TexVertex, texPos);

		// VERTEX INPUT
		auto vertexInputInfoFSQ = vk::PipelineVertexInputStateCreateInfo();
		vertexInputInfoFSQ.vertexBindingDescriptionCount = 1;
		vertexInputInfoFSQ.pVertexBindingDescriptions = &bindDescFSQ;
		// Data spacingFSQ / stride info
		vertexInputInfoFSQ.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDescFSQ.size());
		vertexInputInfoFSQ.pVertexAttributeDescriptions = attribDescFSQ.data();

		// Blending
		vk::PipelineColorBlendAttachmentState colorBlendAttachments = {};
		colorBlendAttachments.colorWriteMask = vk::ColorComponentFlags(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
		colorBlendAttachments.blendEnable = VK_TRUE;

		// Blending uses equation: (srcColorBlendFactor * new colour) colorBlendOp (dstColorBlendFactor * old colour)
		colorBlendAttachments.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		colorBlendAttachments.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		colorBlendAttachments.colorBlendOp = vk::BlendOp::eAdd;

		// Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
		//			   (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)
		colorBlendAttachments.srcAlphaBlendFactor = vk::BlendFactor::eOne;
		colorBlendAttachments.dstAlphaBlendFactor = vk::BlendFactor::eZero;
		colorBlendAttachments.alphaBlendOp = vk::BlendOp::eAdd;

		vk::PipelineColorBlendStateCreateInfo colorBlendStateFSQ = {};
		colorBlendStateFSQ.logicOpEnable = VK_FALSE;
		colorBlendStateFSQ.attachmentCount = 1;
		colorBlendStateFSQ.pAttachments = &colorBlendAttachments;

		pipelineInfo.pVertexInputState = &vertexInputInfoFSQ;
		pipelineInfo.pColorBlendState = &colorBlendStateFSQ;
		pipelineInfo.pDepthStencilState = &depthStencilState;

		viewport.width = (float)device.swapchain.extent.width;
		viewport.height = (float)device.swapchain.extent.height;
		viewportState.pViewports = &viewport;

		pipelineInfo.pViewportState = &viewportState;

		GraphicsPipeline::Create(fsq.pipeline, pipelineInfo, device, vk::PipelineCache(), 1);
		vertModule.Destroy();
		fragModule.Destroy();

		// ----------------------
		// FORWARD PIPELINE
		// ----------------------
		vertInfo = ShaderModule::Load(
			vertModule,
			"forwardVert.spv",
			vk::ShaderStageFlagBits::eVertex,
			device
		);
		fragInfo = ShaderModule::Load(
			fragModule,
			"forwardFrag.spv",
			vk::ShaderStageFlagBits::eFragment,
			device
		);

		// Depth testing
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;

		shaderStages[0] = vertInfo;
		shaderStages[1] = fragInfo;

		pipelineInfo.pStages = shaderStages;
		pipelineInfo.renderPass = device.renderPass;

		PipelineLayout::Create(device.pipelineLayout, layout, device);
		pipelineInfo.layout = device.pipelineLayout.Get();

		// Reuse vertex input from deferred
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		// Reuse blending from FSQ
		pipelineInfo.pColorBlendState = &colorBlendStateFSQ;
		// Reuse depth state from deferred
		pipelineInfo.pDepthStencilState = &depthStencilState;

		viewport.width = (float)device.swapchain.extent.width;
		viewport.height = (float)device.swapchain.extent.height;
		viewportState.pViewports = &viewport;
		pipelineInfo.pViewportState = &viewportState;

		GraphicsPipeline::Create(device.pipeline, pipelineInfo, device, vk::PipelineCache(), 1);
		vertModule.Destroy();
		fragModule.Destroy();

		// ----------------------
		// DEBUG DRAW PIPELINE
		// ----------------------

		// reuse basically everything from forward
		vertInfo = ShaderModule::Load(
			vertModule,
			"debugDrawVert.spv",
			vk::ShaderStageFlagBits::eVertex,
			device
		);
		fragInfo = ShaderModule::Load(
			fragModule,
			"debugDrawFrag.spv",
			vk::ShaderStageFlagBits::eFragment,
			device
		);

		shaderStages[0] = vertInfo;
		shaderStages[1] = fragInfo;

		pipelineInfo.pStages = shaderStages;
		pipelineInfo.renderPass = debugDraw.renderPass;
		inputAssembly.topology = vk::PrimitiveTopology::eLineList;
		rasterizeState.lineWidth = debugDraw.debugLineWidth;

		// VERTEX BINDING DATA
		auto bindDescDebug = vk::VertexInputBindingDescription();
		bindDescDebug.binding = 0;
		bindDescDebug.stride = sizeof(PosVertex);
		bindDescDebug.inputRate = vk::VertexInputRate::eVertex;

		// VERTEX ATTRIB DATA
		std::array<vk::VertexInputAttributeDescription, PosVertex::NUM_ATTRIBS> attribDescDebug;
		attribDescDebug[0].binding = 0;
		attribDescDebug[0].location = 0;
		attribDescDebug[0].format = vk::Format::eR32G32B32Sfloat;
		attribDescDebug[0].offset = offsetof(PosVertex, pos);

		// VERTEX INPUT
		auto vertexInputInfoDebug = vk::PipelineVertexInputStateCreateInfo();
		vertexInputInfoDebug.vertexBindingDescriptionCount = 1;
		vertexInputInfoDebug.pVertexBindingDescriptions = &bindDescDebug;
		// Data spacingDebug / stride info
		vertexInputInfoDebug.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDescDebug.size());
		vertexInputInfoDebug.pVertexAttributeDescriptions = attribDescDebug.data();

		pipelineInfo.pVertexInputState = &vertexInputInfoDebug;
		

		PipelineLayout::Create(debugDraw.pipelineLayout, layout, device);
		pipelineInfo.layout = debugDraw.pipelineLayout.Get();

		GraphicsPipeline::Create(debugDraw.pipeline, pipelineInfo, device, vk::PipelineCache(), 1);
		vertModule.Destroy();
		fragModule.Destroy();
	}

	void RecordDeferred(uint32_t imageIndex)
	{
		auto& cmdBuf = gBuffer.drawBuffers[imageIndex];
		cmdBuf.Begin();

		gBuffer.renderPass.Begin(
			cmdBuf,
			gBuffer.frameBuffers[imageIndex].Get(),
			gBuffer.pipeline.Get()
		);
		gBuffer.renderPass.RenderObjects(
			cmdBuf,
			descriptors.sets[imageIndex],
			gBuffer.pipelineLayout.Get(),
			objects.data(),
			objects.size()
		);

		gBuffer.renderPass.End(cmdBuf);

		cmdBuf.End();

	}

	void RecordFSQ(uint32_t imageIndex)
	{
		auto& cmdBuf = fsq.drawBuffers[imageIndex];
		cmdBuf.Begin();

		fsq.renderPass.Begin(
			cmdBuf,
			fsq.frameBuffers[imageIndex].Get(),
			fsq.pipeline.Get()
		);

		fsq.renderPass.RenderObjects(
			cmdBuf,
			descriptors.sets[imageIndex],
			fsq.pipelineLayout.Get(),
			&fsq.mesh
		);

		cmdBuf.endRenderPass();
		cmdBuf.end();
	}

	void RecordDepthCopyForward(uint32_t imageIndex)
	{
		auto& cmdBuf = depthCopyCmd1[imageIndex];
		cmdBuf.Begin();

		// Transition to read from gBuffer
		gBuffer.depth.image.TransitionLayout(
			cmdBuf,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageLayout::eTransferSrcOptimal,
			vk::ImageAspectFlagBits::eDepth
		);

		// Transition to read from gBuffer
		device.depth.image.TransitionLayout(
			cmdBuf,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageAspectFlagBits::eDepth
		);


		vk::ImageSubresourceRange range = vk::ImageSubresourceRange(
			 vk::ImageAspectFlagBits::eDepth,
			0, 1, 0, 1
		);
		cmdBuf.clearDepthStencilImage(
			device.depth.image.Get(),
			vk::ImageLayout::eTransferDstOptimal,
			vk::ClearDepthStencilValue(1.0f),
			range
		);

		if (copyDepth)
		{
			vk::ImageSubresourceLayers imageSubresource = vk::ImageSubresourceLayers(
				vk::ImageAspectFlagBits::eDepth,
				0, 0, 1
			);

			vk::ImageCopy imageCopy = vk::ImageCopy(
				imageSubresource,
				{},
				imageSubresource,
				{},
				{ device.swapchain.extent.width, device.swapchain.extent.height, 1 }
			);
			cmdBuf.copyImage(
				gBuffer.depth.image.Get(), vk::ImageLayout::eTransferSrcOptimal,
				device.depth.image.Get(), vk::ImageLayout::eTransferDstOptimal,
				imageCopy);
		}

		device.depth.image.TransitionLayout(
			cmdBuf,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageAspectFlagBits::eDepth
		);

		gBuffer.depth.image.TransitionLayout(
			cmdBuf,
			vk::ImageLayout::eTransferSrcOptimal,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageAspectFlagBits::eDepth
		);

		cmdBuf.End();
	}

	void RecordForward(uint32_t imageIndex)
	{
		auto& cmdBuf = device.drawBuffers[imageIndex];
		cmdBuf.Begin();

		device.renderPass.Begin(
			cmdBuf,
			device.frameBuffers[imageIndex].Get(),
			device.pipeline
		);

		device.renderPass.RenderObjects(
			cmdBuf,
			descriptors.sets[imageIndex],
			device.pipelineLayout,
			forwardObjects.data(),
			forwardObjects.size()
		);
		cmdBuf.endRenderPass();
		cmdBuf.end();
	}


	void RecordDepthCopyDebug(uint32_t imageIndex)
	{
		auto& cmdBuf = depthCopyCmd2[imageIndex];
		cmdBuf.Begin();

		device.depth.image.TransitionLayout(
			cmdBuf,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageLayout::eTransferSrcOptimal,
			vk::ImageAspectFlagBits::eDepth
		);

		debugDraw.depth.image.TransitionLayout(
			cmdBuf,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageAspectFlagBits::eDepth
		);


		vk::ImageSubresourceRange range = vk::ImageSubresourceRange(
			 vk::ImageAspectFlagBits::eDepth,
			0, 1, 0, 1
		);
		cmdBuf.clearDepthStencilImage(
			debugDraw.depth.image.Get(),
			vk::ImageLayout::eTransferDstOptimal,
			vk::ClearDepthStencilValue(1.0f),
			range
		);

		if (copyDepth)
		{
			vk::ImageSubresourceLayers imageSubresource = vk::ImageSubresourceLayers(
				vk::ImageAspectFlagBits::eDepth,
				0, 0, 1
			);

			vk::ImageCopy imageCopy = vk::ImageCopy(
				imageSubresource,
				{},
				imageSubresource,
				{},
				{ device.swapchain.extent.width, device.swapchain.extent.height, 1 }
			);
			cmdBuf.copyImage(
				device.depth.image.Get(), vk::ImageLayout::eTransferSrcOptimal,
				debugDraw.depth.image.Get(), vk::ImageLayout::eTransferDstOptimal,
				imageCopy);
		}

		debugDraw.depth.image.TransitionLayout(
			cmdBuf,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageAspectFlagBits::eDepth
		);

		device.depth.image.TransitionLayout(
			cmdBuf,
			vk::ImageLayout::eTransferSrcOptimal,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageAspectFlagBits::eDepth
		);
		
		cmdBuf.End();
	}


	void RecordDebug(uint32_t imageIndex)
	{
		auto& cmdBuf = debugDraw.drawBuffers[imageIndex];
		cmdBuf.Begin();
		debugDraw.mesh.StageDynamic(cmdBuf);

		debugDraw.renderPass.Begin(
			cmdBuf,
			debugDraw.frameBuffers[imageIndex].Get(),
			debugDraw.pipeline
		);


		debugDraw.renderPass.RenderObjects(
			cmdBuf,
			descriptors.sets[imageIndex],
			debugDraw.pipelineLayout,
			&debugDraw.mesh
		);
		cmdBuf.endRenderPass();
		cmdBuf.end();
		
	}



	void OnSurfaceRecreate()
	{
		const auto& extent = device.swapchain.extent;
		camera.SetPerspective(45.0f, (float)extent.width / extent.height, 0.1f, 100.0f);
		uboViewProjection.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		// ImGui_ImplVulkan_SetMinImageCount()
	}


	void Draw() override
	{
		DrawUI();
		if (device.PrepareFrame(currentFrame))
			OnSurfaceRecreate();

		UpdateUniformBuffers(dt, device.imageIndex);
		RecordDeferred(device.imageIndex);
		RecordFSQ(device.imageIndex);
		RecordDepthCopyForward(device.imageIndex);
		RecordForward(device.imageIndex);
		RecordDepthCopyDebug(device.imageIndex);
		RecordDebug(device.imageIndex);
		overlay.RecordCommandBuffers(device.imageIndex);

		// ----------------------
		// Deferred Pass
		// ----------------------
		vk::CommandBuffer commandBuffersDeferred[1] = {
			gBuffer.drawBuffers[device.imageIndex]
		};
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

		vk::SubmitInfo submitInfo = {};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &device.imageAvailable[currentFrame];
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &gBuffer.semaphores[currentFrame];
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = commandBuffersDeferred;

		utils::CheckVkResult(
			device.graphicsQueue.submit(1,
										&submitInfo,
										nullptr),
			"Failed to submit draw queue"
		);

		// ----------------------
		// FSQ Pass
		// ----------------------
		vk::CommandBuffer commandBuffersFSQ[1] = {
			fsq.drawBuffers[device.imageIndex]
		};

		submitInfo.pWaitSemaphores = &gBuffer.semaphores[currentFrame];
		submitInfo.pSignalSemaphores = &fsq.semaphores[currentFrame];
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = commandBuffersFSQ;
		utils::CheckVkResult(
			device.graphicsQueue.submit(1,
										&submitInfo,
										nullptr),
			"Failed to submit draw queue"
		);

		// ----------------------
		// Depth Copy Forward Pass
		// ----------------------
		submitInfo.pWaitSemaphores = &fsq.semaphores[currentFrame];
		submitInfo.pSignalSemaphores = &depthCopySem1[currentFrame];
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &depthCopyCmd1[device.imageIndex];
		utils::CheckVkResult(
			device.graphicsQueue.submit(1, &submitInfo, nullptr), "Failed to submit draw queue"
		);

		// ----------------------
		// Forward Pass
		// ----------------------
		vk::CommandBuffer commandBuffersForward[1] = {
			device.drawBuffers[device.imageIndex]
		};

		submitInfo.pWaitSemaphores = &depthCopySem1[currentFrame];
		submitInfo.pSignalSemaphores = &device.renderFinished[currentFrame];
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = commandBuffersForward;
		utils::CheckVkResult(
			device.graphicsQueue.submit(1,
										&submitInfo,
										nullptr),
			"Failed to submit draw queue"
		);

		// ----------------------
		// Depth Copy Debug Pass
		// ----------------------
		submitInfo.pWaitSemaphores = &device.renderFinished[currentFrame];
		submitInfo.pSignalSemaphores = &depthCopySem2[currentFrame];
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &depthCopyCmd2[device.imageIndex];
		utils::CheckVkResult(
			device.graphicsQueue.submit(1, &submitInfo, nullptr), "Failed to submit draw queue"
		);


		// ----------------------
		// Debug Pass
		// ----------------------
		vk::CommandBuffer commandBuffersDebug[2] = {
			debugDraw.drawBuffers[device.imageIndex],
			overlay.commandBuffers[device.imageIndex]
		};

		submitInfo.pWaitSemaphores = &depthCopySem2[currentFrame];
		submitInfo.pSignalSemaphores = &debugDraw.semaphores[currentFrame];
		submitInfo.commandBufferCount = 2;
		submitInfo.pCommandBuffers = commandBuffersDebug;
		utils::CheckVkResult(
			device.graphicsQueue.submit(1,
										&submitInfo,
										device.drawFences[currentFrame].Get()),
			"Failed to submit draw queue"
		);


		if (device.SubmitFrame(currentFrame, debugDraw.semaphores[currentFrame]))
			OnSurfaceRecreate();

		currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
	}



	void UpdateUniformBuffers(float dt, const uint32_t imageIndex)
	{

		// UPDATE LIGHTS
		// White
		uboComposition.lights[0].position = glm::vec4(0.0f, 10.0f, 1.0f, 0.0f);
		// uboComposition.lights[0].color = glm::vec3(1.5f);
		uboComposition.lights[0].color = glm::vec3(1.0f);
		uboComposition.lights[0].radius = 15.0f * 0.25f;
		// Red
		uboComposition.lights[1].position = glm::vec4(-2.0f, 5.0f, 0.0f, 0.0f);
		// uboComposition.lights[1].color = glm::vec3(1.0f, 0.0f, 0.0f);
		uboComposition.lights[1].color = glm::vec3(1.0f, 1.0f, 1.0f);
		uboComposition.lights[1].radius = 150.0f;
		// Blue
		uboComposition.lights[2].position = glm::vec4(2.0f, -10.0f, 0.0f, 0.0f);
		// uboComposition.lights[2].color = glm::vec3(0.0f, 0.0f, 2.5f);
		uboComposition.lights[2].color = glm::vec3(1.0f, 1.0f, 1.0f);
		uboComposition.lights[2].radius = 50.0f;
		// Yellow
		uboComposition.lights[3].position = glm::vec4(0.0f, -9.f, 0.5f, 0.0f);
		// uboComposition.lights[3].color = glm::vec3(1.0f, 1.0f, 0.0f);
		uboComposition.lights[3].color = glm::vec3(1.0f, 1.0f, 1.0f);
		uboComposition.lights[3].radius = 20.0f;
		// Green
		uboComposition.lights[4].position = glm::vec4(0.0f, 50.0f, 0.0f, 0.0f);
		// uboComposition.lights[4].color = glm::vec3(0.0f, 1.0f, 0.2f);
		uboComposition.lights[4].color = glm::vec3(0.0f, 1.0f, 0.2f);
		uboComposition.lights[4].radius = 50.0f;
		// Yellow
		uboComposition.lights[5].position = glm::vec4(0.0f, 3.0f, 0.0f, 0.0f);
		// uboComposition.lights[5].color = glm::vec3(1.0f, 0.7f, 0.3f);
		uboComposition.lights[5].color = glm::vec3(1.0f, 1.0f, 1.0f);
		uboComposition.lights[5].radius = 75.0f;

		static float timer = 0.0f;
		float lightMovementMod = 0.2f;
		uboComposition.lights[0].position.x = sin(glm::radians(360.0f * timer * lightMovementMod)) * 30.0f;
		uboComposition.lights[0].position.z = cos(glm::radians(360.0f * timer * lightMovementMod)) * 30.0f;

		uboComposition.lights[1].position.x = -4.0f + sin(glm::radians(360.0f * timer * lightMovementMod) + 45.0f) * 2.0f;
		uboComposition.lights[1].position.z = 0.0f + cos(glm::radians(360.0f * timer * lightMovementMod) + 45.0f) * 2.0f;

		uboComposition.lights[2].position.x = 4.0f + sin(glm::radians(360.0f * timer * lightMovementMod)) * 10.0f;
		uboComposition.lights[2].position.z = 0.0f + cos(glm::radians(360.0f * timer * lightMovementMod)) * 20.0f;

		uboComposition.lights[4].position.x = 0.0f + sin(glm::radians(360.0f * timer * lightMovementMod + 90.0f)) * 5.0f;
		uboComposition.lights[4].position.z = 0.0f - cos(glm::radians(360.0f * timer * lightMovementMod + 45.0f)) * 5.0f;

		uboComposition.lights[5].position.x = 0.0f + sin(glm::radians(-360.0f * timer * lightMovementMod + 135.0f)) * 10.0f;
		uboComposition.lights[5].position.z = 0.0f - cos(glm::radians(-360.0f * timer * lightMovementMod - 45.0f)) * 10.0f;

		// Current view position
		uboComposition.viewPos = glm::vec4(camera.position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

		uboComposition.debugDisplayTarget = debugDisplayTarget;
		timer += dt;

		uniformBufferViewProjection[imageIndex].MapToBuffer(&uboViewProjection);
		uniformBufferComposition[imageIndex].MapToBuffer(&uboComposition);
	}


	void UpdateInput(float dt)
	{
		auto win = static_cast<Window*>(window.lock().get())->GetHandle();
		if (glfwGetKey(win, GLFW_KEY_ESCAPE))
			glfwSetWindowShouldClose(win, 1);

		static float spaceCooldown = 1.0f;
		static float spaceTimer = 1.0f;
		if (glfwGetKey(win, GLFW_KEY_SPACE) && spaceTimer > spaceCooldown)
		{
			cursorActive = !cursorActive;
			glfwSetInputMode(win, GLFW_CURSOR,
							 cursorActive ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
			spaceTimer = 0.0f;
		}
		spaceTimer += dt;

	}


	void UpdateDebugMesh()
	{
		std::vector<PosVertex> debugVertices;
		for (auto& object : forwardObjects)
		{
			std::vector<glm::vec3> positions = object.GetVertexBufferData<glm::vec3>(
				offsetof(Vertex, pos)
				);

			std::vector<glm::vec3> normals = object.GetVertexBufferData<glm::vec3>(
				offsetof(Vertex, normal)
				);

			for (int i = 0; i < object.vertexBuffer.GetVertexCount(); ++i)
			{
				auto worldSpacePosition = (glm::vec3)(object.model * glm::vec4(positions[i], 1.0f));
				debugVertices.emplace_back(worldSpacePosition);
				debugVertices.emplace_back(worldSpacePosition + normals[i]*debugDraw.debugLineLength);
			}
		}

		for (auto& object : objects)
		{
			std::vector<glm::vec3> positions = object.GetVertexBufferData<glm::vec3>(
				offsetof(Vertex, pos)
				);

			std::vector<glm::vec3> normals = object.GetVertexBufferData<glm::vec3>(
				offsetof(Vertex, normal)
				);

			for (int i = 0; i < object.vertexBuffer.GetVertexCount(); ++i)
			{
				auto worldSpacePosition = (glm::vec3)(object.model * glm::vec4(positions[i], 1.0f));
				debugVertices.emplace_back(worldSpacePosition);
				debugVertices.emplace_back(worldSpacePosition + normals[i]*debugDraw.debugLineLength);
			}
		}


		debugDraw.mesh.UpdateDynamic(debugVertices);
		//for (auto& object : forwardObjects)
		//{

		//}
	}
	void Update() override
	{
		RenderingContext::Update();
		// TODO: MOVE THIS OUT
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		static float speed = 90.0f;
		UpdateInput(dt);

		// const auto& model = objects[0].model;
		// objects[0].SetModel(glm::rotate(model, glm::radians(speed * dt), { 0.0f, 0.0f,1.0f }));

		camera.Update(dt, !cursorActive);
		// const auto& model2 = objects[1].model;
		// objects[1].SetModel(glm::rotate(model2, glm::radians(speed2 * dt), { 0.0f, 1.0f,1.0f }));

		static glm::vec3 ppScale = { .0001f, .0001f, .0001f };
		ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen);
		//ImGui::InputFloat3("Camera Position", &camera.position[0]);
		//ImGui::InputFloat("Pitch", &camera.pitch);
		//ImGui::InputFloat("Yaw", &camera.yaw);
		ImGui::SliderFloat4("Clear Color", clearColor.data(), 0.0f, 1.0f);
		ImGui::SliderFloat("Light Strength", &uboComposition.globalLightStrength, 0.01f, 5.0f);
		ImGui::Checkbox("Copy Depth", &copyDepth);
		ImGui::SliderFloat("Debug Line Length", &debugDraw.debugLineLength, 0.01f, 5.0f);

		//for (auto& object : objects)
		//	object.SetModel(glm::scale(glm::mat4(1.0f), ppScale));

		ImGui::Text("Debug Target: ");
		for (int i = 0; i < 3; ++i)
		{
			ImGui::RadioButton(std::to_string(i).c_str(), &debugDisplayTarget, i);
			if (i != 4) ImGui::SameLine();
		}

		uboViewProjection.view = camera.matrices.view;
		for (size_t j = 0; j < forwardObjects.size(); ++j)
		{
			auto& mesh = forwardObjects[j];
			mesh.model = glm::rotate(mesh.model,
									 glm::radians(speed * dt * utils::Random(1.0f, 5.0f)),
									 glm::vec3(utils::Random(), utils::Random(), utils::Random())
			);
		}
		UpdateDebugMesh();
	}


	void DrawUI()
	{
		//ImGui::ShowDemoWindow();
		ImGui::Render();
	}
};

RenderingContext& RenderingContext::Get()
{
	static DeferredRenderingContext context;
	return context;
}


