#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif // _MSC_VER

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#include <windowsx.h>
#include <fileapi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#undef near
#undef far
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#endif // _WIN32

#include <stdio.h>
#include <assert.h>

#include <stdint.h>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef s8  b8;
typedef s32 b32;
typedef float f32;
typedef double f64;
#define internal static
#define global static
#define local_persist static

#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define STB_IMAGE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4244)
#include <stb_image.h>
#pragma warning(pop)

#define WIDTH 1600
#define HEIGHT 900

#define HANDMADE_MATH_USE_DEGREES
#include "HandmadeMath.h"

#include "skelly.h"

#define CLAMP(V, Min, Max) ((V) < (Min) ? (Min) : (V) > (Max) ? (Max) : (V))


global Assimp::Importer importer;

global bool window_should_close;

global IDXGISwapChain *swapchain;
global ID3D11Device *d3d_device;
global ID3D11DeviceContext *d3d_context;
global ID3D11RenderTargetView *render_target;

global bool keys_down[256];
global bool keys_pressed[256];

global HMM_Vec2 last_mouse_p;
global HMM_Vec2 mouse_p;

internal HMM_Vec2
get_mouse_delta() {
    HMM_Vec2 result = mouse_p - last_mouse_p;
    return result;
}

internal bool
input_button_down(Keycode key) {
    assert(key > Key_Begin && key < Key_Last);
    return keys_down[(int)key];
}

internal bool
input_button_pressed(Keycode key) {
    assert(key > Key_Begin && key < Key_Last);
    return keys_pressed[(int)key];
}


global LARGE_INTEGER performance_frequency;
internal f32
win32_get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
    float Result = (float)(end.QuadPart - start.QuadPart) / (float)performance_frequency.QuadPart;
    return Result;
}

internal LARGE_INTEGER
win32_get_wall_clock() {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

internal void
win32_get_window_dim(HWND window, int *w, int *h) {
    RECT rc;
    GetClientRect(window, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    if (w) *w = width;
    if (h) *h = height;
}

internal void
os_popup(char *str) {
    MessageBoxA(NULL, str, NULL, MB_OK | MB_ICONERROR | MB_ICONEXCLAMATION);
}

internal void
os_popupf(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(nullptr, 0, fmt, args);
    char *buffer = new char[len + 1];
    vsnprintf(buffer, len, fmt, args);
    buffer[len] = 0;
    va_end(args);
    os_popup(buffer);
    free(buffer);
}

LRESULT CALLBACK
window_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    switch (Msg) {
    case WM_KEYDOWN:
    case WM_KEYUP: {
        int vk_code = (int)wParam;
        bool key_down = (((u32)lParam >> 31) == 0);
        bool key_down_previous = (((u32)lParam >> 30) == 1);
        if (vk_code > (int)Key_Begin && vk_code < (int)Key_Last) {
            keys_pressed[vk_code] = key_down_previous;
            keys_down[vk_code] = key_down;
        }
        break;
    }
    case WM_SIZE: {
        // NOTE: Resize render target view
        if (swapchain) {
            d3d_context->OMSetRenderTargets(0, 0, 0);

            // Release all outstanding references to the swap chain's buffers.
            render_target->Release();

            // Preserve the existing buffer count and format.
            // Automatically choose the width and height to match the client rect for HWNDs.
            HRESULT hr = swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
                                            
            // Perform error handling here!

            // Get buffer and create a render-target-view.
            ID3D11Texture2D* buffer;
            hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**) &buffer);
            // Perform error handling here!

            hr = d3d_device->CreateRenderTargetView(buffer, NULL, &render_target);
            // Perform error handling here!
            buffer->Release();

            d3d_context->OMSetRenderTargets(1, &render_target, NULL);
        }
        break;
    }
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    default:
        result = DefWindowProcA(hWnd, Msg, wParam, lParam);
    }
    return result;
}

internal void
upload_constants(ID3D11Buffer *constant_buffer, void *constants, int constants_size) {
    D3D11_MAPPED_SUBRESOURCE res{};
    if (d3d_context->Map(constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res) != S_OK) {
        printf("Failed to map constant buffer\n");
    }
    memcpy(res.pData, constants, constants_size);
    d3d_context->Unmap(constant_buffer, 0);
}

