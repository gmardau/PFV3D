#ifndef _PFV3D_H_
#define _PFV3D_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <limits>
#include <functional>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>
#include <set>
#include <unordered_set>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <X11/Xlib.h>

#include "../../../Libraries/CppTreeLib/bits/tree.h"

/* === Visualiser blend stuff === */
float *_distances = nullptr;
int _blend_compare (int *a, int *b)
{ return _distances[*a/3] > _distances[*b/3] ? -1 : (_distances[*a/3] < _distances[*b/3] ? 1 : 0); }
/* === Visualiser blend stuff === */

class pfv3d
{
	#define DMAX std::numeric_limits<double>::max()
	#define DMIN std::numeric_limits<double>::lowest()

	
	/* === Point structure === */
	struct Point {
		const bool _input; bool _optimal = 1;
		int _state = 1;
		size_t _n_tri = 0, _aid = 0;
		double _x[3];
		Point (double x1, double x2, double x3, bool input = 0) : _input(input), _x{x1, x2, x3} {}
		Point (double *x, bool input = 0) : _input(input), _x{x[0], x[1], x[2]} {}
	};
	/* === Point structure === */
	/* === Triangle structure === */
	struct Triangle {
		Point *const _v[3];
		Triangle (Point *v1, Point *v2, Point *v3) : _v{v1, v2, v3} {}
	};
	/* === Triangle structure === */


	typedef std::function<bool (double, double)> _Bin_Func;


	/* === Point compare function === */
	struct point_compare {
		const _Bin_Func *_of;
		const int _x1, _x2, _x3;
		point_compare (_Bin_Func *of, int x1, int x2, int x3) : _of(of), _x1(x1), _x2(x2), _x3(x3) {}
		bool operator() (const Point *p1, const Point *p2) const {
			if(_of[_x1](p1->_x[_x1], p2->_x[_x1])) return 1;; if(_of[_x1](p2->_x[_x1], p1->_x[_x1])) return 0;
			if(_of[_x2](p1->_x[_x2], p2->_x[_x2])) return 1;; if(_of[_x2](p2->_x[_x2], p1->_x[_x2])) return 0;
			if(_of[_x3](p1->_x[_x3], p2->_x[_x3])) return 1;; return 0; } };
	/* === Point compare function === */
	/* === Point equal function === */
	struct point_equal {
		bool operator()(const Point *p1, const Point *p2) const {
			return p1->_x[0] == p2->_x[0] && p1->_x[1] == p2->_x[1] && p1->_x[2] == p2->_x[2]; } };
	/* === Point equal function === */
	/* === Point hash function === */
	struct point_hash {
		size_t operator()(const Point *p) const {
			return size_t(p->_x[0]*73856093) ^ size_t(p->_x[1]*19349669) ^ size_t(p->_x[2]*83492791); } };
	/* === Point hash function === */


	typedef tree<tree_rb, Point *, point_compare> _Tree_P;
	typedef std::unordered_set<Point *> _USet_P;
	typedef std::unordered_set<Point *, point_hash, point_equal> _C_USet_P;
	typedef std::list<Triangle> _List_T;
	typedef std::unordered_multimap<Point *, _List_T::iterator> _UMMap_PTs;	


	/* === Variables === */
	bool _oo[3], _rem_non_optimal;
	int _display_mode;
	double _limits[3][2] = {{DMIN, DMAX}, {DMIN, DMAX}, {DMIN, DMAX}}, _extremes[3];
	_Bin_Func _of[3];

	_C_USet_P _all_points, _all_vertices;

	point_compare _p_comp[3] = {point_compare(_of,0,1,2), point_compare(_of,1,2,0), point_compare(_of,2,0,1)};
	_Tree_P _points[3]     = {_Tree_P(_p_comp[0]), _Tree_P(_p_comp[1]), _Tree_P(_p_comp[2])};
	_Tree_P _add[3]        = {_Tree_P(_p_comp[0]), _Tree_P(_p_comp[1]), _Tree_P(_p_comp[2])};
	_Tree_P _rem[3]        = {_Tree_P(_p_comp[0]), _Tree_P(_p_comp[1]), _Tree_P(_p_comp[2])};
	_Tree_P _vertices[3]   = {_Tree_P(_p_comp[0]), _Tree_P(_p_comp[1]), _Tree_P(_p_comp[2])};
	_Tree_P _projection[3] = {_Tree_P(_p_comp[1]), _Tree_P(_p_comp[2]), _Tree_P(_p_comp[0])};

	_USet_P _non_optimal, _possible_unused_vertices;

	_List_T _triangles[3];
	_UMMap_PTs _p_to_ts[3];
		
