
#include <stdio.h>

#include <sys/time.h>
#include "unistd.h"
#include "linux/kd.h"
#include "termios.h"
#include "fcntl.h"
#include "sys/ioctl.h"

#include <signal.h>


#define DEBUG_PRINT(...) printf(__VA_ARGS__)

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(...)
#endif

///////////////////////////////////////////////////////////////////////////////////////////////

static struct termios tty_attr_old;
int old_keyboard_mode = K_XLATE;

static void restoreKeyboard()
{
		tcsetattr(0, TCSAFLUSH, &tty_attr_old);
		ioctl(0, KDSKBMODE, K_XLATE);
}

static void scanKeyboard()
{
	struct termios tty_attr;
    int flags;
    
	/* turn off buffering, echo and key processing */
    tty_attr = tty_attr_old;
    tty_attr.c_lflag &= ~(ICANON | ECHO | ISIG);
    tty_attr.c_iflag &= ~(ISTRIP | INLCR | ICRNL | IGNCR | IXON | IXOFF);
    tcsetattr(0, TCSANOW, &tty_attr);

    ioctl(0, KDSKBMODE, K_RAW);

    if (ioctl(0, KDGKBMODE, &old_keyboard_mode) < 0) {
		DEBUG_PRINT("Could not change keyboard mode\n");
    }
}

void restKeyboard(int val)
{
	DEBUG_PRINT("signal %d, restoring keyboard\n", val);
	restoreKeyboard();
}

///////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char* argv[])
{
	static int keyState = 0;
	// http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
	// http://wiki.libsdl.org/SDL_Keymod?highlight=%28\bCategoryEnum\b%29|%28CategoryKeyboard%29

	char buf[] = {0,0};
	char comment[30];
	int res;
	int byteToRead =0;
	int bGotKey = 0;
	int updateState =0;
	int bOK = 1;
	int x;

	FILE * pFile = fopen("scancodes.txt","w");
	
	if (pFile == NULL){
		printf("Could not open output file\n");
		return 1;
	}

	signal(SIGILL, &restKeyboard);	//illegal instruction
	signal(SIGTERM, &restKeyboard);	
	signal(SIGSEGV, &restKeyboard);
	signal(SIGINT, &restKeyboard);
	signal(SIGQUIT, &restKeyboard);

	tcgetattr(0, &tty_attr_old);

	while(bOK)
	//for (x=0; x < 5; x++)
	{
		scanKeyboard();
		
		printf("Press key to scan: "); fflush(stdout);
		
		read(0, &buf[0], 1);
		buf[0] = 0;
		buf[1] = 0;

		/* read scan code from stdin */
		while ((res = read(0, &buf[0], 1)) <= 0);
			
		printf("0x%02X 0x%02X\n", buf[0], buf[1]);
		
		restoreKeyboard();

		printf("comment: "); fflush(stdout);

		comment[0] = 0;
		scanf("%s", comment);

		comment[sizeof(comment)-1] = 0;

		fprintf(pFile, "%2X %2X, %3d %3d, %s\n",buf[0], buf[1], buf[0], buf[1], comment );

		if (buf[0] == 1) bOK = 0; // ESC
	}

	fclose(pFile);

	return 0;
}
