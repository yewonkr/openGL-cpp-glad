

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

    ~Program();
    uint32_t Get() const { return m_program; }
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


CLASS_PTR(Context)
class Context {
public:
    static ContextUPtr Create();
    void Render();    
private:
    Context() {}
    bool Init();
    ProgramUPtr m_program;
};

ContextUPtr Context::Create() {
    auto context = ContextUPtr(new Context());
    if (!context->Init())
        return nullptr;
    return std::move(context);
}

void Context::Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_program->Get());
    glDrawArrays(GL_POINTS, 0, 1);
}

bool Context::Init() {
    ShaderPtr vertShader = Shader::CreateFromFile("./shader/simple.vs", GL_VERTEX_SHADER);
    ShaderPtr fragShader = Shader::CreateFromFile("./shader/simple.fs", GL_FRAGMENT_SHADER);
    if (!vertShader || !fragShader)
        return false;
    SPDLOG_INFO("vertex shader id: {}", vertShader->Get());
    SPDLOG_INFO("fragment shader id: {}", fragShader->Get());

    m_program = Program::Create({fragShader, vertShader});
    if (!m_program)
        return false;
    SPDLOG_INFO("program id: {}", m_program->Get());

    glClearColor(0.1f, 0.2f, 0.3f, 0.0f);

    uint32_t vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    return true;
}


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

// void Render() {
//     glClearColor(0.1f, 0.2f, 0.3f, 0.0f);
//     glClear(GL_COLOR_BUFFER_BIT);
// }


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

    // auto glVersion = glGetString(GL_VERSION);
	// std::cout << glVersion << std::endl;

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
