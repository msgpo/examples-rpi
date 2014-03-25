/* Created by exoticorn ( http://talk.maemo.org/showthread.php?t=37356 )
 * edited and commented by André Bergner [endboss]
 *
 * libraries needed:   libx11-dev, libgles2-dev
 *
 * compile with:   g++  -lX11 -lEGL -lGLESv2  egl-example.cpp
 */
 
#include  <iostream>
#include  <cstdlib>
#include  <cstring>
#include  <stdio.h>
using namespace std;
 
#include  <cmath>
#include  <sys/time.h>
 
#include  <X11/Xlib.h>
#include  <X11/Xatom.h>
#include  <X11/Xutil.h>
 
#include  <GLES2/gl2.h>
#include  <EGL/egl.h>
#include <bcm_host.h>

 
 
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
      cout << "shader info: " <<  buffer << endl;
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
 
 
Display    *x_display;
Window root;
    Window win;

EGLDisplay  egl_display;
EGLContext  egl_context;
EGLSurface  egl_surface;
 
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
 
   eglSwapBuffers ( egl_display, egl_surface );  // get the rendered buffer to the screen
}
 
 
////////////////////////////////////////////////////////////////////////////////////////////

int Open_Xwindow(int w, int h)
{

    XSetWindowAttributes swa;
    XSetWindowAttributes  xattr;
    Atom wm_state;
    XWMHints hints;
    XEvent xev;


	x_display = XOpenDisplay(NULL);
    if ( x_display == NULL )
    {
        return 1;
    }

	root = DefaultRootWindow(x_display);

    swa.event_mask  =  EnterWindowMask | LeaveWindowMask | StructureNotifyMask | ResizeRedirectMask | VisibilityChangeMask | ExposureMask | PointerMotionMask | KeyPressMask;	
    //swa.event_mask = 0x01FFFFFF;// All events!

	win = XCreateSimpleWindow(x_display, root, 10, 10, w, h, 0, 0, 0);

	XSelectInput (x_display, win, swa.event_mask);

    // make the window visible on the screen
    XMapWindow (x_display, win);
    XStoreName (x_display, win, "my test program");

    // get identifiers for the provided atom name strings
    wm_state = XInternAtom (x_display, "_NET_WM_STATE", false);

    memset ( &xev, 0, sizeof(xev) );
    xev.type                 = ClientMessage;
    xev.xclient.window       = win;
    xev.xclient.message_type = wm_state;
    xev.xclient.format       = 32;
    xev.xclient.data.l[0]    = 1;
    xev.xclient.data.l[1]    = false;
    XSendEvent (
       x_display,
       DefaultRootWindow ( x_display ),
       false,
       SubstructureNotifyMask,
       &xev );
return 0;
}
  
