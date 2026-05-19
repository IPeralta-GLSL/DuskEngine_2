#include "imgui_impl_opengl3.h"

#include <imgui/imgui.h>

#include <QOpenGLWidget>
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_0>

// Vertex shader
static const char* VERTEX_SHADER = R"(
#version 130
in vec2 Position;
in vec2 UV;
in vec4 Color;
uniform mat4 ProjMtx;
out vec2 Frag_UV;
out vec4 Frag_Color;
void main()
{
    Frag_UV = UV;
    Frag_Color = Color;
    gl_Position = ProjMtx * vec4(Position, 0.0, 1.0);
}
)";

// Fragment shader
static const char* FRAGMENT_SHADER = R"(
#version 130
uniform sampler2D Texture;
in vec2 Frag_UV;
in vec4 Frag_Color;
out vec4 Out_Color;
void main()
{
    Out_Color = Frag_Color * texture(Texture, Frag_UV);
}
)";

// OpenGL state
static unsigned int g_VboHandle = 0;
static unsigned int g_EboHandle = 0;
static unsigned int g_VaoHandle = 0;
static int g_ShaderHandle = 0;
static int g_AttribLocationTex = 0;
static int g_AttribLocationProjMtx = 0;
static int g_AttribLocationPosition = 0;
static int g_AttribLocationUV = 0;
static int g_AttribLocationColor = 0;
static unsigned int g_FontTexture = 0;
static QOpenGLFunctions_3_0* g_GLFuncs = nullptr;

bool ImGui_ImplOpenGL3_Init(const char* glsl_version)
{
    (void)glsl_version;
    
    // Get OpenGL 3.0 functions
    g_GLFuncs = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_0>();
    if (!g_GLFuncs)
    {
        qWarning("OpenGL 3.0 not supported!");
        return false;
    }
    g_GLFuncs->initializeOpenGLFunctions();
    
    // Create shaders
    const GLchar* vertex_shader_str = VERTEX_SHADER;
    const GLchar* fragment_shader_str = FRAGMENT_SHADER;
    
    GLuint vert = g_GLFuncs->glCreateShader(GL_VERTEX_SHADER);
    g_GLFuncs->glShaderSource(vert, 1, &vertex_shader_str, nullptr);
    g_GLFuncs->glCompileShader(vert);
    
    GLuint frag = g_GLFuncs->glCreateShader(GL_FRAGMENT_SHADER);
    g_GLFuncs->glShaderSource(frag, 1, &fragment_shader_str, nullptr);
    g_GLFuncs->glCompileShader(frag);
    
    g_ShaderHandle = g_GLFuncs->glCreateProgram();
    g_GLFuncs->glAttachShader(g_ShaderHandle, vert);
    g_GLFuncs->glAttachShader(g_ShaderHandle, frag);
    g_GLFuncs->glLinkProgram(g_ShaderHandle);
    
    g_GLFuncs->glDeleteShader(vert);
    g_GLFuncs->glDeleteShader(frag);
    
    // Get locations
    g_AttribLocationTex = g_GLFuncs->glGetUniformLocation(g_ShaderHandle, "Texture");
    g_AttribLocationProjMtx = g_GLFuncs->glGetUniformLocation(g_ShaderHandle, "ProjMtx");
    g_AttribLocationPosition = g_GLFuncs->glGetAttribLocation(g_ShaderHandle, "Position");
    g_AttribLocationUV = g_GLFuncs->glGetAttribLocation(g_ShaderHandle, "UV");
    g_AttribLocationColor = g_GLFuncs->glGetAttribLocation(g_ShaderHandle, "Color");
    
    // Create VAO/VBO/EBO
    g_GLFuncs->glGenVertexArrays(1, &g_VaoHandle);
    g_GLFuncs->glGenBuffers(1, &g_VboHandle);
    g_GLFuncs->glGenBuffers(1, &g_EboHandle);
    
    g_GLFuncs->glBindVertexArray(g_VaoHandle);
    g_GLFuncs->glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    g_GLFuncs->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_EboHandle);
    
    // Vertex format
    g_GLFuncs->glEnableVertexAttribArray(g_AttribLocationPosition);
    g_GLFuncs->glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void*)IM_OFFSETOF(ImDrawVert, pos));
    
    g_GLFuncs->glEnableVertexAttribArray(g_AttribLocationUV);
    g_GLFuncs->glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void*)IM_OFFSETOF(ImDrawVert, uv));
    
    g_GLFuncs->glEnableVertexAttribArray(g_AttribLocationColor);
    g_GLFuncs->glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (void*)IM_OFFSETOF(ImDrawVert, col));
    
    g_GLFuncs->glBindVertexArray(0);
    
    // Create font texture
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    
    g_GLFuncs->glGenTextures(1, &g_FontTexture);
    g_GLFuncs->glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    g_GLFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    g_GLFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    g_GLFuncs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    
    io.Fonts->TexID = (void*)(intptr_t)g_FontTexture;
    
    return true;
}

