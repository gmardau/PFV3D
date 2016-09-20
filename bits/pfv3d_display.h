#ifndef _PFV3D_DISPLAY_H_
#define _PFV3D_DISPLAY_H_

#include <signal.h>
#include <math.h>
#include <algorithm>
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

class pfv3d_display
{
	#define DMAX std::numeric_limits<double>::max()
	#define DMIN std::numeric_limits<double>::lowest()
	typedef std::list<Triangle> _List_T;


	/* === Variables === */
	/* PFV3D Data */
	bool *_oo;
	_List_T *_triangles;

	/* Window */
	SDL_Window *_window;
	SDL_GLContext _context;
	int _window_w, _window_h, _screen_w, _screen_h, _program;

	/* Thread */
	bool _running = 1;
	std::thread _renderer;
	std::recursive_mutex _mutex_cond, _mutex_data;
	std::condition_variable_any _cond_main, _cond_renderer;

	/* Object */
	double _obj_span;
	glm::dvec3 _obj_initial_look, _obj_look;
	glm::dquat _obj_initial_quat, _obj_quat;
	glm::dmat4 _obj_rotate, _obj_translate;

	/* Camera */
	double _cam_fov, _cam_radius, _cam_zoom = 1.1;
	glm::dvec4 _cam_initial_view, _cam_initial_up = glm::vec4(0, 0, 1, 1);
	glm::dvec3 _cam_initial_look, _cam_view, _cam_look, _cam_up;
	glm::dquat _cam_initial_quat, _cam_quat;
	glm::dmat4 _cam_rotate, _cam_translate;

	/* Mouse */
	bool _left_button = 0, _right_button = 0;
	double _mouse_x, _mouse_y, _mouse_sens = 2.0/3.0;

	/* Keyboard */
	bool _key_ctrl = 0, _key_alt = 0, _key_shift = 0;

	/* Flags and objects data */
	bool _mode = 0;
	uint _n_vertices = 0, _vao = 0, _vbo_vertices = 0, _vbo_indices = 0;
	int *_indices = nullptr;
	glm::dvec3 *_centres = nullptr;
	/* === Variables === */


	/* === Constructor/Destructor === */
	public:
	pfv3d_display (bool *oo, _List_T *triangles) : _oo(oo), _triangles(triangles)
	{
		/* Window */
		Display *display = XOpenDisplay(NULL);
		Screen *screen = DefaultScreenOfDisplay(display);
		_window_w = _screen_w = screen->width;
		_window_h = _screen_h = screen->height;
		XCloseDisplay(display);

		/* SDL */
		SDL_Init(SDL_INIT_VIDEO);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
		_window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _window_w, _window_h,
		                           SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
		_context = SDL_GL_CreateContext(_window);
		SDL_SetWindowTitle(_window, "Pareto Frontier Visualiser for 3 Dimensions");

		/* GLEW */
		glewExperimental = GL_TRUE;
		glewInit();

		/* Open GL */
		glEnable(GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_DEPTH_TEST);

		/* Vertex Shader */
		const char *vertex_src = "\
			#version 330\n\
			layout (location = 0) in vec3 position;\
			layout (location = 1) in vec3 v_normal;\
			uniform mat4 model, view, projection;\
			out vec3 f_normal, f_position;\
			void main() {\
				f_normal = mat3(transpose(inverse(model))) * v_normal;\
				f_position = vec3(model * vec4(position, 1.0f));\
				gl_Position = projection * view * model * vec4(position, 1.0);\
			}";
		int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex_shader, 1, (const GLchar**)&vertex_src, NULL);
		glCompileShader(vertex_shader);

		/* Fragment Shader */
		const char *fragment_src = "\
			#version 330\n\
			in vec3 f_normal, f_position;\
			uniform mat4 model, obj_rotate;\
			uniform vec4 light_colour, object_colour;\
			uniform vec3 view_position;\
			out vec4 colour;\
			void main(){\
				/*vec3 direction = normalize(vec3(obj_rotate * vec4(0.5, 2, 3.5, 1)));*/\
				vec3 direction = normalize(vec3(obj_rotate * vec4(1, 2, 3, 1)));\
				vec3 light_position = vec3(model * vec4(7.5, 7, 8, 1));\
				vec3 light_direction = normalize(light_position-f_position);\
				vec3 view_direction = normalize(view_position - f_position);\
				vec3 reflect_direction = reflect(-light_direction, f_normal);\
				float spec = pow(max(dot(view_direction, reflect_direction), 0.0), 2);\
				\
				vec4 ambient = 0.1f * light_colour;\
				vec4 diffuse = max(dot(normalize(f_normal), direction), 0.0) * light_colour;\
				vec4 specular = 0.5f * spec * light_colour;\
				\
				colour = (ambient + diffuse) * object_colour;\
				/*colour = vec4(vec3(ambient + diffuse), 1) * object_colour;*/\
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