internal Shader_Program
create_shader_program(std::string source, std::string vs_entry, std::string ps_entry, D3D11_INPUT_ELEMENT_DESC *items, int item_count, int constants_size) {
    Shader_Program program{};
    ID3DBlob *vs_blob, *ps_blob, *error_blob;
    if (D3DCompile(source.data(), source.length(), nullptr, NULL, NULL, vs_entry.data(), "vs_5_0", D3DCOMPILE_DEBUG, 0, &vs_blob, &error_blob) != S_OK) {
        fprintf(stderr, "Failed to compile Vertex Shader\n%s", (char *)error_blob->GetBufferPointer());
    }
    if (D3DCompile(source.data(), source.length(), nullptr, NULL, NULL, ps_entry.data(), "ps_5_0", D3DCOMPILE_DEBUG, 0, &ps_blob, &error_blob) != S_OK) {
        fprintf(stderr, "Failed to compile Pixel Shader\n%s", (char *)error_blob->GetBufferPointer());
    }

    if (d3d_device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &program.vertex_shader) != S_OK) {
        fprintf(stderr, "Failed to compile Vertex Shader\n");
    }
    if (d3d_device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), NULL, &program.pixel_shader) != S_OK) {
        fprintf(stderr, "Failed to create Pixel Shader\n");
    }

    ID3D11InputLayout *input_layout = nullptr;
    if (d3d_device->CreateInputLayout(items, item_count, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &program.input_layout) != S_OK) {
        fprintf(stderr, "Failed to create Input Layout\n");
    }

    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = constants_size;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (d3d_device->CreateBuffer(&desc, NULL, &program.constant_buffer) != S_OK) {
        fprintf(stderr, "Failed to create constant buffer\n");
    }

    return program;
}

internal bool
load_texture(std::string file_name, Texture *texture) {
    bool success = true;
    ID3D11ShaderResourceView *srv = nullptr;
    int width, height, nc;
    u8 *image_data = stbi_load(file_name.data(), &width, &height, &nc, 4);
    if (!image_data) {
        success = false;
    }
    ID3D11Texture2D *tex_2d = nullptr;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = image_data;
    data.SysMemPitch = width * sizeof(u32);
    data.SysMemSlicePitch = 0;
    if (d3d_device->CreateTexture2D(&desc, &data, &tex_2d) == S_OK) {
        if (d3d_device->CreateShaderResourceView(tex_2d, NULL, &srv) == S_OK) {
        } else {
            success = false;
        }
    } else {
        success = false;
    }
        
    if (image_data) {
        stbi_image_free(image_data);
    }
    if (!success) {
        printf("Error creating texture '%s'\n", file_name.data());
    }

    if (texture) {
        texture->view = srv;
        texture->texture_2d = tex_2d;
    }
    return success;
}