void ImGui_ImplOpenGL3_Shutdown()
{
    if (g_GLFuncs)
    {
        if (g_VaoHandle) g_GLFuncs->glDeleteVertexArrays(1, &g_VaoHandle);
        if (g_VboHandle) g_GLFuncs->glDeleteBuffers(1, &g_VboHandle);
        if (g_EboHandle) g_GLFuncs->glDeleteBuffers(1, &g_EboHandle);
        if (g_FontTexture) g_GLFuncs->glDeleteTextures(1, &g_FontTexture);
        if (g_ShaderHandle) g_GLFuncs->glDeleteProgram(g_ShaderHandle);
    }
    
    g_VaoHandle = g_VboHandle = g_EboHandle = g_ShaderHandle = g_FontTexture = 0;
    g_GLFuncs = nullptr;
}

void ImGui_ImplOpenGL3_NewFrame()
{
}

void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data)
{
    if (!draw_data || draw_data->CmdListsCount == 0 || !g_GLFuncs)
        return;
    
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;
    
    // Setup GL state
    g_GLFuncs->glEnable(GL_BLEND);
    g_GLFuncs->glBlendEquation(GL_FUNC_ADD);
    g_GLFuncs->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    g_GLFuncs->glDisable(GL_CULL_FACE);
    g_GLFuncs->glDisable(GL_DEPTH_TEST);
    g_GLFuncs->glEnable(GL_SCISSOR_TEST);
    
    g_GLFuncs->glViewport(0, 0, fb_width, fb_height);
    
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    
    const float ortho_projection[4][4] =
    {
        { 2.0f / (R - L), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (T - B), 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
        { (R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f },
    };
    
    g_GLFuncs->glUseProgram(g_ShaderHandle);
    g_GLFuncs->glUniform1i(g_AttribLocationTex, 0);
    g_GLFuncs->glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
    
    g_GLFuncs->glBindVertexArray(g_VaoHandle);
    
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        
        g_GLFuncs->glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), 
                     (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);
        
        g_GLFuncs->glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx),
                     (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);
        
        const ImDrawIdx* idx_buffer_offset = nullptr;
        
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                int scissor_x = (int)pcmd->ClipRect.x;
                int scissor_y = (int)(fb_height - pcmd->ClipRect.w);
                int scissor_w = (int)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                int scissor_h = (int)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                
                g_GLFuncs->glScissor(scissor_x, scissor_y, scissor_w, scissor_h);
                g_GLFuncs->glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                g_GLFuncs->glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                              sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                              (const GLvoid*)idx_buffer_offset);
            }
            
            idx_buffer_offset += pcmd->ElemCount;
        }
    }
    
    g_GLFuncs->glBindVertexArray(0);
    g_GLFuncs->glDisable(GL_SCISSOR_TEST);
}