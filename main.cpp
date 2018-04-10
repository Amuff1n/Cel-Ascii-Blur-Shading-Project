//
//  main.cpp
//  CelAsciiBlur


#ifdef _WIN32
#include "WindowsInclude.h"
#endif

#ifdef __APPLE__
#include "MacInclude.h"
#endif

#ifdef __linux__
#include "LinuxInclude.h"
#endif

#include <iostream>

const char NAME[] = "Computer Graphics Project";
const int WIDTH = 1024;
const int HEIGHT = 720;

bool RUNNING = false;


int main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_GLContext context;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Could not initialize SDL\n");
        SDL_Quit();
        return -1;
    }
    
    // Necessary on Mac
	#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    
    /* Turn on double buffering with a 24bit Z buffer.
     * You may need to change this to 16 or 32 for your system */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    
    // Create window
    window = SDL_CreateWindow(NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window)
    {
        printf("Could not create window.\n");
        SDL_Quit();
        return -1;
    }
        
    // Create OpenGL context and attach it to window
    context = SDL_GL_CreateContext(window);
    if (!context)
    {
        printf("Could not create OpenGL context.\n");
        SDL_Quit();
        return -1;
    }
    
    /* This makes our buffer swap syncronized with the monitor's vertical refresh */
    SDL_GL_SetSwapInterval(1);
    
    // Clear to black
    glClearColor ( 0.0f, 0.0f, 0.0f, 1.0f );
    glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	SDL_GL_SwapWindow(window);
    
    // SDL event loop
    RUNNING = true;
    SDL_Event event;
    while (RUNNING)
    {
        // Check for new events
        while(SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                RUNNING = false;
            }
        }
    }
    
    /* Delete our opengl context, destroy our window, and shutdown SDL */
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
