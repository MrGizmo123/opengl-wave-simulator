#version 330 core

out vec4 FragColor;

in vec2 out_tex_coords;

uniform sampler2D image;
uniform int size;

void main()
{
    float texColor = texture(image, out_tex_coords).r;
    FragColor = vec4(texColor, -texColor , 0, 1);
}
