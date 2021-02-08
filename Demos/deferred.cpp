
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



class DeferredRenderingContext_Impl : public RenderingContext_Impl
{
public:
	const std::vector<TexVertex> meshVerts = {
		{ { -1.0, 1.0, 0.0 },{ 0.0f, 1.0f } },	// 0
		{ { -1.0, -1.0, 0.0 },{ 0.0f, 0.0f } },	    // 1
		{ { 1.0, -1.0, 0.0 },{ 1.0f, 0.0f } },    // 2
		{ { 1.0, 1.0, 0.0 },{ 1.0f, 1.0f } },   // 3

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

	// struct {
	// 	struct {
	// 		vks::Texture2D colorMap;
	// 		vks::Texture2D normalMap;
	// 	} model;
	// 	struct {
	// 		vks::Texture2D colorMap;
	// 		vks::Texture2D normalMap;
	// 	} floor;
	// } textures;

	// struct {
	// 	vkglTF::Model model;
	// 	vkglTF::Model floor;
	// } models;

	Camera camera;

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
		int debugDisplayTarget = 0;
	} uboComposition;

	struct {
		// vks::Buffer offscreen;
		// vks::Buffer composition;
	} uniformBuffers;

	struct {
		GraphicsPipeline offscreen;
		GraphicsPipeline forward;
		GraphicsPipeline composition;
	} pipelines;
	PipelineLayout pipelineLayout;

	// struct {
	// 	vk::DescriptorSet model;
	// 	vk::DescriptorSet floor;
	// } descriptorSets;

	vk::DescriptorSet descriptorSet;
	// vk::DescriptorSetLayout descriptorSetLayout;

	struct
	{
		RenderPass forward;
	} renderPasses;

	// One sampler for the frame buffer color attachments
	vk::Sampler colorSampler;

    std::vector<Mesh<Vertex>> objects;

    // Descriptors
    DescriptorSetLayout descriptorSetLayout;
    vk::PushConstantRange pushRange;

    DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> descriptorSets;

    std::vector<Buffer> uniformBufferModel;
    std::vector<Buffer> uniformBufferViewProjection;
    std::vector<Buffer> uniformBufferComposition;

	// Deferred Data

	vk::CommandBuffer offscreenDrawBuffer;
	vk::Semaphore offscreenSemaphore;
	struct GBuffer
	{
		uint32_t width, height;
		FrameBuffer frameBuffer;
		FrameBufferAttachment position, normal, albedo;
		FrameBufferAttachment depth;
		vk::RenderPass renderPass;
	} gBuffer;

	
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
		const auto& extent = device.swapchain.extent;
		camera.SetPerspective(45.0f, (float)extent.width / extent.height, 0.1f, 100.0f);
		// uboViewProjection.projection = glm::perspective(glm::radians(45.0f), (float)extent.width / extent.height, 0.1f, 100.0f);
		uboViewProjection.projection = camera.matrices.perspective;
		uboViewProjection.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
		// // Invert up direction so that pos y is up 
		// // its down by default in Vulkan
		uboViewProjection.projection[1][1] *= -1;
		

		
    
		// objects.emplace_back().Create(device, cubeVerts, cubeIndices);
		// objects.emplace_back().Create(device, meshVerts, squareIndices);
		// objects.emplace_back().CreateModel(std::string(ASSET_DIR) + "Models/viking_room.obj", device);

		// objects[0].SetModel(
		// 	glm::translate(objects[0].GetModel(), glm::vec3(0.0f, 0.0f, -3.0f))
		// 	* glm::rotate(objects[0].GetModel(), glm::radians(-90.0f), glm::vec3(1.0, 0.0f, 0.0f)));
		// objects[1].SetModel(glm::translate(objects[1].GetModel(), glm::vec3(0.0f, 0.0f, -2.5f)));


		InitializeAttachments();
		InitializeAssets();
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
		gBuffer.height = 2048;
		gBuffer.width = 2048;

