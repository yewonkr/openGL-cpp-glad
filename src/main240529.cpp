

#include <spdlog/spdlog.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <optional>
#include <vector>
#include <memory>

#include <fstream>
#include <sstream>
#include <string>

#define STB_IMAGE_IMPLEMENTATION    // added for link error
#include <stb/stb_image.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

using namespace std;

#define CLASS_PTR(klassName) \
class klassName; \
using klassName ## UPtr = std::unique_ptr<klassName>; \
using klassName ## Ptr = std::shared_ptr<klassName>; \
using klassName ## WPtr = std::weak_ptr<klassName>;

// optional is not to assign a memory for the empty file
optional<string> LoadTextFile(const string& filename) {
    ifstream fin(filename);
    if (!fin.is_open()) {
        SPDLOG_ERROR("failed to open file: {}", filename);
        return {};
    }

    stringstream text;
    text << fin.rdbuf();
    return text.str();
}

glm::vec3 GetAttenuationCoeff(float distance) {
    const auto linear_coeff = glm::vec4(
        8.4523112e-05,
        4.4712582e+00,
        -1.8516388e+00,
        3.3955811e+01
    );
    const auto quad_coeff = glm::vec4(
        -7.6103583e-04,
        9.0120201e+00,
        -1.1618500e+01,
        1.0000464e+02
    );

    float kc = 1.0f;
    float d = 1.0f / distance;
    auto dvec = glm::vec4(1.0f, d, d*d, d*d*d);
    float kl = glm::dot(linear_coeff, dvec);
    float kq = glm::dot(quad_coeff, dvec);
    
    return glm::vec3(kc, glm::max(kl, 0.0f), glm::max(kq*kq, 0.0f));
}


CLASS_PTR(Shader);
class Shader {
public:
    static ShaderUPtr CreateFromFile(const std::string& filename, GLenum shaderType);

    ~Shader();
    uint32_t Get() const { return m_shader; }    
private:
    Shader() {}
    bool LoadFile(const std::string& filename, GLenum shaderType);
    uint32_t m_shader { 0 };
};

ShaderUPtr Shader::CreateFromFile(const std::string& filename, GLenum shaderType) {
    auto shader = std::unique_ptr<Shader>(new Shader());
    if (!shader->LoadFile(filename, shaderType))
        return nullptr;
    return std::move(shader);
}

bool Shader::LoadFile(const std::string& filename, GLenum shaderType) {
    auto result = LoadTextFile(filename);
    if (!result.has_value()) {
        return false;
    }

    auto& code = result.value();
    const char* codePtr = code.c_str();
    int32_t codeLength = (int32_t)code.length();

    // create and compile shader
    m_shader = glCreateShader(shaderType);
    glShaderSource(m_shader, 1, (const GLchar* const*)&codePtr, &codeLength);
    glCompileShader(m_shader);

    // check compile error
    int success = 0;
    glGetShaderiv(m_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(m_shader, 1024, nullptr, infoLog);
        SPDLOG_ERROR("failed to compile shader: \"{}\"", filename);
        SPDLOG_ERROR("reason: {}", infoLog);
        return false;
    }
    return true;
}

Shader::~Shader() {
    if (m_shader) {
        glDeleteShader(m_shader);
    }
}



CLASS_PTR(Program)
class Program {
public:
    static ProgramUPtr Create(const std::vector<ShaderPtr>& shaders);
    static ProgramUPtr Create(const std::string& vertShaderFilename,
        const std::string& fragShaderFilename);

    ~Program();
    uint32_t Get() const { return m_program; }
    void Use() const;

    void SetUniform(const std::string& name, int value) const;
    void SetUniform(const std::string& name, const glm::mat4& value) const;
    
    void SetUniform(const std::string& name, float value) const;
    void SetUniform(const std::string& name, const glm::vec2& value) const;
    void SetUniform(const std::string& name, const glm::vec3& value) const;
    void SetUniform(const std::string& name, const glm::vec4& value) const;
 
private:
    Program() {}
    bool Link(const std::vector<ShaderPtr>& shaders);
    uint32_t m_program { 0 };
};

ProgramUPtr Program::Create(const std::vector<ShaderPtr>& shaders) {
    auto program = ProgramUPtr(new Program());
    if (!program->Link(shaders))
        return nullptr;
    return std::move(program);
}

ProgramUPtr Program::Create(const std::string& vertShaderFilename,
    const std::string& fragShaderFilename) {
    ShaderPtr vs = Shader::CreateFromFile(vertShaderFilename, GL_VERTEX_SHADER);
    ShaderPtr fs = Shader::CreateFromFile(fragShaderFilename, GL_FRAGMENT_SHADER);
    if (!vs || !fs)
        return nullptr;
    return std::move(Create({vs, fs}));
}

