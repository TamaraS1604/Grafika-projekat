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

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;

    float shininess;
};
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform PointLight pointLight;
uniform Material material;

uniform vec3 viewPosition;
// calculates the color when using a point light.
// vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
// {
//     vec3 lightDir = normalize(light.position - fragPos);
//     // diffuse shading
//     float diff = max(dot(normal, lightDir), 0.0);
//     // specular shading
//     vec3 reflectDir = reflect(-lightDir, normal);
//     float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
//     // attenuation
//     float distance = length(light.position - fragPos);
//     float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
//     // combine results
// //     vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
// //     vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
// //     vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords).xxx);
//     vec3 color=vec3(1.0,0.75,0.26);
//     vec3 ambient = light.ambient * color;
//     vec3 diffuse = light.diffuse * diff * color;
//     vec3 specular = light.specular * spec * 0.3;
//     ambient *= attenuation;
//     diffuse *= attenuation;
//     specular *= attenuation;
//     return (ambient + diffuse + specular);
//
// //     return vec3(1.0);
// }

vec3 osvetljenje(PointLight light,vec3 normal,vec3 fragPos,vec3 viewDir){
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    return light.ambient+diff+spec;
}

void main()
{
//     vec3 normal = normalize(Normal);
//     vec3 viewDir = normalize(viewPosition - FragPos);
//     vec3 result = CalcPointLight(pointLight, normal, FragPos, viewDir);
//     FragColor = vec4(result, 1.0);
    vec3 color=vec3(0.2,0.5,0.9);
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);


    FragColor=vec4(color*osvetljenje(pointLight,normal,FragPos,viewDir),1.0);
}