		/* Generate Buffers */
		glGenVertexArrays(1, &_vao);
		glBindVertexArray(_vao);
		glGenBuffers(1, &_vbo_vertices);
		glGenBuffers(1, &_vbo_indices);

		/* Create display thread */
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
		glDeleteVertexArrays(1, &_vao);
		glDeleteBuffers(1, &_vbo_vertices);
		glDeleteBuffers(1, &_vbo_indices);
		glDeleteProgram(_program);
		SDL_GL_DeleteContext(_context);
		SDL_DestroyWindow(_window);
		SDL_Quit();
	}
	/* === Constructor/Destructor === */


	private:
	void _process_triangles (double limits[3][2], double *vertices, int d, size_t begin, size_t len)
	{
		int i, j, v = begin*18, c = begin-1;
		double centre[3], normal[3] = {0, 0, 0}; normal[d] = 1;
		for(_List_T::iterator it = _triangles[d].begin(); it != _triangles[d].end(); ++it) {
			centre[0] = centre[1] = centre[2] = 0;
			for(i = 0; i < 3; ++i) {
				for(j = 0; j < 3; ++j, ++v) {
					centre[j] += vertices[v] = it->_v[i]->_x[j];
					if(vertices[v] < limits[j][0]) limits[j][0] = vertices[v];
					if(vertices[v] > limits[j][1]) limits[j][1] = vertices[v];
				}
				for(j = 0; j < 3; ++j, ++v) vertices[v] = normal[j];
			}
			_centres[++c] = glm::dvec3(centre[0]/3.0, centre[1]/3.0, centre[2]/3.0);
		}
	}

	private:
	void
	_alert_renderer (bool mode)
	{
		_mode = mode;
		_mutex_cond.lock();
		_cond_renderer.notify_one();
		if(_mode == 0) _cond_main.wait(_mutex_cond);
		_mutex_cond.unlock(); }

	/* === Activate frontier display === */
	public:
	void display_frontier (bool mode, bool reset)
	{
		size_t n_triangles012[3] = {_triangles[0].size(), _triangles[1].size(), _triangles[2].size()};
		size_t n_triangles = n_triangles012[0] + n_triangles012[1] + n_triangles012[2];

		/* If there is no triangles to display - alert the redenrer and exit */
		if(n_triangles == 0) { _n_vertices = 0; _alert_renderer(mode); _mode = 0; return; }
		
		/* Get data */
		int i;
		/* Size = (n_triangles * 3 vertices) * 3 dimensions * (coordinates + normals) */
		size_t size_v = (_n_vertices = n_triangles * 3) * 3 * 2 * sizeof(double);
		double *vertices = (double *) malloc(size_v);
		double tmp_limits[3][3][2] = {{{DMAX, DMIN}, {DMAX, DMIN}, {DMAX, DMIN}},
		                              {{DMAX, DMIN}, {DMAX, DMIN}, {DMAX, DMIN}},
		                              {{DMAX, DMIN}, {DMAX, DMIN}, {DMAX, DMIN}}};
		_centres = (glm::dvec3 *) realloc(_centres, n_triangles*sizeof(glm::dvec3));

		std::thread workers[3] = {
			std::thread(&pfv3d_display::_process_triangles, this, tmp_limits[0], vertices, 0, 0,               n_triangles012[0]),
			std::thread(&pfv3d_display::_process_triangles, this, tmp_limits[1], vertices, 1, n_triangles012[0],         n_triangles012[1]),
			std::thread(&pfv3d_display::_process_triangles, this, tmp_limits[2], vertices, 2, n_triangles012[0]+n_triangles012[1], n_triangles012[2])	
		};
		workers[0].join(); workers[1].join(); workers[2].join();
		double limits[3][2];
		for(i = 0; i < 3; ++i) {
			limits[i][0] = fmin(tmp_limits[0][i][0], fmin(tmp_limits[1][i][0], tmp_limits[2][i][0]));
			limits[i][1] = fmax(tmp_limits[0][i][1], fmax(tmp_limits[1][i][1], tmp_limits[2][i][1]));
		}






		_mutex_data.lock();
		size_t size_indices = _n_vertices*sizeof(int);
		_indices = (int *) realloc(_indices, size_indices);

		/* Bind data */
		glBindBuffer(GL_ARRAY_BUFFER, _vbo_vertices);
		glBufferData(GL_ARRAY_BUFFER, size_v, vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, 6*sizeof(double), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_DOUBLE, GL_FALSE, 6*sizeof(double), (GLvoid*)(3*sizeof(double)));
		glEnableVertexAttribArray(1);

		/* Initialise object and camera variables */
		_obj_span = glm::distance(glm::dvec2(limits[0][1], limits[1][0]), glm::dvec2(limits[0][0], limits[1][1]));
		_obj_span = fmax(glm::distance(glm::dvec3(limits[0][0], limits[1][0], limits[2][1]),
		                              glm::dvec3(limits[0][1], limits[1][1], limits[2][0])), _obj_span);
		
		for(i = 0; i < 3; ++i) _obj_initial_look[i] = _cam_initial_look[i] = (limits[i][0]+limits[i][1])/2.0;
		
		_obj_initial_quat = glm::quat(1, 0, 0, 0);
		double angle; glm::dvec3 axis = glm::vec3(0, 1, 0);
		if(_oo[2] == 0) angle = -M_PI/4.0;
		else               angle =  M_PI/4.0;
		_cam_initial_quat = glm::dquat(cos(angle/2.0), axis * sin(angle/2.0));
		axis[1] = 0; axis[2] = 1; angle = M_PI/4.0;
		if(_oo[1] == 1) angle += M_PI;
		if(_oo[0] != _oo[1]) angle += M_PI/2.0;
		_cam_initial_quat = glm::dquat(cos(angle/2.0), axis * sin(angle/2.0)) * _cam_initial_quat;
		
		if(_mode == 0 || reset == 1) _reset();

		blend_sort();

		_mutex_data.unlock();

		free(vertices);

		_alert_renderer(mode);
	}
	/* === Activate frontier display === */

	/* === Reset object and camera variables === */
	private:
	void _reset ()
	{
		_cam_fov = M_PI/4.0;
		if(_window_w >= _window_h) _cam_radius = _obj_span/2.0 / sin(_cam_fov/2.0)                     * 1.1;
		else                       _cam_radius = _obj_span/2.0 / sin(_cam_fov/2.0*_window_w/_window_h) * 1.1;
		_cam_initial_view = glm::dvec4(_cam_radius, 0, 0, 1);

		_obj_look = _obj_initial_look; _obj_translate = glm::translate(glm::dmat4(1), _obj_look);
		_cam_look = _cam_initial_look; _cam_translate = glm::translate(glm::dmat4(1), _cam_look);
		_obj_quat = _obj_initial_quat; _obj_rotate = glm::mat4_cast(_obj_quat);
		_cam_quat = _cam_initial_quat; _cam_rotate = glm::mat4_cast(_cam_quat);
	}
	/* === Reset object and camera variables === */

	/* === Sort triangles by distance to the camera === */
	private:
	void blend_sort ()
	{
		_mutex_data.lock();
		int i, j;
		double *distances = (double *) malloc(_n_vertices/3*sizeof(double));
		int *indices = (int *) malloc(_n_vertices/3*sizeof(int));
		_cam_view = glm::dvec3(_cam_translate * _cam_rotate * _cam_initial_view);

		for(i = 0; i < (int)_n_vertices/3; ++i) {
			indices[i] = i;
			distances[i] = glm::distance(glm::dvec3(_obj_rotate * glm::dvec4(_centres[i], 1)), _cam_view);
		}
		std::sort(&indices[0], &indices[_n_vertices/3],[&](size_t a, size_t b) { return distances[a] > distances[b]; } );

		for(i = 0; i < (int)_n_vertices/3; ++i) for(j = 0; j < 3; ++j) _indices[i*3+j] = indices[i]*3+j;

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_indices);
	    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _n_vertices*sizeof(int), _indices, GL_STATIC_DRAW);
		_mutex_data.unlock();
	}
	/* === Sort triangles by distance to the camera === */

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
						case SDL_KEYUP: case SDL_KEYDOWN: keyboard(event); break;
						case SDL_WINDOWEVENT:
							if(event.window.event == SDL_WINDOWEVENT_RESIZED) {
								_window_w = event.window.data1; _window_h = event.window.data2; }
							break;
						case SDL_QUIT: rendering = 0; SDL_HideWindow(_window); break;
					}
				draw();
			}
			_mutex_cond.lock();
			if(_mode == 0) _cond_main.notify_one();
			if(!_running) _mutex_cond.unlock();
		}
		pthread_exit(NULL);
	}
	/* === Renderer thread === */

	/* === Drawing function === */
	private:
	void draw ()
	{
		_mutex_data.lock();

		/* Reset display */
		glUseProgram(_program);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, _window_w, _window_h);

		/* Matrix transformations - Model, View, Projection */
		glm::mat4 model = glm::dmat4(1) * _obj_translate * _obj_rotate * glm::inverse(_obj_translate);
		_cam_view = glm::dvec3(_cam_translate * _cam_rotate * _cam_initial_view);
		_cam_up = glm::dvec3(_cam_rotate * _cam_initial_up);
		glm::mat4 view = glm::lookAt(_cam_view, _cam_look, _cam_up);
		glm::mat4 projection = glm::perspective(_cam_fov, (double)_window_w/_window_h, (double)0.001, (double)1000);

		int obj_rotate_location = glGetUniformLocation(_program, "obj_rotate");
		glUniformMatrix4fv(obj_rotate_location, 1, GL_FALSE, &glm::mat4(_obj_rotate)[0][0]);

		int      model_location = glGetUniformLocation(_program, "model");
		int       view_location = glGetUniformLocation(_program, "view");
		int projection_location = glGetUniformLocation(_program, "projection");
		glUniformMatrix4fv(     model_location, 1, GL_FALSE, &model[0][0]);
		glUniformMatrix4fv(      view_location, 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(projection_location, 1, GL_FALSE, &projection[0][0]);

		/* Camera Position */
		int view_position_location = glGetUniformLocation(_program, "view_position");
		glUniform3f(view_position_location, _cam_view[0], _cam_view[1], _cam_view[2]);
		/* Object Colour */
		int object_colour_location = glGetUniformLocation(_program, "object_colour");
		// glUniform4f(object_colour_location, 1, 0.1, 0.05, 0.75);
		glUniform4f(object_colour_location, 0.05, 1, 0.1, 0.75);
		// glUniform4f(object_colour_location, 0.05, 0.1, 1, 0.75);
		// glUniform4f(object_colour_location, 1, 0.85, 0, 0.75);
		// glUniform4f(object_colour_location, 0.75, 0.75, 0.75, 0.75);
		// glUniform4f(object_colour_location, 0.8, 0.5, 0.2, 0.75);
		/* Light Colour */
		int light_colour_location = glGetUniformLocation(_program, "light_colour");
		glUniform4f(light_colour_location, 1, 1, 1, 1);

		/* Draw Triangles */
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		// glBindVertexArray(_vao);
		glDrawElements(GL_TRIANGLES, _n_vertices, GL_UNSIGNED_INT, 0);
		// glBindVertexArray(0);

		// glLineWidth(2);
		// glUniform4f(object_colour_location, 1, 1, 1, 1);
		// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		// glDrawElements(GL_TRIANGLES, _n_vertices, GL_UNSIGNED_INT, 0);

		glUseProgram(0);
		SDL_GL_SwapWindow(_window);

		_mutex_data.unlock();
	}
	/* === Drawing function === */


	/* === Keyboard handling === */
	private:
	void keyboard (SDL_Event &event)
	{
		switch(event.key.keysym.sym) {
			case SDLK_r: _reset(); break;
			case SDLK_LCTRL:  case SDLK_RCTRL:  _key_ctrl  = event.type == SDL_KEYUP ? 0 : 1; break;
			case SDLK_LALT:   case SDLK_RALT:   _key_alt   = event.type == SDL_KEYUP ? 0 : 1; break;
			case SDLK_LSHIFT: case SDLK_RSHIFT: _key_shift = event.type == SDL_KEYUP ? 0 : 1; break;
		}
	}
	/* === Keyboard handling === */


	/* === Mouse handling === */
	private:
	void mouse (SDL_Event &event)
	{
		SDL_MouseMotionEvent *motion = &event.motion;
		SDL_MouseButtonEvent *button = &event.button;

		switch(event.type) {
			case SDL_MOUSEMOTION:
				if(_left_button == 1) {
					double cpp = sin(_cam_fov/2.0) * _cam_radius * 2 / _window_h;
					glm::dvec4 translation = glm::dvec4(0, -motion->xrel * cpp, motion->yrel*cpp, 1);
					_cam_look += glm::dvec3(glm::mat4_cast(_cam_quat) * translation);
					_cam_translate = glm::translate(glm::dmat4(1), _cam_look);
					blend_sort();
				}
				if(_right_button == 1) {
					if(_key_ctrl == 1) {
						glm::dvec3 axis = glm::normalize(glm::dvec3(_cam_rotate * _cam_initial_view));
						double angle = motion->xrel / (_screen_w * _mouse_sens) * 2*M_PI;
						_cam_quat = glm::dquat(cos(angle/2.0), axis * sin(angle/2.0)) * _cam_quat;
						_cam_rotate = glm::mat4_cast(_cam_quat);
						blend_sort();
					} else if (_key_alt == 1) {
						double distance = glm::length(glm::vec2(motion->xrel, motion->yrel));
						glm::dvec3 axis = glm::normalize(glm::dvec3(0, motion->yrel, motion->xrel));
						axis = glm::normalize(glm::dvec3(_cam_rotate * glm::dvec4(axis, 1)));
						double angle = distance / (_screen_w * _mouse_sens) * 2*M_PI;
						_obj_quat = glm::dquat(cos(angle/2.0), axis * sin(angle/2.0)) * _obj_quat;
						_obj_rotate = glm::mat4_cast(_obj_quat);
						blend_sort();
					} else if(_key_shift == 1) {
						glm::vec3 tmp_up = glm::vec3(glm::inverse(_cam_rotate) * _obj_rotate * _cam_initial_up);
						glm::dvec3 axis = glm::dvec3(0, 0, tmp_up[2] > 0 ? -1 : 1);
						axis = glm::normalize(glm::dvec3(_obj_rotate * glm::dvec4(axis, 1)));
						double angle = motion->xrel / (_screen_w * _mouse_sens) * 2*M_PI;
						_cam_quat = glm::dquat(cos(angle/2.0), axis * sin(angle/2.0)) * _cam_quat;
						_cam_rotate = glm::mat4_cast(_cam_quat);
						blend_sort();
					} else {
						double distance = glm::length(glm::vec2(motion->xrel, -motion->yrel));
						glm::dvec3 axis = glm::normalize(glm::dvec3(0, -motion->yrel, -motion->xrel));
						axis = glm::normalize(glm::dvec3(_cam_rotate * glm::dvec4(axis, 1)));
						double angle = distance / (_screen_w * _mouse_sens) * 2*M_PI;
						_cam_quat = glm::dquat(cos(angle/2.0), axis * sin(angle/2.0)) * _cam_quat;
						_cam_rotate = glm::mat4_cast(_cam_quat);
						blend_sort();
					}
				}
				_mouse_x = motion->x; _mouse_y = motion->y;
				break;

			case SDL_MOUSEBUTTONUP:
				     if(button->button == SDL_BUTTON_LEFT)   _left_button = 0;
				else if(button->button == SDL_BUTTON_RIGHT) _right_button = 0;
				break;

			case SDL_MOUSEBUTTONDOWN:
				     if(button->button == SDL_BUTTON_LEFT)   _left_button = 1; 
				else if(button->button == SDL_BUTTON_RIGHT) _right_button = 1;
				_mouse_x = button->x; _mouse_y = button->y;
				break;

			case SDL_MOUSEWHEEL:
				     if(button->x ==  1) _cam_radius /= _cam_zoom;
				else if(button->x == -1) _cam_radius *= _cam_zoom;
				_cam_initial_view = glm::dvec4(_cam_radius, 0, 0, 1);
				//      if(button->x ==  1) _cam_fov = fmax(_cam_fov / _cam_zoom, M_PI/512);
				// else if(button->x == -1) _cam_fov = fmin(_cam_fov * _cam_zoom, 3*M_PI/4);
				break;
		}
	}
	/* === Mouse handling === */
};

#endif
