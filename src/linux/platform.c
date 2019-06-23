#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xatom.h>
#include <time.h>
#include <X11/XKBlib.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h> 
#include <errno.h>
#include <dlfcn.h>

#define GET_ATOM(X) window.X = XInternAtom(window.display, #X, False)

struct t_platform_window
{
	Display *display;
	Window parent;
	XVisualInfo *visual_info;
	Colormap cmap;
	Window window;
	GLXContext gl_context;
	XWindowAttributes window_attributes;
	XEvent event;
	
	Atom xdnd_req;
	Atom xdnd_source;
	Atom XdndEnter;
	Atom XdndPosition;
	Atom XdndStatus;
	Atom XdndTypeList;
	Atom XdndActionCopy;
	Atom XdndDrop;
	Atom XdndFinished;
	Atom XdndSelection;
	Atom XdndLeave;
	Atom quit;
	Atom PRIMARY;
	
	// shared window properties
	s32 width;
	s32 height;
	u8 is_open;
	u8 has_focus;
	struct drag_drop_info drag_drop_info;
};

// --- libX11.so
// XLookupKeysym

// --- libXrandr.so
// XRRGetScreenResources
// XRRGetCrtcInfo
// XRRFreeCrtcInfo
// XRRFreeScreenResources

//typedef int t_XResizeWindow(Display *display, Window window, unsigned int w, unsigned int h);
//static t_XResizeWindow *XResizeWindow_;
//#define XResizeWindow XResizeWindow_

int main(int argc, char **argv)
{
	return main_loop();
}

u8 get_active_directory(char *buffer)
{
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		strncpy(buffer, cwd, PATH_MAX);
	} else {
		return false;
	}
	return true;
}

u8 set_active_directory(char *path)
{
	return !chdir(path);
}

u8 platform_write_file_content(char *path, const char *mode, char *buffer, s32 len)
{
	u8 result = false;
	
	FILE *file = fopen(path, mode);
	
	if (!file)
	{
		goto done_failure;
	}
	else
	{
		fprintf(file, "%s", buffer);
	}
	
	//done:
	fclose(file);
	done_failure:
	return result;
}

void platform_window_set_title(platform_window *window, char *name)
{
	XStoreName(window->display, window->window, name);
}

file_content platform_read_file_content(char *path, const char *mode)
{
	file_content result;
	result.content = 0;
	result.content_length = 0;
	result.file_error = 0;
	
	FILE *file = fopen(path, mode);
	if (!file) 
	{
		// TODO(Aldrik): maybe handle more of these so we can give users more info about what happened.
		// http://man7.org/linux/man-pages/man3/errno.3.html
		if (errno == EMFILE)
			result.file_error = FILE_ERROR_TOO_MANY_OPEN_FILES_PROCESS;
		else if (errno == ENFILE)
			result.file_error = FILE_ERROR_TOO_MANY_OPEN_FILES_SYSTEM;
		else if (errno == EACCES)
			result.file_error = FILE_ERROR_NO_ACCESS;
		else if (errno == EPERM)
			result.file_error = FILE_ERROR_NO_ACCESS;
		else if (errno == ENOENT)
			result.file_error = FILE_ERROR_NOT_FOUND;
		else if (errno == ECONNABORTED)
			result.file_error = FILE_ERROR_CONNECTION_ABORTED;
		else if (errno == ECONNREFUSED)
			result.file_error = FILE_ERROR_CONNECTION_REFUSED;
		else if (errno == ENETDOWN)
			result.file_error = FILE_ERROR_NETWORK_DOWN;
		else if (errno == EREMOTEIO)
			result.file_error = FILE_ERROR_REMOTE_IO_ERROR;
		else if (errno == ESTALE)
			result.file_error = FILE_ERROR_STALE;
		else
			printf("ERROR: %d\n", errno);
		
		goto done_failure;
	}
	
	fseek(file, 0 , SEEK_END);
	int length = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	// if file i empty alloc 1 byte
	s32 length_to_alloc = length+1;
	
	result.content = mem_alloc(length_to_alloc);
	if (!result.content) goto done;
	
	fread(result.content, 1, length, file);
	result.content_length = length;
	
	((char*)result.content)[length] = 0;
	
	done:
	fclose(file);
	done_failure:
	return result;
}

inline void platform_destroy_file_content(file_content *content)
{
	assert(content);
	mem_free(content->content);
}