	std::thread _threads[3];
	std::mutex _mutex_vertices, _mutex_all_vertices, _mutex_triangles, _mutex_display, _mutex_possible_unused;

	#include "pfv3d_display.h"
	pfv3d_display *_pfv3d_display = new pfv3d_display(_oo, _vertices, _triangles);
	/* === Variables === */


	/* === Constructor/Destructor === */
	public:
	pfv3d (int display_mode = 0, bool rem_non_optimal = 0) : pfv3d(0, 0, 0, display_mode, rem_non_optimal) {}
	pfv3d (bool orientation1, bool orientation2, bool orientation3, int display_mode = 0, bool rem_non_optimal = 0)
	    : _oo{orientation1, orientation2, orientation3}, _rem_non_optimal(rem_non_optimal), _display_mode(display_mode)
	{
		signal(SIGINT, exit);
		for(int i = 0; i < 3; ++i) {
			if(_oo[i] == 0) { _extremes[i] = DMAX; _of[i] = std::less   <double>(); }
			else            { _extremes[i] = DMIN; _of[i] = std::greater<double>(); }
		}
	}
	~pfv3d ()
	{
		for(_C_USet_P::iterator it =   _all_points.begin(); it !=   _all_points.end(); ++it) delete *it;
		for(_C_USet_P::iterator it = _all_vertices.begin(); it != _all_vertices.end(); ++it) delete *it;
	}
	/* === Constructor/Destructor === */


	/* ##################################################################### */
	/* ############################## Display ############################## */
	/* === Call the visualiser === */
	public:
	inline void display () const { _pfv3d_display->display_frontier(0, 1); }

	private:
	inline
	void
	_display ()
	{
		if(_display_mode && _mutex_display.try_lock()) {
			_mutex_triangles.lock();
			_pfv3d_display->display_frontier(1, _display_mode == 1 || _display_mode == 2);
			_mutex_triangles.unlock();
			_mutex_display.unlock();
		}
	}
	/* === Call the visualiser === */


	/* === Define the display mode === */
	public:
	inline void set_display_mode (int display_mode) { _display_mode = display_mode; }
	/* === Define the display mode === */
	/* ############################## Display ############################## */
	/* ##################################################################### */


	/* ##################################################################### */
	/* ############################ Orientation ############################ */
	/* === Define the optimisation orientation === */
	public:
	void
	set_orientation (int dimension, bool orientation)
	{
		if(_oo[dimension] != orientation) {
			_change_orientation(dimension, orientation);
			_reorder(); _clear_frontier();
		}
	}

	public:
	void
	set_orientation (bool orientation1, bool orientation2, bool orientation3)
	{
		bool change = false, oo[3] = {orientation1, orientation2, orientation3};
		for(int i = 0; i < 3; ++i) if(_oo[i] != oo[i]) { change = true; _change_orientation(i, oo[i]); }
		if(change) { _reorder(); _clear_frontier(); }
	}

	private:
	void
	_change_orientation (int dimension, bool orientation)
	{
		_oo[dimension] = orientation;
		_limits[dimension][0] = DMIN; _limits[dimension][1] = DMAX;
		if(orientation == 0) { _extremes[dimension] = DMAX; _of[dimension] = std::less   <double>(); }
		else                 { _extremes[dimension] = DMIN; _of[dimension] = std::greater<double>(); }
	}
	/* === Define the optimisation orientation === */
	/* ############################ Orientation ############################ */
	/* ##################################################################### */


	/* ##################################################################### */
	/* ############################# Auxiliary ############################# */
	/* === Clear frontier === */
	private:
	void
	_clear_frontier ()
	{
		int i;
		_C_USet_P::iterator it;
		Point *point;
		/* Mark every point for addition, except the ones marked for removal, which are deleted */
		for(it = _all_points.begin(); it != _all_points.end(); ) {
			point = *it; ++it;
			/* If point is neither marked for addition nor for removal - mark for addition */
			if(point->_state == 0) {
				point->_state = point->_optimal = 1;
				for(i = 0; i < 3; ++i) _add[i].insert(point);
			}
			/* If point is marked for removal - delete point */
			else if(point->_state == -1) {
				for(i = 0; i < 3; ++i) _points[i].erase(point);
				_all_points.erase(point); delete point;
			}
		}
		/* Clear removal sets */
		for(i = 0; i < 3; ++i) _rem[i].clear();
		/* Delete all vertices */
		for(it = _all_vertices.begin(); it != _all_vertices.end(); ++it) delete *it;
		_all_vertices.clear();
		for(i = 0; i < 3; ++i) _vertices[i].clear();
		/* Delete all triangles */
		for(i = 0; i < 3; ++i) { _triangles[i].clear(); _p_to_ts[i].clear(); }
	}
	/* === Clear frontier === */


