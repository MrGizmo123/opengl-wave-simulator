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
uniform int size;

// c is the speed of the wave in pixel per frame
const float c = 1;

const float k = 1;

const float A = 1;
const float pulse_time = 1000;

const float pml_width = 0.01;

void main()
{
    float offset = 1.0/float(size);
    
    float my_old_pos = texture(old_pos, out_tex_coords).r;
    float my_old_vel = texture(old_vel, out_tex_coords).r;
    float my_old_acc = texture(old_acc, out_tex_coords).r;

    new_pos = my_old_pos + timestep * my_old_vel;
    new_vel = my_old_vel + timestep * my_old_acc;

    vec2 top = vec2(out_tex_coords.x, out_tex_coords.y + offset);
    vec2 right = vec2(out_tex_coords.x + offset, out_tex_coords.y);
    vec2 bottom = vec2(out_tex_coords.x, out_tex_coords.y - offset);
    vec2 left = vec2(out_tex_coords.x - offset, out_tex_coords.y);

    float top_pos = texture(old_pos, top).r;
    float right_pos = texture(old_pos, right).r;
    float bottom_pos = texture(old_pos, bottom).r;
    float left_pos = texture(old_pos, left).r;
    
    new_acc = c * c * (1.0 - texture(obstacles, out_tex_coords).r) * (top_pos + right_pos + bottom_pos + left_pos - 4 * my_old_pos) - texture(obstacles, out_tex_coords).b * my_old_pos;

    //PML Code to absorb waves at the boundary
    if (out_tex_coords.x < pml_width)
    {
	float dn = right_pos - my_old_pos;
	new_vel = k * c * dn;
	new_acc = 0;
    }
    else if (out_tex_coords.x > (1 - pml_width))
    {
	float dn = left_pos - my_old_pos;
	new_vel = k * c * dn;
	new_acc = 0;
    }
    else if (out_tex_coords.y < pml_width)
    {
	float dn = top_pos - my_old_pos;
	new_vel = k * c * dn;
	new_acc = 0;
    }
    else if (out_tex_coords.y > (1 - pml_width))
    {
	float dn = bottom_pos - my_old_pos;
	new_vel = k * c * dn;
	new_acc = 0;
    }
    
    float source = texture(obstacles, out_tex_coords).g;
    if (source > 0)
    {
	new_pos = A * sin(8 * source * time);
    }

}