// Translate an X11 key code to a GLFW key code.
//
static s32 translate_keycode(platform_window *window, s32 scancode)
{
    s32 keySym;
	
    // Valid key code range is  [8,255], according to the Xlib manual
	
    if (1)
    {
        // Try secondary keysym, for numeric keypad keys
        // Note: This way we always force "NumLock = ON", which is intentional
        // since the returned key code should correspond to a physical
        // location.
        keySym = XkbKeycodeToKeysym(window->display, scancode, 0, 1);
        switch (keySym)
        {
            case XK_KP_0:           return KEY_KP_0;
            case XK_KP_1:           return KEY_KP_1;
            case XK_KP_2:           return KEY_KP_2;
            case XK_KP_3:           return KEY_KP_3;
            case XK_KP_4:           return KEY_KP_4;
            case XK_KP_5:           return KEY_KP_5;
            case XK_KP_6:           return KEY_KP_6;
            case XK_KP_7:           return KEY_KP_7;
            case XK_KP_8:           return KEY_KP_8;
            case XK_KP_9:           return KEY_KP_9;
            case XK_KP_Separator:
            case XK_KP_Decimal:     return KEY_KP_DECIMAL;
            case XK_KP_Equal:       return KEY_KP_EQUAL;
            case XK_KP_Enter:       return KEY_KP_ENTER;
            default:                break;
        }
		
        // Now try primary keysym for function keys (non-printable keys)
        // These should not depend on the current keyboard layout
        keySym = XkbKeycodeToKeysym(window->display, scancode, 0, 0);
    }
	
    switch (keySym)
    {
        case XK_Escape:         return KEY_ESCAPE;
        case XK_Tab:            return KEY_TAB;
        case XK_Shift_L:        return KEY_LEFT_SHIFT;
        case XK_Shift_R:        return KEY_RIGHT_SHIFT;
        case XK_Control_L:      return KEY_LEFT_CONTROL;
        case XK_Control_R:      return KEY_RIGHT_CONTROL;
        case XK_Meta_L:
        case XK_Alt_L:          return KEY_LEFT_ALT;
        case XK_Mode_switch: // Mapped to Alt_R on many keyboards
        case XK_ISO_Level3_Shift: // AltGr on at least some machines
        case XK_Meta_R:
        case XK_Alt_R:          return KEY_RIGHT_ALT;
        case XK_Super_L:        return KEY_LEFT_SUPER;
        case XK_Super_R:        return KEY_RIGHT_SUPER;
        case XK_Menu:           return KEY_MENU;
        case XK_Num_Lock:       return KEY_NUM_LOCK;
        case XK_Caps_Lock:      return KEY_CAPS_LOCK;
        case XK_Print:          return KEY_PRINT_SCREEN;
        case XK_Scroll_Lock:    return KEY_SCROLL_LOCK;
        case XK_Pause:          return KEY_PAUSE;
        case XK_Delete:         return KEY_DELETE;
        case XK_BackSpace:      return KEY_BACKSPACE;
        case XK_Return:         return KEY_ENTER;
        case XK_Home:           return KEY_HOME;
        case XK_End:            return KEY_END;
        case XK_Page_Up:        return KEY_PAGE_UP;
        case XK_Page_Down:      return KEY_PAGE_DOWN;
        case XK_Insert:         return KEY_INSERT;
        case XK_Left:           return KEY_LEFT;
        case XK_Right:          return KEY_RIGHT;
        case XK_Down:           return KEY_DOWN;
        case XK_Up:             return KEY_UP;
        case XK_F1:             return KEY_F1;
        case XK_F2:             return KEY_F2;
        case XK_F3:             return KEY_F3;
        case XK_F4:             return KEY_F4;
        case XK_F5:             return KEY_F5;
        case XK_F6:             return KEY_F6;
        case XK_F7:             return KEY_F7;
        case XK_F8:             return KEY_F8;
        case XK_F9:             return KEY_F9;
        case XK_F10:            return KEY_F10;
        case XK_F11:            return KEY_F11;
        case XK_F12:            return KEY_F12;
        case XK_F13:            return KEY_F13;
        case XK_F14:            return KEY_F14;
        case XK_F15:            return KEY_F15;
        case XK_F16:            return KEY_F16;
        case XK_F17:            return KEY_F17;
        case XK_F18:            return KEY_F18;
        case XK_F19:            return KEY_F19;
        case XK_F20:            return KEY_F20;
        case XK_F21:            return KEY_F21;
        case XK_F22:            return KEY_F22;
        case XK_F23:            return KEY_F23;
        case XK_F24:            return KEY_F24;
        case XK_F25:            return KEY_F25;
		
        // Numeric keypad
        case XK_KP_Divide:      return KEY_KP_DIVIDE;
        case XK_KP_Multiply:    return KEY_KP_MULTIPLY;
        case XK_KP_Subtract:    return KEY_KP_SUBTRACT;
        case XK_KP_Add:         return KEY_KP_ADD;
		
        // These should have been detected in secondary keysym test above!
        case XK_KP_Insert:      return KEY_KP_0;
        case XK_KP_End:         return KEY_KP_1;
        case XK_KP_Down:        return KEY_KP_2;
        case XK_KP_Page_Down:   return KEY_KP_3;
        case XK_KP_Left:        return KEY_KP_4;
        case XK_KP_Right:       return KEY_KP_6;
        case XK_KP_Home:        return KEY_KP_7;
        case XK_KP_Up:          return KEY_KP_8;
        case XK_KP_Page_Up:     return KEY_KP_9;
        case XK_KP_Delete:      return KEY_KP_DECIMAL;
        case XK_KP_Equal:       return KEY_KP_EQUAL;
        case XK_KP_Enter:       return KEY_KP_ENTER;
		
        // Last resort: Check for printable keys (should not happen if the XKB
        // extension is available). This will give a layout dependent mapping
        // (which is wrong, and we may miss some keys, especially on non-US
        // keyboards), but it's better than nothing...
        case XK_a:              return KEY_A;
        case XK_b:              return KEY_B;
        case XK_c:              return KEY_C;
        case XK_d:              return KEY_D;
        case XK_e:              return KEY_E;
        case XK_f:              return KEY_F;
        case XK_g:              return KEY_G;
        case XK_h:              return KEY_H;
        case XK_i:              return KEY_I;
        case XK_j:              return KEY_J;
        case XK_k:              return KEY_K;
        case XK_l:              return KEY_L;
        case XK_m:              return KEY_M;
        case XK_n:              return KEY_N;
        case XK_o:              return KEY_O;
        case XK_p:              return KEY_P;
        case XK_q:              return KEY_Q;
        case XK_r:              return KEY_R;
        case XK_s:              return KEY_S;
        case XK_t:              return KEY_T;
        case XK_u:              return KEY_U;
        case XK_v:              return KEY_V;
        case XK_w:              return KEY_W;
        case XK_x:              return KEY_X;
        case XK_y:              return KEY_Y;
        case XK_z:              return KEY_Z;
        case XK_1:              return KEY_1;
        case XK_2:              return KEY_2;
        case XK_3:              return KEY_3;
        case XK_4:              return KEY_4;
        case XK_5:              return KEY_5;
        case XK_6:              return KEY_6;
        case XK_7:              return KEY_7;
        case XK_8:              return KEY_8;
        case XK_9:              return KEY_9;
        case XK_0:              return KEY_0;
        case XK_space:          return KEY_SPACE;
        case XK_minus:          return KEY_MINUS;
        case XK_equal:          return KEY_EQUAL;
        case XK_bracketleft:    return KEY_LEFT_BRACKET;
        case XK_bracketright:   return KEY_RIGHT_BRACKET;
        case XK_backslash:      return KEY_BACKSLASH;
        case XK_semicolon:      return KEY_SEMICOLON;
        case XK_apostrophe:     return KEY_APOSTROPHE;
        case XK_grave:          return KEY_GRAVE_ACCENT;
        case XK_comma:          return KEY_COMMA;
        case XK_period:         return KEY_PERIOD;
        case XK_slash:          return KEY_SLASH;
        case XK_less:           return KEY_WORLD_1; // At least in some layouts...
        default:                break;
    }
	
    // No matching translation was found
    return KEY_UNKNOWN;
}

