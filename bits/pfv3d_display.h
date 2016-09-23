class pfv3d_display
{
	/* === Variables === */
	/* PFV3D Data */
	bool *__oo;
	_Tree_P *__vertices;
	_List_T *__triangles;

	/* Window */
	SDL_Window *_window;
	SDL_GLContext _context;
	int _window_w, _window_h, _screen_w, _screen_h, _program;

	/* Thread */
	bool _running = 1;
	std::thread _renderer;
	std::mutex _mutex_cond;
	std::recursive_mutex _mutex_data;
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
	uint _vao, _vbo_vertices, _vbo_indices;
	int *_indices = nullptr; double *_vertices = nullptr;
	size_t _size_vertices = 0, _size_triangles = 0, _size_allocated = 0;
	double _allocation_factor = 1.5, _limits[3][2];
	/* === Variables === */


	/* === Constructor/Destructor === */
	public:
	pfv3d_display (bool *oo, _Tree_P *vertices, _List_T *triangles)
	    : __oo(oo), __vertices(vertices), __triangles(triangles)
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
		_renderer = std::thread(&pfv3d_display::_render, this);
		_cond_main.wait(_mutex_cond);
		_mutex_cond.unlock();
	}

	~pfv3d_display ()
	{
		free(_vertices);
		free(_indices);
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
	void
	_notify_renderer (bool mode)
	{
		_mode = mode;
		_mutex_cond.lock();
		_cond_renderer.notify_one();
		if(_mode == 0) _cond_main.wait(_mutex_cond);
		_mutex_cond.unlock();
	}

	private:
	bool
	_pre_process ()
	{
		/* If there is not enough allocated space - reallocate */
		if(_size_triangles > _size_allocated) {
			_size_allocated = _size_triangles * _allocation_factor;
			_vertices = (double *) realloc(_vertices, _size_allocated * 18 * sizeof(double));
			_indices  =    (int *) realloc(_indices,  _size_allocated *  3 * sizeof(int));
			return 1;
		}
		return 0;
	}

	private:
	template <typename It>
	void
	_process (int x, It it, size_t begin, size_t end)
	{
		int j, k;
		size_t vi = begin * 18, ii = begin * 3;
		double normal[3] = {0, 0, 0}; normal[x] = 1;
		for(size_t i = begin; i < end; ++i, ++it) {
			it->_did = i;
			for(j = 0; j < 3; ++j) {
				for(k = 0; k < 3; ++k, ++vi) _vertices[vi] = it->_v[j]->_x[k];
				for(k = 0; k < 3; ++k, ++vi) _vertices[vi] = normal[k];
			}
			for(j = 0; j < 3; ++j, ++ii) _indices[ii] = ii;
		}
	}

	/* === Activate frontier display === */
	public:
	void
	display_frontier (bool mode, bool reset)
	{
		size_t count_triangles[3] = {__triangles[0].size(), __triangles[1].size(), __triangles[2].size()};	
		_size_triangles = count_triangles[0] + count_triangles[1] + count_triangles[2];
		
		/* If there is no triangles to display - notify the renderer and exit */
		if(_size_triangles == 0) { _size_vertices = 0; _notify_renderer(mode); _mode = 0; return; }

		int i, j;
		size_t begin = 0, end = 0;
		std::thread threads[3][2];

		/* Process triangles */
		bool allocation = _pre_process();
		for(i = 0; i < 3; ++i) {
			begin = end; end += (size_t)(count_triangles[i]/2.0+0.5);
			threads[i][0] = std::thread(&pfv3d_display::_process<_List_T::iterator>,
				this, i, __triangles[i]. begin(), begin, end);
			begin = end; end += (size_t)(count_triangles[i]/2.0);
			threads[i][1] = std::thread(&pfv3d_display::_process<_List_T::reverse_iterator>,
				this, i, __triangles[i].rbegin(), begin, end);
		}
		for(i = 0; i < 3; ++i) for(j = 0; j < 2; ++j) threads[i][j].join();

		/* Update frontier limits */
		for(i = 0; i < 3; ++i) {
			for(_Tree_P::iterator it = __vertices[i].begin(); !it.is_sentinel(); ++it)
				if((*it)->_n_tri > 0) { _limits[i][__oo[i]]   = (*it)->_x[i]; break; }
			for(_Tree_P::reverse_iterator it = __vertices[i].rbegin(); !it.is_sentinel(); ++it)
				if((*it)->_n_tri > 0) { _limits[i][__oo[i]^1] = (*it)->_x[i]; break; }
		}

		/* Update display data */
		_mutex_data.lock();

		_size_vertices = _size_triangles * 3;

		/* Bind data */
		glBindBuffer(GL_ARRAY_BUFFER, _vbo_vertices);
		if(allocation) glBufferData(GL_ARRAY_BUFFER, _size_allocated * 18 * sizeof(double), NULL, GL_DYNAMIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, _size_triangles * 18 * sizeof(double), _vertices);
		glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, 6*sizeof(double), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_DOUBLE, GL_FALSE, 6*sizeof(double), (GLvoid*)(3*sizeof(double)));
		glEnableVertexAttribArray(1);

		/* Initialise object and camera variables */
		_obj_span = glm::distance(glm::dvec3(_limits[0][0], _limits[1][0], _limits[2][0]),
		                          glm::dvec3(_limits[0][1], _limits[1][1], _limits[2][1]));
		
		for(i = 0; i < 3; ++i) _obj_initial_look[i] = _cam_initial_look[i] = (_limits[i][0]+_limits[i][1])/2.0;
		
		_obj_initial_quat = glm::quat(1, 0, 0, 0);
		double angle;
		glm::dvec3 axis = glm::vec3(0, 1, 0);
		if(__oo[2] == 0) angle = -M_PI/4.0;
		else             angle =  M_PI/4.0;
		_cam_initial_quat = glm::dquat(cos(angle/2.0), axis * sin(angle/2.0));
		axis[1] = 0; axis[2] = 1; angle = M_PI/4.0;
		if(__oo[1] == 1) angle += M_PI;
		if(__oo[0] != __oo[1]) angle += M_PI/2.0;
		_cam_initial_quat = glm::dquat(cos(angle/2.0), axis * sin(angle/2.0)) * _cam_initial_quat;
		
		if(_mode == 0 || reset == 1) _reset();

		_blend_sort();

		_mutex_data.unlock();

		_notify_renderer(mode);
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
	void _blend_sort ()
	{
		_mutex_data.lock();
		// int i;
		// double *distances = (double *) malloc(_size_vertices/3*sizeof(double));
		// int *indices = (int *) malloc(_size_vertices/3*sizeof(int));
		// _cam_view = glm::dvec3(_cam_translate * _cam_rotate * _cam_initial_view);

		// for(i = 0; i < (int)_size_vertices/3; ++i) {
			// indices[i] = i;
			// distances[i] = glm::distance(glm::dvec3(_obj_rotate * glm::dvec4(_centres[i], 1)), _cam_view);
		// }
		// std::sort(&indices[0], &indices[_size_vertices/3],[&](size_t a, size_t b) { return distances[a] > distances[b]; } );

		// for(i = 0; i < (int)_size_vertices/3; ++i) for(int j = 0; j < 3; ++j) _indices[i*3+j] = indices[i]*3+j;

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_indices);
	    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _size_triangles*3*sizeof(int), _indices, GL_DYNAMIC_DRAW);
		_mutex_data.unlock();
	}
	/* === Sort triangles by distance to the camera === */

	/* === Renderer thread function === */
	private:
	void _render ()
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
							_mouse(event); break;
						case SDL_KEYUP: case SDL_KEYDOWN: _keyboard(event); break;
						case SDL_WINDOWEVENT:
							if(event.window.event == SDL_WINDOWEVENT_RESIZED) {
								_window_w = event.window.data1; _window_h = event.window.data2; }
							break;
						case SDL_QUIT: rendering = 0; SDL_HideWindow(_window); break;
					}
				_draw();
			}
			_mutex_cond.lock();
			if(_mode == 0) _cond_main.notify_one();
			if(!_running) _mutex_cond.unlock();
		}
		pthread_exit(NULL);
	}
	/* === Renderer thread function === */

	/* === Drawing function === */
	private:
	void _draw ()
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
		glDrawElements(GL_TRIANGLES, _size_vertices, GL_UNSIGNED_INT, 0);
		// glBindVertexArray(0);

		// glLineWidth(2);
		// glUniform4f(object_colour_location, 1, 1, 1, 1);
		// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		// glDrawElements(GL_TRIANGLES, _size_vertices, GL_UNSIGNED_INT, 0);

		glUseProgram(0);
		SDL_GL_SwapWindow(_window);

		_mutex_data.unlock();
	}
	/* === Drawing function === */


	/* === Keyboard handling === */
	private:
	void _keyboard (SDL_Event &event)
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
	void _mouse (SDL_Event &event)
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
					_blend_sort();
				}
				if(_right_button == 1) {
					if(_key_ctrl == 1) {
						glm::dvec3 axis = glm::normalize(glm::dvec3(_cam_rotate * _cam_initial_view));
						double angle = motion->xrel / (_screen_w * _mouse_sens) * 2*M_PI;
						_cam_quat = glm::dquat(cos(angle/2.0), axis * sin(angle/2.0)) * _cam_quat;
						_cam_rotate = glm::mat4_cast(_cam_quat);
						_blend_sort();
					} else if (_key_alt == 1) {
						double distance = glm::length(glm::vec2(motion->xrel, motion->yrel));
						glm::dvec3 axis = glm::normalize(glm::dvec3(0, motion->yrel, motion->xrel));
						axis = glm::normalize(glm::dvec3(_cam_rotate * glm::dvec4(axis, 1)));
						double angle = distance / (_screen_w * _mouse_sens) * 2*M_PI;
						_obj_quat = glm::dquat(cos(angle/2.0), axis * sin(angle/2.0)) * _obj_quat;
						_obj_rotate = glm::mat4_cast(_obj_quat);
						_blend_sort();
					} else if(_key_shift == 1) {
						glm::vec3 tmp_up = glm::vec3(glm::inverse(_cam_rotate) * _obj_rotate * _cam_initial_up);
						glm::dvec3 axis = glm::dvec3(0, 0, tmp_up[2] > 0 ? -1 : 1);
						axis = glm::normalize(glm::dvec3(_obj_rotate * glm::dvec4(axis, 1)));
						double angle = motion->xrel / (_screen_w * _mouse_sens) * 2*M_PI;
						_cam_quat = glm::dquat(cos(angle/2.0), axis * sin(angle/2.0)) * _cam_quat;
						_cam_rotate = glm::mat4_cast(_cam_quat);
						_blend_sort();
					} else {
						double distance = glm::length(glm::vec2(motion->xrel, -motion->yrel));
						glm::dvec3 axis = glm::normalize(glm::dvec3(0, -motion->yrel, -motion->xrel));
						axis = glm::normalize(glm::dvec3(_cam_rotate * glm::dvec4(axis, 1)));
						double angle = distance / (_screen_w * _mouse_sens) * 2*M_PI;
						_cam_quat = glm::dquat(cos(angle/2.0), axis * sin(angle/2.0)) * _cam_quat;
						_cam_rotate = glm::mat4_cast(_cam_quat);
						_blend_sort();
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
