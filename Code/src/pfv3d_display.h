class pfv3d_display
{
	/* === Variables === */
	/* PFV3D Data */
	bool *__oo;
	_Set_P *__vertices;
	_List_T *__triangles;

	/* Window */
	SDL_Window *_window;
	SDL_GLContext _context;
	int _window_w, _window_h, _screen_w, _screen_h, _program;

	/* Render thread */
	bool _running = 1;
	std::thread _renderer;
	std::mutex _mutex_cond, _mutex_data;
	std::condition_variable_any _cond_main, _cond_renderer;

	/* Object */
	float _obj_span;
	glm::vec3 _obj_initial_look, _obj_look;
	glm::quat _obj_initial_quat, _obj_quat;
	glm::mat4 _obj_rotate, _obj_translate;

	/* Camera */
	float _cam_fov, _cam_radius, _cam_zoom = 1.1;
	glm::vec4 _cam_initial_view, _cam_initial_up = glm::vec4(0, 0, 1, 1);
	glm::vec3 _cam_initial_look, _cam_view, _cam_look, _cam_up;
	glm::quat _cam_initial_quat, _cam_quat;
	glm::mat4 _cam_rotate, _cam_translate;

	/* Transformations / Shader data */
	int _model_location, _view_location, _projection_location, _obj_rotate_location, _view_position_location;
	glm::mat4 _mat_model, _mat_view, _mat_projection;

	/* Mouse */
	bool _left_button = 0, _right_button = 0;
	int _mouse_x, _mouse_y;
	float _mouse_sens = 2.0/3.0;

	/* Keyboard */
	bool _key_ctrl = 0, _key_alt = 0, _key_shift = 0;

	/* Flags and objects data */
	bool _mode = 0;
	unsigned int _vao, _vbo_vertices, _vbo_indices;
	int *_indices = nullptr; float *_vertices = nullptr;
	size_t _size_vertices = 0, _size_triangles = 0, _size_allocated = 0, _count_triangles[3];
	float _allocation_factor = 1.5, _limits[3][2];
	glm::vec3 *_centroids = nullptr;
	/* === Variables === */


	/* === Constructor / Destructor === */
	public:
	pfv3d_display (bool *oo, _Set_P *vertices, _List_T *triangles)
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
		SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
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
		glViewport(0, 0, _window_w, _window_h);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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
				vec3 direction = normalize(vec3(obj_rotate * vec4(3, 2, 1, 1)));\
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
		glUseProgram(_program);
		           _model_location = glGetUniformLocation(_program, "model");
		            _view_location = glGetUniformLocation(_program, "view");
		      _projection_location = glGetUniformLocation(_program, "projection");
		      _obj_rotate_location = glGetUniformLocation(_program, "obj_rotate");
		   _view_position_location = glGetUniformLocation(_program, "view_position");
		int object_colour_location = glGetUniformLocation(_program, "object_colour");
		int  light_colour_location = glGetUniformLocation(_program, "light_colour");
		// glUniform4f(object_colour_location, 0, 0, 0, 0.75);
		glUniform4f(object_colour_location, 1, 1, 1, 0.75);
		// glUniform4f(object_colour_location, 1, 0.1, 0.05, 0.75);
		// glUniform4f(object_colour_location, 0.05, 1, 0.1, 0.75);
		// glUniform4f(object_colour_location, 0.05, 0.1, 1, 0.75);
		// glUniform4f(object_colour_location, 1, 0.85, 0, 0.75);
		// glUniform4f(object_colour_location, 0.75, 0.75, 0.75, 0.75);
		// glUniform4f(object_colour_location, 0.8, 0.5, 0.2, 0.75);
		glUniform4f(light_colour_location, 1, 1, 1, 1);

		/* Free */
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);

		/* Generate Buffers (VAO and VBOs) */
		glGenVertexArrays(1, &_vao);     glBindVertexArray(_vao);
		glGenBuffers(1, &_vbo_vertices); glBindBuffer(GL_ARRAY_BUFFER,         _vbo_vertices);
		glGenBuffers(1, &_vbo_indices);  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_indices);
		/* Define VAO */
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (GLvoid*)0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (GLvoid*)(3*sizeof(float)));
		glEnableVertexAttribArray(0); glEnableVertexAttribArray(1);

		/* Create render thread */
		_mutex_cond.lock();
		_renderer = std::thread(&pfv3d_display::_render, this);
		_cond_main.wait(_mutex_cond);
		_mutex_cond.unlock();
	}

	~pfv3d_display ()
	{
		if(_size_allocated > 0) {
			free(_vertices);
			free(_indices);
			free(_distances);
			free(_centroids);
		}
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
	/* === Constructor / Destructor === */


	/* #################################################################### */
	/* ###################### Render thread notifier ###################### */
	/* === Notify the render thread that new data is to be displayed === */
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
	/* === Notify the render thread that new data is to be displayed === */
	/* ###################### Render thread notifier ###################### */
	/* #################################################################### */


	/* ##################################################################### */
	/* ####################### Process frontier data ####################### */
	/* === Pre triangle processing - check if there is enough space allocated === */
	private:
	bool
	_pre_process ()
	{
		/* If there is not enough allocated space - reallocate */
		if(_size_triangles > _size_allocated) {
			_size_allocated = _size_triangles * _allocation_factor;
			_vertices  =     (float *) realloc(_vertices,  _size_allocated * 18 * sizeof(float));
			_indices   =       (int *) realloc(_indices,   _size_allocated *  3 * sizeof(int));
			_distances =     (float *) realloc(_distances, _size_allocated      * sizeof(float));
			_centroids = (glm::vec3 *) realloc(_centroids, _size_allocated      * sizeof(glm::vec3));
			return 1;
		}
		return 0;
	}
	/* === Pre triangle processing - check if there is enough space allocated === */


	/* === Process the triangles and their vertices (get coordinates, normals and triangles centroids) === */
	private:
	template <typename It>
	void
	_process (int x, It it, size_t begin, size_t end)
	{
		int j, k;
		size_t vi = begin * 18, ii = begin * 3;
		float normal[3] = {0, 0, 0}; normal[x] = 1;
		for(size_t i = begin; i < end; ++i, ++it) {
			memset(&_centroids[i], 0, 3*sizeof(float));
			// for(k = 0; k < 3; ++k) _centroids[i][k] = 0;
			for(j = 0; j < 3; ++j) {
				for(k = 0; k < 3; ++k, ++vi) _centroids[i][k] += _vertices[vi] = it->_v[j]->_x[k];
				// for(k = 0; k < 3; ++k, ++vi) _vertices[vi] = normal[k];
				memcpy(&_vertices[vi], normal, 3*sizeof(float)); vi += 3;
			}
			for(k = 0; k < 3; ++k) _centroids[i][k] /= 3.0;
			for(j = 0; j < 3; ++j, ++ii) _indices[ii] = ii;
		}
	}
	/* === Process the triangles and their vertices (get coordinates, normals and triangles centroids) === */


	/* === Define frontier data to be displayed === */
	public:
	void
	display_frontier (bool mode, bool reset)
	{
		int i;

		SDL_GL_MakeCurrent(_window, _context);

		_mutex_data.lock();

		/* Update number of triangles */
		for(i = 0; i < 3; ++i) _count_triangles[i] = __triangles[i].size();
		_size_triangles = _count_triangles[0] + _count_triangles[1] + _count_triangles[2];

		/* If there is no triangles to display - notify the renderer and exit */
		if(_size_triangles == 0) { _size_vertices = 0; _mutex_data.unlock(); _notify_renderer(mode); _mode = 0; return; }

		int j;
		size_t begin = 0, end = 0;
		std::thread threads[3][2];

		_size_vertices = _size_triangles * 3;

		/* Process triangles */
		bool allocation = _pre_process();
		for(i = 0; i < 3; ++i) {
			begin = end; end += (size_t)(_count_triangles[i]/2.0+0.5);
			threads[i][0] = std::thread(&pfv3d_display::_process<_List_T::iterator>,
				this, i, __triangles[i]. begin(), begin, end);
			begin = end; end += (size_t)(_count_triangles[i]/2.0);
			threads[i][1] = std::thread(&pfv3d_display::_process<_List_T::reverse_iterator>,
				this, i, __triangles[i].rbegin(), begin, end);
		}
		for(i = 0; i < 3; ++i) for(j = 0; j < 2; ++j) threads[i][j].join();

		/* Update frontier limits */
		for(i = 0; i < 3; ++i) {
			for(_Set_P::iterator it = __vertices[i].begin(); it != __vertices[i].end(); ++it)
				if((*it)->_n_tri > 0) { _limits[i][__oo[i]]   = (*it)->_x[i]; break; }
			for(_Set_P::reverse_iterator it = __vertices[i].rbegin(); it != __vertices[i].rend(); ++it)
				if((*it)->_n_tri > 0) { _limits[i][__oo[i]^1] = (*it)->_x[i]; break; }
		}

		/* - Update display data - */
		/* Reallocate space for vertices and indices buffers */
		if(allocation) {
			glBufferData(GL_ARRAY_BUFFER, _size_allocated * 18 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
	    	glBufferData(GL_ELEMENT_ARRAY_BUFFER, _size_allocated * 3 * sizeof(int), NULL, GL_DYNAMIC_DRAW);
		}
		/* Update vertices buffer data */
		glBufferSubData(GL_ARRAY_BUFFER, 0, _size_triangles * 18 * sizeof(float), _vertices);

		/* - Initialise object and camera variables - */
		/* Span of the object (longest internal diagonal) */
		_obj_span = glm::distance(glm::vec3(_limits[0][0], _limits[1][0], _limits[2][0]),
		                          glm::vec3(_limits[0][1], _limits[1][1], _limits[2][1]));
		/* Centre of the object */
		for(i = 0; i < 3; ++i) _obj_initial_look[i] = _cam_initial_look[i] = (_limits[i][0]+_limits[i][1])/2.0;
		/* Object initial rotation */
		_obj_initial_quat = glm::quat(1, 0, 0, 0);
		/* Camera rotation according to optimisation order (first vertically (3 lines), then horizontally (5 lines)) */
		glm::vec3 axis = glm::vec3(0, 1, 0);
		float angle = __oo[2] == 0 ? -M_PI/4.0 : M_PI/4.0;
		_cam_initial_quat = glm::quat(cosf(angle/2.0), axis * sinf(angle/2.0));
		axis[1] = 0; axis[2] = 1;
		angle = M_PI/4.0;
		if(__oo[1] == 1) angle += M_PI;
		if(__oo[0] != __oo[1]) angle += M_PI/2.0;
		_cam_initial_quat = glm::quat(cosf(angle/2.0), axis * sinf(angle/2.0)) * _cam_initial_quat;

		/* Reset scene (if required) */
		if(_mode == 0 || reset == 1) _reset();

		/* Update matrix transformations */
		_update_all();

		/* Sort triangles by the distance to the camera */
		_blend_sort();

		_mutex_data.unlock();

		/* Notify renderer */
		_notify_renderer(mode);
	}
	/* === Define frontier data to be displayed === */
	/* ####################### Process frontier data ####################### */
	/* ##################################################################### */


	/* ##################################################################### */
	/* ############################### Reset ############################### */
	/* === Reset scene - set object and camera variables to their initial state === */
	private:
	void
	_reset ()
	{
		_cam_fov = M_PI/4.0;
		if(_window_w >= _window_h) _cam_radius = _obj_span/2.0 / sinf(_cam_fov/2.0)                     * 1.1;
		else                       _cam_radius = _obj_span/2.0 / sinf(_cam_fov/2.0*_window_w/_window_h) * 1.1;
		_cam_initial_view = glm::vec4(_cam_radius, 0, 0, 1);

		_obj_look = _obj_initial_look; _obj_translate = glm::translate(glm::mat4(1), _obj_look);
		_cam_look = _cam_initial_look; _cam_translate = glm::translate(glm::mat4(1), _cam_look);
		_obj_quat = _obj_initial_quat; _obj_rotate = glm::mat4_cast(_obj_quat);
		_cam_quat = _cam_initial_quat; _cam_rotate = glm::mat4_cast(_cam_quat);
	}
	/* === Reset scene - set object and camera variables to their initial state === */
	/* ############################### Reset ############################### */
	/* ##################################################################### */


	/* ##################################################################### */
	/* ########################### Blend Sorting ########################### */
	/* === Orientation predicate === */
	private:
	inline
	float
	_orientation (glm::vec2 &s1, glm::vec2 &s2, glm::vec2 &v)
	{ return (s1[0]-v[0]) * (s2[1]-v[1]) - (s1[1]-v[1]) * (s2[0]-v[0]); }
	/* === Orientation predicate === */

	/* === Square distance === */
	private:
	inline
	float
	_square_distance (glm::vec2 &a, glm::vec2 &b)
	{ double dx = b[0]-a[0], dy = b[1]-a[1]; return dx*dx + dy*dy; }
	/* === Square distance === */

	/* === Vertex to line-segment distance === */
	private:
	inline
	float
	_vertex_segment (glm::vec2 &v, glm::vec2 &s1, glm::vec2 &s2)
	{
		float t = ((v[0]-s1[0])*(s2[0]-s1[0]) + (v[1]-s1[1])*(s2[1]-s1[1])) / _square_distance(s1, s2);
		if(t < 0) return glm::distance(v, s1);
		if(t > 1) return glm::distance(v, s2);
		return fabsf((s2[1]-s1[1])*v[0] - (s2[0]-s1[0])*v[1] + s2[0]*s1[1] - s2[1]*s1[0]) / glm::distance(s1, s2);
	}
	/* === Vertex to line-segment distance === */


	/* === Shortest distance to triangle === */
	private:
	void
	_compute_distance_shortest (int x0, int x1, int x2, size_t begin, size_t end)
	{
		int j;
		float tmp_distance;
		size_t vi = begin * 18;
		glm::vec2 projection = glm::vec2(_cam_view[x1], _cam_view[x2]), triangle[3];
		for(size_t i = begin; i < end; ++i, vi += 18) {
			for(j = 0; j < 3; ++j) triangle[j] = glm::vec2(_vertices[vi+j*6+x1], _vertices[vi+j*6+x2]);
			_distances[i] = fabsf(_cam_view[x0] - _vertices[vi+x0]);
			if(_orientation(triangle[0], triangle[1], projection) < 0 ||
			   _orientation(triangle[1], triangle[2], projection) < 0 ||
			   _orientation(triangle[2], triangle[0], projection) < 0) {
				tmp_distance = fminf(_vertex_segment(projection, triangle[0], triangle[1]),
					           fminf(_vertex_segment(projection, triangle[1], triangle[2]),
					                 _vertex_segment(projection, triangle[2], triangle[0])));
				_distances[i] = sqrtf(_distances[i]*_distances[i] + tmp_distance*tmp_distance);
			}
		}
	}
	/* === Shortest distance to triangle === */

	/* === Average distance to triangle's points === */
	private:
	void
	_compute_distance_points (size_t begin, size_t end)
	{
		int j;
		size_t vi = begin * 18;
		for(size_t i = begin; i < end; ++i) {
			for(_distances[i] = 0, j = 0; j < 3; ++j, vi += 6)
				_distances[i] += glm::distance(glm::vec3(_vertices[vi], _vertices[vi+1], _vertices[vi+2]), _cam_view);
			_distances[i] /= 3.0;
		}
	}
	/* === Average distance to triangle's points === */

	/* === Distance to triangle's centroid === */
	private:
	void
	_compute_distance_centroid (size_t begin, size_t end)
	{ for(size_t i = begin; i < end; ++i) _distances[i] = glm::distance(_centroids[i], _cam_view); }
	/* === Distance to triangle's centroid === */


	/* === Sort triangles by the distance to the camera === */
	private:
	void
	_blend_sort ()
	{
		int i, j, size_threads = 2;
		size_t begin, end = 0;
		std::thread threads[3][2];

		/* Compute distance to triangles */
		for(i = 0; i < 3; ++i)
			for(j = 0; j < size_threads; ++j) {
				begin = end;
				end += (size_t)(_count_triangles[i]*(1.0*(j+1)/size_threads)) -
				       (size_t)(_count_triangles[i]*(1.0*j/size_threads));
				// threads[i][j] = std::thread(&pfv3d_display::_compute_distance_shortest, this,
				//                             i, (i+1)%3, (i+2)%3, begin, end);
				// threads[i][j] = std::thread(&pfv3d_display::_compute_distance_points, this, begin, end);
				threads[i][j] = std::thread(&pfv3d_display::_compute_distance_centroid, this, begin, end);
			}
		for(i = 0; i < 3; ++i) for(j = 0; j < size_threads; ++j) threads[i][j].join();

		/* Sort triangles by distance */
		qsort(_indices, _size_triangles, 3 * sizeof(int), (int(*)(const void*,const void*))_blend_compare);

		/* Update indices buffer data */
	    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, _size_triangles * 3 * sizeof(int), _indices);
	}
	/* === Sort triangles by the distance to the camera === */
	/* ########################### Blend Sorting ########################### */
	/* ##################################################################### */


	/* ##################################################################### */
	/* ###################### Transformation Matrices ###################### */
	/* === Update all transformation matrices === */
	private:
	void _update_all () { _update_model(); _update_view_rt(); _update_projection(); }
	/* === Update all transformation matrices === */


	/* === Update model transformation matrix === */
	private:
	void
	_update_model ()
	{
		_mat_model = glm::mat4(1) * _obj_translate * _obj_rotate * glm::inverse(_obj_translate);
		glUniformMatrix4fv(_obj_rotate_location, 1, GL_FALSE, &glm::mat4(_obj_rotate)[0][0]);
		glUniformMatrix4fv(_model_location, 1, GL_FALSE, &_mat_model[0][0]);
	}
	/* === Update model transformation matrix === */

	/* === Update view transformation matrix (with rotation) === */
	private:
	void
	_update_view_rt ()
	{
		_cam_view = glm::vec3(_cam_translate * _cam_rotate * _cam_initial_view);
		_cam_up = glm::vec3(_cam_rotate * _cam_initial_up);
		_mat_view = glm::lookAt(_cam_view, _cam_look, _cam_up);
		glUniform3fv(_view_position_location, 1, &glm::vec3(_cam_view)[0]);
		glUniformMatrix4fv(_view_location, 1, GL_FALSE, &_mat_view[0][0]);
	}
	/* === Update view transformation matrix (with rotation) === */

	/* === Update view transformation matrix (without rotation) === */
	private:
	void
	_update_view_rf ()
	{
		_cam_view = glm::vec3(_cam_translate * _cam_rotate * _cam_initial_view);
		_mat_view = glm::lookAt(_cam_view, _cam_look, _cam_up);
		glUniform3fv(_view_position_location, 1, &glm::vec3(_cam_view)[0]);
		glUniformMatrix4fv(_view_location, 1, GL_FALSE, &_mat_view[0][0]);
	}
	/* === Update view transformation matrix (without rotation) === */

	/* === Update projection transformation matrix === */
	private:
	void
	_update_projection ()
	{
		_mat_projection = glm::perspective(_cam_fov, (float)_window_w/_window_h, (float)0.001, (float)1000.0);
		glUniformMatrix4fv(_projection_location, 1, GL_FALSE, &_mat_projection[0][0]);
	}
	/* === Update projection transformation matrix === */
	/* ###################### Transformation Matrices ###################### */
	/* ##################################################################### */


	/* ##################################################################### */
	/* ############################# Rendering ############################# */
	/* === Render thread function === */
	private:
	void
	_render ()
	{
		bool rendering;
		SDL_Event event;

		SDL_GL_MakeCurrent(_window, _context);

		_mutex_cond.lock();
		_cond_main.notify_one();
		while(_running) {
			/* Wait for main thread to notify */
			_cond_renderer.wait(_mutex_cond);
			SDL_ShowWindow(_window);
			_mutex_cond.unlock();
			if(!_running) break;

			rendering = 1;
			while(_running && rendering) {
				/* Manage events */
				while(SDL_PollEvent(&event))
					switch(event.type) {
						case SDL_MOUSEMOTION: case SDL_MOUSEBUTTONUP: case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEWHEEL:
							_mouse(event); break;
						case SDL_KEYUP: case SDL_KEYDOWN: _keyboard(event); break;
						case SDL_WINDOWEVENT:
							if(event.window.event == SDL_WINDOWEVENT_RESIZED) {
								_window_w = event.window.data1; _window_h = event.window.data2;
								_mutex_data.lock();
								glViewport(0, 0, _window_w, _window_h);
								_update_projection();
								_mutex_data.unlock();
							}
							break;
						case SDL_QUIT: rendering = 0; SDL_HideWindow(_window); break;
					}
				/* Draw scene */
				_draw();
			}
			_mutex_cond.lock();
			if(_mode == 0) _cond_main.notify_one();
			if(!_running) _mutex_cond.unlock();
		}
	}
	/* === Renderer thread function === */


	/* === Drawing function === */
	private:
	void
	_draw ()
	{
		_mutex_data.lock();

		/* Reset display */
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* Draw Triangles */
		glDrawElements(GL_TRIANGLES, _size_vertices, GL_UNSIGNED_INT, 0);

		SDL_GL_SwapWindow(_window);

		_mutex_data.unlock();
	}
	/* === Drawing function === */
	/* ############################# Rendering ############################# */
	/* ##################################################################### */


	/* ##################################################################### */
	/* ###################### Keyboard/Mouse handling ###################### */
	/* === Keyboard handling === */
	private:
	void
	_keyboard (SDL_Event &event)
	{
		if(event.key.state == SDL_PRESSED)
			switch(event.key.keysym.sym) {
				/* R key - reset scene */
				case SDLK_r: _mutex_data.lock(); _reset(); _update_all(); _mutex_data.unlock(); break;
				/* Space key - notify main thread */
				case SDLK_SPACE: _mutex_cond.lock(); _cond_main.notify_one(); _mutex_cond.unlock(); break;
			}
		switch(event.key.keysym.sym) {
			case SDLK_LCTRL:  case SDLK_RCTRL:  _key_ctrl  = event.type == SDL_KEYUP ? 0 : 1; break;
			case SDLK_LALT:   case SDLK_RALT:   _key_alt   = event.type == SDL_KEYUP ? 0 : 1; break;
			case SDLK_LSHIFT: case SDLK_RSHIFT: _key_shift = event.type == SDL_KEYUP ? 0 : 1; break;
		}
	}
	/* === Keyboard handling === */


	/* === Mouse handling === */
	private:
	void
	_mouse (SDL_Event &event)
	{
		SDL_MouseMotionEvent *motion = &event.motion;
		SDL_MouseButtonEvent *button = &event.button;

		_mutex_data.lock();
		switch(event.type) {
			case SDL_MOUSEMOTION:
				/* Mouse left-click - move camera */
				if(_left_button == 1) {
					float cpp = sinf(_cam_fov/2.0) * _cam_radius * 2 / _window_h;
					glm::vec4 translation = glm::vec4(0, -motion->xrel * cpp, motion->yrel*cpp, 1);
					_cam_look += glm::vec3(glm::mat4_cast(_cam_quat) * translation);
					_cam_translate = glm::translate(glm::mat4(1), _cam_look);
					_update_view_rf();
					_blend_sort();
				}
				if(_right_button == 1) {
					/* Ctrl + Mouse right-click - camera roll */
					if(_key_ctrl == 1) {
						glm::vec3 axis = glm::normalize(glm::vec3(_cam_rotate * _cam_initial_view));
						float angle = motion->xrel / (_screen_w * _mouse_sens) * 2*M_PI;
						_cam_quat = glm::quat(cosf(angle/2.0), axis * sinf(angle/2.0)) * _cam_quat;
						_cam_rotate = glm::mat4_cast(_cam_quat);
						_update_view_rt();
						_blend_sort();
					}
					/* Alt + Mouse right-click - rotate object */
					else if (_key_alt == 1) {
						float distance = glm::length(glm::vec2(motion->xrel, motion->yrel));
						glm::vec3 axis = glm::normalize(glm::vec3(0, motion->yrel, motion->xrel));
						axis = glm::normalize(glm::vec3(_cam_rotate * glm::vec4(axis, 1)));
						float angle = distance / (_screen_w * _mouse_sens) * 2*M_PI;
						_obj_quat = glm::quat(cosf(angle/2.0), axis * sinf(angle/2.0)) * _obj_quat;
						_obj_rotate = glm::mat4_cast(_obj_quat);
						_update_model();
						_blend_sort();
					}
					/* Shift + Mouse right-click - rotate camera around the z-axis */
					else if(_key_shift == 1) {
						glm::vec3 tmp_up = glm::vec3(glm::inverse(_cam_rotate) * _obj_rotate * _cam_initial_up);
						glm::vec3 axis = glm::vec3(0, 0, tmp_up[2] > 0 ? -1 : 1);
						axis = glm::normalize(glm::vec3(_obj_rotate * glm::vec4(axis, 1)));
						float angle = motion->xrel / (_screen_w * _mouse_sens) * 2*M_PI;
						_cam_quat = glm::quat(cosf(angle/2.0), axis * sinf(angle/2.0)) * _cam_quat;
						_cam_rotate = glm::mat4_cast(_cam_quat);
						_update_view_rt();
						_blend_sort();
					}
					/* Mouse right-click - rotate camera around the object */
					else {
						float distance = glm::length(glm::vec2(motion->xrel, -motion->yrel));
						glm::vec3 axis = glm::normalize(glm::vec3(0, -motion->yrel, -motion->xrel));
						axis = glm::normalize(glm::vec3(_cam_rotate * glm::vec4(axis, 1)));
						float angle = distance / (_screen_w * _mouse_sens) * 2*M_PI;
						_cam_quat = glm::quat(cosf(angle/2.0), axis * sinf(angle/2.0)) * _cam_quat;
						_cam_rotate = glm::mat4_cast(_cam_quat);
						_update_view_rt();
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

			/* Zoom - adjust distance from camera to object (FOV reduction in comments) */
			case SDL_MOUSEWHEEL:
				     if(button->x ==  1) _cam_radius /= _cam_zoom;
				else if(button->x == -1) _cam_radius *= _cam_zoom;
				_cam_initial_view = glm::vec4(_cam_radius, 0, 0, 1);
				_update_view_rf();
				//      if(button->x ==  1) _cam_fov = fmax(_cam_fov / _cam_zoom, M_PI/512);
				// else if(button->x == -1) _cam_fov = fmin(_cam_fov * _cam_zoom, 3*M_PI/4);
				// _update_projection();
				break;
		}
		_mutex_data.unlock();
	}
	/* === Mouse handling === */
	/* ###################### Keyboard/Mouse handling ###################### */
	/* ##################################################################### */
};