static void create_key_tables(platform_window window)
{
	s32 scancode, key;
	char name[XkbKeyNameLength + 1];
	XkbDescPtr desc = XkbGetMap(window.display, 0, XkbUseCoreKbd);
	XkbGetNames(window.display, XkbKeyNamesMask, desc);
	
	// uncomment for layout independant input for games.
#if 0
	for (scancode = desc->min_key_code;  scancode <= desc->max_key_code;  scancode++)
	{
		memcpy(name, desc->names->keys[scancode].name, XkbKeyNameLength);
		name[XkbKeyNameLength] = '\0';
		
		// Map the key name to a GLFW key code. Note: We only map printable
		// keys here, and we use the US keyboard layout. The rest of the
		// keys (function keys) are mapped using traditional KeySym
		// translations.
		if (strcmp(name, "TLDE") == 0) key = KEY_GRAVE_ACCENT;
		else if (strcmp(name, "AE01") == 0) key = KEY_1;
		else if (strcmp(name, "AE02") == 0) key = KEY_2;
		else if (strcmp(name, "AE03") == 0) key = KEY_3;
		else if (strcmp(name, "AE04") == 0) key = KEY_4;
		else if (strcmp(name, "AE05") == 0) key = KEY_5;
		else if (strcmp(name, "AE06") == 0) key = KEY_6;
		else if (strcmp(name, "AE07") == 0) key = KEY_7;
		else if (strcmp(name, "AE08") == 0) key = KEY_8;
		else if (strcmp(name, "AE09") == 0) key = KEY_9;
		else if (strcmp(name, "AE10") == 0) key = KEY_0;
		else if (strcmp(name, "AE11") == 0) key = KEY_MINUS;
		else if (strcmp(name, "AE12") == 0) key = KEY_EQUAL;
		else if (strcmp(name, "AD01") == 0) key = KEY_Q;
		else if (strcmp(name, "AD02") == 0) key = KEY_W;
		else if (strcmp(name, "AD03") == 0) key = KEY_E;
		else if (strcmp(name, "AD04") == 0) key = KEY_R;
		else if (strcmp(name, "AD05") == 0) key = KEY_T;
		else if (strcmp(name, "AD06") == 0) key = KEY_Y;
		else if (strcmp(name, "AD07") == 0) key = KEY_U;
		else if (strcmp(name, "AD08") == 0) key = KEY_I;
		else if (strcmp(name, "AD09") == 0) key = KEY_O;
		else if (strcmp(name, "AD10") == 0) key = KEY_P;
		else if (strcmp(name, "AD11") == 0) key = KEY_LEFT_BRACKET;
		else if (strcmp(name, "AD12") == 0) key = KEY_RIGHT_BRACKET;
		else if (strcmp(name, "AC01") == 0) key = KEY_A;
		else if (strcmp(name, "AC02") == 0) key = KEY_S;
		else if (strcmp(name, "AC03") == 0) key = KEY_D;
		else if (strcmp(name, "AC04") == 0) key = KEY_F;
		else if (strcmp(name, "AC05") == 0) key = KEY_G;
		else if (strcmp(name, "AC06") == 0) key = KEY_H;
		else if (strcmp(name, "AC07") == 0) key = KEY_J;
		else if (strcmp(name, "AC08") == 0) key = KEY_K;
		else if (strcmp(name, "AC09") == 0) key = KEY_L;
		else if (strcmp(name, "AC10") == 0) key = KEY_SEMICOLON;
		else if (strcmp(name, "AC11") == 0) key = KEY_APOSTROPHE;
		else if (strcmp(name, "AB01") == 0) key = KEY_Z;
		else if (strcmp(name, "AB02") == 0) key = KEY_X;
		else if (strcmp(name, "AB03") == 0) key = KEY_C;
		else if (strcmp(name, "AB04") == 0) key = KEY_V;
		else if (strcmp(name, "AB05") == 0) key = KEY_B;
		else if (strcmp(name, "AB06") == 0) key = KEY_N;
		else if (strcmp(name, "AB07") == 0) key = KEY_M;
		else if (strcmp(name, "AB08") == 0) key = KEY_COMMA;
		else if (strcmp(name, "AB09") == 0) key = KEY_PERIOD;
		else if (strcmp(name, "AB10") == 0) key = KEY_SLASH;
		else if (strcmp(name, "BKSL") == 0) key = KEY_BACKSLASH;
		else if (strcmp(name, "LSGT") == 0) key = KEY_WORLD_1;
		else key = KEY_UNKNOWN;
		
		if ((scancode >= 0) && (scancode < 256))
			keycode_map[scancode] = key;
	}
#endif
	
	for (scancode = 0;  scancode < MAX_KEYCODE;  scancode++)
	{
		// Translate the un-translated key codes using traditional X11 KeySym
		// lookups
		
		keycode_map[scancode] = translate_keycode(&window, scancode);
	}
	
	XkbFreeNames(desc, XkbKeyNamesMask, True);
	XkbFreeKeyboard(desc, 0, True);
}

inline void platform_init()
{
#if 0
	dlerror(); // clear error
	void *x11 = dlopen("libX11.so.6", RTLD_NOW | RTLD_GLOBAL);
	void *randr = dlopen("libXrandr.so", RTLD_NOW | RTLD_GLOBAL);
	
	if (!x11 || !randr)
	{
		platform_show_message("Missing window manager library.", "Fatal error.");
	}
#endif
	
	XInitThreads();
}

inline void platform_window_make_current(platform_window *window)
{
	glXMakeCurrent(window->display, window->window, window->gl_context);
}

void platform_window_set_size(platform_window *window, u16 width, u16 height)
{
	XResizeWindow(window->display, window->window, width, height);
}