bool Program::Link(const std::vector<ShaderPtr>& shaders) {
    m_program = glCreateProgram();
    for (auto& shader: shaders)
        glAttachShader(m_program, shader->Get());
    glLinkProgram(m_program);

    int success = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(m_program, 1024, nullptr, infoLog);
        SPDLOG_ERROR("failed to link program: {}", infoLog);
        return false;
    }

    return true;
}

Program::~Program() {
    if (m_program) {
        glDeleteProgram(m_program);
    }
}

void Program::Use() const {
    glUseProgram(m_program);
}

void Program::SetUniform(const std::string& name, int value) const {
    auto loc = glGetUniformLocation(m_program, name.c_str());
    glUniform1i(loc, value);
}

void Program::SetUniform(const std::string& name, const glm::mat4& value) const {
    auto loc = glGetUniformLocation(m_program, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

void Program::SetUniform(const std::string& name, float value) const {
    auto loc = glGetUniformLocation(m_program, name.c_str());
    glUniform1f(loc, value);
}

void Program::SetUniform(const std::string& name, const glm::vec2& value) const {
    auto loc = glGetUniformLocation(m_program, name.c_str());
    glUniform2fv(loc, 1, glm::value_ptr(value));
}

void Program::SetUniform(const std::string& name, const glm::vec3& value) const {
    auto loc = glGetUniformLocation(m_program, name.c_str());
    glUniform3fv(loc, 1, glm::value_ptr(value));
}

void Program::SetUniform(const std::string& name, const glm::vec4& value) const {
    auto loc = glGetUniformLocation(m_program, name.c_str());
    glUniform4fv(loc, 1, glm::value_ptr(value));
}



CLASS_PTR(Buffer)
class Buffer {
public:
    static BufferUPtr CreateWithData(
        uint32_t bufferType, uint32_t usage,
        const void* data, size_t dataSize);
    ~Buffer();
    uint32_t Get() const { return m_buffer; }
    void Bind() const;

private:
    Buffer() {}
    bool Init(
        uint32_t bufferType, uint32_t usage,
        const void* data, size_t dataSize);
    uint32_t m_buffer { 0 };
    uint32_t m_bufferType { 0 };
    uint32_t m_usage { 0 };
};

BufferUPtr Buffer::CreateWithData(
    uint32_t bufferType, uint32_t usage,
    const void* data, size_t dataSize) {
    
    auto buffer = BufferUPtr(new Buffer());
    if (!buffer->Init(bufferType, usage, data, dataSize))
        return nullptr;
    return std::move(buffer);
}

Buffer::~Buffer() {
    if (m_buffer) {
        glDeleteBuffers(1, &m_buffer);
    }
}

void Buffer::Bind() const {
    glBindBuffer(m_bufferType, m_buffer);
}

bool Buffer::Init(
    uint32_t bufferType, uint32_t usage,
    const void* data, size_t dataSize) {
        
    m_bufferType = bufferType;
    m_usage = usage;
    glGenBuffers(1, &m_buffer);
    Bind();
    glBufferData(m_bufferType, dataSize, data, usage);
    return true;
}



CLASS_PTR(VertexLayout)
class VertexLayout {
public:
    static VertexLayoutUPtr Create();
    ~VertexLayout();

    uint32_t Get() const { return m_vertexArrayObject; }
    void Bind() const;
    void SetAttrib(
        uint32_t attribIndex, int count,
        uint32_t type, bool normalized,
        size_t stride, uint64_t offset) const;
    void DisableAttrib(int attribIndex) const;

private:
    VertexLayout() {}
    void Init();
    uint32_t m_vertexArrayObject { 0 };
};

VertexLayoutUPtr VertexLayout::Create() {
    auto vertexLayout = VertexLayoutUPtr(new VertexLayout());
    vertexLayout->Init();
    return std::move(vertexLayout);
}

VertexLayout::~VertexLayout() {
    if (m_vertexArrayObject) {
        glDeleteVertexArrays(1, &m_vertexArrayObject);
    }
}

void VertexLayout::Bind() const {
    glBindVertexArray(m_vertexArrayObject);
}

void VertexLayout::SetAttrib(
    uint32_t attribIndex, int count,
    uint32_t type, bool normalized,
    size_t stride, uint64_t offset) const {
        
    glEnableVertexAttribArray(attribIndex);
    glVertexAttribPointer(attribIndex, count,
        type, normalized, stride, (const void*)offset);
}

void VertexLayout::Init() {
    glGenVertexArrays(1, &m_vertexArrayObject);
    Bind();
}


CLASS_PTR(Image)
class Image {
public:
    static ImageUPtr Load(const std::string& filepath);
    static ImageUPtr Create(int width, int height, int channelCount = 4);
    static ImageUPtr CreateSingleColorImage(int width, int height, const glm::vec4& color);
    ~Image();

    const uint8_t* GetData() const { return m_data; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    int GetChannelCount() const { return m_channelCount; }

    void SetCheckImage(int gridX, int gridY);
    
private:
    Image() {};
    bool LoadWithStb(const std::string& filepath);
    bool Allocate(int width, int height, int channelCount);
    int m_width { 0 };
    int m_height { 0 };
    int m_channelCount { 0 };
    uint8_t* m_data { nullptr };
};

ImageUPtr Image::Load(const std::string& filepath) {
    auto image = ImageUPtr(new Image());
    if (!image->LoadWithStb(filepath))
        return nullptr;
    return std::move(image);
}

ImageUPtr Image::Create(int width, int height, int channelCount) {
    auto image = ImageUPtr(new Image());
    if (!image->Allocate(width, height, channelCount))
        return nullptr;
    return std::move(image);
}

ImageUPtr Image::CreateSingleColorImage(int width, int height, const glm::vec4& color) {
    glm::vec4 clamped = glm::clamp(color * 255.0f, 0.0f, 255.0f);
    uint8_t rgba[4] = {
        (uint8_t)clamped.r, 
        (uint8_t)clamped.g, 
        (uint8_t)clamped.b, 
        (uint8_t)clamped.a, 
    };
    auto image = Create(width, height, 4);
    for (int i = 0; i < width * height; i++) {
        memcpy(image->m_data + 4 * i, rgba, 4);
    }
    return std::move(image);
}

Image::~Image() {
    if (m_data) {
        stbi_image_free(m_data);
    }
}

void Image::SetCheckImage(int gridX, int gridY) {
    for (int j = 0; j < m_height; j++) {
        for (int i = 0; i < m_width; i++) {
            int pos = (j * m_width + i) * m_channelCount;
            bool even = ((i / gridX) + (j / gridY)) % 2 == 0;
            uint8_t value = even ? 255 : 0;
            for (int k = 0; k < m_channelCount; k++)
                m_data[pos + k] = value;
            if (m_channelCount > 3)
                m_data[3] = 255;
        }
    }
}

bool Image::LoadWithStb(const std::string& filepath) {
    stbi_set_flip_vertically_on_load(true);
    m_data = stbi_load(filepath.c_str(), &m_width, &m_height, &m_channelCount, 0);
    if (!m_data) {
        SPDLOG_ERROR("failed to load image: {}", filepath);
        return false;
    }
    return true;
}

bool Image::Allocate(int width, int height, int channelCount) {
    m_width = width;
    m_height = height;
    m_channelCount = channelCount;
    m_data = (uint8_t*)malloc(m_width * m_height * m_channelCount);
    return m_data ? true : false;
}



CLASS_PTR(Texture)
class Texture {
public:
    static TextureUPtr CreateFromImage(const Image* image);
    ~Texture();

    const uint32_t Get() const { return m_texture; }
    void Bind() const;
    void SetFilter(uint32_t minFilter, uint32_t magFilter) const;
    void SetWrap(uint32_t sWrap, uint32_t tWrap) const;
    
private:
    Texture() {}
    void CreateTexture();
    void SetTextureFromImage(const Image* image);

    uint32_t m_texture { 0 };
};

TextureUPtr Texture::CreateFromImage(const Image* image) {
    auto texture = TextureUPtr(new Texture());
    texture->CreateTexture();
    texture->SetTextureFromImage(image);
    return std::move(texture);
}

Texture::~Texture() {
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
    }
}

void Texture::Bind() const {
    glBindTexture(GL_TEXTURE_2D, m_texture);
}

void Texture::SetFilter(uint32_t minFilter, uint32_t magFilter) const {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
}

void Texture::SetWrap(uint32_t sWrap, uint32_t tWrap) const {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrap);
}
    
void Texture::CreateTexture() {
    glGenTextures(1, &m_texture);
    // bind and set default filter and wrap option
    Bind();
    SetFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    SetWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
}

void Texture::SetTextureFromImage(const Image* image) {
    GLenum format = GL_RGBA;
    switch (image->GetChannelCount()) {
        default: break;
        case 1: format = GL_RED; break;
        case 2: format = GL_RG; break;
        case 3: format = GL_RGB; break;
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        image->GetWidth(), image->GetHeight(), 0,
        format, GL_UNSIGNED_BYTE,
        image->GetData());

    glGenerateMipmap(GL_TEXTURE_2D);
}



CLASS_PTR(Context)
class Context {
public:
    static ContextUPtr Create();
    void Render();  

    void ProcessInput(GLFWwindow* window);
    void Reshape(int width, int height);
    void MouseMove(double x, double y);
    void MouseButton(int button, int action, double x, double y);
 
private:
    Context() {}
    bool Init();

    ProgramUPtr m_program;
    ProgramUPtr m_simpleProgram;


    int m_width {WINDOW_WIDTH};
    int m_height {WINDOW_HEIGHT};

    // uint32_t m_vertexArrayObject;
    // uint32_t m_vertexBuffer;
    // uint32_t m_indexBuffer;

    // clear color
    glm::vec4 m_clearColor { glm::vec4(0.1f, 0.2f, 0.3f, 0.0f) };

    VertexLayoutUPtr m_vertexLayout;
    BufferUPtr m_vertexBuffer;
    BufferUPtr m_indexBuffer;

    // uint32_t m_texture;
    TextureUPtr m_texture;
    TextureUPtr m_texture2;

    // animation
    bool m_animation { true };

    // camera parameter
    bool m_cameraControl { false };
    glm::vec2 m_prevMousePos { glm::vec2(0.0f) };

    float m_cameraPitch { 0.0f };
    float m_cameraYaw { 0.0f };

    glm::vec3 m_cameraFront { glm::vec3(0.0f, 0.0f, -1.0f) };
    glm::vec3 m_cameraPos { glm::vec3(0.0f, 0.0f, 7.0f) };
    glm::vec3 m_cameraUp { glm::vec3(0.0f, 1.0f, 0.0f) };

    // light parameter
    struct Light {
        glm::vec3 position { glm::vec3(2.0f, 2.0f, 2.0f) };
        glm::vec3 direction { glm::vec3(-0.2f, -1.0f, -0.3f) };
        glm::vec2 cutoff { glm::vec2(20.0f, 5.0f) };
        // float cutoff { 20.0f };
        float distance { 32.0f };
        glm::vec3 ambient { glm::vec3(0.1f, 0.1f, 0.1f) };
        glm::vec3 diffuse { glm::vec3(0.8f, 0.8f, 0.8f) };
        glm::vec3 specular { glm::vec3(1.0f, 1.0f, 1.0f) };
    };
    Light m_light;

    glm::vec3 m_lightPos { glm::vec3(3.0f, 3.0f, 3.0f) };
    glm::vec3 m_lightColor { glm::vec3(1.0f, 1.0f, 1.0f) };
    glm::vec3 m_objectColor { glm::vec3(1.0f, 1.0f, 1.0f) };
    float m_ambientStrength { 0.3f };

    float m_specularStrength { 0.5f };
    float m_specularShininess { 32.0f };

    // material parameter
    struct Material {
        // glm::vec3 position { glm::vec3(2.0f, 2.0f, 2.0f) };
        // glm::vec3 direction { glm::vec3(-1.0f, -1.0f, -1.0f) };
        // glm::vec2 cutoff { glm::vec2(20.0f, 5.0f) };
        // float distance { 32.0f };
        TextureUPtr diffuse;
        TextureUPtr specular;
        // glm::vec3 ambient { glm::vec3(0.1f, 0.5f, 0.3f) };
        // glm::vec3 diffuse { glm::vec3(0.8f, 0.5f, 0.3f) };
        // glm::vec3 specular { glm::vec3(0.5f, 0.5f, 0.5f) };
        float shininess { 32.0f };
    };
    Material m_material;

};

ContextUPtr Context::Create() {
    auto context = ContextUPtr(new Context());
    if (!context->Init())
        return nullptr;
    return std::move(context);
}

void Context::ProcessInput(GLFWwindow* window) {
    if (!m_cameraControl)
        return;

    const float cameraSpeed = 0.05f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_cameraPos += cameraSpeed * m_cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_cameraPos -= cameraSpeed * m_cameraFront;

    auto cameraRight = glm::normalize(glm::cross(m_cameraUp, -m_cameraFront));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_cameraPos += cameraSpeed * cameraRight;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_cameraPos -= cameraSpeed * cameraRight;    

    auto cameraUp = glm::normalize(glm::cross(-m_cameraFront, cameraRight));
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        m_cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        m_cameraPos -= cameraSpeed * cameraUp;
}

void Context::Reshape(int width, int height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, m_width, m_height);
}

