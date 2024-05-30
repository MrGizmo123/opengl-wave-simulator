#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 tex_coords;

out vec2 out_tex_coords;

uniform int view;

void main(void)
{
    vec3 final_pos;

    if (view == 1)
    {
	final_pos = aPos;
    }
    else if (view == 2)
    {
	final_pos = aPos + vec3(0,-1,0); // shift it down by 1
    }
    
    gl_Position = vec4(final_pos, 1.0);

    out_tex_coords = tex_coords;
}
