struct Directional_Light {
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float3 direction;
	float pad;
};

struct Material {
    float4 color;
    float4 ambience;
    float4 diffuse;
    float3 specular;
    float pad;
};

cbuffer Constants_Per_Frame : register(b0) {
    Directional_Light lights[3];
    float3 eye_pos_w;
};

cbuffer Constants_Per_Obj : register(b1) {
    matrix world;
    matrix wvp;
    Material material;
};

struct Vertex_In {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct Vertex_Out {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

SamplerState basic_sampler;
Texture2D texture_diffuse;

Vertex_Out vs_main(Vertex_In vin) {
    Vertex_Out vout;
    vout.position = mul(wvp, float4(vin.position, 1));
    vout.normal = vin.normal;
    vout.uv = vin.uv;
    return vout;
}

float4 ps_main(Vertex_Out pin) : SV_TARGET {
    return material.color * texture_diffuse.Sample(basic_sampler, pin.uv);
}