	/* === Reorder trees === */
	private:
	void
	_reorder ()
	{
		_Tree_P::iterator it;
		/* Reinsert every point in every tree (order was modified) */
		for(int i = 0; i < 3; ++i) {
			  _points[i].clear(); for(it =   _points[(i+1)%3].begin(); !it.is_sentinel(); ++it)   _points[i].insert(*it);
			     _add[i].clear(); for(it =      _add[(i+1)%3].begin(); !it.is_sentinel(); ++it)      _add[i].insert(*it);
			     _rem[i].clear(); for(it =      _rem[(i+1)%3].begin(); !it.is_sentinel(); ++it)      _rem[i].insert(*it);
			_vertices[i].clear(); for(it = _vertices[(i+1)%3].begin(); !it.is_sentinel(); ++it) _vertices[i].insert(*it);
		}
	}
	/* === Reorder trees === */
	/* ############################# Auxiliary ############################# */
	/* ##################################################################### */


	/* #################################################################### */
	/* ############################## Export ############################## */
	/* === Export data to file === */
	public:
	void
	export_file(const char *file_name)
	const
	{
		FILE *f = fopen(file_name, "w");
		if(f == nullptr) return;
		fprintf(f, "%lu %lu\n", _all_points.size() + _all_vertices.size(),
			                    _triangles[0].size()+_triangles[1].size()+_triangles[2].size());
		int i = -1;
		for(_C_USet_P::const_iterator it =   _all_points.cbegin(); it !=   _all_points.cend(); ++it)
			if((*it)->_optimal == 1) {
				fprintf(f, "%.15lf %.15lf %.15lf\n", (*it)->_x[0], (*it)->_x[1], (*it)->_x[2]); (*it)->_aid = ++i; }
		for(_C_USet_P::const_iterator it = _all_vertices.cbegin(); it != _all_vertices.cend(); ++it) {
			fprintf(f, "%.15lf %.15lf %.15lf\n", (*it)->_x[0], (*it)->_x[1], (*it)->_x[2]); (*it)->_aid = ++i; }
		for(i = 0; i < 3; ++i)
			for(_List_T::const_iterator it = _triangles[i].cbegin(); it != _triangles[i].cend(); ++it)
				fprintf(f, "%lu %lu %lu\n", it->_v[0]->_aid, it->_v[1]->_aid, it->_v[2]->_aid);
		fclose(f);
	}
	/* === Export data to file === */
	/* ############################## Export ############################## */
	/* #################################################################### */


	/* ##################################################################### */
	/* ############################# Modifiers ############################# */
	/* === Clear all data === */
	public:
	void clear ()
	{
		for(_C_USet_P::iterator it =   _all_points.begin(); it !=   _all_points.end(); ++it) delete *it;
		for(_C_USet_P::iterator it = _all_vertices.begin(); it != _all_vertices.end(); ++it) delete *it;
		_all_points.clear(); _all_vertices.clear();
		for(int i = 0; i < 3; ++i) {
			_points[i].clear(); _add[i].clear(); _rem[i].clear(); _vertices[i].clear();
			_triangles[i].clear(); _p_to_ts[i].clear();
			_limits[i][0] = DMIN; _limits[i][1] = DMAX;
		}
	}
	/* === Clear all data === */


	/* === Add points to the frontier === */
	public:
	inline void add_points (int n, double *points)
	{ for(int i = 0; i < n*3; i += 3) add_point(points[i], points[i+1], points[i+2]); }
	inline void add_points (int n, double points[][3])
	{ for(int i = 0; i < n; ++i) add_point(points[i][0], points[i][1], points[i][2]); }
	inline void add_point (double *point) { add_point(point[0], point[1], point[2]); }
	void add_point (double x1, double x2, double x3)
	{
		Point *point = new Point(x1, x2, x3, 1);
		auto result = _all_points.insert(point);
		/* If it is a new point */
		if(result.second)
			for(int i = 0; i < 3; ++i) { _points[i].insert(point); _add[i].insert(point); }
		/* If there is already a point with the specified coordinates */
		else {
			/* If it was marked for removal, restore it */
			if(point->_state == -1) {
				point->_state = 0;
				for(int i = 0; i < 3; ++i) _rem[i].erase(point);
			}
			delete point;
		}
	}
	/* === Add points to the frontier === */


