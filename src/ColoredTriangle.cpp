

#include <spdlog/spdlog.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>
#include <optional>
#include <vector>
#include <memory>

#include <fstream>
#include <sstream>
#include <string>

// class Shader; 
// using ShaderUPtr = std::unique_ptr<Shader>; 
// using ShaderPtr = std::shared_ptr<Shader>; 
// using ShaderWPtr = std::weak_ptr<Shader>;

#define CLASS_PTR(klassName) \
class klassName; \
using klassName ## UPtr = std::unique_ptr<klassName>; \
using klassName ## Ptr = std::shared_ptr<klassName>; \
using klassName ## WPtr = std::weak_ptr<klassName>;


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


CLASS_PTR(Program)
class Program {
public:
    static ProgramUPtr Create(const std::vector<ShaderPtr>& shaders);

    ~Program();
    uint32_t Get() const { return m_program; }
    void Use() const;
private:
    Program() {}
    bool Link(const std::vector<ShaderPtr>& shaders);
    uint32_t m_program { 0 };
};


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


CLASS_PTR(Context)
class Context {
public:
    static ContextUPtr Create();
    void Render();    
private:
    Context() {}
    bool Init();
    ProgramUPtr m_program;

    // uint32_t m_vertexArrayObject;
    // uint32_t m_vertexBuffer;
    // uint32_t m_indexBuffer;

    VertexLayoutUPtr m_vertexLayout;
    BufferUPtr m_vertexBuffer;
    BufferUPtr m_indexBuffer;
};



///////////////// end of define classes ////////////////////////

// optional is not to assign a memory for the empty file
std::optional<std::string> LoadTextFile(const std::string& filename) {
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        SPDLOG_ERROR("failed to open file: {}", filename);
        return {};
    }

    std::stringstream text;
    text << fin.rdbuf();
    return text.str();
}



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



ProgramUPtr Program::Create(const std::vector<ShaderPtr>& shaders) {
    auto program = ProgramUPtr(new Program());
    if (!program->Link(shaders))
        return nullptr;
    return std::move(program);
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


///////////////////// Context ////////////////////////////////

ContextUPtr Context::Create() {
    auto context = ContextUPtr(new Context());
    if (!context->Init())
        return nullptr;
    return std::move(context);
}

void Context::Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // modified for blinking 
    static float time = 0.0f;
    float t = sinf(time) * 0.5f + 0.5f;
    // auto loc = glGetUniformLocation(m_program->Get(), "color");
    // glUniform4f(loc, 0, 0, (1.0f-t)*(1.0f-t), 1.0f);
    
    // glUseProgram(m_program->Get());
    // glDrawArrays(GL_POINTS, 0, 1);
    // glDrawArrays(GL_TRIANGLES, 0, 3);
    // glDrawArrays(GL_LINE_STRIP, 0, 4);
    // SPDLOG_INFO("m_program->Get(): {}", m_program->Get());

    m_program->Use();
    // glDrawElements(GL_LINE_STRIP, 5, GL_UNSIGNED_INT, 0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    time += 0.5f;
}

bool Context::Init() {

    // glEnable(GL_CULL_FACE);

    float vertices[] = {
         0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f,
         0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
         0.0f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
    };

    uint32_t indices[] = { // GL_UNSIGNED_INT
        1, 0, 2, // first triangle
        // 1, 3, 0, // second triangle
        // 2, 6, 5,
    };

    m_vertexLayout = VertexLayout::Create();
    m_vertexBuffer = Buffer::CreateWithData(GL_ARRAY_BUFFER, GL_STATIC_DRAW,
        vertices, sizeof(float) * 36);

    m_vertexLayout->SetAttrib(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, 0);
    m_vertexLayout->SetAttrib(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, sizeof(float) * 3);
    
    m_indexBuffer = Buffer::CreateWithData(
        GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW,
        indices, sizeof(uint32_t) * 6);

// ***********************************************

    // unsigned int id = glCreateShader(type); //셰이더 객체 생성(마찬가지)
	// const char* src = source.c_str();
	// glShaderSource(id, // 셰이더의 소스 코드 명시, 소스 코드를 명시할 셰이더 객체 id
	// 				1, // 몇 개의 소스 코드를 명시할 것인지
	// 				&src, // 실제 소스 코드가 들어있는 문자열의 주소값
	// 				nullptr); // 해당 문자열 전체를 사용할 경우 nullptr입력, 아니라면 길이 명시
	// glCompileShader(id); // id에 해당하는 셰이더 컴파일


// ****************************************************
    ShaderPtr vertShader = Shader::CreateFromFile("./shader/per_vertex_color.vs", GL_VERTEX_SHADER);
    ShaderPtr fragShader = Shader::CreateFromFile("./shader/per_vertex_color.fs", GL_FRAGMENT_SHADER);
    if (!vertShader || !fragShader)
        return false;
    SPDLOG_INFO("vertex shader id: {}", vertShader->Get());
    SPDLOG_INFO("fragment shader id: {}", fragShader->Get());

    m_program = Program::Create({fragShader, vertShader});
    if (!m_program)
        return false;
    SPDLOG_INFO("program id: {}", m_program->Get());

    glClearColor(0.1f, 0.2f, 0.3f, 0.0f);

    // VOA for a single point
    // uint32_t vao = 0;
    // glGenVertexArrays(1, &vao);
    // glBindVertexArray(vao);

    return true;
}

//////////////////////  Basic Window  //////////////////////////////////

void OnFramebufferSizeChange(GLFWwindow* window, int width, int height) {
    SPDLOG_INFO("framebuffer size changed: ({} x {})", width, height);
    glViewport(0, 0, width, height);
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

    // auto vertShader = Shader::CreateFromFile("./shader/simple.vs", GL_VERTEX_SHADER);
    // auto fragShader = Shader::CreateFromFile("./shader/simple.fs", GL_FRAGMENT_SHADER);

    // ShaderPtr vertShader = Shader::CreateFromFile("./shader/simple.vs", GL_VERTEX_SHADER);
    // ShaderPtr fragShader = Shader::CreateFromFile("./shader/simple.fs", GL_FRAGMENT_SHADER);
    // if (!vertShader || !fragShader)
    //     return false;
    // SPDLOG_INFO("vertex shader id: {}", vertShader->Get());
    // SPDLOG_INFO("fragment shader id: {}", fragShader->Get());

    // auto program = Program::Create({fragShader, vertShader});
    // if (!program)
    //     return false;
    // SPDLOG_INFO("program id: {}", program->Get());

    auto context = Context::Create();
    if (!context) {
        SPDLOG_ERROR("failed to create context");
        glfwTerminate();
        return -1;
    }


    OnFramebufferSizeChange(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSetFramebufferSizeCallback(window, OnFramebufferSizeChange);
    glfwSetKeyCallback(window, OnKeyEvent);

    // OpenGL what rgba color we want to use for screen refresh.
	// glClearColor(0.1f, 0.2f, 0.3f, 1.0f);    // moved to Render

    /////////////////////////  Main Loop  /////////////////////////////

    // glfw 루프 실행, 윈도우 close 버튼을 누르면 정상 종료
    SPDLOG_INFO("Start main loop");

    while (!glfwWindowShouldClose(window)) {
        // Render();
        // glClearColor(0.1f, 0.2f, 0.3f, 0.0f);
        // glClear(GL_COLOR_BUFFER_BIT);

        // glUseProgram(program->Get());
        // glDrawArrays(GL_POINTS, 0, 1);
        context->Render();

		glfwSwapBuffers(window);
        glfwPollEvents();
    }

    context.reset();

    glfwTerminate();
    return 0;
}