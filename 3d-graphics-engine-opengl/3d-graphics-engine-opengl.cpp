// 3D graphics engine OpenGL variant

#define GLEW_STATIC
#include <string>
#include <GL/glew.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_image.h>
#include <algorithm>
#include <iostream>
#include <filesystem>
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

	std::string filePath; // Pad naar OBJ bestand uit command line argument halen
    if (argc > 1) filePath = args[1];
    else {
        std::cout << "Geen OBJ bestand opgegeven via command line arguments.\n";
        return 1;
    }

	std::string fileName = std::filesystem::path(filePath).filename().string();
	std::cout << "OBJ bestand: " << fileName << "\n";
    //Initialiseren
    if (init() == false)
    {
        SDL_Log("Fout bij initialiseren.\n");
        exitCode = 1;
    }
    else
    {
        // Laad OBJ file
        ObjData objData = loadObjFile(filePath);
        std::vector<glm::vec3> vertices = objData.vertices;
        std::vector<ObjFace> faces = objData.faces;
        std::vector<glm::vec2> uvs = objData.textureCoords;
        std::vector<glm::vec3> normals = objData.normals;
        std::vector<Material> materials = objData.materials;

        bool quit{ false };

        SDL_Event e;
        SDL_zero(e);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        // Maak de VAO aan
        GLuint vertexArrayID;
        glGenVertexArrays(1, &vertexArrayID);
        glBindVertexArray(vertexArrayID);
        std::cout << "VAO aangemaakt met id: " << std::to_string(vertexArrayID) << "\n";

        // ---
		// Per material maken we een aparte VBO voor vertices, UVs en een texture. We flatten de mesh per materiaal index zodat we per materiaal kunnen tekenen.

        size_t materialCount = std::max<size_t>(1, materials.size()); // Het aantal materialen
		std::vector<std::vector<glm::vec3>> flatVerticesPerMaterial(materialCount); // Lijst van materialen, met elk materiaal een lijst van vertices)
        std::vector<std::vector<glm::vec2>> flatUVsPerMaterial(materialCount); // Hetzelfde voor UVs

        // Per materiaal
        for (const auto& face : objData.faces)
        {
            int materialIndex = face.materialIndex >= 0 ? face.materialIndex : 0; // ID van materiaal
            if (materialIndex >= static_cast<int>(materialCount))
                materialIndex = 0;
            for (size_t i = 0; i < face.vertexIndices.size(); ++i)
            {
				// Per vertex van de face, push de vertex en UV naar de juiste flat vector
                flatVerticesPerMaterial[materialIndex].push_back(objData.vertices[face.vertexIndices[i]]);
                if (!objData.textureCoords.empty() && !face.textureCoordIndices.empty())
                    flatUVsPerMaterial[materialIndex].push_back(objData.textureCoords[face.textureCoordIndices[i]]);
                else
                    flatUVsPerMaterial[materialIndex].push_back(glm::vec2{ 0.0f, 0.0f });
            }
        }

		// VBOs en textures per materiaal aanmaken
        std::vector<GLuint> vertexBufferIDs(materialCount, 0);
        std::vector<GLuint> uvBufferIDs(materialCount, 0);
        std::vector<GLuint> textureIDs(materialCount, 0);

        for (size_t m = 0; m < materialCount; ++m)
        {
            // Voor elk materiaal:
            if (!flatVerticesPerMaterial[m].empty())
            {
				// Als er vertices zijn, maak een VBO aan en upload de data
                glGenBuffers(1, &vertexBufferIDs[m]);
                glBindBuffer(GL_ARRAY_BUFFER, vertexBufferIDs[m]);
                glBufferData(GL_ARRAY_BUFFER, flatVerticesPerMaterial[m].size() * sizeof(glm::vec3), flatVerticesPerMaterial[m].data(), GL_STATIC_DRAW);
                std::cout << "VBO aangemaakt met id: " << std::to_string(vertexBufferIDs[m]) << "\n";
            }

            if (!flatUVsPerMaterial[m].empty())
            {
                // Zelfde voor UVs
                glGenBuffers(1, &uvBufferIDs[m]);
                glBindBuffer(GL_ARRAY_BUFFER, uvBufferIDs[m]);
                glBufferData(GL_ARRAY_BUFFER, flatUVsPerMaterial[m].size() * sizeof(glm::vec2), flatUVsPerMaterial[m].data(), GL_STATIC_DRAW);
                std::cout << "VBO aangemaakt met id: " << std::to_string(uvBufferIDs[m]) << "\n";
            }

            // Maak een GL texture aan op basis van de diffuse texture van het materiaal
            glGenTextures(1, &textureIDs[m]);
            glBindTexture(GL_TEXTURE_2D, textureIDs[m]);
            if (m < materials.size() && !materials[m].diffuseTexture.isEmpty())
            {
				// Als het materiaal een diffuse texture heeft, upload de pixels naar OpenGL
                const Texture& tex = materials[m].diffuseTexture;
                // 32bit RGBA
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.pixels.data());
            }
            else
            {
                // Witte pixel als er geen texture is
                unsigned char white[4] = { 255, 255, 255, 255 };
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
            }
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			std::cout << "Texture aangemaakt met id: " << std::to_string(textureIDs[m]) << "\n";
        }

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

			// Krijg location van textureSampler uniform in fragment shader en bind de texture (unit 0) aan deze uniform
            GLint samplerID = glGetUniformLocation(programID, "textureSampler");
            if (samplerID != -1)
                // Als -1 bestaat de uniform niet in de shader.
                glUniform1i(samplerID, 0); // Zet op 0.

            // Per materiaal: bind VBOs en texture en teken
            for (size_t m = 0; m < vertexBufferIDs.size(); ++m)
            {
                if (vertexBufferIDs[m] == 0)
                    continue;

                // Vertex positions
                glEnableVertexAttribArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, vertexBufferIDs[m]);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

                // UVs
                if (uvBufferIDs[m] != 0)
                {
                    glEnableVertexAttribArray(1);
                    glBindBuffer(GL_ARRAY_BUFFER, uvBufferIDs[m]);
                    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
                }
                else
                {
                    glDisableVertexAttribArray(1);
                }

                // Bind texture aan unit 0
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureIDs[m]);

                // Teken de driehoeken van het materiaal
                GLsizei count = static_cast<GLsizei>(flatVerticesPerMaterial[m].size());
                glDrawArrays(GL_TRIANGLES, 0, count);

                glDisableVertexAttribArray(0);
                if (uvBufferIDs[m] != 0)
                    glDisableVertexAttribArray(1);
            }

            // Swap buffer naar scherm
            SDL_GL_SwapWindow(gWindow);
        }
    }

    //Clean up
    close();

    return exitCode;
}