void Context::MouseMove(double x, double y) {
    if (!m_cameraControl)
        return;
    auto pos = glm::vec2((float)x, (float)y);
    auto deltaPos = pos - m_prevMousePos;

    const float cameraRotSpeed = 0.8f;
    m_cameraYaw -= deltaPos.x * cameraRotSpeed;
    m_cameraPitch -= deltaPos.y * cameraRotSpeed;

    if (m_cameraYaw < 0.0f)   m_cameraYaw += 360.0f;
    if (m_cameraYaw > 360.0f) m_cameraYaw -= 360.0f;

    if (m_cameraPitch > 89.0f)  m_cameraPitch = 89.0f;
    if (m_cameraPitch < -89.0f) m_cameraPitch = -89.0f;

    m_prevMousePos = pos;    
}

void Context::MouseButton(int button, int action, double x, double y) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            m_prevMousePos = glm::vec2((float)x, (float)y);
            m_cameraControl = true;
        }
        else if (action == GLFW_RELEASE) {
            m_cameraControl = false;
        }
    }
}


void Context::Render() {

    if (ImGui::Begin("ui window")) {
    
        if (ImGui::ColorEdit4("clear color", glm::value_ptr(m_clearColor))) {
            glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
        }

        // camera
        ImGui::Separator();
        ImGui::DragFloat3("camera pos", glm::value_ptr(m_cameraPos), 0.01f);
        ImGui::DragFloat("camera yaw", &m_cameraYaw, 0.5f);
        ImGui::DragFloat("camera pitch", &m_cameraPitch, 0.5f, -89.0f, 89.0f);
        ImGui::Separator();

        if (ImGui::Button("reset camera")) {
            m_cameraYaw = 0.0f;
            m_cameraPitch = 0.0f;
            m_cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
        }

        // light without using structure
        // if (ImGui::CollapsingHeader("light", ImGuiTreeNodeFlags_DefaultOpen)) {
        //     ImGui::DragFloat3("light pos", glm::value_ptr(m_lightPos), 0.01f);
        //     ImGui::ColorEdit3("light color", glm::value_ptr(m_lightColor));
        //     ImGui::ColorEdit3("object color", glm::value_ptr(m_objectColor));
        //     ImGui::SliderFloat("ambient strength", &m_ambientStrength, 0.0f, 1.0f);

        //     ImGui::SliderFloat("specular strength", &m_specularStrength, 0.0f, 1.0f);
        //     ImGui::DragFloat("specular shininess", &m_specularShininess, 1.0f, 1.0f, 256.0f);
        // }

        ImGui::Checkbox("animation", &m_animation);

        if (ImGui::CollapsingHeader("light", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("l.position", glm::value_ptr(m_light.position), 0.01f);
            ImGui::DragFloat3("l.direction", glm::value_ptr(m_light.direction), 0.01f);
            // ImGui::DragFloat2("l.cutoff", &m_light.cutoff, 0.1f, 0.0f, 180.0f);
            ImGui::DragFloat2("l.cutoff", glm::value_ptr(m_light.cutoff), 0.1f, 0.0f, 180.0f);
            ImGui::DragFloat("l.distance", &m_light.distance, 0.5f, 0.0f, 1000.0f);
            ImGui::ColorEdit3("l.ambient", glm::value_ptr(m_light.ambient));
            ImGui::ColorEdit3("l.diffuse", glm::value_ptr(m_light.diffuse));
            ImGui::ColorEdit3("l.specular", glm::value_ptr(m_light.specular));
        }

        // material
        if (ImGui::CollapsingHeader("material", ImGuiTreeNodeFlags_DefaultOpen)) {
            // ImGui::ColorEdit3("m.ambient", glm::value_ptr(m_light.ambient));
            // ImGui::ColorEdit3("m.diffuse", glm::value_ptr(m_light.diffuse));
            // ImGui::ColorEdit3("m.specular", glm::value_ptr(m_light.specular));
            ImGui::DragFloat("m.shininess", &m_material.shininess, 1.0f, 1.0f, 256.0f);
        }
    }
    ImGui::End();

    
    // glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    std::vector<glm::vec3> cubePositions = {
        glm::vec3( 0.0f, 0.0f, 0.0f),
        glm::vec3( 2.0f, 5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3( 2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f, 3.0f, -7.5f),
        glm::vec3( 1.3f, -2.0f, -2.5f),
        glm::vec3( 1.5f, 2.0f, -2.5f),
        glm::vec3( 1.5f, 0.2f, -1.5f),
        glm::vec3(-1.3f, 1.0f, -1.5f),
    };

    m_program->Use();

    m_cameraFront =
        glm::rotate(glm::mat4(1.0f), glm::radians(m_cameraYaw), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(m_cameraPitch), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    
    m_cameraFront = glm::rotate(glm::mat4(1.0f),
        glm::radians(m_cameraYaw), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f),
        glm::radians(m_cameraPitch), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);

    auto view = glm::lookAt( m_cameraPos,
        m_cameraPos + m_cameraFront, m_cameraUp);

    auto projection = glm::perspective(glm::radians(30.0f), 
        // (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.01f, 10.0f);
        (float)m_width / (float)m_height, 0.01f, 20.0f);
    // auto view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));
    // auto model = glm::rotate(glm::mat4(1.0f), glm::radians((float)glfwGetTime() * 60.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    auto model = glm::rotate(glm::mat4(1.0f), glm::radians(60.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    auto transform = projection * view * model;
    m_program->SetUniform("transform", transform);

    // drawing a light emmiting object - didn;;t show -- fix it
    auto lightModelTransform = glm::translate(glm::mat4(1.0), m_lightPos) *
        glm::scale(glm::mat4(1.0), glm::vec3(0.1f));
        
    m_program->Use();
    // m_program->SetUniform("lightPos", m_lightPos);
    // m_program->SetUniform("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    // m_program->SetUniform("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));
    // m_program->SetUniform("ambientStrenght", 1.0f);
/*    
    m_program->SetUniform("light.position", m_light.position);
    m_program->SetUniform("light.ambient", glm::vec3(1.0f));
    m_program->SetUniform("material.ambient", glm::vec3(1.0f));

    m_program->SetUniform("transform", projection * view * lightModelTransform);
    m_program->SetUniform("modelTransform", lightModelTransform);
*/    
    m_simpleProgram->SetUniform("color", glm::vec4(m_light.ambient + m_light.diffuse, 1.0f));
    m_simpleProgram->SetUniform("transform", projection * view * lightModelTransform);
    
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);


    m_program->Use();
    m_program->SetUniform("viewPos", m_cameraPos);
    // m_program->SetUniform("lightPos", m_lightPos);
    // m_program->SetUniform("lightColor", m_lightColor);
    // m_program->SetUniform("objectColor", m_objectColor);
    // m_program->SetUniform("ambientStrenght", m_ambientStrength);
    // m_program->SetUniform("specularStrenght", m_specularStrength);
    // m_program->SetUniform("specularShininess", m_specularShininess);

    m_program->SetUniform("light.position", m_light.position);
    m_program->SetUniform("light.attenuation", GetAttenuationCoeff(m_light.distance));
    // m_program->SetUniform("light.direction", m_light.direction);
    m_program->SetUniform("light.direction", m_light.direction);
    // m_program->SetUniform("light.cutoff", cosf(glm::radians(m_light.cutoff)));
    m_program->SetUniform("light.cutoff", glm::vec2(
        cosf(glm::radians(m_light.cutoff[0])),
        cosf(glm::radians(m_light.cutoff[0] + m_light.cutoff[1]))));
    m_program->SetUniform("light.ambient", m_light.ambient);
    m_program->SetUniform("light.diffuse", m_light.diffuse);
    m_program->SetUniform("light.specular", m_light.specular);

    // m_program->SetUniform("material.ambient", m_material.ambient);
    // m_program->SetUniform("material.diffuse", m_material.diffuse);
    m_program->SetUniform("material.diffuse", 0);   // slut number 0
    // m_program->SetUniform("material.specular", m_material.specular);
    m_program->SetUniform("material.specular", 1);
    m_program->SetUniform("material.shininess", m_material.shininess);

    glActiveTexture(GL_TEXTURE0);
    m_material.diffuse->Bind();
    glActiveTexture(GL_TEXTURE1);
    m_material.specular->Bind();


    for (size_t i = 0; i < cubePositions.size(); i++){
        auto& pos = cubePositions[i];
        auto model = glm::translate(glm::mat4(1.0f), pos);
        model = glm::rotate(model,
            // glm::radians((float)glfwGetTime() * 60.0f + 20.0f * (float)i),
            glm::radians((m_animation ? (float)glfwGetTime() : 0.0f)* 60.0f + 20.0f * (float)i),
            glm::vec3(1.0f, 0.5f, 0.0f));
        auto transform = projection * view * model;
        m_program->SetUniform("transform", transform);
        m_program->SetUniform("modelTransform", model);
        
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }


    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

}

