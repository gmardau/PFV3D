#ifndef _PFV3D_DISPLAY_H_
#define _PFV3D_DISPLAY_H_

#include <signal.h>
#include <math.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <X11/Xlib.h>

#define fmod1(x,y) (x)>=0 ? fmod(x,y) : (x)+(y)

class pfv3d_display
{

	typedef std::set<Point *, point_comp<0,1,2>> _Set_P0;
	typedef std::set<Point> _Set_V;
	typedef std::list<Triangle> _List_T;

	/* === Variables === */
	/* Window */
	SDL_Window *_window;
	SDL_GLContext _context;
	int _window_w, _window_h;

	/* Thread */
	bool _running;
	std::thread _renderer;
	std::mutex _mutex_cond, _mutex_data;
	std::condition_variable_any _cond_main, _cond_renderer;

	/* Display */
	int _program;
	glm::vec3 _camera_view = glm::vec3(2, 0, 0), _camera_look = glm::vec3(0, 0, 0), _camera_up = glm::vec3(0, 0, 1);
	float _radius = 2, _zoom_factor = 1.1;
	float _r_x = 0, _r_y = 0, _r_z = 0, _r_xy = 0;

	bool _lb_state = 0, _rb_state = 0;
	float _pm_x, _pm_y;
	float _sensitivity = 0.66;

	/* Flags and objects data */
	bool _mode;
	float *_colours[4];
	unsigned int _data[5];
	int _np, _nt;
	/* === Variables === */

	/* === Constructor/Destructor === */
	public:
	pfv3d_display () : _running(1), _mode(0), _data{0, 0, 0, 0, 0}
	{
		Screen *screen = DefaultScreenOfDisplay(XOpenDisplay(NULL));
		_window_w = screen->width;
		_window_h = screen->height;

		/* SDL */
		SDL_Init(SDL_INIT_EVERYTHING);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		_window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _window_w, _window_h,
			    SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
		_context = SDL_GL_CreateContext(_window);

		/* GLEW */
		glewExperimental = GL_TRUE;
		glewInit();

		/* Open GL */
		glShadeModel(GL_SMOOTH);
		glEnable(GL_BLEND | GL_ALPHA_TEST | GL_DEPTH_TEST | GL_LINE_SMOOTH | GL_POLYGON_SMOOTH | GL_NORMALIZE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		/* Vertex Shader */
		const char *vertex_src = "\
		    #version 330\n\
			layout (location = 0) in vec3 position;\
			uniform mat4 model;\
			uniform mat4 view;\
			uniform mat4 projection;\
		    void main() {\
		    	gl_Position = projection * view * model * vec4(position, 1.0);\
		    }";
		int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex_shader, 1, (const GLchar**)&vertex_src, NULL);
		glCompileShader(vertex_shader);

		/* Fragment Shader */
		const char *fragment_src = "\
		    #version 330\n\
			out vec4 colour;\
			uniform vec4 light_colour;\
			uniform vec4 object_colour;\
	        void main(){\
	        	vec4 ambient = 1.0f * light_colour;\
	        	colour = (ambient) * object_colour;\
	        }";
		int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment_shader, 1, (const GLchar**)&fragment_src, NULL);
		glCompileShader(fragment_shader);

		/* Program */
		_program = glCreateProgram();
		glAttachShader(_program, vertex_shader);
		glAttachShader(_program, fragment_shader);
		glLinkProgram(_program);