platform_window platform_open_window(char *name, u16 width, u16 height, u16 max_w, u16 max_h)
{
	u8 has_max_size = max_w || max_h;
	
	platform_window window;
	window.has_focus = true;
	window.drag_drop_info.path = 0;
	window.drag_drop_info.state = DRAG_DROP_NONE;
	
	static int att[] =
	{
		GLX_X_RENDERABLE    , True,
		GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
		GLX_RENDER_TYPE     , GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
		GLX_RED_SIZE        , 8,
		GLX_GREEN_SIZE      , 8,
		GLX_BLUE_SIZE       , 8,
		GLX_ALPHA_SIZE      , 8,
		GLX_DEPTH_SIZE      , 24,
		GLX_STENCIL_SIZE    , 8,
		GLX_DOUBLEBUFFER    , True,
		//GLX_SAMPLE_BUFFERS  , 1,
		//GLX_SAMPLES         , 4,
		None
	};
	
	window.display = XOpenDisplay(NULL);
	
	if(window.display == NULL) {
		return window;
	}
	
	window.parent = DefaultRootWindow(window.display);
	
	int fbcount;
	GLXFBConfig* fbc = glXChooseFBConfig(window.display, DefaultScreen(window.display), att, &fbcount);
	int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;
	
	int i;
	for (i=0; i<fbcount; ++i)
	{
		XVisualInfo *vi = glXGetVisualFromFBConfig(window.display, fbc[i] );
		if ( vi )
		{
			int samp_buf, samples;
			glXGetFBConfigAttrib(window.display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
			glXGetFBConfigAttrib(window.display, fbc[i], GLX_SAMPLES       , &samples  );
			
			if ( best_fbc < 0 || (samp_buf && samples > best_num_samp))
				best_fbc = i, best_num_samp = samples;
			if ( worst_fbc < 0 || !samp_buf || samples < worst_num_samp )
				worst_fbc = i, worst_num_samp = samples;
		}
		XFree( vi );
	}
	
	GLXFBConfig bestFbc = fbc[ best_fbc ];
	XFree( fbc );
	
	XVisualInfo *vi = glXGetVisualFromFBConfig(window.display, bestFbc );
	window.visual_info = vi;
	
	if(window.visual_info == NULL) {
		return window;
	}
	
	window.cmap = XCreateColormap(window.display, window.parent, window.visual_info->visual, AllocNone);
	
	// calculate window center
	XRRScreenResources *screens = XRRGetScreenResources(window.display, window.parent);
	XRRCrtcInfo *info = XRRGetCrtcInfo(window.display, screens, screens->crtcs[0]);
	
	s32 center_x = (info->width / 2) - (width / 2);
	s32 center_y = (info->height / 2) - (height / 2);
	
	XRRFreeCrtcInfo(info);
	XRRFreeScreenResources(screens);
	
	XSetWindowAttributes window_attributes;
	window_attributes.colormap = window.cmap;
	window_attributes.border_pixel = 0;
	window_attributes.event_mask = KeyPressMask | KeyReleaseMask | PointerMotionMask |
		ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | FocusChangeMask | LeaveWindowMask;
	
	window.window = XCreateWindow(window.display, window.parent, center_x, center_y, width, height, 0, window.visual_info->depth, InputOutput, window.visual_info->visual, CWColormap | CWEventMask | CWBorderPixel, &window_attributes);
	
	XSizeHints hints;
	
	if (has_max_size)
		hints.flags = PMaxSize | PMinSize | USPosition;
	else
		hints.flags = PMinSize | USPosition;
	hints.x = center_x;
	hints.y = center_y;
	hints.max_width = width;
	hints.max_height = height;
	hints.min_width = width;
	hints.min_height = height;
	
	XSetWMNormalHints(window.display, window.window, &hints);
	
	XMapWindow(window.display, window.window);
	XStoreName(window.display, window.window, name);
	
	static GLXContext share_list = 0;
	
	// get opengl context
	window.gl_context = glXCreateContext(window.display, window.visual_info, 
										 share_list, GL_TRUE);
	
	if (share_list == 0)
		share_list = window.gl_context;
	glXMakeCurrent(window.display, window.window, window.gl_context);
	
	// blending
	glEnable(GL_DEPTH_TEST);
	//glDepthMask(true);
	//glClearDepth(50);
	glDepthFunc(GL_LEQUAL);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	// setup multisampling
#if 0
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	glEnable(GL_SAMPLE_ALPHA_TO_ONE);
	glEnable(GL_MULTISAMPLE);
	glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
#endif
	
	// TODO: is this correct?
	// https://stackoverflow.com/questions/5627229/sub-pixel-drawing-with-opengl
	//glHint(GL_POINT_SMOOTH, GL_NICEST);
	//glHint(GL_LINE_SMOOTH, GL_NICEST);
	//glHint(GL_POLYGON_SMOOTH, GL_NICEST);
	
	//glEnable(GL_SMOOTH);
	//glEnable(GL_POINT_SMOOTH);
	//glEnable(GL_LINE_SMOOTH);
	//glEnable(GL_POLYGON_SMOOTH);
	//////////////////
	
	window.is_open = true;
	window.width = width;
	window.height = height;
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, -1, 1);
	
	//GLint m_viewport[4];
	//glGetIntegerv( GL_VIEWPORT, m_viewport );
	//printf("%d %d %d %d\n", m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
	
	glMatrixMode(GL_MODELVIEW);
	
	create_key_tables(window);
	
	// recieve window close event
	window.quit = XInternAtom(window.display, "WM_DELETE_WINDOW", False);
	
	// drag and drop atoms
	GET_ATOM(XdndEnter);
    GET_ATOM(XdndPosition);
    GET_ATOM(XdndStatus);
    GET_ATOM(XdndTypeList);
    GET_ATOM(XdndActionCopy);
    GET_ATOM(XdndDrop);
    GET_ATOM(XdndFinished);
	GET_ATOM(XdndSelection);
	GET_ATOM(XdndLeave);
	GET_ATOM(PRIMARY);
	
	array atoms = array_create(sizeof(Atom));
	array_push(&atoms, &window.quit);
	array_push(&atoms, &window.XdndEnter);
	array_push(&atoms, &window.XdndPosition);
	array_push(&atoms, &window.XdndStatus);
	array_push(&atoms, &window.XdndTypeList);
	array_push(&atoms, &window.XdndActionCopy);
	array_push(&atoms, &window.XdndDrop);
	array_push(&atoms, &window.XdndFinished);
	array_push(&atoms, &window.XdndSelection);
	array_push(&atoms, &window.XdndLeave);
	array_push(&atoms, &window.PRIMARY);
	
#if 0
	XSetWMProtocols(window.display, window.window, &window.XdndEnter, 1);
	XSetWMProtocols(window.display, window.window, &window.XdndPosition, 1);
	XSetWMProtocols(window.display, window.window, &window.XdndStatus, 1);
	XSetWMProtocols(window.display, window.window, &window.XdndTypeList, 1);
	XSetWMProtocols(window.display, window.window, &window.XdndActionCopy, 1);
	XSetWMProtocols(window.display, window.window, &window.XdndDrop, 1);
	XSetWMProtocols(window.display, window.window, &window.XdndFinished, 1);
	XSetWMProtocols(window.display, window.window, &window.XdndSelection, 1);
	XSetWMProtocols(window.display, window.window, &window.XdndLeave, 1);
	XSetWMProtocols(window.display, window.window, &window.PRIMARY, 1);
#endif
	
	XSetWMProtocols(window.display, window.window, atoms.data, atoms.length);
	
	Atom XdndAware = XInternAtom(window.display, "XdndAware", False);
	Atom xdnd_version = 5;
	XChangeProperty(window.display, window.window, XdndAware, XA_ATOM, 32,
					PropModeReplace, (unsigned char*)&xdnd_version, 1);
	
	array_destroy(&atoms);
	
	return window;
}

inline u8 platform_window_is_valid(platform_window *window)
{
	return window->display && window->window;
}

void platform_close_window(platform_window *window)
{
	XDestroyWindow(window->display, window->window);
	window->display = 0;
	window->window = 0;
	//glXMakeCurrent(window->display, None, NULL);
	//glXDestroyContext(window->display, window->gl_context);
	//XCloseDisplay(window->display);
}

void platform_destroy_window(platform_window *window)
{
	if (!window->display || !window->display) return;
	glXMakeCurrent(window->display, None, NULL);
	glXDestroyContext(window->display, window->gl_context);
	XCloseDisplay(window->display);
}

typedef struct {
    unsigned char *data;
    int format, count;
    Atom type;
} x11Prop;

/* Reads property
   Must call XFree on results
 */
static void XReadProperty(x11Prop *p, Display *disp, Window w, Atom prop)
{
    unsigned char *ret=NULL;
    Atom type;
    int fmt;
    unsigned long count;
    unsigned long bytes_left;
    int bytes_fetch = 0;
	
    do {
        if (ret != 0) XFree(ret);
        XGetWindowProperty(disp, w, prop, 0, bytes_fetch, False, AnyPropertyType, &type, &fmt, &count, &bytes_left, &ret);
        bytes_fetch += bytes_left;
    } while (bytes_left != 0);
	
    p->data=ret;
    p->format=fmt;
    p->count=count;
    p->type=type;
}

/* Find text-uri-list in a list of targets and return it's atom
   if available, else return None */
