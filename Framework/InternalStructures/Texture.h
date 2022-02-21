#pragma once

struct stbtt_fontinfo;

namespace dm
{

class Texture : public IOwned<Device>
{
public:
    void Create(const std::string& path, Device* inOwner);
    void StageTexture(vk::DeviceSize size, uint32_t mipLevels);
    virtual void Destroy();
    ~Texture() override;
    vk::DescriptorImageInfo GetDescriptor(vk::ImageLayout imageLayout);

    int32_t width = 0;
    int32_t height = 0;
    int32_t channels = 0;

    Image image;
    ImageView imageView;
    Sampler sampler;

    void* pixelData = nullptr;

};

class FontTexture final : public Texture
{
public:
    void Create(const std::string& path, Device* inOwner);
    ~FontTexture() noexcept override;
    void Destroy() override;

    void GetFontPosition(char c, glm::vec2& uvScale, glm::vec2& uvOffset);
    float BuildWord(const std::string& word, bool normalizeWord,
                    std::vector<float>& letterAdvances,
                    std::vector<float>& letterProportions,
                    std::vector<float>& letterOffsets);
    float GetKerning(char c1, char c2);

    float scale = 1.0f;
    float lineHeight = 32.0f;

    // vec4 here is x, y, w, h in UV space
    std::unordered_map<char, glm::vec4> characters;

    stbtt_fontinfo* info;
};

}
