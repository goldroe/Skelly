
SamplerState model_sampler;
Texture2D texture_diffuse;

cbuffer Model_Constants {
    matrix mvp;
    float4 color;
};

struct VS_OUT {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

VS_OUT vs_main(float3 in_position : POSITION, float3 in_normal : NORMAL, float2 in_uv : TEXCOORD) {
    VS_OUT output;
    output.position = mul(mvp, float4(in_position, 1));
    output.normal = in_normal;
    output.uv = in_uv;
    return output;
}

float4 ps_main(VS_OUT input) : SV_TARGET {
    return color * texture_diffuse.Sample(model_sampler, input.uv);
}
