#version 330 core

layout (location = 0) out float new_pos;
layout (location = 1) out float new_vel;
layout (location = 2) out float new_acc;

in vec2 out_tex_coords;

uniform sampler2D old_pos;
uniform sampler2D old_vel;
uniform sampler2D old_acc;

uniform sampler2D sources;
uniform sampler2D obstacles;

uniform float timestep;
uniform float time;

const int size = 1024;
const int offset = 1;
const float k = 1;
const float A = 1;
const float pulse_time = 1000;

void main()
{
    ivec2 tex_coords = ivec2(out_tex_coords * size);
    
    float my_old_pos = texelFetch(old_pos, tex_coords, 0).r;
    float my_old_vel = texelFetch(old_vel, tex_coords, 0).r;
    float my_old_acc = texelFetch(old_acc, tex_coords, 0).r;

    new_pos = my_old_pos + timestep * my_old_vel;
    new_vel = my_old_vel + timestep * my_old_acc;

    ivec2 top = ivec2(tex_coords.x, tex_coords.y + offset);
    ivec2 right = ivec2(tex_coords.x + offset, tex_coords.y);
    ivec2 bottom = ivec2(tex_coords.x, tex_coords.y - offset);
    ivec2 left = ivec2(tex_coords.x - offset, tex_coords.y);

    float top_pos = texelFetch(old_pos, top, 0).r;
    float right_pos = texelFetch(old_pos, right, 0).r;
    float bottom_pos = texelFetch(old_pos, bottom, 0).r;
    float left_pos = texelFetch(old_pos, left, 0).r;
    
    new_acc = k * (top_pos + right_pos + bottom_pos + left_pos - 4 * my_old_pos);


    if (texelFetch(sources, tex_coords, 0).r > 0.5)
    {
	if (time < pulse_time)
	{
	    new_pos = A * (sin(time) + sin(2 * time));
	    new_vel = my_old_vel;
	    new_acc = my_old_acc;
	}
	else
	{
	    new_pos = 0;
	    new_vel = my_old_vel;
	    new_acc = my_old_acc;
	}
    }

    if (texelFetch(obstacles, tex_coords, 0).r > 0.5)
    {
	new_pos = 0;
	new_vel = my_old_vel;
	new_acc = my_old_acc;
    }
}