	/* === Remove points from the frontier === */
	public:
	inline void rem_points (int n, double *points)
	{ for(int i = 0; i < n*3; i += 3) rem_point(points[i], points[i+1], points[i+2]); }
	inline void rem_points (int n, double points[][3])
	{ for(int i = 0; i < n; ++i) rem_point(points[i][0], points[i][1], points[i][2]); }
	inline void rem_point (double *point) { rem_point(point[0], point[1], point[2]); }
	void rem_point (double x1, double x2, double x3)
	{
		Point *point = new Point(x1, x2, x3, 1);
		_C_USet_P::iterator it = _all_points.find(point);
		delete point;
		/* If there is a point with the specified coordinates */
		if(it != _all_points.end()) {
			point = *it;
			/* If it was yet to be added, remove it */
			if(point->_state == 1) {
				for(int i = 0; i < 3; ++i) { _points[i].erase(point); _add[i].erase(point); }
				_all_points.erase(point); delete point;
			} 
			/* If not, mark it for removal */
			else if(point->_state == 0) {
				point->_state = -1;
				for(int i = 0; i < 3; ++i) _rem[i].insert(point);
			}
		}
	}
	/* === Remove points from the frontier === */
	/* ############################# Modifiers ############################# */
	/* ##################################################################### */


	/* ##################################################################### */
	/* ######################### Limits / Extremes ######################### */
	/* === Verify and update limits before computing frontier === */
	private:
	void
	_verify_limits_before (int x)
	{
		double limits[2] = {DMIN, DMAX}, extreme;

		/* Get new limits */
		for(_Tree_P::iterator it = _points[x].begin(); !it.is_sentinel(); ++it)
			if((*it)->_state != -1) { limits[_oo[x]]   = (*it)->_x[x]; break; }
		for(_Tree_P::reverse_iterator it = _points[x].rbegin(); !it.is_sentinel(); ++it)
			if((*it)->_state != -1) { limits[_oo[x]^1] = (*it)->_x[x]; break; }

		/* Compute extremes based on the new limits */
		double range = limits[1] - limits[0];
		if(_oo[x] == 0) extreme = limits[1] + (range == 0 ? 1 : range * 0.1);
		else            extreme = limits[0] - (range == 0 ? 1 : range * 0.1);

		/* If the limits have not changed - exit function */
		if(_limits[x][0] == limits[0] && _limits[x][1] == limits[1]) return;

		/* If they did - update surface extremes */
		int i;
		Point *p;
		_Tree_P::reverse_iterator it, tmp_it;

		/* Remove points marked for removal that are beyond the new limits */
		for(it = _points[x].rbegin(); !it.is_sentinel() && (*it)->_state != 0; it = tmp_it) {
			tmp_it = it.next();
			if((*it)->_state == -1) { p = *it;
				/* If next point has same coordinate - maintain removing point to force facet recomputation */
				if(it.has_next() && p->_x[x] == (*it.next())->_x[x] && (*it.next())->_state == 0) break;
				/* Remove triangles created by the point and remove it from dimension point and removal sets */
				_rem_triangles(x, p);
				_points[x].erase(it); _rem[x].erase(p);
				/* If point would be beyond the new extreme - pull it back inside between the extreme and the limit */
				if(!(_of[x](p->_x[x], extreme))) {
					_all_points.erase(p); p->_x[x] = (extreme + limits[_oo[x]^1]) / 2.0; _all_points.insert(p);
				} 
			}
		}

		/* Translate vertices to the new extremes */
		it = _vertices[x].rbegin(); p = *it;

		/* If extremes have extended - update coordinates and reinsert vertices in the hashtable */
		if(_of[x](_extremes[x], extreme))
			for(; !it.is_sentinel() && (*it)->_x[x] == _extremes[x]; ++it, p = *it) {
				_all_vertices.erase(p); p->_x[x] = extreme; _all_vertices.insert(p); }
		/* If shrunk - update coordinates and reinsert vertices in the hastable and trees */
		else
			for(; !it.is_sentinel() && (*it)->_x[x] == _extremes[x]; it = _vertices[x].rbegin(), p = *it) {
				for(i = 0; i < 3; ++i) _vertices[i].erase(p);
				_all_vertices.erase(p); p->_x[x] = extreme; _all_vertices.insert(p);
				for(i = 0; i < 3; ++i) _vertices[i].insert(p);
			}

		/* Update limits and extreme */
		_limits[x][0] = limits[0]; _limits[x][1] = limits[1]; _extremes[x] = extreme;
	}
	/* === Verify and update limits before computing frontier === */