static Atom XPickTarget(Display *disp, Atom list[], int list_count)
{
    Atom request = None;
    char *name;
    int i;
    for (i=0; i < list_count && request == None; i++) {
        name = XGetAtomName(disp, list[i]);
        if ((strcmp("text/uri-list", name) == 0) || (strcmp("text/plain", name) == 0)) {
			request = list[i];
        }
        XFree(name);
    }
    return request;
}

/* Wrapper for XPickTarget for a maximum of three targets, a special
   case in the Xdnd protocol */
static Atom XPickTargetFromAtoms(Display *disp, Atom a0, Atom a1, Atom a2)
{
    int count=0;
    Atom atom[3];
    if (a0 != None) atom[count++] = a0;
    if (a1 != None) atom[count++] = a1;
    if (a2 != None) atom[count++] = a2;
    return XPickTarget(disp, atom, count);
}

static int XURIDecode(char *buf, int len) {
    int ri, wi, di;
    char decode = '\0';
    if (buf == NULL || len < 0) {
        errno = EINVAL;
        return -1;
    }
    if (len == 0) {
        len = strlen(buf);
    }
    for (ri = 0, wi = 0, di = 0; ri < len && wi < len; ri += 1) {
        if (di == 0) {
            /* start decoding */
            if (buf[ri] == '%') {
                decode = '\0';
                di += 1;
                continue;
            }
            /* normal write */
            buf[wi] = buf[ri];
            wi += 1;
            continue;
        } else if (di == 1 || di == 2) {
            char off = '\0';
            char isa = buf[ri] >= 'a' && buf[ri] <= 'f';
            char isA = buf[ri] >= 'A' && buf[ri] <= 'F';
            char isn = buf[ri] >= '0' && buf[ri] <= '9';
            if (!(isa || isA || isn)) {
                /* not a hexadecimal */
                int sri;
                for (sri = ri - di; sri <= ri; sri += 1) {
                    buf[wi] = buf[sri];
                    wi += 1;
                }
                di = 0;
                continue;
            }
            /* itsy bitsy magicsy */
            if (isn) {
                off = 0 - '0';
            } else if (isa) {
                off = 10 - 'a';
            } else if (isA) {
                off = 10 - 'A';
            }
            decode |= (buf[ri] + off) << (2 - di) * 4;
            if (di == 2) {
                buf[wi] = decode;
                wi += 1;
                di = 0;
            } else {
                di += 1;
            }
            continue;
        }
    }
    buf[wi] = '\0';
    return wi;
}

static char* XURIToLocal(char* uri) {
    char *file = NULL;
	u8 local;
	
    if (memcmp(uri,"file:/",6) == 0) uri += 6;      /* local file? */
    else if (strstr(uri,":/") != NULL) return file; /* wrong scheme */
	
    local = uri[0] != '/' || (uri[0] != '\0' && uri[1] == '/');
	
    /* got a hostname? */
    if (!local && uri[0] == '/' && uri[2] != '/') {
		char* hostname_end = strchr(uri+1, '/');
		if (hostname_end != NULL) {
			char hostname[ 257 ];
			if (gethostname(hostname, 255) == 0) {
				hostname[ 256 ] = '\0';
				if (memcmp(uri+1, hostname, hostname_end - (uri+1)) == 0) {
					uri = hostname_end + 1;
					local = 1;
				}
			}
		}
    }
    if (local) {
		file = uri;
		/* Convert URI escape sequences to real characters */
		XURIDecode(file, 0);
		if (uri[1] == '/') {
			file++;
		} else {
			file--;
		}
    }
    return file;
}



