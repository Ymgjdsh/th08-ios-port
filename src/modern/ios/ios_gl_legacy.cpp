#include "ios_gl_legacy.hpp"
#include "ios_touch.hpp"

#include <OpenGLES/ES2/glext.h>
#include <stdio.h>
#include <string.h>
#include <vector>

namespace
{
struct ImmediateVertex
{
    GLfloat position[3];
    GLfloat color[4];
    GLfloat texcoord[2];
};

std::vector<ImmediateVertex> g_vertices;
GLenum g_primitiveMode = GL_TRIANGLES;
GLfloat g_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat g_texcoord[2] = {0.0f, 0.0f};
GLuint g_program;
GLint g_position;
GLint g_colorAttribute;
GLint g_texcoordAttribute;
GLint g_resolution;
GLint g_texture;
GLint g_useTexture;
bool g_initialized;

GLuint CompileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[1024]; GLsizei length = 0;
        glGetShaderInfoLog(shader, sizeof(log), &length, log);
        fprintf(stderr, "th08-modern: GLES2 shader compile failed: %.*s\n", (int)length, log);
        glDeleteShader(shader); return 0;
    }
    return shader;
}

bool Initialize()
{
    if (g_initialized) return g_program != 0;
    g_initialized = true;
    static const char vertexSource[] =
        "attribute vec3 aPosition;\n"
        "attribute vec4 aColor;\n"
        "attribute vec2 aTexCoord;\n"
        "uniform vec2 uResolution;\n"
        "varying vec4 vColor;\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "  vec2 p = aPosition.xy / uResolution * 2.0 - 1.0;\n"
        "  gl_Position = vec4(p.x, -p.y, aPosition.z, 1.0);\n"
        "  vColor = aColor; vTexCoord = aTexCoord;\n"
        "}\n";
    static const char fragmentSource[] =
        "precision mediump float;\n"
        "uniform sampler2D uTexture;\n"
        "uniform int uUseTexture;\n"
        "varying vec4 vColor;\n"
        "varying vec2 vTexCoord;\n"
        "void main() {\n"
        "  vec4 texel = uUseTexture != 0 ? texture2D(uTexture, vTexCoord) : vec4(1.0);\n"
        "  gl_FragColor = texel * vColor;\n"
        "}\n";
    GLuint vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (vertex == 0 || fragment == 0)
    {
        th08::modern::LogStartup("gles2: shader compilation failed");
        return false;
    }
    g_program = glCreateProgram();
    glAttachShader(g_program, vertex); glAttachShader(g_program, fragment);
    glBindAttribLocation(g_program, 0, "aPosition");
    glBindAttribLocation(g_program, 1, "aColor");
    glBindAttribLocation(g_program, 2, "aTexCoord");
    glLinkProgram(g_program);
    glDeleteShader(vertex); glDeleteShader(fragment);
    GLint success = GL_FALSE;
    glGetProgramiv(g_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024]; GLsizei length = 0;
        glGetProgramInfoLog(g_program, sizeof(log), &length, log);
        fprintf(stderr, "th08-modern: GLES2 program link failed: %.*s\n", (int)length, log);
        glDeleteProgram(g_program); g_program = 0;
        th08::modern::LogStartup("gles2: program link failed");
        return false;
    }
    g_position = 0; g_colorAttribute = 1; g_texcoordAttribute = 2;
    g_resolution = glGetUniformLocation(g_program, "uResolution");
    g_texture = glGetUniformLocation(g_program, "uTexture");
    g_useTexture = glGetUniformLocation(g_program, "uUseTexture");
    th08::modern::LogStartup("gles2: immediate shader ready");
    return true;
}
}

void glBegin(GLenum mode)
{
    static bool loggedBegin;
    if (!loggedBegin)
    {
        loggedBegin = true;
        th08::modern::LogStartup("gles2: glBegin reached");
    }
    g_primitiveMode = mode;
    g_vertices.clear();
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    g_color[0] = red / 255.0f; g_color[1] = green / 255.0f;
    g_color[2] = blue / 255.0f; g_color[3] = alpha / 255.0f;
}

void glTexCoord2f(GLfloat u, GLfloat v)
{
    g_texcoord[0] = u; g_texcoord[1] = v;
}

void glVertex2f(GLfloat x, GLfloat y)
{
    glVertex3f(x, y, 0.0f);
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    ImmediateVertex vertex;
    vertex.position[0] = x; vertex.position[1] = y; vertex.position[2] = z;
    memcpy(vertex.color, g_color, sizeof(g_color));
    memcpy(vertex.texcoord, g_texcoord, sizeof(g_texcoord));
    g_vertices.push_back(vertex);
}

void glEnd()
{
    static bool loggedEnd;
    if (!loggedEnd)
    {
        loggedEnd = true;
        th08::modern::LogStartup("gles2: glEnd reached");
    }
    if (g_vertices.empty() || !Initialize()) return;
    GLint viewport[4] = {0, 0, 640, 480};
    glGetIntegerv(GL_VIEWPORT, viewport);
    GLint textureName = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureName);

    while (glGetError() != GL_NO_ERROR) {}
    glUseProgram(g_program);
    glUniform2f(g_resolution, viewport[2] > 0 ? (GLfloat)viewport[2] : 640.0f,
                viewport[3] > 0 ? (GLfloat)viewport[3] : 480.0f);
    glUniform1i(g_texture, 0);
    // The D3D shim's texture-stage state is not visible to this small shader
    // wrapper. A bound texture is the authoritative signal for the legacy
    // immediate calls and is sufficient for the title/background paths.
    glUniform1i(g_useTexture, textureName != 0 ? 1 : 0);
    glEnableVertexAttribArray(g_position); glEnableVertexAttribArray(g_colorAttribute);
    glEnableVertexAttribArray(g_texcoordAttribute);
    const GLsizei stride = sizeof(ImmediateVertex);
    const GLubyte *base = reinterpret_cast<const GLubyte *>(&g_vertices[0]);
    glVertexAttribPointer(g_position, 3, GL_FLOAT, GL_FALSE, stride, base + 0);
    glVertexAttribPointer(g_colorAttribute, 4, GL_FLOAT, GL_FALSE, stride,
                          base + sizeof(GLfloat) * 3);
    glVertexAttribPointer(g_texcoordAttribute, 2, GL_FLOAT, GL_FALSE, stride,
                          base + sizeof(GLfloat) * 7);
    glDrawArrays(g_primitiveMode, 0, (GLsizei)g_vertices.size());
    glDisableVertexAttribArray(g_position); glDisableVertexAttribArray(g_colorAttribute);
    glDisableVertexAttribArray(g_texcoordAttribute);
}