	/* === Verify and update limits after computing frontier === */
	private:
	void
	_verify_limits_after (int x)
	{
		double limits[2] = {DMIN, DMAX}, extreme;

		/* Get new limits */
		for(_Tree_P::iterator it = _points[x].begin(); !it.is_sentinel(); ++it)
			if((*it)->_optimal == 1) { limits[_oo[x]]   = (*it)->_x[x]; break; }
		for(_Tree_P::reverse_iterator it = _points[x].rbegin(); !it.is_sentinel(); ++it)
			if((*it)->_optimal == 1) { limits[_oo[x]^1] = (*it)->_x[x]; break; }

		/* Compute extremes based on the new limits */
		double range = limits[1] - limits[0];
		if(_oo[x] == 0) extreme = limits[1] + (range == 0 ? 1 : range * 0.1);
		else            extreme = limits[0] - (range == 0 ? 1 : range * 0.1);

		/* If the limits have not changed - exit function */
		if(_limits[x][0] == limits[0] && _limits[x][1] == limits[1]) return;

		/* If they did - update surface extremes */
		_Tree_P::reverse_iterator it = _vertices[x].rbegin();

		/* Translate vertices to the new extremes (and reinsert them in the hashtable) */
		for(Point *p = *it; !it.is_sentinel() && (*it)->_x[x] == _extremes[x]; ++it, p = *it) {
			_all_vertices.erase(p); p->_x[x] = extreme; _all_vertices.insert(p); }

		/* Update limits and extreme */
		_limits[x][0] = limits[0]; _limits[x][1] = limits[1]; _extremes[x] = extreme;
	}
	/* === Verify and update limits after computing frontier === */
	/* ######################### Limits / Extremes ######################### */
	/* ##################################################################### */


	/* ##################################################################### */
	/* ############################## Compute ############################## */
	/* === Compute the frontier's facets === */
	public:
	void
	compute ()
	{
		/* If no action is needed */
		if(_add[0].empty() && _rem[0].empty()) { _display(); return; }

		/* If all points from the previous computation are to be removed - clear frontier */
		if(_points[0].size() == _add[0].size() + _rem[0].size() && _rem[0].size() > 0) _clear_frontier();
		/* In case all points have been removed (there is no point in the frontier) - no action is needed */
		if(_points[0].empty()) { _display(); return; }

		int i, j;

		/* Update limits and compute extremes (to be used by the sentinels) */
		// for(i = 0; i < 3; ++i) _threads[i] = std::thread(&pfv3d::_verify_limits_before, this, i);
		// for(i = 0; i < 3; ++i) _threads[i].join();
		for(i = 0; i < 3; ++i) _verify_limits_before(i);

		/* Call each coordinate sweep */
		_threads[0] = std::thread(&pfv3d::_sweep<0, 1, 2>, this);
		_threads[1] = std::thread(&pfv3d::_sweep<1, 2, 0>, this);
		_threads[2] = std::thread(&pfv3d::_sweep<2, 0, 1>, this);
		for(i = 0; i < 3; ++i) _threads[i].join();
		// _sweep<0, 1, 2>(); _sweep<1, 2, 0>(); _sweep<2, 0, 1>();

		/* Remove unused vertices (verify if they are really unused) */
		for(_USet_P::iterator it = _possible_unused_vertices.begin(); it != _possible_unused_vertices.end(); ++it) {
			if((*it)->_n_tri == 0) {
				_all_vertices.erase(*it);
				for(i = 0; i < 3; ++i) _vertices[i].erase(*it);
				delete *it; } }
		_possible_unused_vertices.clear();

		/* Remove non optimal points (if requested) */
		if(_rem_non_optimal == 1)
			for(_USet_P::iterator it = _non_optimal.begin(); it != _non_optimal.end(); it = _non_optimal.begin()) {
				for(i = 0; i < 3; ++i) _points[i].erase(*it);
				if((*it)->_state == 1) for(i = 0; i < 3; ++i) _add[i].erase(*it);
				_all_points.erase(*it); delete *it; _non_optimal.erase(it); }

		/* Remove points marked for removal and clear removal sets */
		for(i = 0; i < 3; ++i) {
			for(_Tree_P::iterator it = _rem[i].begin(); !it.is_sentinel(); ++it) {
				_points[i].erase(*it);
				for(j = 0; j < 2; ++j) if(_rem[(i+j+1)%3].erase(*it)) _points[(i+j+1)%3].erase(*it);
				_all_points.erase(*it); delete *it; }
			_rem[i].clear(); }

		/* Update state of points marked for addition and clear addition sets */
		for(_Tree_P::iterator it = _add[0].begin(); !it.is_sentinel(); ++it) (*it)->_state = 0;
		for(i = 0; i < 3; ++i) _add[i].clear();

		/* Update limits and extremes (non-optimal points may have had influence) */
		// for(i = 0; i < 3; ++i) _threads[i] = std::thread(&pfv3d::_verify_limits_after, this, i);
		// for(i = 0; i < 3; ++i) _threads[i].join();
		for(i = 0; i < 3; ++i) _verify_limits_after(i);

		_display();
	}
	/* === Compute the frontier's facets === */


