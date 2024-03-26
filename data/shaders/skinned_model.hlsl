#include "lighting.hlsl"

#define MAX_BONES 100
#define MAX_WEIGHTS 4

cbuffer Constants_Per_Frame : register(b0) {
    Directional_Light directional_light;
    float3 eye_pos_w;
};

cbuffer Constants_Per_Obj : register(b1) {
    matrix world;
    matrix world_inv_transpose;
    matrix wvp;
    matrix bone_matrices[MAX_BONES];
    Material material;
};

struct Vertex_In {
    float3 pos_l : POSITION;
    float3 normal_l : NORMAL;
    float2 uv : TEXCOORD;
    float4 weights : WEIGHTS;
    int4 bone_indices : BONEINDICES;
};

struct Vertex_Out {
    float3 pos_w : POSITION;
    float3 normal_w : NORMAL;
    float4 pos_h : SV_POSITION;
    float2 uv : TEXCOORD;
};

SamplerState model_sampler;
Texture2D texture_diffuse;

Vertex_Out vs_main(Vertex_In vin) {
    Vertex_Out vout;
    float4 pos_l = float4(0, 0, 0, 0);
    for (int i = 0; i < MAX_WEIGHTS; i++) {
        if (vin.bone_indices[i] == -1) continue;
        if (vin.bone_indices[i] >= MAX_BONES) {
            pos_l = float4(vin.pos_l, 1.0);
            break;
        }

        pos_l += vin.weights[i] * mul(bone_matrices[vin.bone_indices[i]], float4(vin.pos_l, 1.0));
    }
    vout.pos_w = mul(world, pos_l).xyz;
    vout.pos_h = mul(wvp, pos_l);
    vout.normal_w = mul(world_inv_transpose, float4(vin.normal_l, 1.0)).xyz;
    vout.uv = vin.uv;
    return vout;
}

float4 ps_main(Vertex_Out pin) : SV_TARGET {
    float3 to_eye_w = normalize(eye_pos_w - pin.pos_w);

    float4 A, D, S;
    compute_directional_light(directional_light, material, pin.normal_w, to_eye_w, A, D, S);

    float4 ambient = float4(0, 0, 0, 0);
    float4 diffuse = float4(0, 0, 0, 0);
    float4 specular = float4(0, 0, 0, 0);
    ambient += A;
    diffuse += D;
    specular += S;

    float4 light_color = ambient + diffuse + specular;
    light_color.a = material.diffuse.a;

    return light_color * texture_diffuse.Sample(model_sampler, pin.uv);
}
