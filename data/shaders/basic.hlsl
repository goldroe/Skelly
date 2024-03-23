#include "lighting.hlsl"

cbuffer Constants_Per_Frame : register(b0) {
    Directional_Light directional_light;
    float3 eye_pos_w;
};

cbuffer Constants_Per_Obj : register(b1) {
    matrix world;
    matrix world_inv_transpose;
    matrix wvp;
    Material material;
};

struct Vertex_In {
    float3 pos_l : POSITION;
    float3 normal_l : NORMAL;
    float2 uv : TEXCOORD;
};

struct Vertex_Out {
    float4 pos_h : SV_POSITION;
    float3 pos_w : POSITION;
    float3 normal_w : NORMAL;
    float2 uv : TEXCOORD;
};

SamplerState basic_sampler;
Texture2D texture_diffuse;

Vertex_Out vs_main(Vertex_In vin) {
    Vertex_Out vout;
    vout.pos_w = mul(world, float4(vin.pos_l, 1)).xyz;
    vout.normal_w = mul(world_inv_transpose, float4(vin.normal_l, 1.0));
    vout.pos_h = mul(wvp, float4(vin.pos_l, 1));
    vout.uv = vin.uv;
    return vout;
}

float4 ps_main(Vertex_Out pin) : SV_TARGET {
    pin.normal_w = normalize(pin.normal_w);
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

    return light_color * texture_diffuse.Sample(basic_sampler, pin.uv);
}
