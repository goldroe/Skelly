struct Material {
    float4 color;
    float4 ambient;
    float4 diffuse;
    float4 specular;
};

struct Directional_Light {
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float3 direction;
	float pad;
};

void compute_directional_light(Directional_Light L, Material material, float3 normal, float3 to_eye, out float4 ambient, out float4 diffuse, out float4 specular) {
    ambient = material.ambient * L.ambient;
    diffuse = float4(0, 0, 0, 0);
    specular = float4(0, 0, 0, 0);

    float diffuse_factor = dot(-L.direction, normal);
    if (diffuse_factor > 0) {
        float3 r = reflect(L.direction, normal);
        float spec_factor = pow(max(dot(r, to_eye), 0.0f), material.specular.w);

        diffuse = diffuse_factor * material.diffuse * L.diffuse;
        specular = spec_factor * material.specular * L.specular;
    }
}
