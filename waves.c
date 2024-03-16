// (defkeybind "C-c C-r" (async-shell-command "./a.out"))

#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string.h>
#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define VERTEX_SHADER "vertex.glsl"
#define FRAGMENT_SHADER "fragment.glsl"

#define WIDTH 1024
#define HEIGHT 1024

unsigned int vertex_shader;
unsigned int fragment_shader;

float vertices[] = {
    // first triangle
     1.0f,  1.0f, 0.0f,  // top right
     1.0f, -1.0f, 0.0f,  // bottom right
    -1.0f,  1.0f, 0.0f,  // top left 
    // second triangle
     1.0f, -1.0f, 0.0f,  // bottom right
    -1.0f, -1.0f, 0.0f,  // bottom left
    -1.0f,  1.0f, 0.0f   // top left
};

float texture_coords[] = {
    1.0f, 1.0f,
    1.0, 0.0f,
    0.0f, 1.0f,

    1.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 1.0f
};

char* read_file(char* filename)
{
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    char* string = malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);

    string[fsize] = 0;

    return string;

    /* printf("file contents: %s", string); */
}



static void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    {
	glfwSetWindowShouldClose(win, GLFW_TRUE);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

struct FBO
{
    unsigned int fbo_id;
    unsigned int pos_texture;
    unsigned int vel_texture;
    unsigned int acc_texture;
};

struct VAO
{
    unsigned int vao_id;
    unsigned int pos_vbo;
    unsigned int tex_vbo;
};

void generate_rect(struct VAO* vao)
{

    /* generate VAO */
    glGenVertexArrays(1, &vao->vao_id);
    glBindVertexArray(vao->vao_id);

    /* generate vertex VBO */
    glGenBuffers(1, &vao->pos_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vao->pos_vbo); /* bind vbo */
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); /* store the positions */

    /* set the position of our vbo in the VAO */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); /* angry screeching */

    /* generate texture VBO */
    glGenBuffers(1, &vao->tex_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vao->tex_vbo); /* bind vbo */
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), texture_coords, GL_STATIC_DRAW); /* store the coordinates */

    /* set the position of our vbo in the VAO */
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); /* angry screeching */
    
}

void compile_shaders(unsigned int* shader_program,
		     char* vertex_file,
		     char* fragment_file)
{
    char* vertex_shader_source;
    char* fragment_shader_source;

    /* read shader source code */
    vertex_shader_source = read_file(vertex_file);
    fragment_shader_source = read_file(fragment_file);

    /* printf("vertex shader: %s \n", vertex_shader_source); */
    /* printf("fragment shader: %s \n", fragment_shader_source); */

    /* create shader objects */
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    /* set the source code */
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
   

    int success;
    char error_message[512];

    /* compile both shaders and check for errors, report if any error */

    glCompileShader(vertex_shader); 
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
	glGetShaderInfoLog(vertex_shader, 512, NULL, error_message);
	printf("Error when compiling vertex shader \n %s \n", error_message);
    }

    glCompileShader(fragment_shader); 
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
	glGetShaderInfoLog(fragment_shader, 512, NULL, error_message);
	printf("Error when compiling fragment shader \n %s \n", error_message);
    }

    /* create shader program */
    *shader_program = glCreateProgram();

    /* attach the vertex and fragment shader to it */
    glAttachShader(*shader_program, vertex_shader);
    glAttachShader(*shader_program, fragment_shader);

    /* Link the program */
    glLinkProgram(*shader_program);

    /* check for errors in linking and report if necessary */
    glGetProgramiv(*shader_program, GL_LINK_STATUS, &success);
    if(!success) {
	glGetProgramInfoLog(*shader_program, 512, NULL, error_message);
	printf("Error when linking shaders \n %s \n", error_message);
    }

    glValidateProgram(*shader_program);
    glGetProgramiv(*shader_program, GL_VALIDATE_STATUS, &success);
    if (!success) {
	glGetProgramInfoLog(*shader_program, 512, NULL, error_message);
	printf("Error when validating shaders \n %s \n", error_message);
    }
    
}

