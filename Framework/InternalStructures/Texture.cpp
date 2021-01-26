#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

void Texture::Create(const std::string& filepath)
{
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

	ASSERT(pixels != 0, "Failed to load texture image");

	vk::UniqueCommandBuffer cmdBuf = m_owner->commandPool.BeginCommandBuffer();
	Buffer pixelBuffer;

	pixelBuffer.CreateStaged(pixels, imageSize, 
		{}, 
		VMA_MEMORY_USAGE_GPU_ONLY,
		*m_owner);

	vk::Extent3D dimensions(width, height, 1);

	vk::ImageCreateInfo imageCI;
	imageCI.extent = dimensions;
	imageCI.mipLevels = mipLevels;
	imageCI.samples = vk::SampleCountFlagBits::e1;
	imageCI.format = vk::Format::eR8G8B8A8Unorm;
	imageCI.tiling = vk::ImageTiling::eOptimal;
	imageCI.usage = vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eTransferSrc |
		vk::ImageUsageFlagBits::eSampled;
	
	Image image;

	vk::BufferImageCopy imageCopy;

}

void Texture::Destroy()
{
	
}
