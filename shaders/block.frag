#version 330 core

in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D textureAtlas;

void main()
{
    vec3 lightDirection =
        normalize(vec3(-0.5, 1.0, 0.8));

    float diffuse = max(
        dot(normalize(Normal), lightDirection),
        0.0
    );

    float brightness = 0.35 + diffuse * 0.65;

    vec4 texel = texture(textureAtlas, TexCoord);

    FragColor = vec4(
        texel.rgb * brightness,
        texel.a
    );
}