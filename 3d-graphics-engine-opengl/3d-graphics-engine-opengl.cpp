// 3D graphics engine OpenGL variant

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>


constexpr int kScreenWidth{ 640 };
constexpr int kScreenHeight{ 480 };

//Window
SDL_Window* gWindow{ nullptr };

//Surface in de window
SDL_Surface* gScreenSurface{ nullptr };

SDL_Surface* gImgSurface{ nullptr };

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
        //Window maken
        if (gWindow = SDL_CreateWindow("OpenGL", kScreenWidth, kScreenHeight, 0); gWindow == nullptr)
        {
            SDL_Log("SDL error: %s\n", SDL_GetError());
            success = false;
        }
        else
        {
            gScreenSurface = SDL_GetWindowSurface(gWindow);
        }
    }

    return success;
}

bool loadMedia()
{
    bool success{ true };

    //Image laden in img surface
    std::string imagePath{ "test.bmp" };
    if (gImgSurface = SDL_LoadBMP(imagePath.c_str()); gImgSurface == nullptr)
    {
        SDL_Log("SDL Error: %s\n", imagePath.c_str(), SDL_GetError());
        success = false;
    }

    return success;
}

void close()
{
    SDL_DestroySurface(gImgSurface);
    gImgSurface = nullptr;

    SDL_DestroyWindow(gWindow);
    gWindow = nullptr;
    gScreenSurface = nullptr;

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
        if (loadMedia() == false)
        {
            SDL_Log("Fout met img inladen.\n");
            exitCode = 2;
        }
        else
        {
            bool quit{ false };

            SDL_Event e;
            SDL_zero(e);

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

                //Main surface wit
                SDL_FillSurfaceRect(gScreenSurface, nullptr, SDL_MapSurfaceRGB(gScreenSurface, 0xFF, 0xFF, 0xFF));

                //Img surface op main surface blitten
                SDL_BlitSurface(gImgSurface, nullptr, gScreenSurface, nullptr);

                //Update
                SDL_UpdateWindowSurface(gWindow);
            }
        }
    }

    //Clean up
    close();

    return exitCode;
}
