#version 430 core

out vec4 FragColor;

in vec2 out_tex_coords;

uniform sampler3D image;
uniform int size;
uniform float z_slice;
uniform float y_slice;
uniform int view;

void main()
{
    float texColor;
    vec4 increment = vec4(0,0,0,0);
    
    if (view == 1)		// z slice
    {
	texColor = texture(image, vec3(out_tex_coords, z_slice)).r;
	if (abs(out_tex_coords.y - y_slice) < 0.002)
	{
	    increment = vec4(0.5, 0, 0, 0);
	}
    }
    else if (view == 2)		// y slice
    {
	texColor = texture(image, vec3(out_tex_coords.x, y_slice, out_tex_coords.y)).r;
	if (abs(out_tex_coords.y - z_slice) < 0.002)
	{
	    increment = vec4(0.5, 0, 0, 0);
	}
    }

    FragColor = vec4(0, 1, -1, 1) * texColor + increment;
}