void platform_handle_events(platform_window *window, mouse_input *mouse, keyboard_input *keyboard)
{
	mouse->left_state &= ~MOUSE_CLICK;
	mouse->right_state &= ~MOUSE_CLICK;
	mouse->left_state &= ~MOUSE_RELEASE;
	mouse->right_state &= ~MOUSE_RELEASE;
	memset(keyboard->input_keys, 0, MAX_KEYCODE);
	mouse->move_x = 0;
	mouse->move_y = 0;
	mouse->scroll_state = 0;
	
	if (window->drag_drop_info.state == DRAG_DROP_FINISHED)
	{
		window->drag_drop_info.path = 0;
		window->drag_drop_info.state = DRAG_DROP_NONE;
	}
	if (window->drag_drop_info.state == DRAG_DROP_LEAVE)
	{
		window->drag_drop_info.state = DRAG_DROP_NONE;
	}
	
	XClientMessageEvent m;
	
	s32 pending_events = XPending(window->display);
	for (s32 i = 0; i < pending_events; i++)
	{
		XNextEvent(window->display, &window->event);
		
		if (window->event.type == ClientMessage)
		{
			static int xdnd_version=0;
			
			if ((Atom)window->event.xclient.data.l[0] == window->quit) {
				window->is_open = false;
			}
			
			if (window->event.xclient.message_type == window->XdndDrop)
			{
				if (window->xdnd_req == None) {
                    /* say again - not interested! */
                    memset(&m, 0, sizeof(XClientMessageEvent));
                    m.type = ClientMessage;
                    m.display = window->event.xclient.display;
                    m.window = window->event.xclient.data.l[0];
                    m.message_type = window->XdndFinished;
                    m.format=32;
                    m.data.l[0] = window->window;
                    m.data.l[1] = 0;
                    m.data.l[2] = None; /* fail! */
                    XSendEvent(window->display, window->event.xclient.data.l[0], False, NoEventMask, (XEvent*)&m);
                } else {
                    /* convert */
                    if(xdnd_version >= 1) {
                        XConvertSelection(window->display, window->XdndSelection, window->xdnd_req, window->PRIMARY, window->window, window->event.xclient.data.l[2]);
                    } else {
						printf("time to find the time.\n");
                        //XConvertSelection(window->display, window->XdndSelection, window->xdnd_req, window->PRIMARY, window->xwindow, CurrentTime);
                    }
				}
			}
			else if (window->event.xclient.message_type == window->XdndLeave)
			{
				window->drag_drop_info.state = DRAG_DROP_LEAVE;
			}
			else if (window->event.xclient.message_type == window->XdndActionCopy)
			{
				memset(&m, 0, sizeof(XClientMessageEvent));
                m.type = ClientMessage;
                m.display = window->event.xclient.display;
                m.window = window->event.xclient.data.l[0];
                m.message_type = window->XdndStatus;
                m.format=32;
                m.data.l[0] = window->window;
                m.data.l[1] = (window->xdnd_req != None);
                m.data.l[2] = 0; /* specify an empty rectangle */
                m.data.l[3] = 0;
                m.data.l[4] = window->XdndActionCopy; /* we only accept copying anyway */
				
                XSendEvent(window->display, window->event.xclient.data.l[0], False, NoEventMask, (XEvent*)&m);
				XFlush(window->display);
			}
			else if (window->event.xclient.message_type == window->XdndEnter)
			{
				window->drag_drop_info.state = DRAG_DROP_ENTER;
				
				u8 use_list = window->event.xclient.data.l[1] & 1;
                window->xdnd_source = window->event.xclient.data.l[0];
				xdnd_version = (window->event.xclient.data.l[1] >> 24);
				
				if (use_list) {
                    /* fetch conversion targets */
                    x11Prop p;
                    XReadProperty(&p, window->display, window->xdnd_source, window->XdndTypeList);
                    /* pick one */
                    window->xdnd_req = XPickTarget(window->display, (Atom*)p.data, p.count);
                } else {
                    /* pick from list of three */
                    window->xdnd_req = XPickTargetFromAtoms(window->display, window->event.xclient.data.l[2], window->event.xclient.data.l[3], window->event.xclient.data.l[4]);
				}
			}
		}
		else if (window->event.type == SelectionNotify)
		{
			Atom target = window->event.xselection.target;
			
			if (target == window->xdnd_req) {
				x11Prop p;
				XReadProperty(&p, window->display, window->window, window->PRIMARY);
				
				if (p.format == 8) {
                    /* !!! FIXME: don't use strtok here. It's not reentrant and not in stdinc. */
                    char* name = XGetAtomName(window->display, target);
                    char *token = strtok((char *) p.data, "\r\n");
                    while (token != NULL) {
						if (strcmp("text/uri-list", name)==0) {
							char *fn = XURIToLocal(token);
							if (fn)
							{
								window->drag_drop_info.state = DRAG_DROP_FINISHED;
								window->drag_drop_info.path = fn;
							}
						}
                        token = strtok(NULL, "\r\n");
                    }
                }
				
                /* send reply */
                memset(&m, 0, sizeof(XClientMessageEvent));
                m.type = ClientMessage;
                m.display = window->display;
                m.window = window->xdnd_source;
                m.message_type = window->XdndFinished;
                m.format = 32;
                m.data.l[0] = window->window;
                m.data.l[1] = 1;
                m.data.l[2] = window->XdndActionCopy;
                XSendEvent(window->display, window->xdnd_source, False, NoEventMask, (XEvent*)&m);
				
				XSync(window->display, False);
			}
		}
		else if (window->event.type == LeaveNotify)
		{
			mouse->x = MOUSE_OFFSCREEN;
			mouse->y = MOUSE_OFFSCREEN;
		}
		else if (window->event.type == ConfigureNotify)
		{
			XConfigureEvent xce = window->event.xconfigure;
			window->width = xce.width;
			window->height = xce.height;
			glViewport(0, 0, window->width, window->height);
		}
		else if (window->event.type == FocusIn)
		{
			window->has_focus = true;
		}
		else if (window->event.type == FocusOut)
		{
			mouse->x = MOUSE_OFFSCREEN;
			mouse->y = MOUSE_OFFSCREEN;
			window->has_focus = false;
		}
		else if (window->event.type == MotionNotify)
		{
			s32 x = mouse->x;
			s32 y = mouse->y;
			
			mouse->x = window->event.xmotion.x;
			mouse->y = window->event.xmotion.y;
			
			mouse->move_x = x - mouse->x;
			mouse->move_y = y - mouse->y;
		}
		else if (window->event.type == ButtonPress)
		{
			u8 is_left_down = window->event.xbutton.button == Button1;
			u8 is_right_down = window->event.xbutton.button == Button3;
			u8 is_middle_down = window->event.xbutton.button == Button2;
			u8 scroll_up = window->event.xbutton.button == Button4;
			u8 scroll_down = window->event.xbutton.button == Button5;
			
			if (scroll_up)
				mouse->scroll_state = SCROLL_UP;
			if (scroll_down)
				mouse->scroll_state = SCROLL_DOWN;
			
			if (is_left_down)
			{
				mouse->left_state |= MOUSE_DOWN;
				mouse->left_state |= MOUSE_CLICK;
			}
			if (is_right_down)
			{
				mouse->right_state |= MOUSE_DOWN;
				mouse->right_state |= MOUSE_CLICK;
			}
		}
		else if (window->event.type == ButtonRelease)
		{
			u8 is_left_up = window->event.xbutton.button == Button1;
			u8 is_right_up = window->event.xbutton.button == Button3;
			u8 is_middle_up = window->event.xbutton.button == Button2;
			
			if (is_left_up)
			{
				mouse->left_state = MOUSE_RELEASE;
			}
			if (is_right_up)
			{
				mouse->right_state = MOUSE_RELEASE;
			}
		}
		else if(window->event.type == KeyPress) 
		{
			s32 key = window->event.xkey.keycode;
			
			keyboard->keys[keycode_map[key]] = true;
			keyboard->input_keys[keycode_map[key]] = true;
			
			// https://gist.github.com/rickyzhang82/8581a762c9f9fc6ddb8390872552c250
			//printf("state: %d\n", window->event.xkey.state);
			
			// remove key control key from mask so it doesnt block input
			window->event.xkey.state &= ~ControlMask;
			
			KeySym ksym = XLookupKeysym(&window->event.xkey, window->event.xkey.state);
			
			if (keyboard->take_input)
			{
				char *ch = 0;
				switch(ksym)
				{
					case XK_space: ch = " "; break;
					case XK_exclam: ch = "!"; break;
					case XK_quotedbl: ch = "\""; break;
					case XK_numbersign: ch = "#"; break;
					case XK_dollar: ch = "$"; break;
					case XK_percent: ch = "%"; break;
					case XK_ampersand: ch = "&"; break;
					case XK_apostrophe: ch = "`"; break;
					case XK_parenleft: ch = "("; break;
					case XK_parenright: ch = ")"; break;
					case XK_asterisk: ch = "*"; break;
					case XK_plus: ch = "+"; break;
					case XK_comma: ch = ","; break;
					case XK_minus: ch = "-"; break;
					case XK_period: ch = "."; break;
					case XK_slash: ch = "/"; break;
					case XK_0: ch = "0"; break;
					case XK_1: ch = "1"; break;
					case XK_2: ch = "2"; break;
					case XK_3: ch = "3"; break;
					case XK_4: ch = "4"; break;
					case XK_5: ch = "5"; break;
					case XK_6: ch = "6"; break;
					case XK_7: ch = "7"; break;
					case XK_8: ch = "8"; break;
					case XK_9: ch = "9"; break;
					
					case XK_colon: ch = ":"; break;
					case XK_semicolon: ch = ";"; break;
					case XK_less: ch = "<"; break;
					case XK_equal: ch = "="; break;
					case XK_greater: ch = ">"; break;
					case XK_question: ch = "?"; break;
					case XK_at: ch = "@"; break;
					case XK_bracketleft: ch = "["; break;
					case XK_backslash: ch = "\\"; break;
					case XK_bracketright: ch = "]"; break;
					case XK_asciicircum: ch = "^"; break;
					case XK_underscore: ch = "_"; break;
					case XK_grave: ch = "`"; break;
					case XK_braceleft: ch = "{"; break;
					case XK_bar: ch = "|"; break;
					case XK_braceright: ch = "}"; break;
					case XK_asciitilde: ch = "~"; break;
				}
				
				if ((ksym >= XK_A && ksym <= XK_Z) || (ksym >= XK_a && ksym <= XK_z))
				{
					ch = XKeysymToString(ksym);
				}
				
				if (ch && keyboard->input_mode == INPUT_NUMERIC)
				{
					if (!(*ch >= 48 && *ch <= 57))
					{
						ch = 0;
					}
				}
				
				keyboard_handle_input_string(keyboard, ch);
			}
		}
		else if (window->event.type == KeyRelease)
		{
			s32 key = window->event.xkey.keycode;
			keyboard->keys[keycode_map[key]] = false;
			
			KeySym ksym = XLookupKeysym(&window->event.xkey, 0);
		}
	}
}

