#define GL_GLEXT_PROTOTYPES why

#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<stdint.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include "sys.h"

#include "shader.h"
const char* vshader = "#version 430\nvec2 y=vec2(1.,-1);\nvec4 x[4]={y.yyxx,y.xyxx,y.yxxx,y.xxxx};void main(){gl_Position=x[gl_VertexID];}";

#define CANVAS_WIDTH 1920
#define CANVAS_HEIGHT 1080
#define SCANLINE_SIZE 20
#define CHAR_BUFF_SIZE 256

//#define DEBUG
//#define TIME_RENDER
#define SCISSORS

static void quit_asm() {
	SYS_exit_group(0);
	__builtin_unreachable();
}

GLuint vao;
GLuint p;

bool rendered = false;
bool flipped = false;
bool compiled = false;

#ifdef TIME_RENDER
GTimer* gtimer;
#endif

static gboolean check_escape(GtkWidget *widget, GdkEventKey *event)
{
	(void)widget;
	if (event->keyval == GDK_KEY_Escape) {
		quit_asm();
	}

	return FALSE;
}

static void compile_shader()
{
	// compile shader
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(f, 1, &shader_frag, NULL);
	glCompileShader(f);

#ifdef DEBUG
	GLint isCompiled = 0;
	glGetShaderiv(f, GL_COMPILE_STATUS, &isCompiled);
	if(isCompiled == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(f, GL_INFO_LOG_LENGTH, &maxLength);

		char* error = malloc(maxLength);
		glGetShaderInfoLog(f, maxLength, &maxLength, error);
		printf("%s\n", error);

		quit_asm();
	}
#endif

	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v, 1, &vshader, NULL);
	glCompileShader(v);

#ifdef DEBUG
	GLint isCompiled2 = 0;
	glGetShaderiv(v, GL_COMPILE_STATUS, &isCompiled2);
	if(isCompiled2 == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(v, GL_INFO_LOG_LENGTH, &maxLength);

		char* error = malloc(maxLength);
		glGetShaderInfoLog(v, maxLength, &maxLength, error);
		printf("%s\n", error);

		quit_asm();
	}
#endif

	// link shader
	p = glCreateProgram();
	glAttachShader(p,v);
	glAttachShader(p,f);
	glLinkProgram(p);

#ifdef DEBUG
	GLint isLinked = 0;
	glGetProgramiv(p, GL_LINK_STATUS, (int *)&isLinked);
	if (isLinked == GL_FALSE) {
		GLint maxLength = 0;
		glGetProgramiv(p, GL_INFO_LOG_LENGTH, &maxLength);

		char* error = malloc(maxLength);
		glGetProgramInfoLog(p, maxLength, &maxLength,error);
		printf("%s\n", error);

		quit_asm();
	}
#endif

	glGenVertexArrays(1, &vao);
}

static gboolean
on_render (GtkGLArea *glarea, GdkGLContext *context)
{
	(void)context;
	if (rendered || gtk_widget_get_allocated_width((GtkWidget*)glarea) < CANVAS_WIDTH) return TRUE;
	if (!flipped) { gtk_gl_area_queue_render(glarea); flipped = true; return TRUE; }
	if (!compiled) { compile_shader(); compiled = true; }

	rendered = true;
	glUseProgram(p);
	glBindVertexArray(vao);
	glVertexAttrib1f(0, 0);

#ifdef SCISSORS
  glEnable(GL_SCISSOR_TEST);
  for (int i = 0; i < CANVAS_HEIGHT; i += SCANLINE_SIZE) {
	  glScissor(0,i,CANVAS_WIDTH,SCANLINE_SIZE);
#endif
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#ifdef SCISSORS
		glFinish();
		while (gtk_events_pending()) gtk_main_iteration();
  }
#else
	glFinish();
#endif

#ifdef TIME_RENDER
  printf("render time: %f\n", g_timer_elapsed(gtimer, NULL));
#endif
  return TRUE;
}

void _start() {
	asm volatile("sub $8, %rsp\n");
#ifdef TIME_RENDER
	gtimer = g_timer_new();
#endif

	typedef void (*voidWithOneParam)(int*);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
	voidWithOneParam gtk_init_one_param = (voidWithOneParam)gtk_init;
#pragma GCC diagnostic pop
	(*gtk_init_one_param)(NULL);

	GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *glarea = gtk_gl_area_new();
	gtk_container_add(GTK_CONTAINER(win), glarea);

	g_signal_connect(win, "destroy", quit_asm, NULL);
	g_signal_connect(win, "key_press_event", G_CALLBACK(check_escape), NULL);
	// g_signal_connect(glarea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glarea, "render", G_CALLBACK(on_render), NULL);

	gtk_widget_show_all (win);

	gtk_window_fullscreen((GtkWindow*)win);
	GdkWindow* window = gtk_widget_get_window(win);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	GdkCursor* Cursor = gdk_cursor_new(GDK_BLANK_CURSOR);
#pragma GCC diagnostic pop
	gdk_window_set_cursor(window, Cursor);

	gtk_main();

	quit_asm();
}