		/* Free */
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);

		/* Variables */
		display_clear();

		/* Create display thread */
		signal(SIGINT, exit);
		_mutex_cond.lock();
		_renderer = std::thread(&pfv3d_display::renderer, this);
		_cond_main.wait(_mutex_cond);
		_mutex_cond.unlock();
		
	}
	~pfv3d_display ()
	{
		_running = 0;
		_mutex_cond.lock();
		_cond_renderer.notify_one();
		_mutex_cond.unlock();
		_renderer.join();
		display_clear();
		SDL_DestroyWindow(_window);
		SDL_Quit();
	}
	/* === Constructor/Destructor === */

	/* === Clear display variables and objects data === */
	private:
	void display_clear ()
	{

	}
	/* === Clear display variables and objects data === */

	/* === Activate frontier display === */
	public:
	void display_frontier (bool mode, _Set_P0 &points, _Set_V &vertices, _List_T &triangles)
	{
		// int i = 0, *indices = (int*) malloc(triangles.size()*3*sizeof(int));
		// double *coord = (double*) malloc((points.size()+points.size())*3*sizeof(double));

		// /* Get data */
		// for(_Set_P0::iterator it = points.begin(); it != points.end(); ++it) {
		// 	(*it)->_did = i/3;
		// 	coord[i++] = (*it)->_x[0];
		// 	coord[i++] = (*it)->_x[1];
		// 	coord[i++] = (*it)->_x[2];
		// }
		// for(_Set_V::iterator it = vertices.begin(); it != vertices.end(); ++it) {
		// 	it->_did = i/3;
		// 	coord[i++] = it->_x[0];
		// 	coord[i++] = it->_x[1];
		// 	coord[i++] = it->_x[2];
		// }
		// i = 0;
		// for(_List_T::iterator it = triangles.begin(); it != triangles.end(); ++it) {
		// 	indices[i++] = it->_v[0]->_did;
		// 	indices[i++] = it->_v[1]->_did;
		// 	indices[i++] = it->_v[2]->_did;
		// }

		int indices[6] = {0, 1, 2, 2, 3, 4};
		double coord[15] = {0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1};

		_mutex_data.lock();

		/* Free previously bound data */
		if(_data[0] == 1) {
			glDeleteVertexArrays(1, &_data[2]);
			glDeleteBuffers(1, &_data[3]);
			glDeleteBuffers(1, &_data[4]);
		}

		/* Bind new data */
		glGenVertexArrays(1, &_data[2]);
		glGenBuffers(1, &_data[3]);
		glGenBuffers(1, &_data[4]);
		glBindVertexArray(_data[2]);
		glBindBuffer(GL_ARRAY_BUFFER, _data[3]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(coord), coord, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _data[4]);
	    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, 3*sizeof(double), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		_mutex_data.unlock();

		_data[0] = 1; _data[1] = triangles.size()*3;
		_np = points.size(); _nt = triangles.size();
		_mode = mode;
		// colours[o] = colour;

		_mutex_cond.lock();
		_cond_renderer.notify_one();
		if(_mode == 0) _cond_main.wait(_mutex_cond);
		_mutex_cond.unlock();

		// free(indices); free(coord);
	}
	/* === Activate frontier display === */

	/* === Renderer thread === */
	private:
	void renderer ()
	{
		bool rendering;
		SDL_Event event;

		SDL_GL_MakeCurrent(_window, _context);
		_mutex_cond.lock();
		_cond_main.notify_one();
		while(_running) {
			_cond_renderer.wait(_mutex_cond);
			_mutex_cond.unlock();
			if(!_running) break;

			SDL_ShowWindow(_window);
			rendering = 1;
			while(_running && rendering) {
				while(SDL_PollEvent(&event))
					switch(event.type) {
						case SDL_MOUSEMOTION: case SDL_MOUSEBUTTONUP: case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEWHEEL:
							mouse(event); break;
						case SDL_WINDOWEVENT:
							if(event.window.event == SDL_WINDOWEVENT_RESIZED) {
								_window_w = event.window.data1;
								_window_h = event.window.data2;
							}
							break;
						case SDL_QUIT: rendering = 0; display_clear(); SDL_HideWindow(_window); break;
					}
				draw();
			}
			_mutex_cond.lock();
			if(_mode == 0) _cond_main.notify_one();
			// if(!_running) _mutex_cond.unlock();
		}
		pthread_exit(NULL);
	}
	/* === Renderer thread === */

	/* === Drawing function === */
	private:
	void draw ()
	{
		_mutex_data.lock();

		char title[30];
		sprintf(title, "Points:%d   Triangles:%d", _np, _nt);
		SDL_SetWindowTitle(_window, title);

		/* Reset display */
		glUseProgram(_program);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport (0, 0, _window_w, _window_h);

		/* Matrix transformations - Model, View, Projection */
		glm::mat4 model = glm::mat4(1.0);
		glm::mat4 view = glm::lookAt(_camera_view, _camera_look, _camera_up);
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (GLfloat)_window_w/(GLfloat)_window_h, 0.1f, 1000.0f);
		int      model_location = glGetUniformLocation(_program, "model");
		int       view_location = glGetUniformLocation(_program, "view");
		int projection_location = glGetUniformLocation(_program, "projection");
		glUniformMatrix4fv(     model_location, 1, GL_FALSE, &model[0][0]);
		glUniformMatrix4fv(      view_location, 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(projection_location, 1, GL_FALSE, &projection[0][0]);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		/* Object Colour */
		int object_colour_location = glGetUniformLocation(_program, "object_colour");
		glUniform4f(object_colour_location, 0.1, 0.2, 0.8, 1);
		/* Light Colour */
		int light_colour_location = glGetUniformLocation(_program, "light_colour");
		glUniform4f(light_colour_location, 1, 1, 1, 1);

		/* Draw Triangles */
		glBindVertexArray(_data[2]);
		glDrawElements(GL_TRIANGLES, /*_data[1]*/6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		glUseProgram(0);
		SDL_GL_SwapWindow(_window);

		_mutex_data.unlock();
	}
	/* === Drawing function === */

	/* === Mouse handling === */
	private:
	void mouse (SDL_Event event)
	{
		SDL_MouseMotionEvent *motion = (SDL_MouseMotionEvent *) &event;
		SDL_MouseButtonEvent *button = (SDL_MouseButtonEvent *) &event;
		int mx, my;

		switch(event.type) {

			case SDL_MOUSEMOTION:
				mx = motion->x; my = motion->y;
				if(_rb_state == 1) {
					_r_z = fmod1(_r_z - _camera_up[2] * (mx - _pm_x) / (_window_w*_sensitivity) * 2*M_PI, 2*M_PI);
					_r_xy = fmod1(_r_xy + (my - _pm_y) / (_window_w*_sensitivity) * 2*M_PI, 2*M_PI);
					_camera_view[2] = sin(_r_xy) * _radius;
					_camera_view[0] = cos(_r_z) * cos(_r_xy) * _radius;
					_camera_view[1] = sin(_r_z) * cos(_r_xy) * _radius;
					_pm_x = mx; _pm_y = my;
					if(_r_xy > M_PI/2 && _r_xy < 3*M_PI/2) _camera_up[2] = -1;
					else _camera_up[2] = 1;
				}

				break;
			case SDL_MOUSEBUTTONUP:
				if(button->button == SDL_BUTTON_LEFT)  _lb_state = 0;
				if(button->button == SDL_BUTTON_RIGHT) _rb_state = 0;
				break;

			case SDL_MOUSEBUTTONDOWN:
				if(button->button == SDL_BUTTON_LEFT)  { _lb_state = 1; _pm_x = button->x; _pm_y = button->y; }
				if(button->button == SDL_BUTTON_RIGHT) { _rb_state = 1; _pm_x = button->x; _pm_y = button->y; }
				break;

			case SDL_MOUSEWHEEL:
				if(button->x == 1) _radius /= _zoom_factor;
				else if(button->x == -1) _radius *= _zoom_factor;
				_camera_view[0] = cos(_r_z) * _radius;
				_camera_view[1] = sin(_r_z) * _radius;
				break;
		}
	}
	/* === Mouse handling === */
};

#endif