bool Context::Init() {

    // glEnable(GL_CULL_FACE);

    // float vertices[] = {     // pos.xyz, normal.xyz, texcoord.uv
    //      0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    //      0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    //     -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    //     -0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    // };

    // uint32_t indices[] = { // GL_UNSIGNED_INT
    //     1, 3, 2, // first triangle
    //     1, 3, 0, // second triangle
    // };

    // float vertices[] = {        // drawing a cube
    //     -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
    //      0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
    //      0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
    //     -0.5f,  0.5f, -0.5f, 0.0f, 1.0f,

    //     -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
    //      0.5f, -0.5f,  0.5f, 1.0f, 0.0f,
    //      0.5f,  0.5f,  0.5f, 1.0f, 1.0f,
    //     -0.5f,  0.5f,  0.5f, 0.0f, 1.0f,

    //     -0.5f,  0.5f,  0.5f, 1.0f, 0.0f,
    //     -0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
    //     -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    //     -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,

    //      0.5f,  0.5f,  0.5f, 1.0f, 0.0f,
    //      0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
    //      0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    //      0.5f, -0.5f,  0.5f, 0.0f, 0.0f,

    //     -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    //      0.5f, -0.5f, -0.5f, 1.0f, 1.0f,
    //      0.5f, -0.5f,  0.5f, 1.0f, 0.0f,
    //     -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,

    //     -0.5f,  0.5f, -0.5f, 0.0f, 1.0f,
    //      0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
    //      0.5f,  0.5f,  0.5f, 1.0f, 0.0f,
    //     -0.5f,  0.5f,  0.5f, 0.0f, 0.0f,
    // };

    float vertices[] = {    // pos.xyz, normal.xyz, texcoord.uv
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
    };

    uint32_t indices[] = {
         0,  2,  1,  2,  0,  3,
         4,  5,  6,  6,  7,  4,
         8,  9, 10, 10, 11,  8,
        12, 14, 13, 14, 12, 15,
        16, 17, 18, 18, 19, 16,
        20, 22, 21, 22, 20, 23,
    };

    m_vertexLayout = VertexLayout::Create();
    m_vertexBuffer = Buffer::CreateWithData(GL_ARRAY_BUFFER, GL_STATIC_DRAW,
        // vertices, sizeof(float) * 32);
        // vertices, sizeof(float) * 120);
        vertices, sizeof(float) * 8 * 6 * 4);

    m_vertexLayout->SetAttrib(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, 0);
    m_vertexLayout->SetAttrib(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, sizeof(float) * 3);
    m_vertexLayout->SetAttrib(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, sizeof(float) * 6);
    // (layer, dimension, type, normalize, stride, off-set)

    // m_vertexLayout->SetAttrib(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
    // m_vertexLayout->SetAttrib(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, sizeof(float) * 3);
   
    m_indexBuffer = Buffer::CreateWithData(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW,
        // indices, sizeof(uint32_t) * 6);
        indices, sizeof(uint32_t) * 36);

    // shader loading
    // ShaderPtr vertShader = Shader::CreateFromFile("./shader/lighting-1.vs", GL_VERTEX_SHADER);
    // ShaderPtr fragShader = Shader::CreateFromFile("./shader/lighting-1.fs", GL_FRAGMENT_SHADER);
    // if (!vertShader || !fragShader)
    //     return false;
    // SPDLOG_INFO("vertex shader id: {}", vertShader->Get());
    // SPDLOG_INFO("fragment shader id: {}", fragShader->Get());

    // m_program = Program::Create({fragShader, vertShader});
    // if (!m_program)
    //     return false;
    // SPDLOG_INFO("program id: {}", m_program->Get());

    m_simpleProgram = Program::Create("./shader/simple.vs", "./shader/simple.fs");
    if (!m_simpleProgram)
        return false;
    SPDLOG_INFO("simple program id: {}", m_simpleProgram->Get());    

    // m_program = Program::Create("./shader/lighting-1.vs", "./shader/lighting-1.fs");
    m_program = Program::Create("./shader/lighting-3.vs", "./shader/lighting-3.fs");
    if (!m_program)
        return false;
    SPDLOG_INFO("program id: {}", m_program->Get());  

    glClearColor(0.1f, 0.2f, 0.3f, 0.0f);


    // auto image = Image::Load("./image/body.png");
    // if (!image) 
    //     return false;
    // SPDLOG_INFO("image: {}x{}, {} channels",
    //     image->GetWidth(), image->GetHeight(), image->GetChannelCount());

    auto image = Image::Create(512, 512);
    image->SetCheckImage(64, 64);

    m_texture = Texture::CreateFromImage(image.get());
    // m_material.specular = Texture::CreateFromImage(image.get());
  
    auto image2 = Image::Load("./image/face-3.jpg");
    m_texture2 = Texture::CreateFromImage(image2.get());

    m_material.diffuse = Texture::CreateFromImage(Image::Load("./image/face-4.jpg").get());
    // m_material.specular = Texture::CreateFromImage(Image::Load("./image/metal_sp.png").get());
    m_material.specular = Texture::CreateFromImage(image.get());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture->Get());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_texture2->Get());

    m_program->Use();
    // glUniform1i(glGetUniformLocation(m_program->Get(), "tex"), 0);
    // glUniform1i(glGetUniformLocation(m_program->Get(), "tex2"), 1);

    m_program->SetUniform("tex", 0);
    m_program->SetUniform("tex2", 1);

    // auto transform = glm::mat4(1.0f);
    // auto transform = glm::translate(glm::mat4(1.0f), glm::vec3(-0.2f, -0.3f, 0.0f));
    // transform += glm::rotate(
    //     glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)),
    //     glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)
    // );

    auto model = glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(1.0f, 1.0f, 0.0f));
    auto view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
    auto projection = glm::perspective(glm::radians(30.0f), (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.01f, 10.0f);

    auto transform = projection * view * model;

    // auto transformLoc = glGetUniformLocation(m_program->Get(), "transform");
    // glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
    m_program->SetUniform("transform", transform);

    return true;
}


