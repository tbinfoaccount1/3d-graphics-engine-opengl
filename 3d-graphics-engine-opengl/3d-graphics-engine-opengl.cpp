// 3D graphics engine OpenGL variant

#define GLEW_STATIC
#include <string>
#include <GL/glew.h>
#include <SDL3/SDL.h>
#include <iostream>

constexpr int kScreenWidth{ 640 };
constexpr int kScreenHeight{ 480 };

//Window
SDL_Window* gWindow{ nullptr };

SDL_GLContext glContext;

// Test Data
static const GLfloat vertexBufferData[] = {
   -1.0f, -1.0f, 0.0f,
   1.0f, -1.0f, 0.0f,
   0.0f,  1.0f, 0.0f,
};

bool init()
{
    bool success{ true };

    //SDL initialiseren
    if (SDL_Init(SDL_INIT_VIDEO) == false)
    {
        SDL_Log("SDL error: %s\n", SDL_GetError());
        success = false;
    }
    else
    {
        // OpenGL attributen definiëren
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3); // OpenGL versie
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // Enkele buffer
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24); // Pixel buffer grootte

        //Window maken
        if (gWindow = SDL_CreateWindow("OpenGL", kScreenWidth, kScreenHeight, SDL_WINDOW_OPENGL); gWindow == nullptr)
        {
            SDL_Log("SDL error: %s\n", SDL_GetError());
            success = false;
        }
        else {
            // OpenGL context
            SDL_GLContext glContext = SDL_GL_CreateContext(gWindow);
            if (!glContext) {
                std::cerr << "OpenGL Context Error: " << SDL_GetError() << "\n";
                SDL_DestroyWindow(gWindow);
                SDL_Quit();
                return false;
            }

            // GLEW initialiseren
            glewExperimental = GL_TRUE;
            GLenum glewErrorCode = glewInit();
            if (GLEW_OK != glewErrorCode) {
                std::cerr << "GLEW init error: " << glewGetErrorString(glewErrorCode) << "\n";
            }
        }
    }

    return success;
}

void close()
{
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(gWindow);
    SDL_Quit();
}


int main(int argc, char* args[])
{
    int exitCode{ 0 };

    //Initialiseren
    if (init() == false)
    {
        SDL_Log("Fout bij initialiseren.\n");
        exitCode = 1;
    }
    else
    {
        SDL_GLContext glContext = SDL_GL_CreateContext(gWindow);
        if (!glContext) {
            std::cerr << "OpenGL Context Error: " << SDL_GetError() << "\n";
            SDL_DestroyWindow(gWindow);
            SDL_Quit();
            return -1;
        }
        else
        {
            bool quit{ false };

            SDL_Event e;
            SDL_zero(e);

            // Maak de VAO aan
            GLuint vertexArrayID;
            glGenVertexArrays(1, &vertexArrayID);
            glBindVertexArray(vertexArrayID);
            std::cout << "VAO aangemaakt met id: " << std::to_string(vertexArrayID) << "\n";

            // Maak en VBO aan
            GLuint vertexBufferID;
            glGenBuffers(1, &vertexBufferID);
            glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBufferData), vertexBufferData, GL_STATIC_DRAW);

            //Main loop
            while (quit == false)
            {
                while (SDL_PollEvent(&e) == true)
                {
                    if (e.type == SDL_EVENT_QUIT)
                    {
                        //Quit als quit event
                        quit = true;
                    }
                }

                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Draw calls hier
                
                // Swap buffer naar scherm
                SDL_GL_SwapWindow(gWindow);
            }
        }
    }

    //Clean up
    close();

    return exitCode;
}