 	/* === Perform a coordinate sweep for each of the three dimensions === */
	private:
	template <int X0, int X1, int X2>
	void
	_sweep ()
	{
		/* Insert sentinels in the projection tree */
		Point *sentinels[2];
		double sentinel_coord[2][3] = {{0, 0, 0}, {0, 0, 0}};
		sentinel_coord[0][X1] = _extremes[X1]; sentinel_coord[1][X2] = _extremes[X2];
		if(_oo[X1] == 0) sentinel_coord[1][X1] = DMIN; else sentinel_coord[1][X1] = DMAX;
		if(_oo[X2] == 0) sentinel_coord[0][X2] = DMIN; else sentinel_coord[0][X2] = DMAX;
		_projection[X0].insert(sentinels[0] = new Point(&sentinel_coord[0][0], 1));
		_projection[X0].insert(sentinels[1] = new Point(&sentinel_coord[1][0], 1));

		/* Perform the sweep */
		bool redo, save_break = 0;
		int to_add = _add[X0].size(), to_rem = _rem[X0].size(), add_in = 0, rem_in = 0;
		Point *point, *saved[2] = {nullptr, nullptr};
		_Tree_P::iterator it = _points[X0].begin(), it_add = _add[X0].begin(), it_rem = _rem[X0].begin();

		for( ; !it.is_sentinel() && (add_in || rem_in || to_add || to_rem || saved[0] || save_break); ++it) {	
			point = *it;

			/* If point is marked for removal - only insert it in the projection tree */
			if(point->_state == -1) redo = 0;
			/* If point is marked for addition - compute its triangles */
			else if(point->_state == 1) redo = 1;
			/* If there are points marked for removal or addition in the tree still - (re)compute its triangles */
			else if(add_in > 0 || rem_in > 0) redo = 1;
			/* If it has the same sweep coordinate than the next to be added - (re)compute its triangles */
			else if(to_add > 0 && point->_x[X0] == (*it_add)->_x[X0]) redo = 1;
			/* If it has the same sweep coordinate than the next to be removed - (re)compute its triangles */
			else if(to_rem > 0 && point->_x[X0] == (*it_rem)->_x[X0]) redo = 1;
			/* If the case of equal sweep coordinates (X0) has occured - (re)compute its triangles */
			else if(saved[0] != nullptr) redo = 1;
			/* If the case where the equal sweep coordinates case (X0) has been broken - (re)compute its triangles */
			else if(save_break == 1) { redo = 1; }
			/* Otherwise - only insert it in the projection tree */
			else redo = 0;

			save_break = 0; 

			/* Insert the point in the projection tree */
			if(!redo) { if(point->_optimal) _update_projection<X0, X1, X2>(point, rem_in); }
			/* (Re)Compute the facet generated by the point */
			else {
				point->_optimal = _recompute_facet<X0, X1, X2>(it, saved, add_in, rem_in, save_break);
				/* If point is not optimal - add it to list for further removal (if requested) */
				if(X0 == 0) if(_rem_non_optimal == 1 && point->_optimal == 0) _non_optimal.insert(point);
				/* Dynamic display (if set) */
				if(_display_mode == 2 || _display_mode == 4) _display();
			}

			/* Update iterators and number of points to be added or to be removed */
			     if(point->_state ==  1) { ++it_add; --to_add; }
			else if(point->_state == -1) { ++it_rem; --to_rem; }
		}
		
		/* Clear projection tree and sentinels */
		_projection[X0].clear();
		delete sentinels[0]; delete sentinels[1];
	}
	/* === Perform a coordinate sweep for each of the three dimensions === */


	/* === Insert point in the projection tree and update the latter === */
	private:
	template <int X0, int X1, int X2>
	void
	_update_projection (Point *point, int &rem_in)
	{
		/* If point is marked for removal - remove triangles previously created by it */
		if(point->_state == -1) _rem_triangles(X0, point);

		/* Insert point in the projection tree */
		_Tree_P::iterator p_it = _projection[X0].insert(point).first;
		/* If point is not optimal - remove it from the projection tree and exit function */
		if(!(_of[X2](point->_x[X2], (*p_it.prev())->_x[X2]))) { _projection[X0].erase(p_it); return; } 
		/* If point is optimal and marked for removal - update their number in the projection tree */ 
		else if(point->_state == -1) ++rem_in;

		/* Cycle to process dominated points until one that is not dominated is reached */
		_Tree_P::iterator current = p_it.next(), tmp_it;
		for( ; !(_of[X2]((*current)->_x[X2], point->_x[X2])); current = tmp_it) {
			tmp_it = current.next();
			/* Points marked for removal can only remove other points marked for removal and
			   maintained points can only remove other maintained points */
			if(point->_state == (*current)->_state) {
				/* Remove current point from the projection tree */
				_projection[X0].erase(current);
				/* If it was a point marked for removal - update their number in the projection tree */
				if(point->_state == -1) --rem_in;
			}
		}
	}
	/* === Insert point in the projection tree and update the latter === */