//////////////////////  Basic Window  //////////////////////////////////

void OnFramebufferSizeChange(GLFWwindow* window, int width, int height) {
    // SPDLOG_INFO("framebuffer size changed: ({} x {})", width, height);
    // glViewport(0, 0, width, height);
    auto context = (Context*)glfwGetWindowUserPointer(window);
    context->Reshape(width, height);
}

void OnKeyEvent(GLFWwindow* window,
    int key, int scancode, int action, int mods) {
    SPDLOG_INFO("key: {}, scancode: {}, action: {}, mods: {}{}{}",
        key, scancode,
        action == GLFW_PRESS ? "Pressed" :
        action == GLFW_RELEASE ? "Released" :
        action == GLFW_REPEAT ? "Repeat" : "Unknown",
        mods & GLFW_MOD_CONTROL ? "C" : "-",
        mods & GLFW_MOD_SHIFT ? "S" : "-",
        mods & GLFW_MOD_ALT ? "A" : "-");

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

/* casting in c++ 

    (type) value;  in c 
    static_cast<type> value;  in c++
    dynamic_cast<type> value;
    const_cast<type> value;
    reinterpret_cast<type> value;
*/

void OnCharEvent(GLFWwindow* window, unsigned int ch) {
    ImGui_ImplGlfw_CharCallback(window, ch);
}

void OnCursorPos(GLFWwindow* window, double x, double y) {
    auto context = (Context*)glfwGetWindowUserPointer(window);
    // auto context = reinterpret_cast<Context*>glfwGetWindowUserPointer(window);
    context->MouseMove(x, y);
}

void OnMouseButton(GLFWwindow* window, int button, int action, int modifier) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, modifier);
    auto context = (Context*)glfwGetWindowUserPointer(window);
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    context->MouseButton(button, action, x, y);
}

