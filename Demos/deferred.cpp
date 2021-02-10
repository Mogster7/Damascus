
#include "RenderingContext_Impl.h"
#include "deferred.h"

#include "Window.h"
#include "glfw3.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "Camera/Camera.h"
#include "glm/glm.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

constexpr glm::uvec2 FB_SIZE = { 1600, 900 }; 


class DeferredRenderingContext_Impl : public RenderingContext_Impl
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


	const std::vector<ColorVertex> cubeVerts = {
		{ { -1.0, -1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f } },
		{ { 1.0f, -1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },

		{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f } },
		{ { -1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f } },

		{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ { 1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f } },

		{ { 1.0f, 1.0f, -1.0f }, { 1.0f, 1.0f, 0.0f } },
		{ { -1.0f, 1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },
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
    DescriptorSetLayout descriptorSetLayout;
    vk::PushConstantRange pushRange;

    DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> descriptorSets;

    std::vector<Buffer> uniformBufferModel;
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
		std::vector<vk::CommandBuffer> drawBuffers;
		std::vector<Semaphore> semaphores;
	} gBuffer;

	struct FSQ
	{
		RenderPass renderPass;
		GraphicsPipeline pipeline;
		PipelineLayout pipelineLayout;

		std::vector<FrameBuffer> frameBuffers;
		std::vector<vk::CommandBuffer> drawBuffers;
		std::vector<Semaphore> semaphores;
		Mesh<TexVertex> mesh;
	} fsq;

	
	// TODO: MOVE THESE OUT
    DescriptorPool descriptorPoolUI;
	RenderPass renderPassUI;
	CommandPool commandPoolUI;
	std::vector<vk::CommandBuffer> commandBuffersUI;
	std::vector<FrameBuffer> framebuffersUI;

	std::array<float, 4> clearColor = {0.0f, 0.0f, 0.0f, 0.0f};

	bool cursorActive = false;

	void SetCursorCallback(bool set)
	{
		auto win = ((Window*)window.lock().get())->GetHandle();
		static auto cursorPosCallback = [](GLFWwindow* window, double xPos, double yPos)
		{
			auto context = reinterpret_cast<DeferredRenderingContext_Impl*>(glfwGetWindowUserPointer(window));
			context->camera.ProcessMouseMovement({ xPos, yPos });
		};
		if (set)
			glfwSetCursorPosCallback(win, cursorPosCallback);
		else 
			glfwSetCursorPosCallback(win, nullptr);
	}



	void Initialization()
	{
		auto win = ((Window*)window.lock().get())->GetHandle();
		// glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetWindowUserPointer(win, this);

		SetCursorCallback(true);
		
		camera.flipY = false;
		camera.SetPerspective(45.0f, (float)FB_SIZE.x / FB_SIZE.y, 0.1f, 100.0f);
		// uboViewProjection.projection = glm::perspective(glm::radians(45.0f), (float)extent.width / extent.height, 0.1f, 100.0f);
		uboViewProjection.projection = camera.matrices.perspective;
		uboViewProjection.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
		// // Invert up direction so that pos y is up 
		// // its down by default in Vulkan
		uboViewProjection.projection[1][1] *= -1;
		

		
    
		 //objects.emplace_back().Create(device, cubeVerts, cubeIndices);
		 //objects.emplace_back().Create(device, meshVerts, squareIndices);
		 fsq.mesh.Create(device, meshVerts, meshIndices);
		// objects[0].SetModel(
		// 	glm::translate(objects[0].GetModel(), glm::vec3(0.0f, 0.0f, -3.0f))
		// 	* glm::rotate(objects[0].GetModel(), glm::radians(-90.0f), glm::vec3(1.0, 0.0f, 0.0f)));
		// objects[1].SetModel(glm::translate(objects[1].GetModel(), glm::vec3(0.0f, 0.0f, -2.5f)));


		InitializeAttachments();
		//InitializeAssets();
		forwardObjects.emplace_back().CreateModel(std::string(ASSET_DIR) + "Models/viking_room.obj", device);
		objects.emplace_back().CreateModel(std::string(ASSET_DIR) + "Models/viking_room.obj", device);
		objects.emplace_back().CreateModel(std::string(ASSET_DIR) + "Models/viking_room.obj", device);
		objects[0].SetModel(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 2.0f)));
		objects[1].SetModel(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.0f, 3.0f)));
		testTex.Create(std::string(ASSET_DIR) + "Textures/viking_room.png", device);
		InitializeUniformBuffers();
		InitializeDescriptorSetLayouts();
		InitializeDescriptorPool();
		InitializeDescriptorSets();
		InitializePipelines();
		InitializeUI();
	}

	void Destroy()
	{
		descriptorPool.Destroy();
		descriptorSetLayout.Destroy();

		for (size_t i = 0; i < device.swapchain.images.size(); ++i)
		{
			uniformBufferViewProjection[i].Destroy();
		}

		for (size_t i = 0; i < objects.size(); ++i)
			objects[i].Destroy();

		// UI
		// -------------
		for (auto framebuffer : framebuffersUI)
			framebuffer.Destroy();

		renderPassUI.Destroy();
		device.freeCommandBuffers(commandPoolUI.Get(),
								  commandBuffersUI.size(),
								  commandBuffersUI.data());
		commandPoolUI.Destroy();
		ImGui_ImplVulkan_Shutdown();
		ImGui::DestroyContext();
		descriptorPoolUI.Destroy();
		// --------------
	}

	

	void InitializeUI()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForVulkan(static_cast<Window*>(window.lock().get())->GetHandle(), true);
		ImGui_ImplVulkan_InitInfo initInfo = {};

		initInfo.Instance = instance;
		initInfo.PhysicalDevice = physicalDevice;
		initInfo.Device = device;
		initInfo.QueueFamily = 0;
		initInfo.Queue = device.graphicsQueue;
		initInfo.PipelineCache = VK_NULL_HANDLE;
		initInfo.DescriptorPool = descriptorPoolUI;
		initInfo.Allocator = nullptr;
		initInfo.MinImageCount = device.swapchain.images.size();
		initInfo.ImageCount = device.swapchain.images.size();
		initInfo.CheckVkResultFn = utils::AssertVkBase;

		vk::AttachmentDescription attachment = {};
		attachment.format = device.swapchain.imageFormat;
		attachment.samples = vk::SampleCountFlagBits::e1;
		attachment.loadOp = vk::AttachmentLoadOp::eLoad;
		attachment.storeOp = vk::AttachmentStoreOp::eStore;
		attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
		attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

		vk::AttachmentReference colorAttach = {};
		colorAttach.attachment = 0;
		colorAttach.layout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::SubpassDescription subpass = {};
		subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttach;

		vk::SubpassDependency dependency = {};
		dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
		dependency.dstSubpass = 0;
		dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

		vk::RenderPassCreateInfo createInfo = {};
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &attachment;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;
		renderPassUI.Create(createInfo, device);

		ImGui_ImplVulkan_Init(&initInfo, renderPassUI.Get());


		// COMMAND POOL 
		vk::CommandPoolCreateInfo poolInfo = {};
		poolInfo.queueFamilyIndex = physicalDevice.GetQueueFamilyIndices().graphics.value();
		poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		commandPoolUI.Create(commandPoolUI, poolInfo, device);

		// COMMAND BUFFER
		commandBuffersUI.resize(device.swapchain.imageViews.size());

		vk::CommandBufferAllocateInfo bufferInfo = {};
		bufferInfo.level = vk::CommandBufferLevel::ePrimary;
		bufferInfo.commandPool = commandPoolUI.Get();
		bufferInfo.commandBufferCount = commandBuffersUI.size();
		utils::CheckVkResult(
			device.allocateCommandBuffers(&bufferInfo, commandBuffersUI.data()),
			"Failed to allocate ImGui command buffers");

		
		auto cmdBuf = commandPoolUI.BeginCommandBuffer();
		ImGui_ImplVulkan_CreateFontsTexture(cmdBuf.get());
		device.commandPool.EndCommandBuffer(cmdBuf.get());

		{
			vk::ImageView attachment[1];
			vk::FramebufferCreateInfo info = {};
			info.renderPass = renderPassUI;
			info.attachmentCount = 1;
			info.pAttachments = attachment;
			auto extent = device.swapchain.GetExtentDimensions();
			info.width = extent.x;
			info.height = extent.y;
			info.layers = 1;

			framebuffersUI.resize(device.swapchain.images.size());

			for(uint32_t i = 0; i < device.swapchain.images.size(); ++i)
			{
				auto view = device.swapchain.imageViews[i];
				attachment[0] = view.Get();
				framebuffersUI[i].Create(framebuffersUI[i], info, device);
			}
		}
			
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
					attachmentDescs[i].initialLayout = vk::ImageLayout::eUndefined;
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

			gBuffer.renderPass.Create(rpInfo, device);
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

			vk::CommandBufferAllocateInfo cmdInfo = {};
			cmdInfo.commandPool = device.commandPool.Get();
			cmdInfo.level = vk::CommandBufferLevel::ePrimary;
			cmdInfo.commandBufferCount = static_cast<uint32_t>(imageViewCount);

			utils::CheckVkResult(
				device.allocateCommandBuffers(&cmdInfo, gBuffer.drawBuffers.data()),
				"Failed to allocate deferred command buffers"
			);
		}
		// Semaphores
		{
			gBuffer.semaphores.resize(MAX_FRAME_DRAWS);
			vk::SemaphoreCreateInfo semInfo = {};
			for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
			{
				utils::CheckVkResult(
					device.createSemaphore(&semInfo, nullptr, &gBuffer.semaphores[i]),
					"Failed to allocate deferred semaphore"
				);
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

				// Framebuffer data will be stored as an image, but images can be given different data layouts
				// to give optimal use for certain operations
				colorAttachment.initialLayout = vk::ImageLayout::eUndefined;			// Image data layout before render pass starts
				colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;		// Image data layout after render pass (to change to)

				std::array<vk::AttachmentDescription, 1> attachmentsDescFSQ = { colorAttachment };

				// REFERENCES
				// Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
				vk::AttachmentReference colorAttachmentRef;
				colorAttachmentRef.attachment = 0;
				colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

				vk::SubpassDescription subpass = {};
				subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
				subpass.colorAttachmentCount = 1;
				subpass.pColorAttachments = &colorAttachmentRef;
				subpass.pDepthStencilAttachment = nullptr;


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
				dependency.dependencyFlags = {};

				// Operations to wait on and what stage they occur
				// Wait on swap chain to finish reading from the image to access it
				dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				dependency.srcAccessMask = {};

				// Operations that should wait on the above operations to finish are in the color attachment writing stage
				dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

				vk::RenderPassCreateInfo createInfo;
				createInfo.attachmentCount = static_cast<uint32_t>(attachmentsDescFSQ.size());
				createInfo.pAttachments = attachmentsDescFSQ.data();
				createInfo.subpassCount = 1;
				createInfo.pSubpasses = &subpass;
				createInfo.dependencyCount = 1;
				createInfo.pDependencies = &dependency;

				fsq.renderPass.Create(createInfo, device);
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
						imageViews[i].Get(),
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
				utils::CheckVkResult(
					device.createSemaphore(&semInfo, nullptr, &fsq.semaphores[i]),
					"Failed to allocate deferred semaphore"
				);
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
			int i = 0;
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
				objects.back().Create(device, data.vertices, data.indices);
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

	void InitializeDescriptorSetLayouts()
	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings = {
			// Binding 0: ViewProjection
			DescriptorSetLayoutBinding::Create(vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, 0),
			// Binding 1-3: G-Buffer
			DescriptorSetLayoutBinding::Create(vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 1),
			DescriptorSetLayoutBinding::Create(vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 2),
			DescriptorSetLayoutBinding::Create(vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 3),
			// Binding 4: Composition / Lights
			DescriptorSetLayoutBinding::Create(vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 4),
			// Binding 5: Texture Sampler
			DescriptorSetLayoutBinding::Create(vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 5)
		};

		vk::DescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();

		// Create descriptor set layout
		DescriptorSetLayout::Create(descriptorSetLayout, createInfo, device);
	}

	void InitializeDescriptorSets()
	{
		// TODO: ABSTRACT THIS TO NOT BE EXPLICIT
		size_t imageCount = device.swapchain.images.size();
		size_t descriptorCount = imageCount;
		// One descriptor set for every uniform buffer
		descriptorSets.resize(imageCount);

		// Create one copy of our descriptor set layout per buffer
		std::vector<vk::DescriptorSetLayout> setLayouts(descriptorCount, descriptorSetLayout.Get());

		vk::DescriptorSetAllocateInfo allocInfo = {};
		allocInfo.descriptorPool = descriptorPool.Get();
		// Number of sets to allocate
		allocInfo.descriptorSetCount = static_cast<uint32_t>(descriptorCount);
		// Layouts to use to allocate sets (1:1 relationship)
		allocInfo.pSetLayouts = setLayouts.data();

		utils::CheckVkResult(
			device.allocateDescriptorSets(
				&allocInfo,
				descriptorSets.data()),
			"Failed to allocate descriptor sets");


		// CONNECT DESCRIPTOR SET TO BUFFER
		// UPDATE ALL DESCRIPTOR SET BINDINGS
		std::array<vk::WriteDescriptorSet, 6> setWrite = {};

		// Update all descriptor set buffer bindings
		for (size_t i = 0; i < descriptorCount; ++i)
		{
			setWrite = {
			WriteDescriptorSet::Create(
				descriptorSets[i],
				vk::DescriptorType::eUniformBuffer,
				0,
				uniformBufferViewProjection[i].descriptorInfo),

			WriteDescriptorSet::Create(
				descriptorSets[i],
				vk::DescriptorType::eCombinedImageSampler,
				1,
				gBuffer.position.GetDescriptor(vk::ImageLayout::eShaderReadOnlyOptimal)),

			WriteDescriptorSet::Create(
				descriptorSets[i],
				vk::DescriptorType::eCombinedImageSampler,
				2,
				gBuffer.normal.GetDescriptor(vk::ImageLayout::eShaderReadOnlyOptimal)),

			WriteDescriptorSet::Create(
				descriptorSets[i],
				vk::DescriptorType::eCombinedImageSampler,
				3,
				gBuffer.albedo.GetDescriptor(vk::ImageLayout::eShaderReadOnlyOptimal)),

			WriteDescriptorSet::Create(
				descriptorSets[i],
				vk::DescriptorType::eUniformBuffer,
				4,
				uniformBufferComposition[i].descriptorInfo),

			WriteDescriptorSet::Create(
				descriptorSets[i],
				vk::DescriptorType::eCombinedImageSampler,
				5,
				testTex.GetDescriptor(vk::ImageLayout::eShaderReadOnlyOptimal))
			};


			device.updateDescriptorSets(
				static_cast<uint32_t>(setWrite.size()),
				setWrite.data(),
				0,
				nullptr);
		};

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
		layout.pSetLayouts = &descriptorSetLayout;
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


	}

	void InitializeDescriptorPool()
	{
		uint32_t imageCount = device.swapchain.images.size();
		// How many descriptors, not descriptor sets

		vk::DescriptorPoolSize pool_sizes[] =
		{
			{ vk::DescriptorType::eSampler, 1000 },
			{ vk::DescriptorType::eCombinedImageSampler, 1000 },
			{ vk::DescriptorType::eSampledImage, 1000 },
			{ vk::DescriptorType::eStorageImage, 1000 },
			{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
			{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
			{ vk::DescriptorType::eUniformBuffer, 1000 },
			{ vk::DescriptorType::eStorageBuffer, 1000 },
			{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
			{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
			{ vk::DescriptorType::eInputAttachment, 1000 }
		};

		vk::DescriptorPoolCreateInfo pool_info = {};
		pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
		pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
		pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;


		// VP UBO
		std::array<vk::DescriptorPoolSize, 2> poolSizes;
		poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
		poolSizes[0].descriptorCount = imageCount * 2;

		// Texture Sampler
		poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
		poolSizes[1].descriptorCount = imageCount * 3;

		// Pool creation
		vk::DescriptorPoolCreateInfo poolCreateInfo = {};
		// Maximum number of descriptor sets that can be created from the pool
		poolCreateInfo.maxSets = static_cast<uint32_t>(device.swapchain.images.size());
		// Number of pool sizes being passed 
		poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolCreateInfo.pPoolSizes = poolSizes.data();

		// Create descriptor pool
		DescriptorPool::Create(descriptorPool, poolCreateInfo, device);
		DescriptorPool::Create(descriptorPoolUI, poolCreateInfo, device);
	}

	void PrepareCommandBuffersDeferred(uint32_t imageIndex)
	{
		std::array<vk::ClearValue, 4> clearValues = {};
		clearValues[0].color = vk::ClearColorValue(clearColor);
		clearValues[1].color = vk::ClearColorValue(clearColor);
		clearValues[2].color = vk::ClearColorValue(clearColor);
		clearValues[3].depthStencil = vk::ClearDepthStencilValue(1.0f);

		vk::RenderPassBeginInfo renderPassInfo;
		renderPassInfo.renderPass = gBuffer.renderPass;
		renderPassInfo.renderArea.extent = vk::Extent2D(FB_SIZE.x, FB_SIZE.y);
		renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		// Record command buffers
		vk::CommandBufferBeginInfo info(
			{},
			nullptr
		);

		//// Transition to read from gBuffer
		//gBuffer.depth.image.TransitionLayout(
		//	vk::ImageLayout::eTransferSrcOptimal,
		//	vk::ImageLayout::eDepthStencilAttachmentOptimal,
		//	vk::ImageAspectFlagBits::eDepth
		//);


		auto& cmdBuf = gBuffer.drawBuffers[imageIndex];

		utils::CheckVkResult(cmdBuf.begin(&info), "Failed to begin recording command buffer");
		renderPassInfo.framebuffer = gBuffer.frameBuffers[imageIndex].Get();

		cmdBuf.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
		cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, gBuffer.pipeline.Get());

		for (size_t j = 0; j < objects.size(); ++j)
		{
			const auto& mesh = objects[j];
			// Buffers to bind
			vk::Buffer vertexBuffers[] = { mesh.GetVertexBuffer() };
			// Offsets
			vk::DeviceSize offsets[] = { 0 };

			cmdBuf.bindVertexBuffers(0, 1, vertexBuffers, offsets);
			bool hasIndex = mesh.GetIndexCount() > 0;
			if (hasIndex)
				cmdBuf.bindIndexBuffer(mesh.GetIndexBuffer(), 0, vk::IndexType::eUint32);

			cmdBuf.pushConstants(
				gBuffer.pipelineLayout.Get(),
				// Stage
				vk::ShaderStageFlagBits::eVertex,
				// Offset
				0,
				// Size of data being pushed
				sizeof(glm::mat4),
				// Actual data being pushed
				&objects[j].GetModel()
			);

			//// Bind descriptor sets
			cmdBuf.bindDescriptorSets(
				// Point of pipeline and layout
				vk::PipelineBindPoint::eGraphics,
				gBuffer.pipelineLayout.Get(),
				0,
				1,
				&descriptorSets[imageIndex], // 1 to 1 with command buffers
				0,
				nullptr
			);

			// Execute pipeline
			hasIndex ?
				cmdBuf.drawIndexed(mesh.GetIndexCount(), 1, 0, 0, 0)
				:
				cmdBuf.draw(mesh.GetVertexCount(), 1, 0, 0);
		}


		cmdBuf.endRenderPass();
		cmdBuf.end();

		// Transition to read from gBuffer
		gBuffer.depth.image.TransitionLayout(
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageLayout::eTransferSrcOptimal,
			vk::ImageAspectFlagBits::eDepth
		);

		device.waitIdle();
	}

	void PrepareCommandBuffersFSQ(uint32_t imageIndex)
	{
		std::array<vk::ClearValue, 1> clearValues = {};
		clearValues[0].color = vk::ClearColorValue(clearColor);

		vk::RenderPassBeginInfo renderPassInfo;
		renderPassInfo.renderPass = fsq.renderPass;
		renderPassInfo.renderArea.extent = device.swapchain.extent;
		renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		// Record command buffers
		vk::CommandBufferBeginInfo info(
			{},
			nullptr
		);

		auto& cmdBuf = fsq.drawBuffers[imageIndex];

		utils::CheckVkResult(cmdBuf.begin(&info), "Failed to begin recording command buffer");
		renderPassInfo.framebuffer = fsq.frameBuffers[imageIndex].Get();

		cmdBuf.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
		cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, fsq.pipeline.Get());

		//for (size_t j = 0; j < objects.size(); ++j)
		//{
		const auto& mesh = fsq.mesh;
		// Buffers to bind
		vk::Buffer vertexBuffers[] = { mesh.GetVertexBuffer() };
		// Offsets
		vk::DeviceSize offsets[] = { 0 };

		cmdBuf.bindVertexBuffers(0, 1, vertexBuffers, offsets);
		bool hasIndex = mesh.GetIndexCount() > 0;
		if (hasIndex)
			cmdBuf.bindIndexBuffer(mesh.GetIndexBuffer(), 0, vk::IndexType::eUint32);


		//// Bind descriptor sets
		cmdBuf.bindDescriptorSets(
			// Point of pipeline and layout
			vk::PipelineBindPoint::eGraphics,
			fsq.pipelineLayout,
			0,
			1,
			&descriptorSets[imageIndex], // 1 to 1 with command buffers
			0,
			nullptr
		);

		// Execute pipeline
		hasIndex ?
			cmdBuf.drawIndexed(mesh.GetIndexCount(), 1, 0, 0, 0)
			:
			cmdBuf.draw(mesh.GetVertexCount(), 1, 0, 0);
		//}

		cmdBuf.endRenderPass();
		cmdBuf.end();

	}

	void PrepareCommandBuffersForward(uint32_t imageIndex)
	{
		std::array<vk::ClearValue, 2> clearValues = {};
		clearValues[0].color = vk::ClearColorValue(clearColor);
		clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f);

		vk::RenderPassBeginInfo renderPassInfo;
		auto& extent = device.swapchain.extent;
		renderPassInfo.renderPass = device.renderPass;
		renderPassInfo.renderArea.extent = extent;
		renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		// Record command buffers
		vk::CommandBufferBeginInfo info(
			{},
			nullptr
		);

		// Transition to use as depth buffer
		device.depthBuffer.image.TransitionLayout(
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageAspectFlagBits::eDepth
		);

		vk::ImageSubresourceRange range = vk::ImageSubresourceRange(
			 vk::ImageAspectFlagBits::eDepth,
			0, 1, 0, 1
		);
		auto imageCopyCmd = device.commandPool.BeginCommandBuffer();
		imageCopyCmd->clearDepthStencilImage(
			device.depthBuffer.image.Get(),
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
			imageCopyCmd->copyImage(
				gBuffer.depth.image.Get(), vk::ImageLayout::eTransferSrcOptimal,
				device.depthBuffer.image.Get(), vk::ImageLayout::eTransferDstOptimal,
				imageCopy);
		}
		device.commandPool.EndCommandBuffer(imageCopyCmd.get());

		// Transition to use as depth buffer
		device.depthBuffer.image.TransitionLayout(
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageAspectFlagBits::eDepth
		);

		auto& cmdBuf = device.drawBuffers[imageIndex];

		utils::CheckVkResult(cmdBuf.begin(&info), "Failed to begin recording command buffer");
		renderPassInfo.framebuffer = device.frameBuffers[imageIndex].Get();

		cmdBuf.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
		cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, device.pipeline.Get());
		for (size_t j = 0; j < forwardObjects.size(); ++j)
		{
			const auto& mesh = forwardObjects[j];
			// Buffers to bind
			vk::Buffer vertexBuffers[] = { mesh.GetVertexBuffer() };
			// Offsets
			vk::DeviceSize offsets[] = { 0 };

			cmdBuf.bindVertexBuffers(0, 1, vertexBuffers, offsets);
			bool hasIndex = mesh.GetIndexCount() > 0;
			if (hasIndex)
				cmdBuf.bindIndexBuffer(mesh.GetIndexBuffer(), 0, vk::IndexType::eUint32);

			cmdBuf.pushConstants(
				device.pipelineLayout.Get(),
				// Stage
				vk::ShaderStageFlagBits::eVertex,
				// Offset
				0,
				// Size of data being pushed
				sizeof(glm::mat4),
				// Actual data being pushed
				&mesh.GetModel()
			);

			//// Bind descriptor sets
			cmdBuf.bindDescriptorSets(
				// Point of pipeline and layout
				vk::PipelineBindPoint::eGraphics,
				device.pipelineLayout.Get(),
				0,
				1,
				&descriptorSets[imageIndex], // 1 to 1 with command buffers
				0,
				nullptr
			);

			// Execute pipeline
			hasIndex ?
				cmdBuf.drawIndexed(mesh.GetIndexCount(), 1, 0, 0, 0)
				:
				cmdBuf.draw(mesh.GetVertexCount(), 1, 0, 0);
		}

		cmdBuf.endRenderPass();
		cmdBuf.end();

		device.waitIdle();
	}



	void PrepareCommandBuffersUI(uint32_t imageIndex)
	{
		auto extent = device.swapchain.extent;
		vk::RenderPassBeginInfo info = { };
		info.renderPass = renderPassUI;
		info.framebuffer = framebuffersUI[imageIndex].Get();
		info.renderArea.extent = extent;
		info.clearValueCount = 1;
		auto clearColor = vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 1.0f, 1.0f });
		vk::ClearValue clearValue = {};
		clearValue.color = clearColor;
		info.pClearValues = &clearValue;

		// Record command buffers
		vk::CommandBufferBeginInfo cmdBufBeginInfo(
			{},
			nullptr
		);


		auto cmdBuf = commandBuffersUI[imageIndex];
		cmdBuf.begin(cmdBufBeginInfo);
		cmdBuf.beginRenderPass(info, vk::SubpassContents::eInline);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuf);
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


	void Draw()
	{
		device.waitIdle();
		if (device.PrepareFrame(currentFrame))
			OnSurfaceRecreate();

		PrepareCommandBuffersDeferred(device.imageIndex);
		PrepareCommandBuffersFSQ(device.imageIndex);
		PrepareCommandBuffersForward(device.imageIndex);
		PrepareCommandBuffersUI(device.imageIndex);

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
		// Forward Pass
		// ----------------------
		vk::CommandBuffer commandBuffersForward[2] = {
			device.drawBuffers[device.imageIndex],
			commandBuffersUI[device.imageIndex]
		};

		submitInfo.pWaitSemaphores = &fsq.semaphores[currentFrame];
		submitInfo.pSignalSemaphores = &device.renderFinished[currentFrame];
		submitInfo.commandBufferCount = 2;
		submitInfo.pCommandBuffers = commandBuffersForward;
		utils::CheckVkResult(
			device.graphicsQueue.submit(1,
										&submitInfo,
										device.drawFences[currentFrame].Get()),
			"Failed to submit draw queue"
		);


		if (device.SubmitFrame(currentFrame))
			OnSurfaceRecreate();

		currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
	}



	void UpdateUniformBuffers(float dt, const uint32_t imageIndex)
	{
		uniformBufferViewProjection[imageIndex].MapToBuffer(&uboViewProjection);

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

		camera.keys.up = (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS);
		camera.keys.down = (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS);
		camera.keys.left = (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS);
		camera.keys.right = (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS);
	}


	void Update(float dt)
	{
		// TODO: MOVE THIS OUT
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		static float speed = 90.0f;
		UpdateInput(dt);

		// const auto& model = objects[0].GetModel();
		// objects[0].SetModel(glm::rotate(model, glm::radians(speed * dt), { 0.0f, 0.0f,1.0f }));

		camera.Update(dt, !cursorActive);
		uboViewProjection.view = camera.matrices.view;
		// const auto& model2 = objects[1].GetModel();
		// objects[1].SetModel(glm::rotate(model2, glm::radians(speed2 * dt), { 0.0f, 1.0f,1.0f }));

		static glm::vec3 ppScale = { .0001f, .0001f, .0001f };
		ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::InputFloat3("Power Plant Scale", &ppScale[0], "%.9f");
		ImGui::SliderFloat4("Clear Color", clearColor.data(), 0.0f, 1.0f);
		ImGui::SliderFloat("Light Strength", &uboComposition.globalLightStrength, 0.01f, 5.0f);
		ImGui::Checkbox("Copy Depth", &copyDepth);

		/*for (auto& object : objects)
			object.SetModel(glm::scale(glm::mat4(1.0f), ppScale));*/

		ImGui::Text("Debug Target: ");
		for (int i = 0; i < 5; ++i)
		{
			ImGui::RadioButton(std::to_string(i).c_str(), &debugDisplayTarget, i);
			if (i != 4) ImGui::SameLine();
		}

		UpdateUniformBuffers(dt, device.imageIndex);
	}


	void DrawUI()
	{
		//ImGui::ShowDemoWindow();
		ImGui::Render();
	}
};




DeferredRenderingContext_Impl* DeferredRenderingContext::impl = {};
void DeferredRenderingContext::Initialize(std::weak_ptr<Window> window)
{
	impl = new DeferredRenderingContext_Impl();
	RenderingContext::impl = impl;
	impl->InitVulkan(window);
	impl->Initialization();
}

void DeferredRenderingContext::Update()
{
	RenderingContext::Update();

}


void DeferredRenderingContext::DrawFrame()
{
	impl->DrawUI();
	impl->Draw();
}
