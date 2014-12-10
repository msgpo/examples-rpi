

#include <SDL_haptic.h>
#include <SDL.h>
#include <stdio.h>


int main (int argc, char* argv[])
{

	if (SDL_Init(SDL_INIT_JOYSTICK) < 0)
	{
   		// Unrecoverable error, exit here.
    		printf("SDL_Init failed: %s\n", SDL_GetError());
    		return 1;
	}

	if (SDL_NumJoysticks() < 1)
	{
		printf("No joysticks connected\n");
		return 2;
	}

	SDL_Joystick* joy = SDL_JoystickOpen(0);

	if (!joy)
	{
		printf("Failed to open Joystick (0)\n");
		return 3;
	}

	printf("Opened Joystick 0\n");
	printf("Name: %s\n", SDL_JoystickNameForIndex(0));
	printf("Number of Axes: %d\n", SDL_JoystickNumAxes(joy));
        printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(joy));
        printf("Number of Balls: %d\n", SDL_JoystickNumBalls(joy));

	SDL_Haptic* ff_joy = SDL_HapticOpenFromJoystick(joy);
	if (!ff_joy)
	{
		printf("Failed to open Force Feedback Joystick: %s\n", SDL_GetError());
		return 3;
	}

	// Initialize simple rumble
	if (SDL_HapticRumbleInit( ff_joy ) != 0)
   		return 4;

	// Play effect at 50% strength for 2 seconds
	SDL_HapticRumblePlay( ff_joy, 0.5, 2000 );

// -------- Shut down --------------

	SDL_HapticClose(ff_joy);

	if (SDL_JoystickGetAttached(joy)) {
        	SDL_JoystickClose(joy);
    	}

	return 0;
}