		vk::Format RGBA = vk::Format::eR16G16B16A16Sfloat;
		auto colorAttach = vk::ImageUsageFlagBits::eColorAttachment;
		auto sampled = vk::ImageUsageFlagBits::eSampled;

		gBuffer.position.Create(RGBA, { gBuffer.width, gBuffer.height },
							  colorAttach | sampled, vk::ImageLayout::eColorAttachmentOptimal,
							  device);

		gBuffer.normal.Create(RGBA, { gBuffer.width, gBuffer.height },
							  colorAttach | sampled, vk::ImageLayout::eColorAttachmentOptimal,
							  device);

		gBuffer.albedo.Create(vk::Format::eR8G8B8A8Unorm, { gBuffer.width, gBuffer.height },
							  colorAttach | sampled, vk::ImageLayout::eColorAttachmentOptimal,
							  device);

		gBuffer.depth.Create(FrameBufferAttachment::GetDepthFormat(), { gBuffer.width, gBuffer.height },
							  vk::ImageUsageFlagBits::eDepthStencilAttachment | sampled, vk::ImageLayout::eDepthStencilAttachmentOptimal,
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
			{ 1, vk::ImageLayout::eColorAttachmentOptimal },

			{ 2, vk::ImageLayout::eColorAttachmentOptimal },
		};

		vk::AttachmentReference depthRef;
		depthRef.attachment = 3;
		depthRef.layout = vk::ImageLayout::eDepthAttachmentOptimal;

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

		utils::CheckVkResult(
			device.createRenderPass(&rpInfo, nullptr, &gBuffer.renderPass),
			"Failed to create deferred render pass"
			);
		
		std::array<ImageView, 4> attachments;
		attachments[0] = gBuffer.position.imageView;
		attachments[1] = gBuffer.normal.imageView;
		attachments[2] = gBuffer.albedo.imageView;
		attachments[3] = gBuffer.depth.imageView;

		
		vk::FramebufferCreateInfo fbInfo = {};
		fbInfo.renderPass = gBuffer.renderPass;
		fbInfo.attachmentCount = attachments.size();
		fbInfo.pAttachments = attachments.data();
		fbInfo.width = gBuffer.width;
		fbInfo.height = gBuffer.height;
		fbInfo.layers = 1;

