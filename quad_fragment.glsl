#version 330 core

out vec4 FragColor;

in vec2 out_tex_coords;

uniform sampler2D image;
uniform sampler2D scene;
uniform int size;

void main()
{
    float texColor = texture(image, out_tex_coords).r;
    vec4 scene_color = texture(scene, out_tex_coords);
    FragColor = scene_color * 0.5 + vec4(0, 2, -2, 1) * texColor;
}
