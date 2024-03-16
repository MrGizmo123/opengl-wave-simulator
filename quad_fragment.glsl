#version 330 core

out vec4 FragColor;

in vec2 out_tex_coords;

uniform sampler2D image;

const int size = 1024;

void main()
{
    float texColor = texelFetch(image, ivec2(out_tex_coords * size), 0).r;
    FragColor = vec4(texColor, -texColor , 0, 1);
}