		FrameBuffer::Create(gBuffer.frameBuffer, fbInfo, device);
		
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
		utils::CheckVkResult(device.createSampler(&samplerInfo, nullptr, &colorSampler),
							 "Failed to create deferred sampler");
	}

	void InitializeAssets()
	{
		testTex.Create(std::string(ASSET_DIR) + "Textures/viking_room.png", device);
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
				objects.back().Create(device, data.vertices, data.indices);
			}
		}
	}

	void InitializeUniformBuffers()
	{
		vk::DeviceSize vpBufferSize = sizeof(UboViewProjection);
		//vk::DeviceSize modelBufferSize = modelUniformAlignment * MAX_OBJECTS;

		const auto& images = device.swapchain.images;
		// One uniform buffer for each image
		//uniformBufferModel.resize(images.size());
		uniformBufferViewProjection.resize(images.size());
		uniformBufferComposition.resize(images.size());

		//vk::BufferCreateInfo modelCreateInfo = {};
		//modelCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		//modelCreateInfo.size = modelBufferSize;

		vk::BufferCreateInfo vpCreateInfo = {};
		vpCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		vpCreateInfo.size = vpBufferSize;

		VmaAllocationCreateInfo aCreateInfo = {};
		aCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		for (size_t i = 0; i < images.size(); ++i)
		{
			uniformBufferViewProjection[i].Create(vpCreateInfo, aCreateInfo, device);
			uniformBufferComposition[i].Create(vpCreateInfo, aCreateInfo, device);
		}

		UpdateUniformBuffers(0.0f, device.imageIndex);
	}

	void InitializeDescriptorSetLayouts()
	{
		// UboViewProjection binding info
		vk::DescriptorSetLayoutBinding vpBinding = {};
		vpBinding.binding = 0;
		vpBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
		vpBinding.descriptorCount = 1;
		vpBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
		// Sampler becomes immutable if set
		vpBinding.pImmutableSamplers = nullptr;

		vk::DescriptorSetLayoutBinding compositionBinding = {};
		compositionBinding.binding = 1;
		compositionBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
		compositionBinding.descriptorCount = 1;
		compositionBinding.stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex;
		compositionBinding.pImmutableSamplers = nullptr;

		// Texture sampler
		vk::DescriptorSetLayoutBinding samplerBinding = {};
		samplerBinding.binding = 2;
		samplerBinding.descriptorCount = 1;
		samplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		samplerBinding.pImmutableSamplers = nullptr;
		samplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

		std::vector<vk::DescriptorSetLayoutBinding> bindings = { vpBinding, compositionBinding, samplerBinding };

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
		std::array<vk::WriteDescriptorSet, 3> setWrite = {};
		std::array<vk::DescriptorBufferInfo, 2> bufferInfo = {};
		std::array<vk::DescriptorImageInfo, 1> imageInfo = {};

		// VP BUFFER INFO
		bufferInfo[0].offset = 0;
		bufferInfo[0].range = sizeof(UboViewProjection);

		// SET WRITING INFO
		// Matches with shader layout binding
		setWrite[0].dstBinding = 0;
		// Index of array to update
		setWrite[0].dstArrayElement = 0;
		setWrite[0].descriptorType = vk::DescriptorType::eUniformBuffer;
		// Amount to update
		setWrite[0].descriptorCount = 1;


		// Composition Buffer
		// COMPOSITION BUFFER INFO
		bufferInfo[1].offset = 0;
		bufferInfo[1].range = sizeof(UboComposition);

		// SET WRITING INFO
		// Matches with shader layout binding
		setWrite[1].dstBinding = 1;
		// Index of array to update
		setWrite[1].dstArrayElement = 0;
		setWrite[1].descriptorType = vk::DescriptorType::eUniformBuffer;
		// Amount to update
		setWrite[1].descriptorCount = 1;



		// Sampler image Info
		imageInfo[0].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo[0].imageView = testTex.imageView.Get();
		imageInfo[0].sampler = testTex.sampler.get();
		
		setWrite[2].dstBinding = 2;
		setWrite[2].dstArrayElement = 0;
		setWrite[2].descriptorType = vk::DescriptorType::eCombinedImageSampler;
		setWrite[2].descriptorCount = 1;
		setWrite[2].pImageInfo = &imageInfo[0];
		

		//// Model version
		//vk::DescriptorBufferInfo modelBufferInfo = {};
		//modelBufferInfo.offset = 0;
		//modelBufferInfo.range = modelUniformAlignment;

		//vk::WriteDescriptorSet modelSetWrite = {};
		//modelSetWrite.dstBinding = 1;
		//modelSetWrite.dstArrayElement = 0;
		//modelSetWrite.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
		//modelSetWrite.descriptorCount = 1;

		// Update all descriptor set buffer bindings
		for (size_t i = 0; i < descriptorCount; ++i)
		{
			// Buffer to get data from
			bufferInfo[0].buffer = uniformBufferViewProjection[i];
			bufferInfo[1].buffer = uniformBufferComposition[i];

			// Current descriptor set 
			setWrite[0].dstSet = descriptorSets[i];
			setWrite[1].dstSet = descriptorSets[i];
			setWrite[2].dstSet = descriptorSets[i];

			// Update with new buffer info
			setWrite[0].pBufferInfo = &bufferInfo[0];
			setWrite[1].pBufferInfo = &bufferInfo[1];
			//modelSetWrite.pBufferInfo = &modelBufferInfo;

			device.updateDescriptorSets(
				static_cast<uint32_t>(setWrite.size()),
				setWrite.data(),
				0,
				nullptr);
		}

		// PUSH CONSTANTS
		// NO create needed
		// Where push constant is located
		pushRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
		pushRange.offset = 0;
		pushRange.size = sizeof(glm::mat4);
	}

	void InitializePipelines()
	{
		auto vertSrc = utils::ReadFile(std::string(ASSET_DIR) + "Shaders/vert.spv");
		auto fragSrc = utils::ReadFile(std::string(ASSET_DIR) + "Shaders/frag.spv");

		vk::ShaderModuleCreateInfo shaderInfo;
		shaderInfo.codeSize = vertSrc.size();
		shaderInfo.pCode = reinterpret_cast<const uint32_t*>(vertSrc.data());
		ShaderModule vertModule;
		vertModule.Create(vertModule, shaderInfo, device);

		shaderInfo.codeSize = fragSrc.size();
		shaderInfo.pCode = reinterpret_cast<const uint32_t*>(fragSrc.data());
		ShaderModule fragModule;
		fragModule.Create(fragModule, shaderInfo, device);

		static const char* entryName = "main";

		vk::PipelineShaderStageCreateInfo vertInfo(
			{},
			vk::ShaderStageFlagBits::eVertex,
			vertModule.Get(),
			entryName
		);

		vk::PipelineShaderStageCreateInfo fragInfo(
			{},
			vk::ShaderStageFlagBits::eFragment,
			fragModule.Get(),
			entryName
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
		// Binding the data is at (should be same as above)
		attribDesc[1].binding = 0;
		// Location in shader where data is read from
		attribDesc[1].location = 1;
		// Format for the data being sent
		attribDesc[1].format = vk::Format::eR32G32B32Sfloat;
		// Where the attribute begins as an offset from the beginning
		// of the structures
		attribDesc[1].offset = offsetof(Vertex, normal);


		// Color attribute
		// Binding the data is at (should be same as above)
		attribDesc[2].binding = 0;
		// Location in shader where data is read from
		attribDesc[2].location = 2;
		// Format for the data being sent
		attribDesc[2].format = vk::Format::eR32G32B32Sfloat;
		// Where the attribute begins as an offset from the beginning
		// of the structures
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

		vk::Extent2D extent = device.swapchain.extent;

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

		vk::PipelineViewportStateCreateInfo viewportState;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		// -- DYNAMIC STATES --
		// Dynamic states to enable
		//std::vector<VkDynamicState> dynamicStateEnables;
		//dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);	// Dynamic Viewport : Can resize in command buffer with vkCmdSetViewport(commandbuffer, 0, 1, &viewport);
		//dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);	// Dynamic Scissor	: Can resize in command buffer with vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

		//// Dynamic State creation info
		//VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
		//dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		//dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		//dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();

		vk::PipelineRasterizationStateCreateInfo rasterizeState;
		rasterizeState.depthClampEnable = VK_FALSE;			// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
		rasterizeState.rasterizerDiscardEnable = VK_FALSE;	// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
		rasterizeState.polygonMode = vk::PolygonMode::eFill;	// How to handle filling points between vertices
		rasterizeState.lineWidth = 1.0f;						// How thick lines should be when drawn
		rasterizeState.cullMode = vk::CullModeFlagBits::eNone;		// Which face of a triangle to cull
		rasterizeState.frontFace = vk::FrontFace::eCounterClockwise;	// Winding to determine which side is front
		rasterizeState.depthBiasEnable = VK_FALSE;			// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)

		vk::PipelineMultisampleStateCreateInfo multisampleState;
		// Whether or not to multi-sample
		multisampleState.sampleShadingEnable = VK_FALSE;
		// Number of samples per fragment
		multisampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;


		// Blending
		vk::PipelineColorBlendAttachmentState colorBlendAttachments;
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


		vk::PipelineColorBlendStateCreateInfo colorBlendState;
		colorBlendState.logicOpEnable = VK_FALSE;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachments;

		vk::PipelineLayoutCreateInfo layout = {};
		layout.setLayoutCount = 1;
		layout.pSetLayouts = &descriptorSetLayout;
		layout.pushConstantRangeCount = 1;
		layout.pPushConstantRanges = &pushRange;

		pipelineLayout.Create(pipelineLayout, layout, device);

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
		pipelineInfo.layout = (pipelineLayout).Get();							// Pipeline Layout pipeline should use
		pipelineInfo.renderPass = device.renderPass;							// Render pass description the pipeline is compatible with
		pipelineInfo.subpass = 0;										// Subpass of render pass to use with pipeline

		pipelines.forward.Create(pipelines.forward, pipelineInfo, device, vk::PipelineCache(), 1);

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
		poolSizes[0].descriptorCount = imageCount;

		// Texture Sampler
		poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
		poolSizes[1].descriptorCount = imageCount;

		//vk::DescriptorPoolSize modelPoolSize = {};
		//modelPoolSize.type = vk::DescriptorType::eUniformBufferDynamic;
		//modelPoolSize.descriptorCount = static_cast<uint32_t>(uniformBufferModel.size());

		// Pool creation
		vk::DescriptorPoolCreateInfo poolCreateInfo = {};
		// Maximum number of descriptor sets that can be created from the pool
		poolCreateInfo.maxSets = static_cast<uint32_t>(device.swapchain.images.size());
		// Number of pool sizes being passed 
		poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolCreateInfo.pPoolSizes = poolSizes.data();

		// Create descriptor pool
		descriptorPool.Create(descriptorPool, poolCreateInfo, device);
		descriptorPoolUI.Create(descriptorPoolUI, poolCreateInfo, device);

	}

	void PrepareCommandBuffers(uint32_t imageIndex)
	{
		std::array<vk::ClearValue, 2> clearValues = {};
		clearValues[0].color = vk::ClearColorValue(clearColor);
		clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f);


		vk::RenderPassBeginInfo renderPassInfo;
		renderPassInfo.renderPass = device.renderPass;
		renderPassInfo.renderArea.extent = device.swapchain.extent;
		renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		// Record command buffers
		vk::CommandBufferBeginInfo info(
			{},
			nullptr
		);

		auto& cmdBuf = device.drawCmdBuffers[imageIndex];

		utils::CheckVkResult(cmdBuf.begin(&info), "Failed to begin recording command buffer");
		renderPassInfo.framebuffer = device.framebuffers[imageIndex].Get();

		cmdBuf.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
		cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.forward.Get());

		for (int j = 0; j < objects.size(); ++j)
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

			// Dynamic offset amount
			//uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;

			cmdBuf.pushConstants(
				pipelineLayout,
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
				pipelineLayout,

				// First set, num sets, pointer to set
				0,
				1,
				&descriptorSets[imageIndex], // 1 to 1 with command buffers

				// Dynamic offsets
				//1,
				//&dynamicOffset
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

	}

	void PrepareCommandBuffersUI(uint32_t imageIndex)
	{
		auto extent = device.swapchain.extent;
		vk::RenderPassBeginInfo info = { };
		info.renderPass = renderPassUI;
		info.framebuffer = framebuffersUI[imageIndex].Get();
		info.renderArea.extent = extent;
		info.clearValueCount = 1;
		auto clearColor = vk::ClearColorValue(std::array<float,4>{ 0.0f, 0.0f, 1.0f, 1.0f });
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

		PrepareCommandBuffers(device.imageIndex);
		PrepareCommandBuffersUI(device.imageIndex);

		vk::CommandBuffer commandBuffers[2] = {
			device.drawCmdBuffers[device.imageIndex],
			commandBuffersUI[device.imageIndex]
		};

		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::SubmitInfo submitInfo;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &device.imageAvailable[currentFrame];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &device.renderFinished[currentFrame];
		submitInfo.commandBufferCount = 2;
		submitInfo.pCommandBuffers = commandBuffers;
		submitInfo.pWaitDstStageMask = waitStages;
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
		uboComposition.lights[0].position = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		// uboComposition.lights[0].color = glm::vec3(1.5f);
		uboComposition.lights[0].color = glm::vec3(1.0f);
		uboComposition.lights[0].radius = 15.0f * 0.25f;
		// Red
		uboComposition.lights[1].position = glm::vec4(-2.0f, 0.0f, 0.0f, 0.0f);
		// uboComposition.lights[1].color = glm::vec3(1.0f, 0.0f, 0.0f);
		uboComposition.lights[1].color = glm::vec3(1.0f, 1.0f, 1.0f);
		uboComposition.lights[1].radius = 15.0f;
		// Blue
		uboComposition.lights[2].position = glm::vec4(2.0f, -1.0f, 0.0f, 0.0f);
		// uboComposition.lights[2].color = glm::vec3(0.0f, 0.0f, 2.5f);
		uboComposition.lights[2].color = glm::vec3(1.0f, 1.0f, 1.0f);
		uboComposition.lights[2].radius = 5.0f;
		// Yellow
		uboComposition.lights[3].position = glm::vec4(0.0f, -0.9f, 0.5f, 0.0f);
		// uboComposition.lights[3].color = glm::vec3(1.0f, 1.0f, 0.0f);
		uboComposition.lights[3].color = glm::vec3(1.0f, 1.0f, 1.0f);
		uboComposition.lights[3].radius = 2.0f;
		// Green
		uboComposition.lights[4].position = glm::vec4(0.0f, -0.5f, 0.0f, 0.0f);
		// uboComposition.lights[4].color = glm::vec3(0.0f, 1.0f, 0.2f);
		uboComposition.lights[4].color = glm::vec3(0.0f, 1.0f, 0.2f);
		uboComposition.lights[4].radius = 5.0f;
		// Yellow
		uboComposition.lights[5].position = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
		// uboComposition.lights[5].color = glm::vec3(1.0f, 0.7f, 0.3f);
		uboComposition.lights[5].color = glm::vec3(1.0f, 1.0f, 1.0f);
		uboComposition.lights[5].radius = 25.0f;

		static float timer = 0.0f;
		uboComposition.lights[0].position.x = sin(glm::radians(360.0f * timer)) * 5.0f;
		uboComposition.lights[0].position.z = cos(glm::radians(360.0f * timer)) * 5.0f;

		uboComposition.lights[1].position.x = -4.0f + sin(glm::radians(360.0f * timer) + 45.0f) * 2.0f;
		uboComposition.lights[1].position.z = 0.0f + cos(glm::radians(360.0f * timer) + 45.0f) * 2.0f;

		uboComposition.lights[2].position.x = 4.0f + sin(glm::radians(360.0f * timer)) * 2.0f;
		uboComposition.lights[2].position.z = 0.0f + cos(glm::radians(360.0f * timer)) * 2.0f;

		uboComposition.lights[4].position.x = 0.0f + sin(glm::radians(360.0f * timer + 90.0f)) * 5.0f;
		uboComposition.lights[4].position.z = 0.0f - cos(glm::radians(360.0f * timer + 45.0f)) * 5.0f;

		uboComposition.lights[5].position.x = 0.0f + sin(glm::radians(-360.0f * timer + 135.0f)) * 10.0f;
		uboComposition.lights[5].position.z = 0.0f - cos(glm::radians(-360.0f * timer - 45.0f)) * 10.0f;

		// Current view position
		uboComposition.viewPos = glm::vec4(camera.position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

		uboComposition.debugDisplayTarget = 0;
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
		if(glfwGetKey(win, GLFW_KEY_SPACE) && spaceTimer > spaceCooldown)
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

		static glm::vec3 ppScale = {.0001f, .0001f, .0001f};
		ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::InputFloat3("Power Plant Scale", &ppScale[0], "%.9f");
		ImGui::SliderFloat4("Clear Color", clearColor.data(), 0.0f, 1.0f);

		for(auto& object : objects)
			object.SetModel(glm::scale(glm::mat4(1.0f), ppScale));

		UpdateUniformBuffers(dt, device.imageIndex);
	}


	void DrawUI()
	{
		ImGui::ShowDemoWindow();
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