	/* === (Re)Compute the facet generated by a point === */
	private:
	template <int X0, int X1, int X2>
	bool
	_recompute_facet (_Tree_P::iterator &p, Point *saved[2], int &add_in, int &rem_in, bool &save_break)
	{
		Point *point = *p;

		/* Remove triangles previously created by the point */
		_rem_triangles(X0, point);

		/* Insert point in the projection tree */
		_Tree_P::iterator p_it = _projection[X0].insert(point).first, tmp_it;
		/* Get the predecessor (it must not be marked for removal) */
		for(tmp_it = p_it.prev(); (*tmp_it)->_state == -1; tmp_it = tmp_it.prev()) {}
		/* If point is not optimal - remove it from the projection tree and exit function */
		if(!(_of[X2](point->_x[X2], (*tmp_it)->_x[X2]))) { _projection[X0].erase(p_it); return 0; } 
		/* If point is optimal and marked for addition - update their number in the projection tree */ 
		else if(point->_state == 1) ++add_in;

		/* Get the intersected point in the projection tree */
		Point *vertices[3];
		_Tree_P::iterator current = p_it.next();
		/* If the previous point called into this function has the same sweep coordinate (X0) as the
		   current one - recover saved vertex 1 (was vertex 3 before) */
		if(saved[0] != nullptr) vertices[0] = saved[0];
		/* If does not - find and create vertex 1 */
		else {
			/* Find the successor (it must not be marked for removal) */
			for( ; (*current)->_x[X1] == point->_x[X1] && (*current)->_state == -1; current = p_it.next()) {
				/* Successor is partially dominated and is marked for removal - remove it from the projection tree
				   and update their number in the projection tree */
				_projection[X0].erase(current); --rem_in; }
			/* If successor does not have an equal X1 coordinate - intersected point
			   is the predecessor obtained earlier */
			if((*current)->_x[X1] != point->_x[X1]) current = tmp_it;
			/* If intersected point is partially dominated - remove it from the representation tree */
			else {
				/* If it was a point marked for addition - update their number in the projection tree */
				if((*current)->_state == 1) --add_in;
				_projection[X0].erase(current);
			}

			/* Create vertex 1 */
			vertices[0] = _add_vertex<X0, X1, X2>(point->_x[X0], point->_x[X1], (*current)->_x[X2]);
		}

		/* Cycle to process points until one that is intercected is reached */
		tmp_it = p_it.next();
		while(1) {
			/* Find the successor (it must not be marked for removal) */
			for(current = tmp_it; (*current)->_state == -1; current = tmp_it) {
				tmp_it = current.next();
				/* Successor is marked for removal. If it also is dominated - remove it from the projection
				   tree and update their number in the projection tree */
				if(!(_of[X2]((*current)->_x[X2], point->_x[X2]))) { _projection[X0].erase(current); --rem_in; }
			}

			/* If the previous point called into this function has the same sweep coordinate (X0) as the
			   current one - recover saved vertex 2 (was vertex 2 before) */
			if(saved[0] != nullptr) { vertices[1] = saved[1]; saved[0] = nullptr; }
			/* If does not - create vertex 2 */
			else vertices[1] = _add_vertex<X0, X1, X2>(point->_x[X0], (*current)->_x[X1], vertices[0]->_x[X2]);

			/* If current point is not fully dominated by the point called into this function - leave cycle */
			if(!(_of[X2](point->_x[X2], (*current)->_x[X2]))) break;

			/* Create vertex 3 */
			vertices[2] = _add_vertex<X0, X1, X2>(point->_x[X0], (*current)->_x[X1], (*current)->_x[X2]);

			/* Add triangles 103 and 123 (being 0 the point called into this function) */
			_add_triangle(X0, *p_it, vertices[0], *p_it, vertices[2]);
			_add_triangle(X0, *p_it, vertices[0], vertices[1], vertices[2]);

			/* - Remove current point from the projection tree (it is fully dominated) - */
			/* If it was a point marked for addition - update their number in the projection tree */
			if((*current)->_state == 1) --add_in;
			tmp_it = current.next();
			_projection[X0].erase(current);
			/* Vertex 3 is convertex into vertex 1 */
			vertices[0] = vertices[2];
		}

		/* Find the next point to be called into this function (must not be marked for removal) */
		tmp_it = p.next();
		for( ; !tmp_it.is_sentinel() && (*tmp_it)->_state == -1; tmp_it = tmp_it.next()) {}
		/* If it has an equal sweep coordinate (X0) as the point called into this function, it is not dominated by the
		   latter, and it intersects the latter in non-dominated space - the case of equal sweep coordinates occurs */
		if(!tmp_it.is_sentinel() && point->_x[X0] == (*tmp_it)->_x[X0]
		   && _of[X2]((*tmp_it)->_x[X2], point->_x[X2])
		   && _of[X1]((*tmp_it)->_x[X1], (*current)->_x[X1])) {
			/* Create vertex 3 */
			vertices[2] = _add_vertex<X0, X1, X2>(point->_x[X0], (*tmp_it)->_x[X1], point->_x[X2]);
			/* Save vertices 2 and 3 for next function (vertex 3 will be 1 and vertex 2 will still be 2) */
			saved[0] = vertices[2]; saved[1] = vertices[1];
		}
		/* The case of qual sweep coordinates (X0) does not occur (default case) */
		else {
			/* Create vertex 3 */
			vertices[2] = _add_vertex<X0, X1, X2>(point->_x[X0], (*current)->_x[X1], point->_x[X2]);
			/* If intersected point is partially dominated - remove it from the representation tree */
			if((*current)->_x[X2] == point->_x[X2]) {
				/* If intersected point was marked for addition and breaks the case of equal sweep coordinates (X0) */
				if((*current)->_state == 1 && !tmp_it.is_sentinel() && point->_x[X0] == (*tmp_it)->_x[X0]
				   && point->_state == 0 && (*tmp_it)->_state == 0) save_break = 1;

				/* If it was a point marked for addition - update their number in the projection tree */
				if((*current)->_state == 1) --add_in;
				_projection[X0].erase(current);
			}
		}

		/* Add triangles 103 and 123 (being 0 the point called into this function) */
		_add_triangle(X0, *p_it, vertices[0], *p_it, vertices[2]);
		_add_triangle(X0, *p_it, vertices[0], vertices[1], vertices[2]);

		return 1;
	}
	/* === (Re)Compute the facet generated by a point === */
	/* ############################## Compute ############################## */
	/* ##################################################################### */


