// 3D graphics engine OpenGL variant

#define GLEW_STATIC
#include <string>
#include <GL/glew.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_image.h>
#include <iostream>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include "shader_loader.cpp"
#include "obj_loader.h"
#include "controls.h"

constexpr int kScreenWidth{ 640 };
constexpr int kScreenHeight{ 480 };

//Window
SDL_Window* gWindow{ nullptr };

SDL_GLContext glContext;

ObjData objData = loadObjFile("C:\\Users\\s032010061601\\Downloads\\cube.obj");
std::vector<glm::vec3> vertices = objData.vertices;
std::vector<ObjFace> faces = objData.faces;
std::vector<glm::vec2> uvs = objData.textureCoords;
std::vector<glm::vec3> normals = objData.normals;
std::vector<Material> materials = objData.materials;

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
            glContext = SDL_GL_CreateContext(gWindow);
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

            // Z-buffer
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
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
        bool quit{ false };

        SDL_Event e;
        SDL_zero(e);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        // Maak de VAO aan
        GLuint vertexArrayID;
        glGenVertexArrays(1, &vertexArrayID);
        glBindVertexArray(vertexArrayID);
        std::cout << "VAO aangemaakt met id: " << std::to_string(vertexArrayID) << "\n";

		std::vector<glm::vec3> flatVertices;
        std::vector<glm::vec2> flatUVs;
		std::vector<glm::vec3> flatNormals;

		// Flatten de data zodat we een VBO kunnen maken
        for (const auto& face : objData.faces)
        {
            for (size_t i = 0; i < face.vertexIndices.size(); ++i)
            {
                flatVertices.push_back(objData.vertices[face.vertexIndices[i]]);
                if (!objData.textureCoords.empty() && !face.textureCoordIndices.empty())
                    flatUVs.push_back(objData.textureCoords[face.textureCoordIndices[i]]);
                if (!objData.normals.empty() && !face.normalIndices.empty())
                    flatNormals.push_back(objData.normals[face.normalIndices[i]]);
            }
        }

        // Maak de VBOs aan
        GLuint vertexBufferID;
        glGenBuffers(1, &vertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, flatVertices.size() * sizeof(glm::vec3), &flatVertices[0], GL_STATIC_DRAW);

        GLuint uvBufferID;
        glGenBuffers(1, &uvBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
        glBufferData(GL_ARRAY_BUFFER, flatUVs.size() * sizeof(glm::vec2), &flatUVs[0], GL_STATIC_DRAW);

        // Laad de shader in een shader program en sla de ID op.
        GLuint programID = LoadShaders("vertex_shader.vert", "fragment_shader.frag");
		// Krijg de locatie van de uniform variabele MVP in de shader
        GLint MatrixID = glGetUniformLocation(programID, "MVP");

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

				if (e.type == SDL_EVENT_KEY_DOWN)
				{
					//Quit als escape
					if (e.key.key == SDLK_ESCAPE)
					{
						quit = true;
					}
				}
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Draw calls hier
            glUseProgram(programID);

            // Bereken MVP matrix van input
            computeMatricesFromInputs();
            glm::mat4 ProjectionMatrix = getProjectionMatrix();
            glm::mat4 ViewMatrix = getViewMatrix();
            glm::mat4 ModelMatrix = glm::mat4(1.0);
            glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

            // Stuur naar shader
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

            // 1rst attribute buffer : vertices
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
            glVertexAttribPointer(
                0,                  // attribute
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
            );

            // 2nd attribute buffer : UVs
            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
            glVertexAttribPointer(
                1,                                // attribute slot
                2,                                // grootte
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void*)0                          // array buffer offset
            );

            // Teken de triangles
            glDrawArrays(GL_TRIANGLES, 0, flatVertices.size());

            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);

            // Swap buffer naar scherm
            SDL_GL_SwapWindow(gWindow);
        }
    }

    //Clean up
    close();

    return exitCode;
}