inline void platform_window_swap_buffers(platform_window *window)
{
	glXSwapBuffers(window->display, window->window);
}

u64 platform_get_time(time_type time_type, time_precision precision)
{
	s32 type = CLOCK_REALTIME;
	switch(time_type)
	{
		case TIME_FULL: type = CLOCK_REALTIME; break;
		case TIME_THREAD: type = CLOCK_THREAD_CPUTIME_ID; break;
		case TIME_PROCESS: type = CLOCK_PROCESS_CPUTIME_ID; break;
	}
	
	struct timespec tms;
	if (clock_gettime(type,&tms)) {
		return -1;
	}
	
	long result = 0;
	
	if (precision == TIME_NS)
	{
		result = tms.tv_sec * 1000000000;
		result += tms.tv_nsec;
		if (tms.tv_nsec % 1000 >= 500) {
			++result;
		}
	}
	else if (precision == TIME_US)
	{
		result = tms.tv_sec * 1000000;
		result += tms.tv_nsec/1000;
		if (tms.tv_nsec % 1000 >= 500) {
			++result;
		}
	}
	else if (precision == TIME_MILI_S)
	{
		result = tms.tv_sec * 1000;
		result += tms.tv_nsec/1000000;
		if (tms.tv_nsec % 1000 >= 500) {
			++result;
		}
	}
	else if (precision == TIME_S)
	{
		result = tms.tv_sec;
		result += tms.tv_nsec/1000000000;
		if (tms.tv_nsec % 1000 >= 500) {
			++result;
		}
	}
	
	return result;
}

inline s32 platform_get_cpu_count()
{
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
}

inline s32 platform_get_memory_size()
{
	uint64_t aid = (uint64_t) sysconf(_SC_PHYS_PAGES);
	aid         *= (uint64_t) sysconf(_SC_PAGESIZE);
	aid         /= (uint64_t) (1024 * 1024);
	return (int)(aid);
}

cpu_info platform_get_cpu_info()
{
	cpu_info result;
	
	FILE *data = fopen("/proc/cpuinfo", "r");
	if (!data) return result;
	
	fseek(data, 0, SEEK_END);
	s64 file_size = ftell(data);
	fseek(data, 0, SEEK_SET);
	
	char *file_buffer = mem_alloc(50000);
	fread(file_buffer, 1, 50000, data);
	
	// 3 = model, 4 = model name, 7 = frequency, 8 = cache size, 22  = cache alignment
	
	char tmp_buffer[1000];
	int tmp_buffer_len = 0;
	u8 collect_string = false;
	int line_nr = 0;
	for (int i = 0; i < 5000; i++)
	{
		char ch = file_buffer[i];
		
		if (ch == ':')
		{
			collect_string = true;
			tmp_buffer_len = 0;
			tmp_buffer[0] = 0;
			i++;
			continue;
		}
		
		if (ch == '\n')
		{
			collect_string = false;
			tmp_buffer[tmp_buffer_len] = 0;
			
			if (line_nr == 3)
			{
				result.model = atoi(tmp_buffer);
			}
			else if (line_nr == 4)
			{
				sprintf(result.model_name, "%s", tmp_buffer);
			}
			else if (line_nr == 7)
			{
				result.frequency = atof(tmp_buffer);
			}
			else if (line_nr == 8)
			{
				result.cache_size = atoi(tmp_buffer);
			}
			else if (line_nr == 22)
			{
				result.cache_alignment = atoi(tmp_buffer);
				goto done;
			}
			line_nr++;
		}
		
		if (collect_string)
		{
			tmp_buffer[tmp_buffer_len++] = ch;
		}
	}
	
	done:
	fclose(data);
	mem_free(file_buffer);
	
	return result;
}

void platform_show_message(platform_window *window, char *message, char *title)
{
	char command[MAX_INPUT_LENGTH];
	sprintf(command, "zenity --info --text=\"%s\" --title=\"%s\" --width=240", message, title);
	FILE *f = popen(command, "r");
}

static void* platform_open_file_dialog_dd(void *data)
{
	struct open_dialog_args *args = data;
	
	FILE *f;
	
	char *current_val = mem_alloc(MAX_INPUT_LENGTH);
	strncpy(current_val, args->buffer, MAX_INPUT_LENGTH);
	
	char file_filter[MAX_INPUT_LENGTH];
	file_filter[0] = 0;
	if (args->file_filter)
		sprintf(file_filter, "--file-filter=\"%s\"", args->file_filter);
	
	char start_path[MAX_INPUT_LENGTH];
	start_path[0] = 0;
	if (args->start_path)
		sprintf(start_path, "--filename=\"%s\"", args->start_path);
	
	
	char command[MAX_INPUT_LENGTH];
	
	if (args->type == OPEN_FILE)
	{
		sprintf(command, "zenity --file-selection %s %s", file_filter, start_path);
	}
	else if (args->type == OPEN_DIRECTORY)
	{
		sprintf(command, "zenity --file-selection --directory %s %s", file_filter, start_path);
	}
	else if (args->type == SAVE_FILE)
	{
		sprintf(command, "zenity --file-selection --save --confirm-overwrite %s %s", file_filter, start_path);
	}
	
	f = popen(command, "r");
	
	char *buffer = mem_alloc(MAX_INPUT_LENGTH);
	fgets(buffer, MAX_INPUT_LENGTH, f);
	
	// NOTE(Aldrik): buffer should be longer then 2 because zenity returns a single garbage character when closed without selecting a path. (lol?)
	if (strcmp(buffer, current_val) != 0 && strcmp(buffer, "") != 0)
	{
		if (strlen(buffer) > 2 || strcmp(buffer, "/") == 0)
		{
			strncpy(args->buffer, buffer, MAX_INPUT_LENGTH);
			s32 len = strlen(args->buffer);
			args->buffer[len-1] = 0;
		}
	}
	
	mem_free(current_val);
	mem_free(buffer);
	
	return 0;
}

void *platform_open_file_dialog_block(void *arg)
{
	thread thr = thread_start(platform_open_file_dialog_dd, arg);
	thread_join(&thr);
	mem_free(arg);
	return 0;
}