void OnScroll(GLFWwindow* window, double xoffset, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

// void Render() {
//     glClearColor(0.1f, 0.2f, 0.3f, 0.0f);
//     glClear(GL_COLOR_BUFFER_BIT);
// }

///////////////////// Main() ///////////////////////////////

int main(int argc, const char** argv) {
    // 시작을 알리는 로그
    SPDLOG_INFO("Start program");

    // glfw 라이브러리 초기화, 실패하면 에러 출력후 종료
    SPDLOG_INFO("Initialize glfw");
    if (!glfwInit()) {
        const char* description = nullptr;
        glfwGetError(&description);
        SPDLOG_ERROR("failed to initialize glfw: {}", description);
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw 윈도우 생성, 실패하면 에러 출력후 종료
    SPDLOG_INFO("Create glfw window");
    auto window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME,
      nullptr, nullptr);
    
    // GLFW to make the window the current context
	glfwMakeContextCurrent(window);

    if (!window) {
        SPDLOG_ERROR("failed to create glfw window");
        glfwTerminate();
        return -1;
    }

    // glad를 활용한 OpenGL 함수 로딩
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Couldn't load opengl" << std::endl;
        SPDLOG_ERROR("failed to initialize glad");
		glfwTerminate();
		return -1;
	}

    auto glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    SPDLOG_INFO("OpenGL context version: {}", glVersion);

    auto imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(imguiContext);
    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init();
    ImGui_ImplOpenGL3_CreateFontsTexture();
    ImGui_ImplOpenGL3_CreateDeviceObjects();


    auto context = Context::Create();
    if (!context) {
        SPDLOG_ERROR("failed to create context");
        glfwTerminate();
        return -1;
    }

    glfwSetWindowUserPointer(window, context.get());
    OnFramebufferSizeChange(window, WINDOW_WIDTH, WINDOW_HEIGHT);  // reshape()
    glfwSetFramebufferSizeCallback(window, OnFramebufferSizeChange);

    glfwSetKeyCallback(window, OnKeyEvent);

    glfwSetCharCallback(window, OnCharEvent);
    glfwSetCursorPosCallback(window, OnCursorPos);
    glfwSetMouseButtonCallback(window, OnMouseButton);
    glfwSetScrollCallback(window, OnScroll);

    /////////////////////////  Main Loop  /////////////////////////////

    // glfw 루프 실행, 윈도우 close 버튼을 누르면 정상 종료
    SPDLOG_INFO("Start main loop");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplGlfw_NewFrame();  //
        ImGui::NewFrame();

        context->ProcessInput(window);
        context->Render();

        ImGui::Render();    //
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
        
    }

    context.reset();

    ImGui_ImplOpenGL3_DestroyFontsTexture();    //
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(imguiContext);

    glfwTerminate();
    return 0;
}