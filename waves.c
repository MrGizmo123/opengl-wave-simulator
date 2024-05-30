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
#define COMPUTE_SHADER "compute.glsl"

#define WIDTH 512
#define HEIGHT 1024

#define TEX_WIDTH 256
#define TEX_HEIGHT 256
#define TEX_DEPTH 256

#define CYCLES_PER_FRAME 50
#define SHOULD_ANIMATE 0

unsigned int vertex_shader;
unsigned int fragment_shader;

unsigned int compute_shader;

/* used to initialize textures to 0 */
float* empty;

float vertices[] = {
    // first triangle
     1.0f,  1.0f, 0.0f,  // top right
     1.0f, 0.0f, 0.0f,  // bottom right
    -1.0f,  1.0f, 0.0f,  // top left 
    // second triangle
     1.0f, 0.0f, 0.0f,  // bottom right
    -1.0f, 0.0f, 0.0f,  // bottom left
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

struct TexBundle
{
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
    glShaderSource(vertex_shader, 1, (const GLchar * const*)&vertex_shader_source, NULL);
    glShaderSource(fragment_shader, 1, (const GLchar * const*)&fragment_shader_source, NULL);
   

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

void compile_compute_shader(unsigned int* shader_program,
			    char* compute_file)
{
    char* compute_shader_source;

    /* read shader source code */
    compute_shader_source = read_file(compute_file);

    /* printf("vertex shader: %s \n", vertex_shader_source); */
    /* printf("fragment shader: %s \n", fragment_shader_source); */

    /* create shader objects */
    compute_shader = glCreateShader(GL_COMPUTE_SHADER);

    /* set the source code */
    glShaderSource(compute_shader, 1, (const GLchar * const*)&compute_shader_source, NULL);
   

    int success;
    char error_message[512];

    /* compile both shaders and check for errors, report if any error */

    glCompileShader(compute_shader); 
    glGetShaderiv(compute_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
	glGetShaderInfoLog(compute_shader, 512, NULL, error_message);
	printf("Error when compiling compute shader \n %s \n", error_message);
    }
    
    /* create shader program */
    *shader_program = glCreateProgram();

    /* attach the vertex and fragment shader to it */
    glAttachShader(*shader_program, compute_shader);

    /* Link the program */
    glLinkProgram(*shader_program);

    /* check for errors in linking and report if necessary */
    glGetProgramiv(*shader_program, GL_LINK_STATUS, &success);
    if(!success) {
	glGetProgramInfoLog(*shader_program, 512, NULL, error_message);
	printf("Error when linking compute shader \n %s \n", error_message);
    }

    glValidateProgram(*shader_program);
    glGetProgramiv(*shader_program, GL_VALIDATE_STATUS, &success);
    if (!success) {
	glGetProgramInfoLog(*shader_program, 512, NULL, error_message);
	printf("Error when validating compute shaders \n %s \n", error_message);
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

void generate_empty_float_texture_3D(unsigned int* texture)
{
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_3D, *texture);

    glTexImage3D(
	GL_TEXTURE_3D,
	0,
	GL_R32F,
	TEX_WIDTH,
	TEX_HEIGHT,
	TEX_DEPTH,
	0,
	GL_RED,
	GL_FLOAT,
	empty);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_3D, 0);
}

void generate_tex_bundle(struct TexBundle* bundle)
{
    generate_empty_float_texture_3D(&bundle->pos_texture);
    generate_empty_float_texture_3D(&bundle->vel_texture);
    generate_empty_float_texture_3D(&bundle->acc_texture);
}

void bind_tex_bundles(struct TexBundle* src)
{

    glBindImageTexture(0, src->pos_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32F);
    glBindImageTexture(1, src->vel_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32F);
    glBindImageTexture(2, src->acc_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32F);
}

void step(struct TexBundle* src,
	  unsigned int shader,
	  float timestep,
	  float time)
{
    glUseProgram(shader);

    bind_tex_bundles(src);

    glUniform1f(glGetUniformLocation(shader, "timestep"), timestep);
    glUniform1f(glGetUniformLocation(shader, "time"), time);
    glUniform1i(glGetUniformLocation(shader, "offset"), 1);
    glUniform1i(glGetUniformLocation(shader, "width"), TEX_WIDTH);

    glDispatchCompute(TEX_WIDTH/8, TEX_HEIGHT/8, TEX_DEPTH/8);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void saveScreenshotToFile(char* filename, int windowWidth, int windowHeight) {    
    const int numberOfPixels = windowWidth * windowHeight * 3;
    unsigned char pixels[numberOfPixels];

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, windowWidth, windowHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixels);

    FILE *outputFile = fopen(filename, "w");
    short header[] = {0, 2, 0, 0, 0, 0, (short) windowWidth, (short) windowHeight, 24};

    fwrite(&header, sizeof(header), 1, outputFile);
    fwrite(pixels, numberOfPixels, 1, outputFile);
    fclose(outputFile);

    printf("Wrote %s\n", filename);
}

int main(int argc, char** argv)
{

    empty = calloc(TEX_WIDTH * TEX_HEIGHT * TEX_DEPTH, 4);
    
    /* initialize glfw */
    if (!glfwInit())
    {
	printf("Failed to initialize glfw\n");
    }

    /* create window */
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
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

    
    //glEnable(GL_TEXTURE_3D);
    glEnable (GL_BLEND);
    


    /* texture code */

    /* unsigned int sources_texture; */
    /* load_texture(&sources_texture, "sources.png"); */

    /* unsigned int obstacles_texture; */
    /* load_texture(&obstacles_texture, "obstacles.png"); */

    
    /* vao/vbo code */

    struct VAO vao;
    generate_rect(&vao);
    /* testure code */
    unsigned int positions;
    generate_empty_float_texture_3D(&positions);
    

    unsigned int velocities;
    generate_empty_float_texture_3D(&velocities);

    unsigned int accelerations;
    generate_empty_float_texture_3D(&accelerations);

    /* shader code */

    unsigned int quad_shader;
    compile_shaders(&quad_shader,
		    "vertex.glsl",
		    "quad_fragment.glsl");

    glUseProgram(quad_shader);
    glUniform1i(glGetUniformLocation(quad_shader, "size"), WIDTH);

    unsigned int wave_shader;
    compile_compute_shader(&wave_shader, COMPUTE_SHADER);

    glUseProgram(wave_shader);
    glUniform1i(glGetUniformLocation(wave_shader, "width"), TEX_WIDTH);
    
    /* time code */

    float time = 0.0f;
    float timestep = 0.0005f;

    int frames = 0;

    /* int max_work_groups; */
    /* int max_wg_size; */
    /* int max_wg_invocations; */

    /* glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &max_work_groups); */
    /* glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &max_wg_size); */
    /* glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, 0, &max_wg_invocations); */
    
    /* printf("Max work groups: %d\n", max_work_groups); */
    /* printf("Max work group size: %d\n", max_wg_size); */
    /* printf("Max work group invocations: %d\n", max_wg_invocations); */

    struct TexBundle b1;
    generate_tex_bundle(&b1);
    
    struct TexBundle b2;
    generate_tex_bundle(&b2);
    
    
    /* main loop */
    
    while (!glfwWindowShouldClose(window))
    {
	
	step(&b1,
	     wave_shader,
	     timestep,
	     time);
	
	/* step(&b2, */
	/*      &b1, */
	/*      wave_shader, */
	/*      timestep, */
	/*      time); */

	glClearColor(0.6f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
        


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, b1.pos_texture);
	
	glBindVertexArray(vao.vao_id);

	glUseProgram(quad_shader);
	glUniform1f(glGetUniformLocation(quad_shader, "z_slice"), 0.3);
	glUniform1f(glGetUniformLocation(quad_shader, "y_slice"), 0.5);

	/* draw z slice (transverse section) */
	glUniform1i(glGetUniformLocation(quad_shader, "view"), 1);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	/* draw y slice (longitudinal section) */
	glUniform1i(glGetUniformLocation(quad_shader, "view"), 2);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindTexture(GL_TEXTURE_3D, 0);
	
	
	glfwSwapBuffers(window);
	glfwPollEvents();

	time += timestep;
	frames++;

	if (SHOULD_ANIMATE && frames % CYCLES_PER_FRAME == 0)
	{
	    char file[128];
	    sprintf(file, "output/test2/frame%05d.tga", frames / CYCLES_PER_FRAME);
	    
	    saveScreenshotToFile(file, WIDTH, HEIGHT);
	}
    }

    /* gracefully close */

    glfwTerminate();

    free(empty);
}
