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
#include <gtc/type_ptr.hpp>

#include "shader_loader.cpp"
#include "obj_loader.h"
#include "controls.h"
#include "light.h"

constexpr int kScreenWidth{ 640 };
constexpr int kScreenHeight{ 480 };

//Window
SDL_Window* gWindow{ nullptr };

SDL_GLContext glContext;

// Lichtbron
constexpr Light light{
    glm::vec3{ 5.0f, 4.0f, 5.0f }, // positie
    glm::vec3{ 0.5f, 0.5f, 0.5f }, // la
    glm::vec3{ 0.5f, 0.5f, 0.5f }, // ld
    glm::vec3{ 0.5f, 0.5f, 0.5f }  // ls
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

    // Fix pad errors door absolute paths te gebruiken
    std::filesystem::path exePath = std::filesystem::absolute(args[0]);
    std::filesystem::path baseDir = exePath.parent_path();

    std::filesystem::path vertexPath = baseDir / "vertex_shader.vert";
    std::filesystem::path fragmentPath = baseDir / "fragment_shader.frag";

    std::string filePath; // Pad naar OBJ bestand uit command line argument halen
    if (argc > 1) filePath = args[1];
    else {
        std::cout << "Geen OBJ bestand opgegeven via command line arguments.\n";
        std::cout << "Druk op een toets om af te sluiten.\n";
        system("pause");
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
        calculateTangents(objData);

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
        // Per material maken we een aparte VBO voor vertices, UVs, normals en een texture. We flatten de mesh per materiaal index zodat we per materiaal kunnen tekenen.

        size_t materialCount = std::max<size_t>(1, materials.size()); // Het aantal materialen
        std::vector<std::vector<glm::vec3>> flatVerticesPerMaterial(materialCount); // Lijst van materialen, met elk materiaal een lijst van vertices)
        std::vector<std::vector<glm::vec2>> flatUVsPerMaterial(materialCount); // Hetzelfde voor UVs
        std::vector<std::vector<glm::vec3>> flatNormalsPerMaterial(materialCount); // Hetzelfde voor normals
		std::vector<std::vector<glm::vec3>> flatTangentsPerMaterial(materialCount); // Hetzelfde voor tangents
		std::vector<std::vector<glm::vec3>> flatBitangentsPerMaterial(materialCount); // Hetzelfde voor bitangents

        // Per materiaal
        for (const auto& face : objData.faces)
        {
            int materialIndex = face.materialIndex >= 0 ? face.materialIndex : 0; // ID van materiaal
            if (materialIndex >= static_cast<int>(materialCount))
                materialIndex = 0;
            for (size_t i = 0; i < face.vertexIndices.size(); ++i)
            {
                // Per vertex van de face, push de vertex, UV, normal en tangents naar de juiste flat vector
                flatVerticesPerMaterial[materialIndex].push_back(objData.vertices[face.vertexIndices[i]]);
                if (!objData.textureCoords.empty() && !face.textureCoordIndices.empty())
                    flatUVsPerMaterial[materialIndex].push_back(objData.textureCoords[face.textureCoordIndices[i]]);
                else
                    flatUVsPerMaterial[materialIndex].push_back(glm::vec2{ 0.0f, 0.0f });
                if (!objData.normals.empty() && !face.normalIndices.empty())
                    flatNormalsPerMaterial[materialIndex].push_back(objData.normals[face.normalIndices[i]]);
                else
                    flatNormalsPerMaterial[materialIndex].push_back(glm::vec3{ 0.0f, 0.0f, 0.0f });
				flatTangentsPerMaterial[materialIndex].push_back(face.tangents[i]);
				flatBitangentsPerMaterial[materialIndex].push_back(face.bitangents[i]);
            }
        }

        // VBOs en textures per materiaal aanmaken
        std::vector<GLuint> vertexBufferIDs(materialCount, 0);
        std::vector<GLuint> uvBufferIDs(materialCount, 0);
        std::vector<GLuint> normalBufferIDs(materialCount, 0);
        std::vector<GLuint> tangentBufferIDs(materialCount, 0);
        std::vector<GLuint> bitangentBufferIDs(materialCount, 0);
        std::vector<GLuint> textureIDs(materialCount, 0);
		std::vector<GLuint> heightMapTextureIDs(materialCount, 0);

        // Definieer de contanten uit de material
        std::vector<glm::vec3> kaVector;
        std::vector<glm::vec3> kdVector;
        std::vector<glm::vec3> ksVector;
        std::vector<float> nsVector; // shininess per material

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

            if (!flatNormalsPerMaterial[m].empty())
            {
                // Zelfde voor normals
                glGenBuffers(1, &normalBufferIDs[m]);
                glBindBuffer(GL_ARRAY_BUFFER, normalBufferIDs[m]);
                glBufferData(GL_ARRAY_BUFFER, flatNormalsPerMaterial[m].size() * sizeof(glm::vec3), flatNormalsPerMaterial[m].data(), GL_STATIC_DRAW);
                std::cout << "VBO aangemaakt met id: " << std::to_string(normalBufferIDs[m]) << "\n";
            }

            if (!flatTangentsPerMaterial[m].empty())
            {
                // Zelfde voor tangents
                glGenBuffers(1, &tangentBufferIDs[m]);
                glBindBuffer(GL_ARRAY_BUFFER, tangentBufferIDs[m]);
                glBufferData(GL_ARRAY_BUFFER, flatTangentsPerMaterial[m].size() * sizeof(glm::vec3), flatTangentsPerMaterial[m].data(), GL_STATIC_DRAW);
                std::cout << "VBO aangemaakt met id: " << std::to_string(tangentBufferIDs[m]) << "\n";
            }

            if (!flatBitangentsPerMaterial[m].empty())
            {
                // Zelfde voor bitangents
                glGenBuffers(1, &bitangentBufferIDs[m]);
                glBindBuffer(GL_ARRAY_BUFFER, bitangentBufferIDs[m]);
                glBufferData(GL_ARRAY_BUFFER, flatBitangentsPerMaterial[m].size() * sizeof(glm::vec3), flatBitangentsPerMaterial[m].data(), GL_STATIC_DRAW);
                std::cout << "VBO aangemaakt met id: " << std::to_string(bitangentBufferIDs[m]) << "\n";
            }

			float nsValue = 32.0f;
            if (!materials.empty() && m < materials.size() && !materials[m].diffuseTexture.isEmpty())
            {
                // Als het materiaal een diffuse texture heeft, upload ka/kd/ks en ns
                kaVector.push_back(materials[m].ambientColor);
                kdVector.push_back(materials[m].diffuseColor);
                ksVector.push_back(materials[m].specularColor);
                nsValue = materials[m].shininess;
            }
            else
            {
                kaVector.push_back(glm::vec3{ 1.0f, 1.0f, 1.0f });
                kdVector.push_back(glm::vec3{ 1.0f, 1.0f, 1.0f });
                ksVector.push_back(glm::vec3{ 1.0f, 1.0f, 1.0f });
                nsValue = 32.0f;
            }
            nsVector.push_back(nsValue);

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

            // Maak een GL texture aan op basis van de heightmap texture van het materiaal
            glGenTextures(1, &heightMapTextureIDs[m]);
            glBindTexture(GL_TEXTURE_2D, heightMapTextureIDs[m]);
            if (m < materials.size() && !materials[m].bumpTexture.isEmpty())
            {
                // Als het materiaal een heightmap texture heeft, upload de pixels naar OpenGL
                const Texture& tex = materials[m].bumpTexture;
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
        GLuint programID = LoadShaders(vertexPath.string().c_str(), fragmentPath.string().c_str());

        // Uniform locaties voor shader
        GLint modelLoc = glGetUniformLocation(programID, "model");
        GLint viewLoc = glGetUniformLocation(programID, "view");
        GLint projLoc = glGetUniformLocation(programID, "projection");
        GLint camPosLoc = glGetUniformLocation(programID, "camPos");
        GLint samplerLoc = glGetUniformLocation(programID, "textureSampler");
		GLint heightMapSamplerLoc = glGetUniformLocation(programID, "heightMapSampler");

        GLint kaLoc = glGetUniformLocation(programID, "ka");
        GLint kdLoc = glGetUniformLocation(programID, "kd");
        GLint ksLoc = glGetUniformLocation(programID, "ks");
        GLint nsLoc = glGetUniformLocation(programID, "ns");

        GLint lposLoc = glGetUniformLocation(programID, "lpos");
        GLint laLoc = glGetUniformLocation(programID, "la");
        GLint ldLoc = glGetUniformLocation(programID, "ld");
        GLint lsLoc = glGetUniformLocation(programID, "ls");

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

            // Bereken matrices van input
            computeMatricesFromInputs();
            glm::mat4 ProjectionMatrix = getProjectionMatrix();
            glm::mat4 ViewMatrix = getViewMatrix();
            glm::mat4 ModelMatrix = glm::mat4(1.0);

            // Upload matrices naar shader
            if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(ProjectionMatrix));
            if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(ViewMatrix));
            if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(ModelMatrix));

            // Krijg cam pos
            glm::vec3 camPos = getCameraPosition();

            // Upload camera position
            if (camPosLoc != -1) glUniform3fv(camPosLoc, 1, glm::value_ptr(camPos));

            // Upload licht uniforms
            if (lposLoc != -1) glUniform3fv(lposLoc, 1, glm::value_ptr(light.light_position));
            if (laLoc != -1) glUniform3fv(laLoc, 1, glm::value_ptr(light.la));
            if (ldLoc != -1) glUniform3fv(ldLoc, 1, glm::value_ptr(light.ld));
            if (lsLoc != -1) glUniform3fv(lsLoc, 1, glm::value_ptr(light.ls));

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

                // Normals
                if (normalBufferIDs[m] != 0)
                {
                    glEnableVertexAttribArray(2);
                    glBindBuffer(GL_ARRAY_BUFFER, normalBufferIDs[m]);
                    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
                }
                else
                {
                    glDisableVertexAttribArray(2);
                }

                // Tangents
				if (tangentBufferIDs[m] != 0)
				{
					glEnableVertexAttribArray(3);
					glBindBuffer(GL_ARRAY_BUFFER, tangentBufferIDs[m]);
					glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
				}
				else
				{
					glDisableVertexAttribArray(3);
				}

                // Bitangents 
				if (bitangentBufferIDs[m] != 0)
				{
					glEnableVertexAttribArray(4);
					glBindBuffer(GL_ARRAY_BUFFER, bitangentBufferIDs[m]);
					glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
				}
				else
				{
					glDisableVertexAttribArray(4);
				}

                // Stuur material uniforms naar shader
                if (kaLoc != -1) glUniform3fv(kaLoc, 1, glm::value_ptr(kaVector[m]));
                if (kdLoc != -1) glUniform3fv(kdLoc, 1, glm::value_ptr(kdVector[m]));
                if (ksLoc != -1) glUniform3fv(ksLoc, 1, glm::value_ptr(ksVector[m]));
                if (nsLoc != -1) glUniform1f(nsLoc, nsVector[m]);

                // Bind texture unit 0 and upload sampler uniform
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureIDs[m]);
                if (samplerLoc != -1) glUniform1i(samplerLoc, 0);

				// Bind bump map texture unit 1 and upload sampler uniform
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, heightMapTextureIDs[m]);
				if (heightMapSamplerLoc != -1) glUniform1i(heightMapSamplerLoc, 1);

                // Teken de driehoeken
                GLsizei vertexCount = static_cast<GLsizei>(flatVerticesPerMaterial[m].size());
                glDrawArrays(GL_TRIANGLES, 0, vertexCount);

                // Cleanup vertex attribs
                glDisableVertexAttribArray(0);
                if (uvBufferIDs[m] != 0) glDisableVertexAttribArray(1);
                if (normalBufferIDs[m] != 0) glDisableVertexAttribArray(2);
                if (tangentBufferIDs[m] != 0) glDisableVertexAttribArray(3);
                if (bitangentBufferIDs[m] != 0) glDisableVertexAttribArray(4);
            }

            // Swap buffer naar scherm
            SDL_GL_SwapWindow(gWindow);
        }
    }

    //Clean up
    close();

    std::cout << "Druk op een toets om af te sluiten.\n";
    system("pause");
    return exitCode;
}