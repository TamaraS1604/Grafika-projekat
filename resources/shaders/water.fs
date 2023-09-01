#version 330 core
out vec4 FragColor;

struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in vec3 Tangent;
in vec3 Bitangent;


struct Material {
    float shininess;
};

uniform Material material;


uniform PointLight pointLight;

uniform vec3 viewPosition;

uniform sampler2D water_diffuse;
uniform sampler2D water_normal;
float textureScale=100.0;


vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec3 color=texture(water_diffuse,TexCoords*textureScale).rgb;
    vec3 ambient = light.ambient * color;
    vec3 diffuse = light.diffuse * diff * color;
    vec3 specular = light.specular * spec * 0.3;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);

//     return vec3(1.0);
}

void main()
{
    vec3 mappedNormal=texture(water_normal,TexCoords*textureScale).rgb;
    mappedNormal-=vec3(0.5);
    mappedNormal*=2.0;
    mat3 TBN=mat3(Tangent,Bitangent,Normal);
    mappedNormal=TBN*mappedNormal;

    vec3 viewDir = normalize(viewPosition - FragPos);
    vec3 result = CalcPointLight(pointLight, mappedNormal, FragPos, viewDir);


    FragColor=vec4(result,0.5);
}
