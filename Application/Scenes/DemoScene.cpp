#include "Window.h"
#include "RenderingContext.h"
#include "glfw3.h"
#include "imgui_impl_glfw.h"
#include "Camera/Camera.h"
#include "Overlay/Blocks/EntityEditorBlock.h"
#include "SpatialPartitioning/Octree/Octree.hpp"
#include "SpatialPartitioning/BSP/BSP.hpp"
#include "Job/Job.h"

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

	const std::vector<uint32_t> meshIndices = {
		0, 1, 2,
		2, 3, 0
	};

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

    std::vector<entt::entity> entities;

    // Descriptors
	Descriptors descriptors;
    vk::PushConstantRange pushRange;

    std::vector<Buffer> uniformBufferViewProjection;
    std::vector<Buffer> uniformBufferComposition;
    std::vector<Buffer> uniformBufferCollider;

	// Deferred Data
	struct GBuffer
	{
		uint32_t width, height;
		FrameBufferAttachment position, normal, albedo;
		FrameBufferAttachment depth;
		RenderPass renderPass;

		GraphicsPipeline pipeline;
		GraphicsPipeline wireframePipeline;

		

		bool wireframeEnabled = false;
		PipelineLayout pipelineLayout;
		bool render = true;

		std::vector<FrameBuffer> frameBuffers;
		std::vector<CommandBuffer> drawBuffers;
		std::vector<Semaphore> semaphores;
	} gBuffer;

	struct ThreadData
	{
		CommandPool pool;
		std::vector<CommandBuffer> commandBuffers;
	};

	std::vector<ThreadData> threadData;

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

	struct DebugLineList
	{
		RenderPass renderPass;
		GraphicsPipeline pipeline;
		PipelineLayout pipelineLayout;
		FrameBufferAttachment depth;

		std::vector<FrameBuffer> frameBuffers;
		std::vector<CommandBuffer> drawBuffers;
		std::vector<Semaphore> semaphores;
		Mesh<PosVertex> mesh;
		float lineLength = 1.0f;
		float lineWidth = 1.0f;

		bool render = false;
	} debugLineList;

	struct DebugLineStrip
	{
		RenderPass renderPass;
		GraphicsPipeline pipeline;
		PipelineLayout pipelineLayout;
		// REUSE DEPTH FROM DEBUG NORMAL

		std::vector<FrameBuffer> frameBuffers;
		std::vector<CommandBuffer> drawBuffers;
		std::vector<Semaphore> semaphores;
		float lineWidth = 1.0f;

		bool render = true;
	} debugLineStrip;


	std::array<float, 4> clearColor = {0.0f, 0.0f, 0.0f, 0.0f};

	bool cursorActive = false;

	RenderComponentSystem* renderSystem = nullptr;
	Octree octree;
	BSP bsp;

	entt::entity sphere;
	float sphereSpeed = 1.0f;


	void SetInputCallbacks()
	{
		auto win = ((Window*)window.lock().get())->GetHandle();
		static auto cursorPosCallback = [](GLFWwindow* window, double xPos, double yPos)
		{
			auto context = reinterpret_cast<DeferredRenderingContext*>(glfwGetWindowUserPointer(window));
			context->camera.ProcessMouseInput({ xPos, yPos });
		};
		glfwSetCursorPosCallback(win, cursorPosCallback);
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
		camera.SetPosition({ 10.0f, -10.0f, 16.0f });
		
		camera.pitch = 0.0f;
		camera.yaw = 180.0f;
		uboViewProjection.projection = camera.matrices.perspective;
    
		// // Invert up direction so that pos y is up 
		// // its down by default in Vulkan
		uboViewProjection.projection[1][1] *= -1;
		std::vector<PosVertex> dummy{ {} };
		debugLineList.mesh.CreateDynamic(dummy, device);


		renderSystem = &ECS::GetSystem<RenderComponentSystem>();
		auto& reg = ECS::Get();



		const std::vector<Vertex> quadVerts = {
		{ { -1.0, 1.0, 0.5 },{ 0.0f, 0.0f, 1.0f} , {1.0f, 0.0f, 0.0f}, { 0.0f, 1.0f } },	// 0
		{ { -1.0, -1.0, 0.5 },{ 0.0f, 0.0f, 1.0f} , {0.0f, 1.0f, 0.0f}, { 0.0f, 1.0f } },	// 0
		{ { 1.0, -1.0, 0.5 },{ 0.0f, 0.0f, 1.0f} , {0.0f, 0.0f, 1.0f}, { 0.0f, 1.0f } },	// 0
		{ { 1.0, 1.0, 0.5 },{ 0.0f, 0.0f, 1.0f} , {1.0f, 1.0f, 0.0f}, { 0.0f, 1.0f } }	// 0
		};

	const std::vector<uint32_t> quadIndices = {
		0, 1, 2,
		2, 3, 0
	};

    
		srand(133333337);
		for (int i = 0; i < 1000; ++i)
		{
			auto entity = reg.create();
			auto& transform1 = reg.emplace<TransformComponent>(entity);
			transform1.SetScale(glm::vec3((utils::Random()+0.5f) * 1.0f));
			transform1.SetPosition(glm::vec3(utils::Random(-10.0f, 10.0f), utils::Random(-10.0f, 10.0f), utils::Random(-10.0f, 10.0f)));
			//transform1.SetPosition({ -1.0f, -1.0f, -1.0f });
			//const std::string room = std::string(ASSET_DIR) + "Models/teapot.obj";
			transform1.SetRotation(glm::vec3(utils::Random(0.0f, 360.0f), utils::Random(0.0f, 360.0f), utils::Random(0.0f, 360.0f)));

			auto& render = reg.emplace<DeferredRenderComponent>(entity);
			//render.mesh.CreateModel(room, false, device);

			render.mesh.CreateStatic(quadVerts, quadIndices, device);

			//obj.model = glm::scale(obj.model, glm::vec3(utils::Random() * 3.0f));
			//obj.model = glm::translate(obj.model, glm::vec3(
			//	utils::Random(-10.0f, 10.0f), utils::Random(-10.0f, 10.0f), utils::Random(-10.0f, 10.0f)));
			//obj.SetRotation(glm::vec3(utils::Random(), utils::Random(), utils::Random()));
		}
		fsq.mesh.CreateStatic(meshVerts, meshIndices, device);
		auto fsqEntity = reg.create();
		reg.emplace<TransformComponent>(fsqEntity);
		reg.emplace<PostRenderComponent>(fsqEntity, fsq.mesh);

		

		//{
		//	auto entity = reg.create();
		//	auto& transform1 = reg.emplace<TransformComponent>(entity);
		//	auto& physics = reg.emplace<PhysicsComponent>(entity);
		//	// Set off-scene
		//	transform1.SetPosition({ 9999.9f, 9999.9f, 9999.9f});
		//	auto& render = reg.emplace<DebugRenderComponent>(entity);
		//	render.mesh.Create(Mesh<PosVertex>::Sphere);
		//	sphere = entity;
		//}

		InitializeThreading();
		InitializeAttachments();
		InitializeUniformBuffers();
		InitializeDescriptorSets();
		InitializePipelines();
		InitializeDebugMesh();
		InitializeOverlay();
	}

	void DestroyPipelines()
	{
		device.waitIdle();
		gBuffer.pipeline.Destroy();
		gBuffer.pipelineLayout.Destroy();
		fsq.pipeline.Destroy();
		fsq.pipelineLayout.Destroy();
		debugLineList.pipeline.Destroy();
		debugLineList.pipelineLayout.Destroy();
		debugLineStrip.pipeline.Destroy();
		debugLineStrip.pipelineLayout.Destroy();
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
		
		gBuffer.renderPass.Destroy();
		
		utils::VectorDestroyer(fsq.frameBuffers);
		utils::VectorDestroyer(fsq.semaphores);

		fsq.renderPass.Destroy();
		fsq.mesh.Destroy();

		utils::VectorDestroyer(debugLineList.frameBuffers);
		utils::VectorDestroyer(debugLineList.semaphores);
		debugLineList.depth.Destroy();
		debugLineList.renderPass.Destroy();
		debugLineList.mesh.Destroy();

		utils::VectorDestroyer(debugLineStrip.frameBuffers);
		utils::VectorDestroyer(debugLineStrip.semaphores);
		debugLineStrip.renderPass.Destroy();

		DestroyPipelines();
				
		device.commandPool.FreeCommandBuffers(
			gBuffer.drawBuffers, 
			fsq.drawBuffers, 
			debugLineList.drawBuffers,
			debugLineStrip.drawBuffers
		);

		utils::VectorDestroyer(uniformBufferViewProjection);
		utils::VectorDestroyer(uniformBufferComposition);
		//utils::VectorDestroyer(forwardObjects);
		
		RenderingContext::Destroy();
	}

	void InitializeThreading()
	{
		const size_t imageViewCount = device.swapchain.imageViews.size();
		threadData.resize(JobSystem::ThreadCount);
		
		for (uint32_t i = 0; i < JobSystem::ThreadCount; ++i)
		{
			ThreadData& data = threadData[i];

			// Command pool for each thread
			vk::CommandPoolCreateInfo cpCI;
			cpCI.setQueueFamilyIndex(physicalDevice.GetQueueFamilyIndices().graphics.value());
			cpCI.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

			CommandPool::Create(data.pool, cpCI, device);

			// Secondary command buffer for each thread
			vk::CommandBufferAllocateInfo cbAI;
			cbAI.setCommandPool(data.pool);
			cbAI.setCommandBufferCount(imageViewCount);
			cbAI.level = vk::CommandBufferLevel::eSecondary;

			data.commandBuffers.resize(imageViewCount);

			utils::CheckVkResult(device.allocateCommandBuffers(&cbAI, data.commandBuffers.data()), 
								 "Failed to allocate thread command buffers");
		}
	}

	void InitializeOverlay()
	{
		auto entityWindow = new EditorWindow("Entities");
		auto entityEditor = new EntityEditorBlock(entities);
		auto dims = window.lock()->GetDimensions();
		entityEditor->updateCallbacks.emplace_back(
			[dims]()
		{
			ImGui::SetWindowPos(ImVec2(0, dims.y - 300));
			ImGui::SetWindowSize(ImVec2(300, 300));
		}
		);
		entityWindow->AddBlock(entityEditor);
		overlay.PushEditorWindow(entityWindow);
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

			std::array<vk::SubpassDependency, 1> dependencies;

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
			dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
			dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
			dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

			vk::RenderPassCreateInfo rpInfo = {};
			rpInfo.pAttachments = attachmentDescs.data();
			rpInfo.attachmentCount = attachmentDescs.size();
			rpInfo.subpassCount = 1;
			rpInfo.pSubpasses = &subpass;
			rpInfo.dependencyCount = 1;
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

				//fsq.depth.Create(FrameBufferAttachment::GetDepthFormat(), { gBuffer.width, gBuffer.height },
				//				 vk::ImageUsageFlagBits::eDepthStencilAttachment,
				//				 vk::ImageAspectFlagBits::eDepth,
				//				 vk::ImageLayout::eDepthStencilAttachmentOptimal,
				//				 // No sampler
				//				 device);

				//vk::AttachmentDescription depthAttachment;
				//depthAttachment.format = FrameBufferAttachment::GetDepthFormat();
				//depthAttachment.samples = vk::SampleCountFlagBits::e1;			
				//depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;			
				//depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;		
				//depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
				//depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;	
				//depthAttachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				//depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;		


				std::array<vk::AttachmentDescription, 1> attachmentsDescFSQ = { colorAttachment };

				// REFERENCES
				vk::AttachmentReference colorAttachmentRef;
				colorAttachmentRef.attachment = 0;
				colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

				vk::SubpassDescription subpass = {};
				subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
				subpass.colorAttachmentCount = 1;
				subpass.pColorAttachments = &colorAttachmentRef;
				subpass.pDepthStencilAttachment = nullptr;

				vk::SubpassDependency dependency = {};
				dependency.srcSubpass = 0;
				dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
				dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
				dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
				dependency.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
				dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;

				vk::RenderPassCreateInfo createInfo;
				createInfo.attachmentCount = static_cast<uint32_t>(1);
				createInfo.pAttachments = attachmentsDescFSQ.data();
				createInfo.subpassCount = 1;
				createInfo.pSubpasses = &subpass;
				createInfo.dependencyCount = 1;
				createInfo.pDependencies = &dependency;

				std::vector<vk::ClearValue> clearValues(1);
				clearValues[0].color = vk::ClearColorValue(clearColor);

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
				createInfo.attachmentCount = 1;

				for (size_t i = 0; i < imageViewsSize; ++i)
				{
					std::array<vk::ImageView, 1> attachments = {
						imageViews[i].Get()
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
				debugLineList.depth.Create(FrameBufferAttachment::GetDepthFormat(), device.swapchain.extent,
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
				depthAttachment.format = debugLineList.depth.format;
				depthAttachment.samples = vk::SampleCountFlagBits::e1;
				depthAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
				depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
				depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;	
				depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;	
				depthAttachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

				std::array<vk::AttachmentDescription, 2> attachmentsDesc = { colorAttachment, depthAttachment };

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
				dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				dependency.srcAccessMask = vk::AccessFlagBits::eMemoryRead;

				// Happens before 
				dependency.dstSubpass = 0;
				dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
				//dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;

				vk::RenderPassCreateInfo createInfo;
				createInfo.attachmentCount = static_cast<uint32_t>(attachmentsDesc.size());
				createInfo.pAttachments = attachmentsDesc.data();
				createInfo.subpassCount = 1;
				createInfo.pSubpasses = &subpass;
				createInfo.dependencyCount = 1;
				createInfo.pDependencies = &dependency;


				std::vector<vk::ClearValue> clearValues(2);
				clearValues[0].color = vk::ClearColorValue(clearColor);
				clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f);

				const auto& extent = device.swapchain.extent;

				debugLineList.renderPass.Create(createInfo, device, extent, clearValues);
			}

			// Frame buffers
			{
				const auto& imageViews = device.swapchain.imageViews;
				const auto& extent = device.swapchain.extent;
				size_t imageViewsSize = imageViews.size();
				debugLineList.frameBuffers.resize(imageViewsSize);

				vk::FramebufferCreateInfo createInfo;
				// FB width/height
				createInfo.width = extent.width;
				createInfo.height = extent.height;
				// FB layers
				createInfo.layers = 1;
				createInfo.renderPass = debugLineList.renderPass;
				createInfo.attachmentCount = 2;

				for (size_t i = 0; i < imageViewsSize; ++i)
				{
					std::array<vk::ImageView, 2> attachments = {
						imageViews[i].Get(),
						debugLineList.depth.imageView
					};

					// List of attachments 1 to 1 with render pass
					createInfo.pAttachments = attachments.data();

					FrameBuffer::Create(&debugLineList.frameBuffers[i], createInfo, device);
				}
			}

			// Command buffers
			{
				debugLineList.drawBuffers.resize(imageViewCount);

				vk::CommandBufferAllocateInfo cmdInfo = {};
				cmdInfo.commandPool = device.commandPool.Get();
				cmdInfo.level = vk::CommandBufferLevel::ePrimary;
				cmdInfo.commandBufferCount = static_cast<uint32_t>(imageViewCount);

				utils::CheckVkResult(
					device.allocateCommandBuffers(&cmdInfo, debugLineList.drawBuffers.data()),
					"Failed to allocate deferred command buffers"
				);
			}

			// Semaphores
			debugLineList.semaphores.resize(MAX_FRAME_DRAWS);
			vk::SemaphoreCreateInfo semInfo = {};
			for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
			{
				Semaphore::Create(debugLineList.semaphores[i], semInfo, device);
			}
		}

		// ----------------------
		// Debug Line Strip Render Pass
		// ----------------------
		{
			// Render pass & subpass
			{
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


				// LOAD FROM DEBUG NORMAL PASS
				vk::AttachmentDescription depthAttachment = {};
				depthAttachment.format = debugLineList.depth.format;
				depthAttachment.samples = vk::SampleCountFlagBits::e1;
				depthAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
				depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
				depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;	
				depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;	
				depthAttachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

				std::array<vk::AttachmentDescription, 2> attachmentsDesc = { colorAttachment, depthAttachment };

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
				dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				dependency.srcAccessMask = vk::AccessFlagBits::eMemoryRead;

				// Happens before 
				dependency.dstSubpass = 0;
				dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
				//dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;

				vk::RenderPassCreateInfo createInfo;
				createInfo.attachmentCount = static_cast<uint32_t>(attachmentsDesc.size());
				createInfo.pAttachments = attachmentsDesc.data();
				createInfo.subpassCount = 1;
				createInfo.pSubpasses = &subpass;
				createInfo.dependencyCount = 1;
				createInfo.pDependencies = &dependency;

				std::vector<vk::ClearValue> clearValues(2);
				clearValues[0].color = vk::ClearColorValue(clearColor);
				clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f);

				const auto& extent = device.swapchain.extent;

				debugLineStrip.renderPass.Create(createInfo, device, extent, clearValues);
			}

			// Frame buffers
			{
				const auto& imageViews = device.swapchain.imageViews;
				const auto& extent = device.swapchain.extent;
				size_t imageViewsSize = imageViews.size();
				debugLineStrip.frameBuffers.resize(imageViewsSize);

				vk::FramebufferCreateInfo createInfo;
				// FB width/height
				createInfo.width = extent.width;
				createInfo.height = extent.height;
				// FB layers
				createInfo.layers = 1;
				createInfo.renderPass = debugLineStrip.renderPass;
				createInfo.attachmentCount = 2;

				for (size_t i = 0; i < imageViewsSize; ++i)
				{
					std::array<vk::ImageView, 2> attachments = {
						imageViews[i].Get(),
						debugLineList.depth.imageView
					};

					// List of attachments 1 to 1 with render pass
					createInfo.pAttachments = attachments.data();

					FrameBuffer::Create(&debugLineStrip.frameBuffers[i], createInfo, device);
				}
			}

			// Command buffers
			{
				debugLineStrip.drawBuffers.resize(imageViewCount);

				vk::CommandBufferAllocateInfo cmdInfo = {};
				cmdInfo.commandPool = device.commandPool.Get();
				cmdInfo.level = vk::CommandBufferLevel::ePrimary;
				cmdInfo.commandBufferCount = static_cast<uint32_t>(imageViewCount);

				utils::CheckVkResult(
					device.allocateCommandBuffers(&cmdInfo, debugLineStrip.drawBuffers.data()),
					"Failed to allocate deferred command buffers"
				);
			}

			// Semaphores
			debugLineStrip.semaphores.resize(MAX_FRAME_DRAWS);
			vk::SemaphoreCreateInfo semInfo = {};
			for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
			{
				Semaphore::Create(debugLineStrip.semaphores[i], semInfo, device);
			}
		}

	}


	void LoadSection(int section = -1)
	{
		std::vector<std::vector<Mesh<Vertex>::Data>> meshData(20);
		ASSERT(section < 21, "Invalid power plant section index");

		auto loadSection = [&meshData](const std::string& sectionPath,
									   Device& device, int threadID)
		{
			std::ifstream sectionFile;
			sectionFile.open(sectionPath);
			std::string modelsPath = std::string(ASSET_DIR) + "Models/";
			std::string input;
			std::vector<Mesh<Vertex>::Data> section;
			while (sectionFile >> input)
			{
				std::string combinedPath = modelsPath + input;
				section.emplace_back(Mesh<Vertex>::LoadModel(combinedPath));
				input.clear();
			}
			meshData[threadID] = std::move(section);
		};

		if (section != -1)
		{
			const std::string sectionString = std::string(ASSET_DIR) + "Models/Section" + std::to_string(section + 1) + ".txt";
			loadSection(sectionString, std::ref(device), 0);
			for (auto& data : meshData[0])
			{
				auto& reg = ECS::Get();
				auto entity = reg.create();
				auto& transform =  reg.emplace<TransformComponent>(entity);
				transform.SetScale(glm::vec3(0.0001f));
				auto& render = reg.emplace<DeferredRenderComponent>(entity);
				render.mesh.CreateStatic(data.vertices, data.indices, device);
			}
			return;
		}


		std::vector<std::thread> loadGroup = {};
		for (int i = 1; i < 21; ++i)
		{
			const std::string sectionString = std::string(ASSET_DIR) + "Models/Section" + std::to_string(section + 1) + ".txt";
			loadGroup.emplace_back(loadSection, sectionString, std::ref(device), i - 1);
		}

		for (auto& loader : loadGroup)
		{
			loader.join();
		}

		for (auto& vector : meshData)
		{
			for (auto& data : vector)
			{
				auto& reg = ECS::Get();
				auto entity = reg.create();
				auto& transform = reg.emplace<TransformComponent>(entity);
				transform.SetScale(glm::vec3(0.0001f));

				auto& render = reg.emplace<DeferredRenderComponent>(entity);
				render.mesh.CreateStatic(data.vertices, data.indices, device);
			}
		}

		device.waitIdle();
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
	}

	void InitializeDescriptorSets()
	{
		auto vpDescInfos = Buffer::AggregateDescriptorInfo(uniformBufferViewProjection);
		auto compositionDescInfos = Buffer::AggregateDescriptorInfo(uniformBufferComposition);

		auto posDesc = gBuffer.position.GetDescriptor(vk::ImageLayout::eShaderReadOnlyOptimal);
		auto normalDesc = gBuffer.normal.GetDescriptor(vk::ImageLayout::eShaderReadOnlyOptimal);
		auto albedoDesc = gBuffer.albedo.GetDescriptor(vk::ImageLayout::eShaderReadOnlyOptimal);


		std::array<DescriptorInfo, 5> descriptorInfos =
		{
			DescriptorInfo::CreateAsync(0,
				vk::ShaderStageFlagBits::eVertex,
				vk::DescriptorType::eUniformBuffer,
				vpDescInfos),
			DescriptorInfo::Create(1,
				vk::ShaderStageFlagBits::eFragment,
				vk::DescriptorType::eCombinedImageSampler,
				posDesc),
			DescriptorInfo::Create(2,
				vk::ShaderStageFlagBits::eFragment,
				vk::DescriptorType::eCombinedImageSampler,
				normalDesc),
			DescriptorInfo::Create(3,
				vk::ShaderStageFlagBits::eFragment,
				vk::DescriptorType::eCombinedImageSampler,
				albedoDesc),
			DescriptorInfo::CreateAsync(4,
				vk::ShaderStageFlagBits::eFragment,
				vk::DescriptorType::eUniformBuffer,
				compositionDescInfos),
		};

		descriptors.Create<5>(
			descriptorInfos,
			device.swapchain.images.size(),
			device
		);

		// PUSH CONSTANTS
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
		attribDesc[0].binding = 0;
		attribDesc[0].location = 0;
		attribDesc[0].format = vk::Format::eR32G32B32Sfloat;
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

		// ----------------------
		// Deferred wireframe
		// ----------------------
		rasterizeState.setPolygonMode(vk::PolygonMode::eLine);
		GraphicsPipeline::Create(gBuffer.wireframePipeline, pipelineInfo, device, vk::PipelineCache(), 1);

		rasterizeState.setPolygonMode(vk::PolygonMode::eFill);
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
		pipelineInfo.pDepthStencilState = nullptr;

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
		// NORMAL DEBUG PIPELINE
		// ----------------------

		// reuse basically everything from forward
		vertInfo = ShaderModule::Load(
			vertModule,
			"debugLineListVert.spv",
			vk::ShaderStageFlagBits::eVertex,
			device
		);
		fragInfo = ShaderModule::Load(
			fragModule,
			"debugLineListFrag.spv",
			vk::ShaderStageFlagBits::eFragment,
			device
		);

		shaderStages[0] = vertInfo;
		shaderStages[1] = fragInfo;

		pipelineInfo.pStages = shaderStages;
		pipelineInfo.renderPass = debugLineList.renderPass;
		inputAssembly.topology = vk::PrimitiveTopology::eLineList;
		rasterizeState.lineWidth = debugLineList.lineWidth;

		vk::DynamicState dynamicState = vk::DynamicState::eLineWidth;
		vk::PipelineDynamicStateCreateInfo dsInfo = {};
		dsInfo.pDynamicStates = &dynamicState;
		dsInfo.dynamicStateCount = 1;
		pipelineInfo.pDynamicState = &dsInfo;

		// VERTEX BINDING DATA
		auto bindDescDebug = vk::VertexInputBindingDescription();
		bindDescDebug.binding = 0;
		bindDescDebug.stride = sizeof(PosVertex);
		bindDescDebug.inputRate = vk::VertexInputRate::eVertex;


		// VERTEX INPUT
		auto vertexInputInfoDebug = vk::PipelineVertexInputStateCreateInfo();
		vertexInputInfoDebug.vertexBindingDescriptionCount = 1;
		vertexInputInfoDebug.pVertexBindingDescriptions = &bindDescDebug;
		// Data spacingDebug / stride info
		vertexInputInfoDebug.vertexAttributeDescriptionCount = static_cast<uint32_t>(1);
		vertexInputInfoDebug.pVertexAttributeDescriptions = attribDesc.data();

		pipelineInfo.pVertexInputState = &vertexInputInfoDebug;
		

		PipelineLayout::Create(debugLineList.pipelineLayout, layout, device);
		pipelineInfo.layout = debugLineList.pipelineLayout.Get();

		GraphicsPipeline::Create(debugLineList.pipeline, pipelineInfo, device, vk::PipelineCache(), 1);
		vertModule.Destroy();
		fragModule.Destroy();

		// ----------------------
		// DEBUG LINE STRIP PIPELINE
		// ----------------------
		// secretly just triangle strip rendered as a line lul

		// reuse basically everything from debug normal
		vertInfo = ShaderModule::Load(
			vertModule,
			"debugLineStripVert.spv",
			vk::ShaderStageFlagBits::eVertex,
			device
		);
		fragInfo = ShaderModule::Load(
			fragModule,
			"debugLineStripFrag.spv",
			vk::ShaderStageFlagBits::eFragment,
			device
		);

		shaderStages[0] = vertInfo;
		shaderStages[1] = fragInfo;

		pipelineInfo.pStages = shaderStages;
		pipelineInfo.renderPass = debugLineStrip.renderPass;
		inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
		rasterizeState.polygonMode = vk::PolygonMode::eLine;
		rasterizeState.lineWidth = debugLineStrip.lineWidth;

		vk::PushConstantRange pcr[2];
		// Model
		pcr[0].setOffset(0);
		pcr[0].setSize(sizeof(utils::UBOModel));
		pcr[0].setStageFlags(vk::ShaderStageFlagBits::eVertex);
		//// Color
		pcr[1].setOffset(sizeof(glm::mat4));
		pcr[1].setSize(sizeof(utils::UBOColor));
		pcr[1].setStageFlags(vk::ShaderStageFlagBits::eFragment);
		layout.pPushConstantRanges = pcr;
		layout.pushConstantRangeCount = 2;

		PipelineLayout::Create(debugLineStrip.pipelineLayout, layout, device);
		pipelineInfo.layout = debugLineStrip.pipelineLayout.Get();

		GraphicsPipeline::Create(debugLineStrip.pipeline, pipelineInfo, device, vk::PipelineCache(), 1);
	}

	void RecordDeferred(uint32_t imageIndex)
	{
		std::vector<vk::CommandBuffer> secondary;

		const uint32_t numThreads = threadData.size();

		auto& cmdBuf = gBuffer.drawBuffers[imageIndex];
		cmdBuf.Begin();

		// Use secondary command buffers for threading
		gBuffer.renderPass.Begin(
			cmdBuf,
			gBuffer.frameBuffers[imageIndex].Get(),
			(gBuffer.wireframeEnabled) ? gBuffer.wireframePipeline.Get() : gBuffer.pipeline.Get(),
			//vk::SubpassContents::eSecondaryCommandBuffers
			vk::SubpassContents::eInline
		);

		vk::CommandBufferInheritanceInfo inherit = { };
		inherit.setFramebuffer(gBuffer.frameBuffers[imageIndex].Get());
		inherit.setRenderPass(gBuffer.renderPass);

		if (gBuffer.render)
		{
			renderSystem->RenderEntities<DeferredRenderComponent>(
			cmdBuf,
			descriptors.sets[imageIndex],
			gBuffer.pipelineLayout.Get()
				);
			//auto& registry = ECS::Get();
			//registry.prepare<DeferredRenderComponent>();
			//registry.prepare<TransformComponent>();

			//const auto view = registry.view<DeferredRenderComponent>();
			//const auto size = view.size();

			//uint32_t objectsPerThread = view.size() / numThreads;
			//uint32_t extra = view.size() % numThreads;

			//std::vector<Job> jobs;

			//for (int i = 0; i < numThreads; ++i)
			//{
			//	uint32_t start = objectsPerThread * i;
			//	uint32_t end;
			//	end = start + (objectsPerThread - 1);
			//	if (i == JobSystem::ThreadCount - 1)
			//		end += extra;

			//	jobs.emplace_back(JobSystem::Push(
			//		[=]()
			//	{
			//		auto& registry = ECS::Get();
			//		ThreadData& thread = this->threadData[i];
			//		CommandBuffer cmdBuf = thread.commandBuffers[imageIndex];

			//		vk::CommandBufferBeginInfo beginInfo = {};
			//		beginInfo.pInheritanceInfo = &inherit;
			//		beginInfo.flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;

			//		cmdBuf.Begin(beginInfo);

			//		cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics,
			//							this->gBuffer.pipeline);

			//		// Bind descriptor sets
			//		cmdBuf.bindDescriptorSets(
			//			// Point of pipeline and layout
			//			vk::PipelineBindPoint::eGraphics,
			//			gBuffer.pipelineLayout,
			//			0,
			//			1,
			//			&descriptors.sets[imageIndex], // 1 to 1 with command buffers
			//			0,
			//			nullptr
			//		);

			//		int32_t lol = i;

			//		for (auto it = view.begin() + start; it != view.begin() + end; ++it)
			//		{
			//			auto entity = (*it);
			//			auto& transform = registry.get<TransformComponent>(entity);
			//			const auto& render = registry.get<DeferredRenderComponent>(entity);

			//			
			//			render.mesh.Bind(cmdBuf);
			//			transform.PushModel(cmdBuf, this->gBuffer.pipelineLayout);
			//			render.mesh.Draw(cmdBuf);
			//		}
			//		
			//		cmdBuf.End();
			//	}));
			//}
			//JobSystem::Execute();
			//JobSystem::WaitAll();

			//// Aggregate secondary buffers
			//for (int i = 0; i < numThreads; ++i)
			//{
			//	secondary.push_back(threadData[i].commandBuffers[imageIndex]);
			//}

			//// Execute the secondary buffers
			//cmdBuf.executeCommands(secondary.size(), secondary.data());
		}

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

		renderSystem->RenderEntities<PostRenderComponent>(
			cmdBuf,
			descriptors.sets[imageIndex],
			fsq.pipelineLayout.Get()
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

		renderSystem->RenderEntities<ForwardRenderComponent>(
			cmdBuf, descriptors.sets[imageIndex],
			device.pipelineLayout
		);
		//device.renderPass.RenderObjects(
		//	cmdBuf,
		//	descriptors.sets[imageIndex],
		//	device.pipelineLayout,
		//	forwardObjects.data(),
		//	forwardObjects.size()
		//);
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

		debugLineList.depth.image.TransitionLayout(
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
			debugLineList.depth.image.Get(),
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
				debugLineList.depth.image.Get(), vk::ImageLayout::eTransferDstOptimal,
				imageCopy);
		}

		debugLineList.depth.image.TransitionLayout(
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


	void RecordDebugLineList(uint32_t imageIndex)
	{
		auto& cmdBuf = debugLineList.drawBuffers[imageIndex];
		cmdBuf.Begin();

		if (debugLineList.render)
			debugLineList.mesh.StageDynamic(cmdBuf);

		debugLineList.renderPass.Begin(
			cmdBuf,
			debugLineList.frameBuffers[imageIndex].Get(),
			debugLineList.pipeline
		);


		if (debugLineList.render == true)
		{
			cmdBuf.setLineWidth(debugLineList.lineWidth);

			cmdBuf.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				debugLineList.pipelineLayout,
				0,
				1,
				&descriptors.sets[imageIndex],
				0,
				nullptr);

			//debugLineList.renderPass.RenderObjects(
			//	cmdBuf,
			//	descriptors.sets[imageIndex],
			//	debugLineList.pipelineLayout,
			//	&debugLineList.mesh
			//);
			octree.RenderCells(cmdBuf, debugLineList.pipelineLayout);
		}
		cmdBuf.endRenderPass();
		cmdBuf.end();
		
	}

	void RecordDebugLineStrip(uint32_t imageIndex)
	{
		auto& cmdBuf = debugLineStrip.drawBuffers[imageIndex];
		cmdBuf.Begin();

		debugLineStrip.renderPass.Begin(
			cmdBuf,
			debugLineStrip.frameBuffers[imageIndex].Get(),
			debugLineStrip.pipeline
		);

		if (debugLineStrip.render == true)
		{
			cmdBuf.setLineWidth(debugLineStrip.lineWidth);
			cmdBuf.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				debugLineStrip.pipelineLayout,
				0,
				1,
				&descriptors.sets[imageIndex],
				0,
				nullptr);

			//for (size_t i = 0; i < objects.size(); ++i)
			//{
			//	auto& object = objects[i];
			//	if (object.collider == nullptr) continue;

			//	object.collider->Draw(
			//		cmdBuf,
			//		object.GetPosition(),
			//		object.GetStoredRotation(),
			//		object.GetScale(),
			//		debugLineStrip.pipelineLayout
			//		);
			//}
			if (octree.IsInitialized())
				octree.RenderObjects(cmdBuf, debugLineStrip.pipelineLayout);
			if (bsp.IsInitialized())
				bsp.RenderObjects(cmdBuf, debugLineStrip.pipelineLayout);

			//auto* sphereRender = &ECS::Get().get<DebugRenderComponent>(sphere);
			//sphereRender->mesh.Bind(cmdBuf);

			//static glm::vec3 red = { 1.0f, 0.0f, 0.0f };
			//cmdBuf.pushConstants(
			//	debugLineStrip.pipelineLayout,
			//	vk::ShaderStageFlagBits::eFragment,
			//	sizeof(glm::mat4), sizeof(utils::UBOColor), &red
			//);
			//ECS::Get().get<TransformComponent>(sphere).PushModel(cmdBuf, debugLineStrip.pipelineLayout);

			//sphereRender->mesh.Draw(cmdBuf);
			//test.Bind(cmdBuf);
			//utils::PushIdentityModel(cmdBuf, debugLineStrip.pipelineLayout);
			//glm::vec4 purple(1.0f, 0.0f, 1.0f, 1.0f);
			//cmdBuf.pushConstants(
			//	debugLineStrip.pipelineLayout,
			//	vk::ShaderStageFlagBits::eFragment,
			//	sizeof(glm::mat4),
			//	sizeof(glm::vec4),
			//	&purple
			//);
		}
		cmdBuf.endRenderPass();
		cmdBuf.End();
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
		if (RenderQueue::Begin(device, currentFrame))
			OnSurfaceRecreate();

		MapUBO(device.imageIndex);

		RecordDeferred(device.imageIndex);
		RecordFSQ(device.imageIndex);
		RecordDepthCopyForward(device.imageIndex);
		RecordForward(device.imageIndex);
		RecordDepthCopyDebug(device.imageIndex);
		RecordDebugLineList(device.imageIndex);
		RecordDebugLineStrip(device.imageIndex);
		overlay.RecordCommandBuffers(device.imageIndex);

		// ----------------------
		// Deferred Pass
		// ----------------------
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		RenderQueue::SetCommandBuffers(&gBuffer.drawBuffers[device.imageIndex]);
		RenderQueue::SetStageMask(waitStages);
		RenderQueue::SetSemaphores(&device.imageAvailable[currentFrame], 
								   &gBuffer.semaphores[currentFrame]);
		RenderQueue::Submit(device);
		// ----------------------
		// FSQ Pass
		// ----------------------
		RenderQueue::SetCommandBuffers(&fsq.drawBuffers[device.imageIndex]);
		RenderQueue::SetSyncChain(&fsq.semaphores[currentFrame]);
		RenderQueue::Submit(device);
		// ----------------------
		// Depth Copy Forward Pass
		// ----------------------
		RenderQueue::SetCommandBuffers(&depthCopyCmd1[device.imageIndex]);
		RenderQueue::SetSyncChain(&depthCopySem1[currentFrame]);
		RenderQueue::Submit(device);
		// ----------------------
		// Forward Pass
		// ----------------------
		RenderQueue::SetCommandBuffers(&device.drawBuffers[device.imageIndex]);
		RenderQueue::SetSyncChain(&device.renderFinished[currentFrame]);
		RenderQueue::Submit(device);
		// ----------------------
		// Depth Copy Debug Pass
		// ----------------------
		RenderQueue::SetCommandBuffers(&depthCopyCmd2[device.imageIndex]);
		RenderQueue::SetSyncChain(&depthCopySem2[currentFrame]);
		RenderQueue::Submit(device);
		// ----------------------
		// Debug Line List Pass
		// ----------------------
		RenderQueue::SetCommandBuffers(&debugLineList.drawBuffers[device.imageIndex]);
		RenderQueue::SetSyncChain(&debugLineList.semaphores[currentFrame]);
		RenderQueue::Submit(device);
		// ----------------------
		// Debug Line Strip Pass
		// ----------------------
		vk::CommandBuffer commandBuffersDebugLineStrip[2] = {
			debugLineStrip.drawBuffers[device.imageIndex],
			overlay.commandBuffers[device.imageIndex]
		};
		RenderQueue::SetCommandBuffers(commandBuffersDebugLineStrip, 2);
		RenderQueue::SetSyncChain(&debugLineStrip.semaphores[currentFrame]);
		RenderQueue::Submit(device, device.drawFences[currentFrame]);

		if (RenderQueue::End(device, currentFrame))
			OnSurfaceRecreate();

		currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
	}



	void UpdateObjects(float dt)
	{
		// UPDATE LIGHTS
		// White
		uboComposition.lights[0].position = glm::vec4(0.0f, 20.0f, 1.0f, 0.0f);
		// uboComposition.lights[0].color = glm::vec3(1.5f);
		uboComposition.lights[0].color = glm::vec3(1.0f);
		uboComposition.lights[0].radius = 200.0f;
		//// Red
		//uboComposition.lights[1].position = glm::vec4(-2.0f, 5.0f, 0.0f, 0.0f);
		//// uboComposition.lights[1].color = glm::vec3(1.0f, 0.0f, 0.0f);
		//uboComposition.lights[1].color = glm::vec3(1.0f, 1.0f, 1.0f);
		//uboComposition.lights[1].radius = 150.0f;
		//// Blue
		//uboComposition.lights[2].position = glm::vec4(2.0f, -10.0f, 0.0f, 0.0f);
		//// uboComposition.lights[2].color = glm::vec3(0.0f, 0.0f, 2.5f);
		//uboComposition.lights[2].color = glm::vec3(1.0f, 1.0f, 1.0f);
		//uboComposition.lights[2].radius = 50.0f;
		//// Yellow
		//uboComposition.lights[3].position = glm::vec4(0.0f, -9.f, 0.5f, 0.0f);
		//// uboComposition.lights[3].color = glm::vec3(1.0f, 1.0f, 0.0f);
		//uboComposition.lights[3].color = glm::vec3(1.0f, 1.0f, 1.0f);
		//uboComposition.lights[3].radius = 20.0f;
		//// Green
		//uboComposition.lights[4].position = glm::vec4(0.0f, 50.0f, 0.0f, 0.0f);
		//// uboComposition.lights[4].color = glm::vec3(0.0f, 1.0f, 0.2f);
		//uboComposition.lights[4].color = glm::vec3(0.0f, 1.0f, 0.2f);
		//uboComposition.lights[4].radius = 50.0f;
		//// Yellow
		//uboComposition.lights[5].position = glm::vec4(0.0f, 3.0f, 0.0f, 0.0f);
		//// uboComposition.lights[5].color = glm::vec3(1.0f, 0.7f, 0.3f);
		//uboComposition.lights[5].color = glm::vec3(1.0f, 1.0f, 1.0f);
		//uboComposition.lights[5].radius = 75.0f;

		static float timer = 0.0f;
		float lightMovementMod = 0.2f;
		//uboComposition.lights[0].position.x = sin(glm::radians(360.0f * timer * lightMovementMod)) * 30.0f;
		//uboComposition.lights[0].position.z = cos(glm::radians(360.0f * timer * lightMovementMod)) * 30.0f;

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

		uboViewProjection.view = camera.matrices.view;

		//auto& sphere1 = objects[0];
		//auto& sphere2 = objects[1];
		//sphere1.SetPosition(glm::vec3(-20.0f, 10.0f * (glm::sin(timer * 2.0f) + 0.5f) * 0.75f, 20.0f));
		//sphere2.SetPosition(glm::vec3(-20.0f, -10.0f * (glm::sin(timer * 2.5f) + 0.5f) * 0.75f, 20.0f));

		//auto& box1 = objects[2];
		//auto& sphere3 = objects[3];
		//box1.SetPosition(glm::vec3(-10.0f, 10.0f * (glm::sin(timer * 2.0f) + 0.5f) * 0.75f, 20.0f));
		//sphere3.SetPosition(glm::vec3(-10.0f, -10.0f * (glm::sin(timer * 2.5f) + 0.5f) * 0.5f, 20.0f));

		//auto& box2 = objects[4];
		//auto& box3 = objects[5];
		//box2.SetPosition(glm::vec3(-0.0f, 10.0f * (glm::sin(timer * 2.0f) + 0.5f) * 0.75f, 20.0f));
		//box3.SetPosition(glm::vec3(-0.0f, -10.0f * (glm::sin(timer * 2.5f) + 0.5f) * 0.75f, 20.0f));

		//auto& plane1 = objects[6];
		//auto& box4 = objects[7];
		//plane1.SetPosition(glm::vec3(10.0f, 5.0f * (glm::sin(timer * 0.75f)), 20.0f));
		//box4.SetPosition(glm::vec3(10.0f, -10.0f * (glm::sin(timer * 0.5f)), 20.0f));
		//
		//auto& plane2 = objects[8];
		//auto& sphere4 = objects[9];
		//plane2.SetPosition(glm::vec3(20.0f, 5.0f * (glm::sin(timer * 0.75f)), 20.0f));
		//sphere4.SetPosition(glm::vec3(20.0f, -10.0f * (glm::sin(timer * 0.5f)), 20.0f));

		//auto& point1 = objects[10];
		//auto& box5 = objects[11];
		//point1.SetPosition(glm::vec3(-20.0f, 5.0f * (glm::sin(timer * 0.75f)), 0.0f));
		//box5.SetPosition(glm::vec3(-20.0f, -10.0f * (glm::sin(timer * 0.5f)), 0.0f));

		//auto& point2 = objects[12];
		//auto& sphere5 = objects[13];
		//point2.SetPosition(glm::vec3(-10.0f, 5.0f * (glm::sin(timer * 1.5f)), 0.0f));
		//sphere5.SetPosition(glm::vec3(-10.0f, -10.0f * (glm::sin(timer * 1.1f)), 0.0f));

		//auto& point3 = objects[14];
		//auto& plane3 = objects[15];
		//point3.SetPosition(glm::vec3(0.0f, 5.0f * (glm::sin(timer * 0.75f)), 0.0f));
		//plane3.SetPosition(glm::vec3(0.0f, -10.0f * (glm::sin(timer * 0.5f)), 0.0f));

		//auto& ray1 = objects[16];
		//auto& box6 = objects[17];
		//ray1.SetPosition(glm::vec3(10.0f, 5.0f * (glm::sin(timer * 0.75f)), 0.0f));
		//box6.SetPosition(glm::vec3(10.0f, -10.0f * (glm::sin(timer * 1.0f)), 0.0f));

		//auto& ray2 = objects[18];
		//auto& sphere6 = objects[19];
		//ray2.SetPosition(glm::vec3(20.0f, 5.0f * (glm::sin(timer * 0.75f)), 0.0f));
		//sphere6.SetPosition(glm::vec3(20.0f, -10.0f * (glm::sin(timer * 1.0f)), 0.0f));

		//auto& ray3 = objects[20];
		//auto& plane4 = objects[21];
		//ray3.SetPosition(glm::vec3(-20.0f, 5.0f * (glm::sin(timer * 1.25f)), -10.0f));
		//plane4.SetPosition(glm::vec3(-20.0f, -10.0f * (glm::sin(timer * 1.0f)), -10.0f));




		timer += dt;
	}

	void MapUBO(uint32_t imageIndex)
	{
		uniformBufferViewProjection[imageIndex].MapToBuffer(&uboViewProjection);
		uniformBufferComposition[imageIndex].MapToBuffer(&uboComposition);
	}


	void UpdateInput(float dt)
	{
		auto win = static_cast<Window*>(window.lock().get())->GetHandle();
		if (glfwGetKey(win, GLFW_KEY_ESCAPE))
			glfwSetWindowShouldClose(win, 1);

		static float lockCooldown = 1.0f;
		static float lockTimer = 1.0f;
		if (glfwGetKey(win, GLFW_KEY_SPACE) && lockTimer > lockCooldown)
		{
			cursorActive = !cursorActive;
			glfwSetInputMode(win, GLFW_CURSOR,
							 cursorActive ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
			lockTimer = 0.0f;
		}
		static float sphereCooldown = 1.0f;
		static float sphereTimer = 1.0f;
		camera.ProcessKeyboardInput(win);
		//if (glfwGetKey(win, GLFW_KEY_SPACE) && sphereTimer > sphereCooldown)
		//{
		//	auto& reg = ECS::Get();
		//	auto& spherePhysics = reg.get<PhysicsComponent>(sphere);
		//	auto& sphereTransform = reg.get<TransformComponent>(sphere);
		//	spherePhysics.velocity = camera.camFront * glm::vec3(-1.0f, -1.0f, -1.0f) * sphereSpeed;
		//	sphereTransform.SetPosition(camera.position * glm::vec3(-1.0f, -1.0f, -1.0f));
		//	sphereTimer = 0.0f;
		//}
		lockTimer += dt;
		sphereTimer += dt;
	}


	void InitializeDebugMesh()
	{
		//if (debugLineList.render != true) return;

		//std::vector<PosVertex> debugVertices;

		//size_t vertexCount = 0;
		//for (auto& object : objects)
		//{
		//	vertexCount += object.mesh.GetVertexCount();
		//}

		//debugVertices.resize(vertexCount * 2);
		//size_t vertIndex = 0;
		//for (auto& object : objects)
		//{
		//	auto vertices = object.mesh.GetVertexBufferData();

		//	for (int i = 0; i < object.mesh.vertexBuffer.GetVertexCount(); ++i, vertIndex += 2)
		//	{
		//		auto worldSpacePosition = (glm::vec3)(object.GetModel() * glm::vec4(vertices[i].pos, 1.0f));
		//		debugVertices[vertIndex] = worldSpacePosition;
		//		debugVertices[vertIndex+1] = worldSpacePosition + vertices[i].normal*debugLineList.lineLength;
		//	}
		//}

		//debugLineList.mesh.UpdateDynamic(debugVertices);
	}
	void Update() override
	{
		RenderingContext::Update();
		overlay.Begin();

		InitializeDebugMesh();
		static float speed = 90.0f;
		UpdateInput(dt);
		UpdateObjects(dt);
		bsp.Update(dt);

		//static auto sphereBox = ECS::Get().get<DebugRenderComponent>(sphere).mesh.GetBoundingBox();
		//static glm::vec3 localPosition = sphereBox.position;
		//static glm::vec3 localScale = sphereBox.halfExtent;
		//auto* sphereTransform = &ECS::Get().get<TransformComponent>(sphere);
		//auto sphereScale = glm::scale(utils::identity, sphereTransform->GetScale());
		//auto sphereTranslate = glm::translate(utils::identity, sphereTransform->GetPosition());

		//sphereBox.position = sphereTranslate * glm::vec4(localPosition, 1.0f);
		//sphereBox.halfExtent = sphereScale * glm::vec4(localScale, 1.0f);
		//if (octree.CollisionTest(sphereBox, sphereTransform->model))
		//	ECS::Get().get<PhysicsComponent>(sphere).velocity = glm::vec3(0.0f);

		camera.Update(dt, !cursorActive);


		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(0, 0));
		ImGui::Begin("Scene Settings", NULL);

		if (ImGui::TreeNode("Camera"))
		{
			ImGui::InputFloat3("Camera Position", &camera.position[0]);
			ImGui::InputFloat("Pitch", &camera.pitch);
			ImGui::InputFloat("Yaw", &camera.yaw);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Lights"))
		{
			ImGui::SliderFloat("Light Strength", &uboComposition.globalLightStrength, 0.01f, 5.0f);
			ImGui::TreePop();
		}
		//ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen);

		if (ImGui::TreeNode("Deferred & Forward Targets"))
		{
			ImGui::Checkbox("Render Deferred Pass", &gBuffer.render);
			ImGui::Checkbox("Copy Depth", &copyDepth);
			ImGui::Checkbox("Wireframe Toggle", &gBuffer.wireframeEnabled);
			ImGui::Text("Debug Target: ");
			for (int i = 0; i < 3; ++i)
			{
				ImGui::RadioButton(std::to_string(i).c_str(), &debugDisplayTarget, i);
				if (i != 2) ImGui::SameLine();
			}
			//static float color[4];
			//ImGui::ColorPicker4("Clear Color", color);
			//for(int i = 0; i < 4; ++i)
			//	clearColor[i] = color[i] / 255.0f;

			ImGui::TreePop();
		}

		if(ImGui::TreeNode("Octree Settings"))
		{
			static int depth = 1;
			static glm::vec3 position = glm::vec3(0.0f);
			static float halfExtent = 10.0f;

			ImGui::InputInt("Minimum Triangles", (int*)&Octree::MinimumTriangles);
			//ImGui::SliderInt("Depth", &depth, 0, 6);
			ImGui::InputFloat3("Position", &position[0]);
			ImGui::InputFloat("Half-Extent", &halfExtent);
			ImGui::Checkbox("Display Cells", &debugLineList.render);
			ImGui::Checkbox("Display Objects", &debugLineStrip.render);

			if (!octree.IsInitialized())
			{
				if (ImGui::Button("Create"))
				{
					octree.Create(position, halfExtent, depth, device);
				}
			}
			else
			{
				if (ImGui::Button("Destroy"))
				{
					octree.Destroy();
				}
			}

			ImGui::TreePop();
		}

		//if(ImGui::TreeNode("BSP Settings"))
		//{
		//	static int depth = 1;

		//	ImGui::InputInt("Minimum Triangles", (int*)&BSP::MinimumTriangles);
		//	ImGui::SliderFloat("Split Blend", &BSP::SplitBlend, 0.0f, 1.0f);
		//	ImGui::SliderInt("Plane Samples", (int*)&BSP::PlaneSamples, 1, 32);
		//	ImGui::Checkbox("Display Objects", &debugLineStrip.render);

		//	if (!bsp.IsInitialized())
		//	{
		//		if (ImGui::Button("Create"))
		//		{
		//			bsp.Create(depth, device);
		//		}
		//	}
		//	else
		//	{
		//		if (ImGui::Button("Destroy"))
		//		{
		//			bsp.Destroy();
		//		}
		//	}

		//	ImGui::TreePop();
		//}

		if(ImGui::TreeNode("Sphere Collider Settings"))
		{
			static float scale = 1.0f;

			ImGui::InputFloat("Scale", &scale);
			ImGui::InputFloat("Speed", &sphereSpeed);
			//ImGui::Checkbox("Display Cells", &debugLineList.render);
			//ImGui::Checkbox("Display Objects", &debugLineStrip.render);
			ECS::Get().get<TransformComponent>(sphere).SetScale(glm::vec3(scale));

			ImGui::TreePop();
		}


		if (ImGui::TreeNode("Model Loader"))
		{
			if(ImGui::TreeNode("Power Plant Sections"))
			{
				static bool sectionLoaded[20] = { false };
				bool fullyLoaded = true;
				for (bool loaded : sectionLoaded) fullyLoaded &= loaded;
				if (!fullyLoaded && ImGui::Button("Load All Sections"))
				{
					std::memset(sectionLoaded, true, sizeof(bool) * 20);
					LoadSection(-1);
				}
				for (int i = 0; i < 20; ++i)
				{
					if (sectionLoaded[i] == false)
					{
						if (ImGui::Button(std::string("Load Section" + std::to_string(i + 1)).c_str()))
						{
							LoadSection(i);
							sectionLoaded[i] = true;
						}
					}
				}
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}



		ImVec2 size = ImGui::GetWindowSize();
		ImGui::End();

		ImGui::SetNextWindowPos(ImVec2(0, size.y));

		overlay.Update(dt);
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