void platform_open_file_dialog(file_dialog_type type, char *buffer, char *file_filter, char *start_path)
{
	struct open_dialog_args *args = mem_alloc(sizeof(struct open_dialog_args));
	args->buffer = buffer;
	args->type = type;
	args->file_filter = file_filter;
	args->start_path = start_path;
	
	thread thr;
	thr.valid = false;
	
	while (!thr.valid)
		thr = thread_start(platform_open_file_dialog_block, args);
	thread_detach(&thr);
}

void platform_list_files_block(array *list, char *start_dir, char *filter, u8 recursive)
{
	assert(list);
	
	// TODO(Aldrik): should we include symbolic links?
	
	s32 len = strlen(filter);
	
	char *subdirname_buf = mem_alloc(MAX_INPUT_LENGTH);
	
	DIR *d;
	struct dirent *dir;
	d = opendir(start_dir);
	if (d) {
		set_active_directory(start_dir);
		while ((dir = readdir(d)) != NULL) {
			if (platform_cancel_search) break;
			set_active_directory(start_dir);
			
			if (dir->d_type == DT_DIR && recursive)
			{
				if ((strcmp(dir->d_name, ".") == 0) || (strcmp(dir->d_name, "..") == 0))
					continue;
				
				strncpy(subdirname_buf, start_dir, MAX_INPUT_LENGTH);
				strcat(subdirname_buf, dir->d_name);
				strcat(subdirname_buf, "/");
				
				// do recursive search
				platform_list_files_block(list, subdirname_buf, filter, recursive);
			}
			// we handle DT_UNKNOWN for file systems that do not support type lookup.
			else if (dir->d_type == DT_REG || dir->d_type == DT_UNKNOWN)
			{
				// check if name matches pattern
				if (string_match(filter, dir->d_name))
				{
					char *buf = mem_alloc(PATH_MAX);
					//realpath(dir->d_name, buf);
					sprintf(buf, "%s%s",start_dir, dir->d_name);
					
					found_file f;
					f.path = buf;
					f.matched_filter = mem_alloc(len+1);
					strncpy(f.matched_filter, filter, len+1);
					
					array_push_size(list, &f, sizeof(found_file));
				}
			}
		}
		closedir(d);
	}
	
	mem_free(subdirname_buf);
}

char *platform_get_full_path(char *file)
{
	char *buf = mem_alloc(PATH_MAX);
	realpath(file, buf);
	return buf;
}

void* platform_list_files_t_t(void *args)
{
	list_file_args *info = args;
	platform_list_files_block(info->list, info->start_dir, info->pattern, info->recursive);
	mem_free(info);
	return 0;
}

void *platform_list_files_t(void *args)
{
	list_file_args *info = args;
	
	// TODO(Aldrik): hardcoded max filter length
	s32 max_filter_len = MAX_PATH_LENGTH;
	
	array filters = array_create(max_filter_len);
	
	char current_filter[max_filter_len];
	s32 filter_len = 0;
	
	array *list = info->list;
	char *start_dir = info->start_dir;
	char *pattern = info->pattern;
	u8 recursive = info->recursive;
	
	while(*pattern)
	{
		char ch = *pattern;
		
		if (ch == ',')
		{
			current_filter[filter_len] = 0;
			array_push(&filters, current_filter);
			filter_len = 0;
		}
		else
		{
			// TODO(Aldrik): show error and dont continue search
			assert(filter_len < MAX_PATH_LENGTH);
			
			current_filter[filter_len++] = ch;
		}
		
		pattern++;
	}
	current_filter[filter_len] = 0;
	array_push(&filters, current_filter);
	
	array threads = array_create(sizeof(thread));
	array_reserve(&threads, filters.length);
	
	for (s32 i = 0; i < filters.length; i++)
	{
		char *filter = array_at(&filters, i);
		
		thread thr;
		thr.valid = false;
		
		list_file_args *args_2 = mem_alloc(sizeof(list_file_args));
		if (args_2)
		{
			args_2->list = list;
			args_2->start_dir = start_dir;
			args_2->pattern = filter;
			args_2->recursive = recursive;
			
			thr = thread_start(platform_list_files_t_t, args_2);
		}
		
		if (platform_cancel_search) break;
		
		if (thr.valid)
		{
			array_push(&threads, &thr);
		}
		else
		{
			i--;
		}
	}
	
	for (s32 i = 0; i < threads.length; i++)
	{
		thread* thr = array_at(&threads, i);
		thread_join(thr);
	}
	
	if (!platform_cancel_search)
		*(info->state) = !(*info->state);
	
	array_destroy(&threads);
	array_destroy(&filters);
	mem_free(args);
	
	return 0;
}

void platform_list_files(array *list, char *start_dir, char *filter, u8 recursive, u8 *state)
{
	platform_cancel_search = false;
	list_file_args *args = mem_alloc(sizeof(list_file_args));
	args->list = list;
	args->start_dir = start_dir;
	args->pattern = filter;
	args->recursive = recursive;
	args->state = state;
	
	thread thr = thread_start(platform_list_files_t, args);
	thread_detach(&thr);
}

inline u64 string_to_u64(char *str)
{
	return (u64)strtoull(str, 0, 10);
}

inline u32 string_to_u32(char *str)
{
	return (u32)strtoul(str, 0, 10);
}

inline u16 string_to_u16(char *str)
{
	return (u16)strtoul(str, 0, 10);
}

inline u8 string_to_u8(char *str)
{
	return (u8)strtoul(str, 0, 10);
}

inline s64 string_to_s64(char *str)
{
	return (s64)strtoll(str, 0, 10);
}

inline s32 string_to_s32(char *str)
{
	return (u32)strtol(str, 0, 10);
}

inline s16 string_to_s16(char *str)
{
	return (s16)strtol(str, 0, 10);
}

inline s8 string_to_s8(char *str)
{
	return (s8)strtol(str, 0, 10);
}

inline void platform_open_url(char *url)
{
	//https://stackoverflow.com/questions/3037088/how-to-open-the-default-web-browser-in-windows-in-c
	char buffer[MAX_INPUT_LENGTH];
	sprintf(buffer, "xdg-open %s", url);
	platform_run_command(buffer);
}

inline void platform_run_command(char *command)
{
	// ShellExecute for windows
	system(command);
}

void platform_set_icon(platform_window *window, image *img)
{
	s32 w = img->width;
	s32 h = img->height;
	
	s32 nelements = (w * h) + 2;
	
	unsigned long data[nelements];
	int i = 0;
	(data)[i++] = w;
	(data)[i++] = h;
	
	for (s32 y = 0; y < h; y++)
	{
		for (s32 x = 0; x < w; x++)
		{
			s32 *pixel = (s32*)(&((data)[i++]));
			
			s32 img_pixel = *(((s32*)img->data+(x+(y*w))));
			*pixel = img_pixel;
		}
	}
	
	Atom property = XInternAtom(window->display, "_NET_WM_ICON", 0);
	Atom cardinal = XInternAtom(window->display, "CARDINAL", False);
	
	int result = XChangeProperty(window->display, window->window, 
								 property, cardinal, 32, PropModeReplace, 
								 (unsigned char *)data, nelements);
}