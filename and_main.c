#include "pch.h"

#include <jni.h>
#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

struct android_app* __state;

//static int __initialized;
//static int __suspended;
static EGLDisplay __eglDisplay = EGL_NO_DISPLAY;
static EGLContext __eglContext = EGL_NO_CONTEXT;
static EGLSurface __eglSurface = EGL_NO_SURFACE;
static EGLConfig __eglConfig = 0;
//static int __width;
//static int __height;
int main(int argc, char* argv[]);
void Display(void);
void Idle(void);

void seInit() {
	srand(time(0));
	print("seInit");
	__eglDisplay = eglGetDisplay((EGLNativeDisplayType) 0);
	if((__eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
		print("eglGetDisplay() returned error %d", eglGetError());
		exit(-1);
	}

	// Step 2 - Initialize EGL.
	EGLint maj = 0, min = 0;
	if(!eglInitialize(__eglDisplay, &maj, &min)) {
		print("eglInitialize() returned error %d", eglGetError());
		exit(-1);
	}
	print("eglInit: %d,%d", maj, min);

	EGLint eglConfigAttrs[] = {
		EGL_SAMPLE_BUFFERS,     0,
		EGL_SAMPLES,            0,
		EGL_DEPTH_SIZE,         24,
		EGL_RED_SIZE,           8,
		EGL_GREEN_SIZE,         8,
		EGL_BLUE_SIZE,          8,
		EGL_ALPHA_SIZE,         8,
		EGL_STENCIL_SIZE,       8,
		EGL_SURFACE_TYPE,       EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT,//EGL_OPENGL_ES_BIT
		EGL_NONE
	};

	EGLint eglConfigCount;
	const EGLint eglContextAttrs[] = {
		EGL_CONTEXT_CLIENT_VERSION,    2.0, //1.1
		EGL_NONE
	};

	const EGLint eglSurfaceAttrs[] = {
		EGL_RENDER_BUFFER,    EGL_BACK_BUFFER,
		EGL_NONE
	};

	//int validConfig = false;
	EGLint depthSizes[] = { 24, 16 };
	unsigned int i;
	for(i = 0; i < 2; ++i) {
		eglConfigAttrs[5] = depthSizes[i];

		if(eglChooseConfig(__eglDisplay, eglConfigAttrs, &__eglConfig, 1, &eglConfigCount) == EGL_TRUE && eglConfigCount > 0) {
			print("validConfig = true");
			break;
		}
	}

	__eglContext = eglCreateContext(__eglDisplay, __eglConfig, EGL_NO_CONTEXT, eglContextAttrs);
	print("eglCreateContext: %p", __eglContext);

	EGLint format;
	eglGetConfigAttrib(__eglDisplay, __eglConfig, EGL_NATIVE_VISUAL_ID, &format);
	print("format: %p", format);
	print("__state: %p", __state);
	print("->window: %p", __state->window);
	ANativeWindow_setBuffersGeometry(__state->window, 0, 0, format);

	__eglSurface = eglCreateWindowSurface(__eglDisplay, __eglConfig, __state->window, eglSurfaceAttrs);
	if(__eglSurface == EGL_NO_SURFACE) {
		//checkErrorEGL("eglCreateWindowSurface");
		//goto error;
		print("__eglSurface == EGL_NO_SURFACE");
	}

	if(eglMakeCurrent(__eglDisplay, __eglSurface, __eglSurface, __eglContext) != EGL_TRUE) {
		//checkErrorEGL("eglMakeCurrent");
		//goto error;
		print("eglMakeCurrent");
	}

	EGLint interval = 0;
	eglSwapInterval(__eglDisplay, interval);

	int w, h;
	eglQuerySurface(__eglDisplay, __eglSurface, EGL_WIDTH, &w);
	eglQuerySurface(__eglDisplay, __eglSurface, EGL_HEIGHT, &h);

	print("App.size: %dx%d", w, h);

	glClearColor(0.0, 0.5, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	eglSwapBuffers(__eglDisplay, __eglSurface);

	main(0, 0);
}

int __initialized;

static void term_display() {
	if(__eglDisplay != EGL_NO_DISPLAY) {
		eglMakeCurrent(__eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroyContext(__eglDisplay, __eglContext);
		eglDestroySurface(__eglDisplay, __eglSurface);
		eglTerminate(__eglDisplay);
	}

	__eglDisplay = EGL_NO_DISPLAY;
	__eglContext = EGL_NO_CONTEXT;
	__eglSurface = EGL_NO_SURFACE;
}
// Process the next main command.
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	switch(cmd) {
	case APP_CMD_INIT_WINDOW:
		if(app->window != NULL) {
			print("APP_CMD_INIT_WINDOW\n");
			seInit();
			__initialized = 1;
		}
		break;
	case APP_CMD_TERM_WINDOW:
		__initialized = 1;
		term_display();
		break;
	}
}

void android_main(struct android_app* state) {
	int __suspended = 0;
	print("android_main");
	// Android specific : Dummy function that needs to be called to
	// ensure that the native activity works properly behind the scenes.
	app_dummy();
	__initialized = 0;
	__state = state;
	__state->onAppCmd = engine_handle_cmd;

	//main(0,0);
	while(1) {
		// Read all pending events.
		int ident;
		int events;
		struct android_poll_source* source;
		//while ((ident=ALooper_pollAll(!__suspended ? 0 : -1, NULL, &events, (void**)&source)) >= 0) {
		while((ident = ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0) {
			// Process this event.
			if(source != NULL)
				source->process(__state, source);

			if(__state->destroyRequested != 0) {
				term_display();
				return;
			}
		}

		if(__initialized && !__suspended) {
			Idle();
			Display();
			//print("eglSwapBuffers %p %p",__eglDisplay, __eglSurface);
			int rc = eglSwapBuffers(__eglDisplay, __eglSurface);
			if(rc != EGL_TRUE) {
				print("eglSwapBuffers rc != EGL_TRUE");
				EGLint error = eglGetError();
				if(error == EGL_BAD_NATIVE_WINDOW) {
					if(__state->window != NULL) {
						//destroyEGLSurface();
						//initEGL();
					}
					__initialized = 1;
				} else {
					print("eglSwapBuffers");
					break;
				}
			}
		}
	}


	// Android specific : the process needs to exit to trigger
	// cleanup of global and static resources (such as the game).
	exit(0);
}