internal std::string
read_file(std::string file_name) {
    std::string result{};
    HANDLE file_handle = CreateFileA((LPCSTR)file_name.data(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle != INVALID_HANDLE_VALUE) {
        u64 bytes_to_read;
        if (GetFileSizeEx(file_handle, (PLARGE_INTEGER)&bytes_to_read)) {
            assert(bytes_to_read <= UINT32_MAX);
            char *buffer = new char[bytes_to_read + 1];
            buffer[bytes_to_read] = 0;
            DWORD bytes_read;
            if (ReadFile(file_handle, buffer, (DWORD)bytes_to_read, &bytes_read, NULL) && (DWORD)bytes_to_read ==  bytes_read) {
                result = std::string(buffer);
            } else {
                // TODO: error handling
                printf("ReadFile: error reading file, %s!\n", file_name.data());
            }
       } else {
            // TODO: error handling
            printf("GetFileSize: error getting size of file: %s!\n", file_name.data());
       }
       CloseHandle(file_handle);
    } else {
        // TODO: error handling
        printf("CreateFile: error opening file: %s!\n", file_name.data());
    }
    return result;
}

int main() {
    QueryPerformanceFrequency(&performance_frequency);
    UINT desired_scheduler_ms = 1;
    timeBeginPeriod(desired_scheduler_ms);

    HRESULT hr = 0;
#define CLASSNAME "skelly_hwnd_class"
    HINSTANCE hinstance = GetModuleHandle(NULL);
    WNDCLASSA window_class{};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = window_proc;
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.lpszClassName = CLASSNAME;
    window_class.hInstance = hinstance;
    window_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    if (!RegisterClassA(&window_class)) {
        os_popupf("RegisterClassA failed, err:%d\n", GetLastError());
    }

    HWND window = 0;
    {
        RECT rc = {0, 0, WIDTH, HEIGHT};
        if (AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE)) {
            window = CreateWindowA(CLASSNAME, "Skelly", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hinstance, NULL);
        } else {
            fprintf(stderr, "AdjustWindowRect failed, err:%d\n", GetLastError());
        }
        if (!window) {
            os_popupf("CreateWindowA failed, err:%d\n", GetLastError());
        }
    }

    // INITIALIZE SWAP CHAIN
    DXGI_SWAP_CHAIN_DESC swapchain_desc{};
    {
        DXGI_MODE_DESC buffer_desc{};
        buffer_desc.Width = WIDTH;
        buffer_desc.Height = HEIGHT;
        buffer_desc.RefreshRate.Numerator = 60;
        buffer_desc.RefreshRate.Denominator = 1;
        buffer_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        buffer_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        buffer_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

        swapchain_desc.BufferDesc = buffer_desc;
        swapchain_desc.SampleDesc.Count = 1;
        swapchain_desc.SampleDesc.Quality = 0;
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.BufferCount = 1;
        swapchain_desc.OutputWindow = window;
        swapchain_desc.Windowed = TRUE;
        swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    }

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, NULL, NULL, D3D11_SDK_VERSION, &swapchain_desc, &swapchain, &d3d_device, NULL, &d3d_context);

    ID3D11Texture2D *backbuffer;
    hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backbuffer);
    
    hr = d3d_device->CreateRenderTargetView(backbuffer, NULL, &render_target);
    backbuffer->Release();

    ID3D11DepthStencilView *depth_stencil_view = nullptr;

    // DEPTH STENCIL BUFFER
    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = WIDTH;
        desc.Height = HEIGHT;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        ID3D11Texture2D *depth_stencil_buffer = nullptr;
        hr = d3d_device->CreateTexture2D(&desc, NULL, &depth_stencil_buffer);
        hr = d3d_device->CreateDepthStencilView(depth_stencil_buffer, NULL, &depth_stencil_view);
    }

    // RASTERIZER STATE
    ID3D11RasterizerState *rasterizer_state = nullptr;
    {
        D3D11_RASTERIZER_DESC desc{};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.FrontCounterClockwise = true;
        desc.ScissorEnable = false;
        desc.DepthClipEnable = false;
        d3d_device->CreateRasterizerState(&desc, &rasterizer_state);
    }

    // DEPTH-STENCIL STATE
    ID3D11DepthStencilState *depth_stencil_state = nullptr;
    {
        D3D11_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_LESS;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.BackFace = desc.FrontFace;
        d3d_device->CreateDepthStencilState(&desc, &depth_stencil_state);
    }

    // BLEND STATE
    ID3D11BlendState *blend_state = nullptr;
    {
        D3D11_BLEND_DESC desc{};
        desc.AlphaToCoverageEnable = false;
        desc.IndependentBlendEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (d3d_device->CreateBlendState(&desc, &blend_state) != S_OK) {
            printf("Error CreateBlendState\n");
        }
    }

    ID3D11SamplerState *model_sampler = nullptr;
    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD = 0;
        desc.MaxLOD = D3D11_FLOAT32_MAX;
        if (d3d_device->CreateSamplerState(&desc, &model_sampler) != S_OK) {
            printf("Failed to create sampler\n");
        }
    }

    // Model program
    Model_Constants model_constants{};
    D3D11_INPUT_ELEMENT_DESC model_input_layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, uv), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        
    };
    std::string model_shader = read_file("data/model.hlsl");
    Shader_Program model_program = create_shader_program(model_shader, "vs_main", "ps_main", model_input_layout, ARRAYSIZE(model_input_layout), sizeof(Model_Constants));

    Texture white_texture;
    load_texture("data/white.png", &white_texture);

    std::string file_name = "data/Crate1/Crate.fbx";
    std::string model_path = file_name.substr(0, file_name.find_last_of("/\\"));

    // Import model scene
    u32 flags = aiProcess_Triangulate | aiProcess_FlipUVs;
    const aiScene *scene = importer.ReadFile(file_name.data(), flags);
    if (!scene) {
        printf("Failed to load mesh %s: %s\n", file_name.data(), importer.GetErrorString());
    }

    // TODO: go through root and recursively process meshes
    // Load meshes
    std::vector<Mesh> meshes;
    for (u32 mesh_index = 0; mesh_index < scene->mNumMeshes; mesh_index++) {
        aiMesh *ai_mesh = scene->mMeshes[mesh_index];
        Mesh mesh{};
        mesh.vertices.reserve(ai_mesh->mNumVertices);
        for (u32 vert_index = 0; vert_index < ai_mesh->mNumVertices; vert_index++) {
            aiVector3D p = ai_mesh->mVertices[vert_index];
            aiVector3D n = ai_mesh->mNormals[vert_index];
            aiVector3D t = {};
            if (ai_mesh->HasTextureCoords(0)) {
                t = ai_mesh->mTextureCoords[0][vert_index];
            }
            Vertex v = {HMM_V3(p.x, p.y, p.z), HMM_V3(n.x, n.y, n.z), HMM_V2(t.x, t.y)};
            mesh.vertices.push_back(v);
        }

        // NOTE: Load bone datat
        // for (u32 bone_index = 0; bone_index < ai_mesh->mNumBones; bone_index++) {
        //     aiBone *bone = ai_mesh->mBones[bone_index];
        //     for (u32 vertex_index = 0; vertex_index < bone->mNumWeights; vertex_index++) {
        //         aiVertexWeight weight = bone->mWeights[vertex_index];
        //         // mesh.vertices[weight.mVertexId].bone_weights[0] = weight.mWeight;
        //     }
        // }

        for (u32 i = 0; i < ai_mesh->mNumFaces; i++) {
            aiFace *face = ai_mesh->mFaces + i;
            assert(face->mNumIndices == 3);
            mesh.indices.push_back(face->mIndices[0]);
            mesh.indices.push_back(face->mIndices[1]);
            mesh.indices.push_back(face->mIndices[2]);
        }

        aiMaterial *material = scene->mMaterials[ai_mesh->mMaterialIndex];
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString name;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &name) == AI_SUCCESS) {
                std::string material_file = model_path + "/" + name.data;
                printf("material:%s\n", material_file.data());
                if (!load_texture(material_file, &mesh.texture)) {
                    printf("Error loading texture:%s\n", material_file.data());
                }
            }
        }

        // NOTE: Create vertex buffer
        {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = sizeof(Vertex) * (int)mesh.vertices.size();
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = mesh.vertices.data();
            if (d3d_device->CreateBuffer(&desc, &data, &mesh.vertex_buffer) != S_OK) {
                fprintf(stderr, "Failed to create vertex buffer\n");
            }
        }

        // NOTE: Create index buffer
        {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = sizeof(u32) * (int)mesh.indices.size();
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = mesh.indices.data();
            if (d3d_device->CreateBuffer(&desc, &data, &mesh.index_buffer) != S_OK) {
                fprintf(stderr, "Failed to create index buffer\n");
            }
        }

        meshes.push_back(mesh);
    }

    Camera camera{};
    camera.up = HMM_V3(0.0f, 1.0f, 0.0f);
    camera.right = HMM_V3(1.0f, 0.0f, 0.0f);
    camera.forward = HMM_V3(0.0f, 0.0f, -1.0f);

    // NOTE: point toward -Z axis
    camera.yaw = -90.0f;
    camera.pitch =  0.0f;

    {
        POINT pt;
        if (GetCursorPos(&pt)) {
            if (ScreenToClient(window, &pt)) {
                last_mouse_p = mouse_p = HMM_V2((f32)pt.x, (f32)pt.y);
            }
        }
    }

    LARGE_INTEGER last_counter = win32_get_wall_clock();
    while (!window_should_close) {
        MSG message{};
        while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                window_should_close = true;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        int render_width, render_height;
        win32_get_window_dim(window, &render_width, &render_height);
        HMM_Vec2 render_dim = HMM_V2((float)render_width, (float)render_height);

        HMM_Mat4 projection = HMM_Perspective_RH_ZO(45.0f, render_dim.X/render_dim.Y, 0.1f, 1000.0f);

        // ==============================
        // INPUT
        // ==============================
        // Update mouse input
        {
            POINT pt;
            if (GetCursorPos(&pt)) {
                if (ScreenToClient(window, &pt)) {
                    last_mouse_p = mouse_p;
                    mouse_p = HMM_V2((f32)pt.x, (f32)pt.y);
                }
            }
        }

        if (input_button_down(Key_Escape)) {
            window_should_close = true;
        }

        f32 forward_dt = 0.0f;
        f32 right_dt = 0.0f;

        HMM_Vec2 mouse_delta = get_mouse_delta();

        if (input_button_down(Key_W)) {
            forward_dt += 0.1f;
        }
        if (input_button_down(Key_S)) {
            forward_dt -= 0.1f;
        }

        if (input_button_down(Key_A)) {
            right_dt -= 0.1f;
        }
        if (input_button_down(Key_D)) {
            right_dt += 0.1f;
        }

        if (input_button_down(Key_Q)) {
            camera.position.Y -= 0.1f;
        }
        if (input_button_down(Key_E)) {
            camera.position.Y += 0.1f;
        }

        {
            // Rotate around Y-axis
            f32 yaw_dt = 0.2f * mouse_delta.X;
            f32 pitch_dt = 0.2f * mouse_delta.Y;
            camera.yaw += yaw_dt;
            camera.pitch -= pitch_dt;

            camera.pitch = CLAMP(camera.pitch, -89.0f, 89.0f);
            
            HMM_Vec3 dir{};
            dir.X = HMM_CosF(camera.yaw) * HMM_CosF(camera.pitch);
            dir.Y = HMM_SinF(camera.pitch); 
            dir.Z = HMM_SinF(camera.yaw) * HMM_CosF(camera.pitch);

            camera.forward = HMM_NormV3(dir);
            camera.right = HMM_NormV3(HMM_Cross(camera.forward, camera.up));
        }

        camera.position += camera.forward * forward_dt;
        camera.position += camera.right * right_dt;

        camera.view_matrix = HMM_LookAt_RH(camera.position, camera.position + camera.forward, camera.up);

        // RENDER
        float bg_color[4] = {};
        d3d_context->ClearRenderTargetView(render_target, bg_color);
        d3d_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
        d3d_context->OMSetRenderTargets(1, &render_target, depth_stencil_view);

        D3D11_VIEWPORT viewport{};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = render_dim.X;
        viewport.Height = render_dim.Y;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        d3d_context->RSSetViewports(1, &viewport);

        d3d_context->RSSetState(rasterizer_state);

        d3d_context->OMSetBlendState(blend_state, NULL, 0xffffffff);
        // d3d_context->OMSetDepthStencilState(depth_stencil_state, 0);

        HMM_Mat4 view = camera.view_matrix;
        HMM_Mat4 trans = HMM_Translate(camera.position);
        HMM_Mat4 mvp = projection * view;

        model_constants.mvp = mvp;
        model_constants.color = HMM_V4(1.0f, 1.0f, 1.0f, 1.0f);

        upload_constants(model_program.constant_buffer, &model_constants, sizeof(model_constants));

        d3d_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        d3d_context->IASetInputLayout(model_program.input_layout);

        d3d_context->VSSetConstantBuffers(0, 1, &model_program.constant_buffer);
        d3d_context->PSSetConstantBuffers(0, 1, &model_program.constant_buffer);
        d3d_context->VSSetShader(model_program.vertex_shader, nullptr, 0);
        d3d_context->PSSetShader(model_program.pixel_shader, nullptr, 0);

        d3d_context->PSSetSamplers(0, 1, &model_sampler);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        for (int mesh_index = 0; mesh_index < meshes.size(); mesh_index++) {
            Mesh mesh = meshes[mesh_index];
            d3d_context->IASetIndexBuffer(mesh.index_buffer, DXGI_FORMAT_R32_UINT, 0);
            d3d_context->IASetVertexBuffers(0, 1, &mesh.vertex_buffer, &stride, &offset);
            if (mesh.texture.view) {
                d3d_context->PSSetShaderResources(0, 1, &mesh.texture.view);
            }  else {
                d3d_context->PSSetShaderResources(0, 1, &white_texture.view);
            }
            d3d_context->DrawIndexed((int)mesh.indices.size(), 0, 0);
        }

        swapchain->Present(0, 0);

        for (int i = 0; i < 256; i++) {
            keys_pressed[i] = false;
        }

        LARGE_INTEGER end_counter = win32_get_wall_clock();
#if 0
        float seconds_elapsed = 1000.0f * win32_get_seconds_elapsed(last_counter, end_counter);
        printf("seconds: %f\n", seconds_elapsed);
#endif
        last_counter = end_counter;
    }

    return 0; 
}
