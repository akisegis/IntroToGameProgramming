/**
* Author: Aki Segismundo
* Assignment: Simple 2D Scene
* Date due: 2024-02-17, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum Coordinate
{
    x_coordinate,
    y_coordinate
};

#define LOG(argument) std::cout << argument << '\n'

const int WINDOW_WIDTH = 640,
          WINDOW_HEIGHT = 480;

float       BG_RED = 0.0f,
            BG_BLUE = 0.0f,
            BG_GREEN = 0.0f;

const float BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const float DEGREES_PER_SECOND = 90.0f;
const float ROT_ANGLE = glm::radians(1.5f); // Let's try a smaller angle

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

const char DOG_SPRITE_FILEPATH[] = "assets/dog.png";
const char BALL_SPRITE_FILEPATH[] = "assets/tennis.png";

SDL_Window* g_display_window;
bool g_game_is_running = true;
bool ball_is_growing      = true;
float  ball_rotate   = 0.0;

ShaderProgram g_shader_program;
glm::mat4 view_matrix, dog_model_matrix, ball_model_matrix, projection_matrix, trans_matrix;

float g_previous_ticks = 0.0f;

GLuint dog_texture_id, ball_texture_id;

// position
glm::vec3 dog_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 ball_position = glm::vec3(1.0f, 1.0f, 0.0f);

// movement
glm::vec3 dog_movement = glm::vec3(0.5f, 0.5f, 0.0f);
glm::vec3 ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        LOG(filepath);
        assert(false);
    }
    
    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);
    
    return textureID;
}

void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);
    
    g_display_window = SDL_CreateWindow("Hello, Textures!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    dog_model_matrix = glm::mat4(1.0f);
    ball_model_matrix = glm::mat4(1.0f);
    view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.
    
    g_shader_program.SetProjectionMatrix(projection_matrix);
    g_shader_program.SetViewMatrix(view_matrix);
    // Notice we haven't set our model matrix yet!
    
    glUseProgram(g_shader_program.programID);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    dog_texture_id = load_texture(DOG_SPRITE_FILEPATH);
    ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);
    
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void processinput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            g_game_is_running = false;
        }
    }
}

void update()
{
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;
    
    // background color changer
    if (int(ticks) == ticks){
        float diff = 0.2;
        if (BG_RED < 1.0){
            BG_RED += diff;
        } else {
            BG_RED -= diff;
        }
        glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    }

    // Add             direction       * elapsed time * units per second
    dog_position += dog_movement * delta_time * 1.0f;
    dog_model_matrix = glm::mat4(1.0f);
    dog_model_matrix = glm::translate(dog_model_matrix, dog_position); 
    
    ball_rotate += DEGREES_PER_SECOND * delta_time * 2; // the ball is rotating at 180-degree per second
    ball_model_matrix = glm::mat4(1.0f);
    
    ball_model_matrix = glm::translate(dog_model_matrix, ball_position); // the ball follows the dog
    ball_model_matrix = glm::rotate(ball_model_matrix, glm::radians(ball_rotate), glm::vec3(0.0f, 0.0f, 1.0f));
}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.SetModelMatrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Vertices
    float dog_vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };
    float ball_vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float dog_texture[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };
    
    float ball_texture[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };
    
    
    
    glVertexAttribPointer(g_shader_program.positionAttribute, 2, GL_FLOAT, false, 0, dog_vertices);
    glEnableVertexAttribArray(g_shader_program.positionAttribute);
    
    glVertexAttribPointer(g_shader_program.texCoordAttribute, 2, GL_FLOAT, false, 0, dog_texture);
    glEnableVertexAttribArray(g_shader_program.texCoordAttribute);
    
    // Bind texture
    draw_object(dog_model_matrix, dog_texture_id);
    
    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.positionAttribute);
    glDisableVertexAttribArray(g_shader_program.texCoordAttribute);
    
    glVertexAttribPointer(g_shader_program.positionAttribute, 2, GL_FLOAT, false, 0, ball_vertices);
    glEnableVertexAttribArray(g_shader_program.positionAttribute);
    
    glVertexAttribPointer(g_shader_program.texCoordAttribute, 2, GL_FLOAT, false, 0, ball_texture);
    glEnableVertexAttribArray(g_shader_program.texCoordAttribute);
    
    // Bind texture
    draw_object(ball_model_matrix, ball_texture_id);
    
    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.positionAttribute);
    glDisableVertexAttribArray(g_shader_program.texCoordAttribute);
    
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
}

/**
 Start here—we can see the general structure of a game loop without worrying too much about the details yet.
 */
int main(int argc, char* argv[])
{
    initialise();
    
    while (g_game_is_running)
    {
        processinput();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
