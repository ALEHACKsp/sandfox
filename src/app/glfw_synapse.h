namespace glfw_signals {

	typedef struct key_ ( * key ) ( GLFWwindow *, int key, int scancode, int action, int mods );
	typedef struct character_( * character ) ( GLFWwindow *, unsigned int codepoint );
	typedef struct character_modifier_ ( * character_modifier )( GLFWwindow *, unsigned int codepoint, int mods );
	typedef struct cursor_pos_ ( * cursor_pos ) ( GLFWwindow *, double xpos, double ypos );
	typedef struct cursor_enter_ ( * cursor_enter ) ( GLFWwindow *, int entered );
	typedef struct mouse_button_ ( * mouse_button ) ( GLFWwindow *, int button, int action, int mods );
	typedef struct scroll_ ( * scroll ) ( GLFWwindow *, double xoffset, double yoffset );
	typedef struct drop_ ( * drop ) ( GLFWwindow *, int count, char const * * paths );

	typedef struct window_close_ ( * window_close ) ( GLFWwindow * );
	typedef struct window_size_ ( * window_size ) ( GLFWwindow *, int width, int height );
	typedef struct framebuffer_size_ ( * framebuffer_size ) ( GLFWwindow *, int width, int height );
	typedef struct window_pos_ ( * window_pos ) ( GLFWwindow *, int xpos, int ypos );
	typedef struct window_iconify_ ( * window_iconify) ( GLFWwindow *, int iconified );
	typedef struct window_focus_ ( * window_focus ) ( GLFWwindow *, int focused );
	typedef struct window_refresh_ ( * window_refresh ) ( GLFWwindow * );

	typedef struct exception_caught_ ( * exception_caught) ( GLFWwindow * );
}

template <class signal> class glfw_synapse;

template <class R, class... A> class glfw_synapse<R(*)(GLFWwindow *,A...)> {

	typedef R(*signal)(GLFWwindow *,A...);
	typedef void (*GLFWfun)( GLFWwindow *,A... );
	static GLFWfun prev_;

	static void handler( GLFWwindow * w, A... a ) {

		using namespace boost::synapse;
		try {
			(void) emit<signal>(w,w,a...);
		} catch(...) {
			bool handled = emit<glfw_signals::exception_caught>(w,w)>0;
			assert(handled);
		}
		if(prev_) prev_(w,a...);
	}

	public:

	explicit glfw_synapse(GLFWfun(*setter)(GLFWwindow *, GLFWfun)) {

		using namespace boost::synapse;

		connect<meta::connected<signal>>(meta::emitter(), [setter]( connection & c, unsigned flags ) {

			if(flags & meta::connect_flags::connecting) {
				if(flags & meta::connect_flags::first_for_this_emitter) prev_=setter(c.emitter<GLFWwindow>().get(),&handler);
			} else {
				if( flags & meta::connect_flags::last_for_this_emitter ) {
					GLFWfun p = setter(c.emitter<GLFWwindow>().get(), prev_);
					assert(p == &handler);
				}
			}
		});
	}
};

template <class R, class... A>
typename glfw_synapse<R(*)(GLFWwindow *,A...)>::GLFWfun glfw_synapse<R(*)(GLFWwindow *,A...)>::prev_;

glfw_synapse<glfw_signals::window_close> s1(&glfwSetWindowCloseCallback);
glfw_synapse<glfw_signals::window_size> s2(&glfwSetWindowSizeCallback);
glfw_synapse<glfw_signals::framebuffer_size> s3(&glfwSetFramebufferSizeCallback);
glfw_synapse<glfw_signals::window_pos> s4(&glfwSetWindowPosCallback);
glfw_synapse<glfw_signals::window_iconify> s5(&glfwSetWindowIconifyCallback);
glfw_synapse<glfw_signals::window_focus> s6(&glfwSetWindowFocusCallback);
glfw_synapse<glfw_signals::window_refresh> s7(&glfwSetWindowRefreshCallback);
glfw_synapse<glfw_signals::key> s8(&glfwSetKeyCallback);
glfw_synapse<glfw_signals::character> s9(&glfwSetCharCallback);
glfw_synapse<glfw_signals::character_modifier> s10(&glfwSetCharModsCallback);
glfw_synapse<glfw_signals::cursor_pos> s11(&glfwSetCursorPosCallback);
glfw_synapse<glfw_signals::cursor_enter> s12(&glfwSetCursorEnterCallback);
glfw_synapse<glfw_signals::mouse_button> s13(&glfwSetMouseButtonCallback);
glfw_synapse<glfw_signals::scroll> s14(&glfwSetScrollCallback);
glfw_synapse<glfw_signals::drop> s15(&glfwSetDropCallback);