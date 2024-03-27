// JE_TRANSLATE
#version 420

layout(location = 0) in vec3 vcol;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 vnorm;
layout(location = 3) in vec3 vpos;

layout(location = 0) out vec3 color;

layout (binding = 0) uniform JE_TRANSLATE {
    mat4 matrices[4];
    vec3 camera[2];
    vec3 ambience;
};

void main(){
    vec3 normal = normalize(vnorm);
    vec3 lightDir = vec3(-1, 1, 0);
    float distance = length(lightDir);
    distance = distance * distance;
    lightDir = normalize(lightDir);

    float lambertian = max(dot(lightDir, normal), 0.0);
    float specular = 0.0;

    if (lambertian > 0.0) {

        vec3 viewDir = normalize(-vpos);

        vec3 halfDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(halfDir, normal), 0.0);
        specular = pow(specAngle, 1);
    }

    color = ambience + vcol * lambertian * vec3(1.0);
}