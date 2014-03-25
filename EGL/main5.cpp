
#include "rpiGLES.h"

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <stdlib.h>
#include <iostream>

#include  <cmath>
#include  <sys/time.h>

using namespace std;

const char vertex_src [] =
"                                        \
   attribute vec4        position;       \
   varying mediump vec2  pos;            \
   uniform vec4          offset;         \
                                         \
   void main()                           \
   {                                     \
      gl_Position = position + offset;   \
      pos = position.xy;                 \
   }                                     \
";
 
 
const char fragment_src [] =
"                                                      \
   varying mediump vec2    pos;                        \
   uniform mediump float   phase;                      \
                                                       \
   void  main()                                        \
   {                                                   \
      gl_FragColor  =  vec4( 1., 0.9, 0.7, 0.5 ) *     \
        cos( 30.*sqrt(pos.x*pos.x + 1.5*pos.y*pos.y)   \
             + atan(pos.y,pos.x) - phase );            \
   }                                                   \
";
//  some more formulas to play with...
//      cos( 20.*(pos.x*pos.x + pos.y*pos.y) - phase );
//      cos( 20.*sqrt(pos.x*pos.x + pos.y*pos.y) + atan(pos.y,pos.x) - phase );
//      cos( 30.*sqrt(pos.x*pos.x + 1.5*pos.y*pos.y - 1.8*pos.x*pos.y*pos.y)
//            + atan(pos.y,pos.x) - phase );
 
 
void
print_shader_info_log (
   GLuint  shader      // handle to the shader
)
{
   GLint  length;
 
   glGetShaderiv ( shader , GL_INFO_LOG_LENGTH , &length );
 
   if ( length ) {
      char* buffer  =  new char [ length ];
      glGetShaderInfoLog ( shader , length , NULL , buffer );
      //cout << "shader info: " <<  buffer << endl;
      delete [] buffer;
 
      GLint success;
      glGetShaderiv( shader, GL_COMPILE_STATUS, &success );
      if ( success != GL_TRUE )   exit ( 1 );
   }
}
 
 
GLuint
load_shader (
   const char  *shader_source,
   GLenum       type
)
{
   GLuint  shader = glCreateShader( type );
 
   glShaderSource  ( shader , 1 , &shader_source , NULL );
   glCompileShader ( shader );
 
   print_shader_info_log ( shader );
 
   return shader;
}

GLfloat
   norm_x    =  0.0,
   norm_y    =  0.0,
   offset_x  =  0.0,
   offset_y  =  0.0,
   p1_pos_x  =  0.0,
   p1_pos_y  =  0.0;
 
GLint
   phase_loc,
   offset_loc,
   position_loc;
 
 
bool        update_pos = false;
 
const float vertexArray[] = {
   0.0,  0.5,  0.0,
  -0.5,  0.0,  0.0,
   0.0, -0.5,  0.0,
   0.5,  0.0,  0.0,
   0.0,  0.5,  0.0 
};
 
 
void  render()
{
   static float  phase = 0;

   glClear ( GL_COLOR_BUFFER_BIT );
 
   glUniform1f ( phase_loc , phase );  // write the value of phase to the shaders phase
   phase  =  fmodf ( phase + 0.5f , 2.f * 3.141f );    // and update the local variable
 
   if ( update_pos ) {  // if the position of the texture has changed due to user action
      GLfloat old_offset_x  =  offset_x;
      GLfloat old_offset_y  =  offset_y;
 
      offset_x  =  norm_x - p1_pos_x;
      offset_y  =  norm_y - p1_pos_y;
 
      p1_pos_x  =  norm_x;
      p1_pos_y  =  norm_y;
 
      offset_x  +=  old_offset_x;
      offset_y  +=  old_offset_y;
 
      update_pos = false;
   }
 
   glUniform4f ( offset_loc  ,  offset_x , offset_y , 0.0 , 0.0 );
 
   glVertexAttribPointer ( position_loc, 3, GL_FLOAT, false, 0, vertexArray );
   glEnableVertexAttribArray ( position_loc );
   glDrawArrays ( GL_TRIANGLE_STRIP, 0, 5 );
 
   RPI_SwapBuffers();
}
 

