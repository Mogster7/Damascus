//------------------------------------------------------------------------------
//
// File Name:	Image.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        7/24/2020 
//
//------------------------------------------------------------------------------
void Image::Create(vk::ImageCreateInfo& imageCreateInfo,
				   VmaAllocationCreateInfo& allocCreateInfo,
				   Device& owner)
{
	m_owner = &owner;
	this->allocator = m_owner->allocator;
	
	utils::CheckVkResult((vk::Result)vmaCreateImage(allocator, (VkImageCreateInfo*)&imageCreateInfo, &allocCreateInfo,
													(VkImage*)&m_object, &allocation, &allocationInfo),
						 "Failed to allocate image");
}


void Image::Create2D(glm::uvec2 size,
				   vk::Format format,
				   uint32_t mipLevels,
				   vk::ImageTiling tiling,
				   vk::ImageUsageFlags usage,
				   vk::ImageLayout dstLayout,
					 vk::ImageAspectFlags aspectMask,
				   Device& owner)
{
	m_owner = &owner;

	vk::ImageCreateInfo imageCreateInfo = {};

	imageCreateInfo.imageType = vk::ImageType::e2D;
	imageCreateInfo.extent.width = size.x;
	imageCreateInfo.extent.height = size.y;
	// Just 1, no 3D aspect
	imageCreateInfo.extent.depth = 1;
	// Number of mipmap levels
	imageCreateInfo.mipLevels = mipLevels;
	// Number of levels in image array (used in cube maps)
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = format;
	// How image data should be arranged for optimal reading
	imageCreateInfo.tiling = tiling;
	// Layout of image data on creation
	imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
	// Bit flags defining what image will be used for
	imageCreateInfo.usage = usage;
	// Number of samples for multisampling
	imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
	// Whether image can be shared between queues
	imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	Create(imageCreateInfo, allocInfo, *m_owner);

	TransitionLayout(vk::ImageLayout::eUndefined, dstLayout, aspectMask, mipLevels);
}

void Image::CreateDepthImage(glm::vec2 size, Device& owner)
{
	Create2D(size,
			 FrameBufferAttachment::GetDepthFormat(),
			 1,
			 vk::ImageTiling::eOptimal,
			 vk::ImageUsageFlagBits::eDepthStencilAttachment,
			 vk::ImageLayout::eDepthStencilAttachmentOptimal,
			 vk::ImageAspectFlagBits::eDepth,
			 owner);
}

void Image::TransitionLayout(vk::ImageLayout oldLayout,
                             vk::ImageLayout newLayout,
							 vk::ImageAspectFlags aspectMask,
                             uint32_t mipLevels) 
{
	auto& rc = RenderingContext::Get();
	auto cmdBuf = rc.commandPool.BeginCommandBuffer();

	TransitionLayout(cmdBuf.get(),
					 oldLayout, newLayout,
					 aspectMask, mipLevels);

	rc.commandPool.EndCommandBuffer(cmdBuf.get());
}


void Image::TransitionLayout(vk::CommandBuffer commandBuffer,
					  vk::ImageLayout oldLayout,
					  vk::ImageLayout newLayout,
					  vk::ImageAspectFlags aspectMask,
					  uint32_t mipLevels)
{
	// Create a barrier with sensible defaults - some properties will change
		// depending on the old -> new layout combinations.
	vk::ImageMemoryBarrier barrier;
	barrier.image = Get();
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.subresourceRange.aspectMask = aspectMask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	vk::PipelineStageFlagBits srcFlags;
	vk::PipelineStageFlagBits dstFlags;

	// Scenario: undefined -> color attachment optimal
	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal)
	{
		barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

		srcFlags = vk::PipelineStageFlagBits::eTopOfPipe;
		dstFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	}
	// Scenario: undefined -> depth stencil attachment optimal
	else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		srcFlags = vk::PipelineStageFlagBits::eTopOfPipe;
		dstFlags = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	// Scenario: undefined -> transfer destination optimal
	else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferSrcOptimal)
	{
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

		srcFlags = vk::PipelineStageFlagBits::eTopOfPipe;
		dstFlags = vk::PipelineStageFlagBits::eTransfer;
	}
	// Scenario: undefined -> transfer destination optimal
	else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		srcFlags = vk::PipelineStageFlagBits::eTopOfPipe;
		dstFlags = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal && newLayout == vk::ImageLayout::eTransferSrcOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

		srcFlags = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		dstFlags = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		srcFlags = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		dstFlags = vk::PipelineStageFlagBits::eTransfer;

	}
	else if (oldLayout == vk::ImageLayout::eTransferSrcOptimal && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;

		srcFlags = vk::PipelineStageFlagBits::eTopOfPipe;
		dstFlags = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else if (oldLayout == vk::ImageLayout::eTransferSrcOptimal && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		srcFlags = vk::PipelineStageFlagBits::eTopOfPipe;
		dstFlags = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eTransferSrcOptimal)
	{
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

		srcFlags = vk::PipelineStageFlagBits::eTransfer;
		dstFlags = vk::PipelineStageFlagBits::eTransfer;
	}

	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;

		srcFlags = vk::PipelineStageFlagBits::eTopOfPipe;
		dstFlags = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	// Scenario: transfer destination -> shader resource
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;

		srcFlags = vk::PipelineStageFlagBits::eTopOfPipe;
		dstFlags = vk::PipelineStageFlagBits::eFragmentShader;
	}


	else ASSERT(false, "Unhandled image layout transition");

	// Pipeline stage flags sets where to apply the command in the pipeline
	commandBuffer.pipelineBarrier(
		srcFlags,
		dstFlags,
		vk::DependencyFlags(),
		0, nullptr,
		0, nullptr,
		1, &barrier);
}



void Image::Destroy()
{
	vmaDestroyImage(allocator, (VkImage)m_object, allocation);
}