int  main()
{
   int32_t success = 0;

   static EGL_DISPMANX_WINDOW_T nativewindow;

   DISPMANX_ELEMENT_HANDLE_T dispman_element;
   DISPMANX_DISPLAY_HANDLE_T dispman_display;
   DISPMANX_UPDATE_HANDLE_T dispman_update;
   VC_RECT_T dst_rect, dummy_rect;
   VC_RECT_T src_rect;
   
    VC_DISPMANX_ALPHA_T dispman_alpha = {DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS,255,0};

	if (Open_Xwindow(640,480)) 
	{
		cerr << "Could not get Xwindow" << endl;
		return 1;
	}
   uint32_t display_width;
   uint32_t display_height;
	uint32_t temp;
	int32_t display_left = 0, display_top = 0;

	bcm_host_init();

   // create an EGL window surface, passing context width/height
   success = graphics_get_display_size(0 /* LCD */, &display_width, &display_height);
   if ( success < 0 )
   {
		cout << "graphics_get_display_size failed" << endl;
      return 1;
   }
   cout << "Screen resolution is:" << display_width << "x" << display_height << endl;
   cout << "Window ID is:" << win << endl;


/* Open display */
	Window screen = XDefaultScreen(x_display);
	display_width = DisplayWidth(x_display,screen);
	display_height = DisplayHeight(x_display,screen);
	cout << "Window at " << display_left << "," << display_top << " " << display_width << "," << display_height << endl;
	
	
	XGetGeometry(x_display, win, (Window*)&temp, &display_left, &display_top, &display_width, &display_height, &temp, &temp);

	cout << "Window at " << display_left << "," << display_top << " " << display_width << "," << display_height << endl;

   dst_rect.x = display_left;
   dst_rect.y = display_top;
   dst_rect.width = display_width;
   dst_rect.height = display_height;
   
   dummy_rect.x = 0;
   dummy_rect.y = 0;
   dummy_rect.width = 1;
   dummy_rect.height = 1;
      
   src_rect.x = display_left << 16;
   src_rect.y = display_top << 16;
   src_rect.width = display_width << 16;
   src_rect.height = display_height << 16;   

   dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
   dispman_update = vc_dispmanx_update_start( 0 /* Priority*/);
	
   dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
      0/*layer*/, &dst_rect, 0/*src*/,
      &src_rect, DISPMANX_PROTECTION_NONE, &dispman_alpha, 0/*clamp*/, (DISPMANX_TRANSFORM_T)0/*transform*/);
   nativewindow.element = dispman_element;
   nativewindow.width = display_width;
   nativewindow.height = display_height;

   vc_dispmanx_update_submit_sync( dispman_update );
   
   //esContext->hWnd = &nativewindow;
 
 
   ///////  the egl part  //////////////////////////////////////////////////////////////////
   //  egl provides an interface to connect the graphics related functionality of openGL ES
   //  with the windowing interface and functionality of the native operation system (X11
   //  in our case.
 
   egl_display  =  eglGetDisplay(EGL_DEFAULT_DISPLAY);
   if ( egl_display == EGL_NO_DISPLAY ) {
      cerr << "Got no EGL display." << endl;
      return 1;
   }
    EGLint majorVersion;
   EGLint minorVersion;
   
	if ( !eglInitialize( egl_display,  &majorVersion, &minorVersion ) ) {
      cerr << "Unable to initialize EGL" << endl;
      return 1;
   }
 	cout << "EGL " << majorVersion << "." << minorVersion << " Initialized" << endl;
   
	EGLint attr[] = {       // some attributes to set up our egl-interface
      EGL_BUFFER_SIZE, 16,
      EGL_RENDERABLE_TYPE,
      EGL_OPENGL_ES2_BIT,
      EGL_NONE
   };
 
   EGLConfig  ecfg;
   EGLint     num_config;
   if ( !eglChooseConfig( egl_display, attr, &ecfg, 1, &num_config ) ) {
      cerr << "Failed to choose config (eglError: " << eglGetError() << ")" << endl;
      return 1;
   }
 
   if ( num_config != 1 ) {
      cerr << "Didn't get exactly one config, but " << num_config << endl;
      return 1;
   }
 
   egl_surface = eglCreateWindowSurface ( egl_display, ecfg, &nativewindow, NULL );
   if ( egl_surface == EGL_NO_SURFACE ) {
      cerr << "Unable to create EGL surface (eglError: " << eglGetError() << ")" << endl;
      return 1;
   }
 
   //// egl-contexts collect all state descriptions needed required for operation
   EGLint ctxattr[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
   };
   egl_context = eglCreateContext ( egl_display, ecfg, EGL_NO_CONTEXT, ctxattr );
   if ( egl_context == EGL_NO_CONTEXT ) {
      cerr << "Unable to create EGL context (eglError: " << eglGetError() << ")" << endl;
      return 1;
   }
 
   //// associate the egl-context with the egl-surface
   eglMakeCurrent( egl_display, egl_surface, egl_surface, egl_context );
 
 
   ///////  the openGL part  ///////////////////////////////////////////////////////////////
 
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
 
 
   const float
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
 
      while ( XPending ( x_display ) ) {   // check for events from the x-server
 
         XEvent  xev;
         XNextEvent( x_display, &xev );
 	
	printf("Event %d\n",xev.type);
        
        switch (xev.type)
	{	
	case MotionNotify:   // if mouse has moved
            	cout << "move to:" << xev.xmotion.x << "," << xev.xmotion.y << endl;
            	window_y  =  (window_height - xev.xmotion.y) - window_height / 2.0;
            	norm_y            =  window_y / (window_height / 2.0);
            	window_x  =  xev.xmotion.x - window_width / 2.0;
            	norm_x            =  window_x / (window_width / 2.0);
            	update_pos = true;
		break;         
	case LeaveNotify:
		dispman_update = vc_dispmanx_update_start( 0 /* Priority*/);
	
		vc_dispmanx_element_change_attributes( dispman_update, dispman_element, 0, 
			0, 255, &dummy_rect, &src_rect, DISPMANX_PROTECTION_NONE,
			(DISPMANX_TRANSFORM_T)0 );
   
   		vc_dispmanx_update_submit_sync( dispman_update );
   		break;
	case EnterNotify:
		dispman_update = vc_dispmanx_update_start( 0 /* Priority*/);
	
		vc_dispmanx_element_change_attributes( dispman_update, dispman_element, 0, 
			0, 255, &dst_rect, &src_rect, DISPMANX_PROTECTION_NONE,
			(DISPMANX_TRANSFORM_T)0 );
   
   		vc_dispmanx_update_submit_sync( dispman_update );
   		break;
	case ResizeRequest:
	case ConfigureNotify:
		cout << " Screen moved " << endl;
		display_left = xev.xconfigure.x;
		display_top = xev.xconfigure.y;
		display_width = xev.xconfigure.width;
      		display_height = xev.xconfigure.height;
		
		cout << "Window at " << display_left << "," << display_top << " " << 
			display_width << "," << display_height << endl;
		
		dst_rect.x = display_left;
		dst_rect.y = display_top;
		dst_rect.width = display_width;
		dst_rect.height = display_height;
			  
		//src_rect.x = display_left << 16;
		//src_rect.y = display_top << 16;
		//src_rect.width = display_width << 16;
		//src_rect.height = display_height << 16;  
		glViewport ( 0 , 0 , display_width , display_height );
		dispman_update = vc_dispmanx_update_start( 0 /* Priority*/);
	
		vc_dispmanx_element_change_attributes( dispman_update, dispman_element, 0, 
			0, 255, &dst_rect, &src_rect, DISPMANX_PROTECTION_NONE,
			(DISPMANX_TRANSFORM_T)0 );
   
   		vc_dispmanx_update_submit_sync( dispman_update );
		
		break; 
	case ButtonPress:
		cout << "Button Press: " << xev.xbutton.state << ", " << xev.xbutton.button << endl;
		break;
	case KeyPress:
		quit = true;
	default: 
		;
		break;
	}
      }
      render();   // now we finally put something on the screen
 
      if ( ++num_frames % 100 == 0 ) {
         gettimeofday( &t2, &tz );
         float dt  =  t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6;
         cout << "fps: " << num_frames / dt << endl;
         num_frames = 0;
         t1 = t2;
      }
   }
 
 
   ////  cleaning up...
   eglDestroyContext ( egl_display, egl_context );
   eglDestroySurface ( egl_display, egl_surface );
   eglTerminate      ( egl_display );
   XCloseDisplay     ( x_display );
   XDestroyWindow    ( x_display, root );
   
 	cout << "program complete" << endl;
   return 0;
}