int main(void)
{
   uint32_t display_width;
   uint32_t display_height;
   uint32_t bPause = 0,bFullscreen = 0;
   
RPI_OpenWindow("my trest program 5", 640, 480, bFullscreen, PointerMotionMask | KeyPressMask );

   GLuint vertexShader   = load_shader ( vertex_src , GL_VERTEX_SHADER  );     // load vertex shader
   GLuint fragmentShader = load_shader ( fragment_src , GL_FRAGMENT_SHADER );  // load fragment shader
 
   GLuint shaderProgram  = glCreateProgram ();                 // create program object
   glAttachShader ( shaderProgram, vertexShader );             // and attach both...
   glAttachShader ( shaderProgram, fragmentShader );           // ... shaders to it
 
	glLinkProgram ( shaderProgram );    // link the program
	glUseProgram  ( shaderProgram );    // and select it for usage
 
	//// now get the locations (kind of handle) of the shaders variables
	position_loc  = glGetAttribLocation  ( shaderProgram , "position" );
	phase_loc     = glGetUniformLocation ( shaderProgram , "phase"    );
   	offset_loc    = glGetUniformLocation ( shaderProgram , "offset"   );
	
	if ( position_loc < 0  ||  phase_loc < 0  ||  offset_loc < 0 ) {
		cerr << "Unable to get uniform location" << endl;
		return 1;
   	}
 
 	RPI_GetWindowSize(&display_width, &display_height);
 
   	float
      	window_width  =(float)(display_width),
      	window_height = (float)(display_height);
 
	//// this is needed for time measuring  -->  frames per second
	struct  timezone  tz;
	timeval  t1, t2;
	gettimeofday ( &t1 , &tz );
	int  num_frames = 0;

	glViewport ( 0 , 0 , display_width , display_height );
	glClearColor ( 0.08 , 0.06 , 0.07 , 1.);    // background color

	bool quit = false;
 
	GLfloat window_y,window_x;
 
	while ( !quit ) {    // the main loop
 		XEvent  xev;
		while (RPI_NextXEvent(&xev) )
		{   // check for events from the x-server
	
			//cout << "Event " << xev.type << endl;
        
        		switch (xev.type)
			{	
				case ConfigureNotify:
				case ResizeRequest:
					RPI_GetWindowSize(&display_width, &display_height);
 				     	window_width  =(float)(display_width),
      					window_height = (float)(display_height);
      					cout << "window size " << display_width << "x" << display_height << endl;
					break;
				case MotionNotify:   // if mouse has moved
            				//cout << "move to:" << xev.xmotion.x << "," << xev.xmotion.y << endl;
            				window_y  =  (window_height - xev.xmotion.y) - window_height / 2.0;
            				norm_y            =  window_y / (window_height / 2.0);
            				window_x  =  xev.xmotion.x - window_width / 2.0;
            				norm_x            =  window_x / (window_width / 2.0);
            				update_pos = true;
					break;         
				case ButtonPress:
					//cout << "Button Press: " << xev.xbutton.state << ", " << xev.xbutton.button << endl;
					break;
				case KeyPress:
					cout << "Key Press: " << xev.xkey.keycode << endl;
		
					if (9 == xev.xkey.keycode) quit = true; // ESC
					if (33 == xev.xkey.keycode) // p
					{
						bPause = !bPause;
						RPI_Pause(bPause);
					}
					if (41 == xev.xkey.keycode)
					{
						bFullscreen = !bFullscreen;
						RPI_FullScreen(bFullscreen);
					}
				default: 
					break;
			}
      		}
		render();   // now we finally put something on the screen
 /*
      		if ( ++num_frames % 100 == 0 ) {
			gettimeofday( &t2, &tz );
			float dt  =  t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6;
			cout << "fps: " << num_frames / dt << endl;
			num_frames = 0;
			t1 = t2;
		}*/
	}   
   	RPI_CloseWindow();
   	return 0;
}

