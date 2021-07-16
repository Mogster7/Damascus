//------------------------------------------------------------------------------
//
// File Name:	Texture.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        02/03/2020 
//
//------------------------------------------------------------------------------
#define STB_IMAGE_IMPLEMENTATION
#include "RenderingContext.h"
#include "stb/stb_image.h"

namespace dm {

void Texture::Create(const std::string& filepath, Device* owner)
{
	this->owner = owner;
	stbi_uc* pixels = stbi_load(
		filepath.c_str(),
		&width,
		&height,
		&channels,
		STBI_rgb_alpha
	);

	vk::DeviceSize imageSize = width * height * 4;
	uint32_t mipLevels = {
		static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1
	};

	ASSERT(pixels != nullptr, "Failed to load texture image");

	vk::BufferCreateInfo stageInfo = {};
	stageInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	stageInfo.size = imageSize;

	VmaAllocationCreateInfo stageAllocInfo = {};
	stageAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	Buffer stagingBuffer(stageInfo, stageAllocInfo, owner);

	// TODO: abstract staging buffers
	void* mapped;
	auto allocator = owner->allocator;
	// Map and copy data to the memory, then unmap
	vmaMapMemory(allocator, stagingBuffer.allocation, &mapped);
	std::memcpy(mapped, pixels, (size_t) imageSize);
	vmaUnmapMemory(allocator, stagingBuffer.allocation);

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	image.Create2D(glm::uvec2(width, height),
		vk::Format::eR8G8B8A8Unorm,
		mipLevels,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eTransferSrc |
		vk::ImageUsageFlagBits::eSampled,
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageAspectFlagBits::eColor,
		owner);

	vk::ImageSubresourceLayers imageSubresource{
		vk::ImageAspectFlagBits::eColor, // Aspect mask
		0,                               // Mip level
		0,                               // Base array layer
		1
	};                              // Layer count

	auto& commandPool = OwnerGet<RenderingContext>().commandPool;
	vk::UniqueCommandBuffer cmdBuf = commandPool.BeginCommandBuffer();

	vk::Extent3D imageExtent{
		static_cast<uint32_t>(width),  // Width
		static_cast<uint32_t>(height), // Height
		1
	};          // Depth

	vk::BufferImageCopy bufferImageCopy{
		0,                // Buffer offset
		0,                // Buffer row length
		0,                // Buffer image height
		imageSubresource, // Image subresource
		vk::Offset3D(),   // Image offset
		imageExtent
	};     // Image extent

	vk::BufferImageCopy imageCopy = {};
	imageCopy.imageSubresource = imageSubresource;
	imageCopy.imageOffset = vk::Offset3D();
	imageCopy.imageExtent = imageExtent;

	cmdBuf->copyBufferToImage(stagingBuffer.VkType(),
		image.VkType(),
		vk::ImageLayout::eTransferDstOptimal,
		1,
		&bufferImageCopy);

	commandPool.EndCommandBuffer(cmdBuf.get());

	// Transition to shader resource
	image.TransitionLayout(vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::ImageAspectFlagBits::eColor,
		mipLevels);

	imageView.CreateTexture2DView(image.VkType(), owner);

	auto& physicalDevice = OwnerGet<PhysicalDevice>();
	vk::PhysicalDeviceProperties physicalProps = physicalDevice.GetProperties();
	vk::SamplerCreateInfo samplerInfo = {};
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = false;
	samplerInfo.compareEnable = false;
	samplerInfo.compareOp = vk::CompareOp::eNever;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

	sampler.Create(samplerInfo, owner);
}


vk::DescriptorImageInfo Texture::GetDescriptor(
	vk::ImageLayout imageLayout
)
{
	return vk::DescriptorImageInfo(
		sampler.VkType(),
		imageView.VkType(),
		imageLayout
	);
}




}