	/* #################################################################### */
	/* ############################# Elements ############################# */
	/* === Add vertex === */
	private:
	template <int X0, int X1, int X2>
	Point *
	_add_vertex (double x1, double x2, double x3)
	{
		double coordinates[3];
		/* Create vertex */
		coordinates[X0] = x1; coordinates[X1] = x2; coordinates[X2] = x3;
		Point *point = new Point(coordinates);
		/* Try to insert vertex */
		_mutex_all_vertices.lock();
		auto result = _all_vertices.insert(point);
		_mutex_all_vertices.unlock();
		/* If the vertex already exists - get pointer and exit */
		if(!result.second) { delete point; return *result.first; }
		/* Otherwise - insert it in the trees */
		else {
			_mutex_vertices.lock();
			for(int i = 0; i < 3; ++i) _vertices[i].insert(point);
			_mutex_vertices.unlock();
		}
		return point;
	}
	/* === Add vertex === */


	/* === Add triangle === */
	private:
	void
	_add_triangle (int x, Point *p, Point *p1, Point *p2, Point *p3)
	{
		/* Create and insert triangle */
		_mutex_triangles.lock();
		_p_to_ts[x].insert({p, _triangles[x].insert(_triangles[x].end(), Triangle(p1, p2, p3))});
		_mutex_triangles.unlock();
		/* Increase the number of associated triangles in each vertex */
		++p1->_n_tri; ++p2->_n_tri; ++p3->_n_tri;
	}
	/* === Add triangle === */


	/* === Remove triangles generated by a user inputted point */
	private:
	void
	_rem_triangles (int x, Point *point)
	{
		Triangle *t;
		auto range = _p_to_ts[x].equal_range(point);
		_mutex_triangles.lock();
		for(auto it = range.first; it != range.second; ++it) {
			t = &*it->second;
			for(int i = 0; i < 3; i++)
				/* Mark vertex to possible further removal if it ceases to have associated triangles and
				   it wasn't given by the user (it may be in use by other threads still) */
				if(--t->_v[i]->_n_tri == 0 && t->_v[i]->_input == 0) {
					_mutex_possible_unused.lock();
					_possible_unused_vertices.insert(t->_v[i]);
					_mutex_possible_unused.unlock(); }
			/* Remove triangle */
			_triangles[x].erase(it->second);
		}
		_mutex_triangles.unlock();
		_p_to_ts[x].erase(point);
	}
	/* === Remove triangles generated by a point (user input point) */
	/* ############################# Elements ############################# */
	/* #################################################################### */
};

#endif