void load_texture(unsigned int* texture, char* path)
{

    /* generate texture object */
    glGenTextures(1, texture);
    /* bind it */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
    
    int width;
    int height;
    int num_channels;

    stbi_set_flip_vertically_on_load(1);
    /* load image file */
    unsigned char* data = stbi_load(path, &width, &height, &num_channels, 4);

    if (data)
    {
	/* add the data to the texture */
	glTexImage2D(
	    GL_TEXTURE_2D,
	    0,
	    GL_RGBA,
	    width,
	    height,
	    0,
	    GL_RGBA,
	    GL_UNSIGNED_BYTE,
	    data);
	//glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
	printf("Error loading texture\n");
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    /* free the data */
    stbi_image_free(data);
}

void generate_empty_float_texture(unsigned int* texture)
{
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);

    glTexImage2D(
	GL_TEXTURE_2D,
	0,
	GL_R32F,
	WIDTH,
	HEIGHT,
	0,
	GL_RED,
	GL_FLOAT,
	NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void generate_fbo(struct FBO* fbo)
{
    glGenFramebuffers(1, &fbo->fbo_id);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo_id);

    generate_empty_float_texture(&fbo->pos_texture);
    generate_empty_float_texture(&fbo->vel_texture);
    generate_empty_float_texture(&fbo->acc_texture);

    glFramebufferTexture2D(GL_FRAMEBUFFER,
			   GL_COLOR_ATTACHMENT0,
			   GL_TEXTURE_2D,
			   fbo->pos_texture,
			   0);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER,
			   GL_COLOR_ATTACHMENT1,
			   GL_TEXTURE_2D,
			   fbo->vel_texture,
			   0);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER,
			   GL_COLOR_ATTACHMENT2,
			   GL_TEXTURE_2D,
			   fbo->acc_texture,
			   0);

    const unsigned int attachments[] = {
	GL_COLOR_ATTACHMENT0,
	GL_COLOR_ATTACHMENT1,
	GL_COLOR_ATTACHMENT2
    };
    
    glDrawBuffers(3, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
	printf("Error in creating FBO\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void bind_fbo_to_uniforms(struct FBO* fbo,
			  unsigned int sources_texture,
			  unsigned int obstacles_texture,
			  unsigned int shader)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbo->pos_texture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, fbo->vel_texture);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, fbo->acc_texture);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, sources_texture);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, obstacles_texture);
    
    int old_pos_location = glGetUniformLocation(shader, "old_pos");
    int old_vel_location = glGetUniformLocation(shader, "old_vel");
    int old_acc_location = glGetUniformLocation(shader, "old_acc");
    int sources_location = glGetUniformLocation(shader, "sources");
    int obstacles_location = glGetUniformLocation(shader, "obstacles");
    
    glUniform1i(old_pos_location, 0);
    glUniform1i(old_vel_location, 1);
    glUniform1i(old_acc_location, 2);
    glUniform1i(sources_location, 3);
    glUniform1i(obstacles_location, 4);
}

void step(struct FBO* src,
	  struct FBO* dest,
	  unsigned int sources_texture,
	  unsigned int obstacles_texture,
	  unsigned int shader,
	  unsigned int vao,
	  float timestep,
	  float time)
{
    /* read from fbo1 write to fbo2 */
    glBindFramebuffer(GL_FRAMEBUFFER, dest->fbo_id);
	
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader);

    bind_fbo_to_uniforms(src, sources_texture, obstacles_texture, shader);
    glUniform1f(glGetUniformLocation(shader, "timestep"), timestep);
    glUniform1f(glGetUniformLocation(shader, "time"), time);
    glUniform1i(glGetUniformLocation(shader, "size"), WIDTH);

    
    glBindVertexArray(vao);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main(int argc, char** argv)
{
    
    /* initialize glfw */
    if (!glfwInit())
    {
	printf("Failed to initialize glfw\n");
    }

    /* create window */
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH,
					  HEIGHT,
					  "Waves",
					  NULL,
					  NULL);
    if (!window)
    {
	printf("Failed to create window\n");
    }

    
    glfwMakeContextCurrent(window);

    GLenum error = glewInit();
    if (error != GLEW_OK)
    {
	printf("failed to initialize GLEW\n");
    }
    
    glfwSwapInterval(0);
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    /* enable debug output */
    glEnable( GL_DEBUG_OUTPUT );
    //glDebugMessageCallback( MessageCallback, 0 );

    
    //glEnable(GL_TEXTURE_2D);
    glEnable (GL_BLEND);


    /* texture code */

    unsigned int sources_texture;
    load_texture(&sources_texture, "sources.png");

    unsigned int obstacles_texture;
    load_texture(&obstacles_texture, "obstacles.png");

    
    /* vao/vbo code */

    struct VAO vao;
    generate_rect(&vao);
    

    /* shader code */

    unsigned int quad_shader;
    compile_shaders(&quad_shader,
		    "vertex.glsl",
		    "quad_fragment.glsl");

    glUseProgram(quad_shader);
    glUniform1i(glGetUniformLocation(quad_shader, "size"), WIDTH);

    unsigned int wave_shader;
    compile_shaders(&wave_shader,
		    "vertex.glsl",
		    "wave_fragment.glsl");

    glUseProgram(wave_shader);
    glUniform1i(glGetUniformLocation(wave_shader, "size"), WIDTH);

    /* fbo code */
    struct FBO fbo1;
    generate_fbo(&fbo1);

    struct FBO fbo2;
    generate_fbo(&fbo2);
    
    /* time code */

    float time = 0.0f;
    float timestep = 0.0005f;
    
    /* main loop */
    
    while (!glfwWindowShouldClose(window))
    {
	
	step(&fbo1,
	     &fbo2,
	     sources_texture,
	     obstacles_texture,
	     wave_shader,
	     vao.vao_id,
	     timestep,
	     time);

	step(&fbo2,
	     &fbo1,
	     sources_texture,
	     obstacles_texture,
	     wave_shader,
	     vao.vao_id,
	     timestep,
	     time);

	glClearColor(0.6f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fbo1.pos_texture);

	glUseProgram(quad_shader);
	glBindVertexArray(vao.vao_id);
	
	glDrawArrays(GL_TRIANGLES, 0, 6);
	
	
	glfwSwapBuffers(window);
	glfwPollEvents();

	time += timestep;
    }

    /* gracefully close */

    glfwTerminate();
}
