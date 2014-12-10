
#include <SDL.h>
#include <stdio.h>

static unsigned char myKeyState[SDL_NUM_SCANCODES];
unsigned int ok = 1;

void event_sdl_keydown(int keysym, int keymod)
{
        myKeyState[keysym] = 1;
	printf("%d Key down\n", keysym); 

	if (keysym == 27) ok = 0;
}

void event_sdl_keyup(int keysym, int keymod)
{
	myKeyState[keysym] = 0;
	printf("%d Key up\n", keysym);
}

static int SDLCALL event_sdl_filter(void *userdata, SDL_Event *event)
{
    int cmd, action;

	printf("event_sdl_filter()\n");

    switch(event->type)
    {
        // user clicked on window close button
        case SDL_QUIT:
		printf("SDL_QUIT\n");
		ok = 0;

            break;

        case SDL_KEYDOWN:
#if SDL_VERSION_ATLEAST(1,3,0)
            if (event->key.repeat)
                return 0;

            event_sdl_keydown(event->key.keysym.scancode, event->key.keysym.mod);
#else
            event_sdl_keydown(event->key.keysym.sym, event->key.keysym.mod);
#endif
            return 0;
        case SDL_KEYUP:
#if SDL_VERSION_ATLEAST(1,3,0)
            event_sdl_keyup(event->key.keysym.scancode, event->key.keysym.mod);
#else
            event_sdl_keyup(event->key.keysym.sym, event->key.keysym.mod);
#endif
            return 0;
	}
}

int main( int argc, char* args[] )
{
	unsigned int i;

	printf("starting keyboard test\n");

	//Initialize SDL 
	if( SDL_Init( SDL_INIT_EVENTS ) < 0 ) 
	{ 
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() ); 
	}

    	for (i = 0; i < SDL_NUM_SCANCODES; i++)
    	{
       		myKeyState[i] = 0;
    	}

#if !SDL_VERSION_ATLEAST(2,0,0)
    SDL_EnableKeyRepeat(0, 0);
#endif
	SDL_SetEventFilter(event_sdl_filter, NULL);

	while (ok)
	{
		printf("."); fflush(stdout);
		SDL_PumpEvents();
		usleep(100000);
	}

	//Quit SDL subsystems 
	SDL_Quit(); 
	printf("finished\n");
	return 0;
}
