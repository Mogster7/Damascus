#pragma once

class Texture
{
public:
	void Create(const std::string& filepath);
	void Destroy();

	int32_t width = 0;
	int32_t height = 0;
	int32_t channels = 0;

	Image image;

	Device* m_owner;
};
