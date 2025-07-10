#version 330 core

// Vertex attributes
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

// Uniforms
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

// Lighting uniforms
uniform vec3 uLightPosition;
uniform vec3 uViewPosition;

// Material uniforms
uniform bool uHasTexture;
uniform bool uHasNormalMap;

// Output to fragment shader
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 TangentLightPos;
out vec3 TangentViewPos;
out vec3 TangentFragPos;

// Selection/highlight
uniform bool uSelected;
out float Selected;

void main()
{
    // Transform vertex position
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    FragPos = worldPos.xyz;
    gl_Position = uProjection * uView * worldPos;
    
    // Transform normal
    Normal = normalize(uNormalMatrix * aNormal);
    
    // Pass texture coordinates
    TexCoord = aTexCoord;
    
    // Pass selection state
    Selected = uSelected ? 1.0 : 0.0;
    
    // Calculate tangent space vectors for normal mapping
    if (uHasNormalMap) {
        vec3 T = normalize(uNormalMatrix * aTangent);
        vec3 B = normalize(uNormalMatrix * aBitangent);
        vec3 N = normalize(uNormalMatrix * aNormal);
        
        // Create TBN matrix for tangent space transformation
        mat3 TBN = transpose(mat3(T, B, N));
        
        TangentLightPos = TBN * uLightPosition;
        TangentViewPos = TBN * uViewPosition;
        TangentFragPos = TBN * FragPos;
    }
}
