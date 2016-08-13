#ifndef _PFV3D_H_
#define _PFV3D_H_

#include <limits>
#include <functional>
#include <list>
#include <unordered_set>
#include <unordered_map>

struct Point
{
	bool _optimal;
	int _state, _n_tri;
	double _x[3];
	Point (double x1, double x2, double x3, bool optimal = 0) : _optimal(optimal), _state(1), _n_tri(0), _x{x1, x2, x3} {}
	Point (double *x, bool optimal = 0) : _optimal(optimal), _state(1), _n_tri(0), _x{x[0], x[1], x[2]} {}
};
std::function<bool(double,double)> _oo[3];
/* === Point compare function === */
struct point_comparison {
	int _x1, _x2, _x3;
	point_comparison () {}
	point_comparison (int x1, int x2, int x3) : _x1(x1), _x2(x2), _x3(x3) {}
	bool operator()(const Point *p1, const Point *p2) {
		if(_oo[_x1](p1->_x[_x1], p2->_x[_x1])) return 1;; if(_oo[_x1](p2->_x[_x1], p1->_x[_x1])) return 0;
		if(_oo[_x2](p1->_x[_x2], p2->_x[_x2])) return 1;; if(_oo[_x2](p2->_x[_x2], p1->_x[_x2])) return 0;
		if(_oo[_x3](p1->_x[_x3], p2->_x[_x3])) return 1;; return 0; } };
/* === Point compare function === */

struct Triangle
{
	int _normal;
	Point *_v[3];
	Triangle (int normal, Point *v1, Point *v2, Point *v3) : _normal(normal), _v{v1, v2, v3} {}
	~Triangle () {}
};

#include "pfv3d_display.h"

struct pfv3d
{
	/* === Projection compare function === */
	struct projection_comparison {
		int _po[3];
		bool operator()(const Point* p1, const Point* p2) {
			if(_oo[_po[0]](p1->_x[_po[0]],p2->_x[_po[0]])) return 1;; if(_oo[_po[0]](p2->_x[_po[0]],p1->_x[_po[0]])) return 0;
			if(_oo[_po[1]](p1->_x[_po[1]],p2->_x[_po[1]])) return 1;; if(_oo[_po[1]](p2->_x[_po[1]],p1->_x[_po[1]])) return 0;
			if(_oo[_po[2]](p1->_x[_po[2]],p2->_x[_po[2]])) return 1;; return 0; } };
	/* === Projection compare function === */

	#define DMAX std::numeric_limits<double>::max()
	#define DMIN std::numeric_limits<double>::lowest()
	#define _po _projection_comparison._po
	typedef tree<tree_avl, Point *, point_comparison> _Tree_P;
	typedef tree<tree_avl, Point *, projection_comparison> _Tree_Proj;
	typedef std::unordered_set<Point *> _Set_P;
	typedef std::list<Triangle> _List_T;
	typedef std::unordered_multimap<Point *, _List_T::iterator> _Map_PTs;

	/* === Variables === */
	bool _minmax[3];
	int _display_mode;
	double _limits[3][2], _extremes[3];

	point_comparison _point_comparison[3] = {point_comparison(0,1,2), point_comparison(1,2,0), point_comparison(2,0,1)};
	_Tree_P _points[3]   = {_Tree_P(_point_comparison[0]), _Tree_P(_point_comparison[1]), _Tree_P(_point_comparison[2])};
	_Tree_P _add[3]      = {_Tree_P(_point_comparison[0]), _Tree_P(_point_comparison[1]), _Tree_P(_point_comparison[2])};
	_Tree_P _rem[3]      = {_Tree_P(_point_comparison[0]), _Tree_P(_point_comparison[1]), _Tree_P(_point_comparison[2])};
	_Tree_P _vertices[3] = {_Tree_P(_point_comparison[0]), _Tree_P(_point_comparison[1]), _Tree_P(_point_comparison[2])};

	projection_comparison _projection_comparison;
	_Tree_Proj _projection = _Tree_Proj(_projection_comparison);

	_Set_P _non_optimal;
	_List_T _triangles;
	_Map_PTs _p_to_ts[3];

	pfv3d_display _display;
	/* === Variables === */


	/* === Constructor/Destructor === */
	public:
	pfv3d () : pfv3d(0, 0, 0) {}
	pfv3d (bool mm_0, bool mm_1, bool mm_2) : _minmax{mm_0, mm_1, mm_2}, _limits{{DMIN, DMAX}, {DMIN, DMAX}, {DMIN, DMAX}}
	{
		for(int i = 0; i < 3; ++i) {
			if(_minmax[i] == 0) { _extremes[i] = DMAX; _oo[i] = std::less   <double>(); }
			else                { _extremes[i] = DMIN; _oo[i] = std::greater<double>(); }
		}
	}
	~pfv3d ()
	{
		for(_Tree_P::iterator it =   _points[0].begin(); !it.is_sentinel(); ++it) delete *it;
		for(_Tree_P::iterator it = _vertices[0].begin(); !it.is_sentinel(); ++it) delete *it;
	}
	/* === Constructor/Destructor === */


	/* === Define the optimisation orientation === */
	public:
	void minimise (int dimension)
	{
		if(_minmax[dimension] == 0) return;
		_minmax[dimension] = 0;
		_limits[dimension][0] = DMIN; _limits[dimension][1] = DMAX; _extremes[dimension] = DMAX;
		_oo[dimension] = std::less<double>();
		reorder();
		clear_frontier();
	}
	void maximise (int dimension)
	{
		if(_minmax[dimension] == 1) return;
		_minmax[dimension] = 1;
		_limits[dimension][0] = DMIN; _limits[dimension][1] = DMAX; _extremes[dimension] = DMIN;
		_oo[dimension] = std::greater<double>();
		reorder();
		clear_frontier();
	}
	void orientation (bool mm_0, bool mm_1, bool mm_2)
	{
		bool mm[3] = {mm_0, mm_1, mm_2};
		for(int i = 0; i < 3; ++i) {
			if(_minmax[i] == mm[i]) return;
			_minmax[i] = mm[i];
			_limits[i][0] = DMIN; _limits[i][1] = DMAX; _extremes[i] = mm[i] == 0 ? DMAX : DMIN;
			if(mm[i] == 0) _oo[i] = std::less<double>(); else _oo[i] = std::greater<double>();
		}
		reorder();
		clear_frontier();
	}
	/* === Define the optimisation orientation === */


	/* === Reorder trees === */
	private:
	void reorder ()
	{
		_Tree_P::iterator it;
		for(int i = 0; i < 3; ++i) {
			  _points[i].clear(); for(it =   _points[(i+1)%3].begin(); !it.is_sentinel(); ++it)   _points[i].insert(*it);
			     _add[i].clear(); for(it =      _add[(i+1)%3].begin(); !it.is_sentinel(); ++it)      _add[i].insert(*it);
			     _rem[i].clear(); for(it =      _rem[(i+1)%3].begin(); !it.is_sentinel(); ++it)      _rem[i].insert(*it);
			_vertices[i].clear(); for(it = _vertices[(i+1)%3].begin(); !it.is_sentinel(); ++it) _vertices[i].insert(*it);
		}
	}
	/* === Reorder trees === */


	/* === Call the visualiser === */
	public:
	void display () { _display.display_frontier(0, _minmax, _triangles); }
	private:
	void display_i () { _display.display_frontier(1, _minmax, _triangles); }
	/* === Call the visualiser === */


	/* === Clear frontier === */
	private:
	void clear_frontier ()
	{
		int i;
		_Tree_P::iterator it;
		Point *p;
		/* Mark every point for addition, except the ones marked for removal, which are deleted */
		for(it = _points[0].begin(); !it.is_sentinel(); ) {
			p = (*it); ++it;
			if(p->_state == 0) {
				p->_state = 1;
				for(i = 0; i < 3; ++i) _add[i].insert(p);
			} else if(p->_state == -1) {
				for(i = 0; i < 3; ++i) _points[i].erase(p);
				delete p;
			}
		}
		/* Clear removal sets */
		for(i = 0; i < 3; ++i) _rem[i].clear();
		/* Delete all vertices */
		for(it = _vertices[0].begin(); !it.is_sentinel(); ++it) delete *it;
		for(i = 0; i < 3; ++i) _vertices[i].clear();
		/* Remove all triangles */
		for(i = 0; i < 3; ++i) _p_to_ts[i].clear();
		_triangles.clear();
	}
	/* === Clear frontier === */


	/* === Clear all data === */
	public:
	void clear ()
	{
		for(_Tree_P::iterator it =   _points[0].begin(); !it.is_sentinel(); ++it) delete *it;
		for(_Tree_P::iterator it = _vertices[0].begin(); !it.is_sentinel(); ++it) delete *it;
		for(int i = 0; i < 3; ++i)
			{ _points[i].clear(); _add[i].clear(); _rem[i].clear(); _vertices[i].clear(); _p_to_ts[i].clear(); }
		_triangles.clear();
		_limits[0][0] = _limits[1][0] = _limits[2][0] = DMIN;
		_limits[0][1] = _limits[1][1] = _limits[2][1] = DMAX;
	}
	/* === Clear all data === */


	/* === Add points to the frontier === */
	public:
	void add_point (double x1, double x2, double x3) { double p[3] = {x1, x2, x3}; add_points(1, p); }
	void add_point (double point[]) { add_points(1, point); }
	void add_points (int n, double points[][3]) { add_points(n, &points[0][0]); }
	void add_points (int n, double points[])
	{
		Point *tmp, *p;
		for(int i = 0; i < n*3; i += 3) {
			p = *_points[0].insert(tmp = new Point(&points[i], 1)).first;
			/* If there is already a point with the specified coordinates */
			if(p != tmp) {
				/* If it was marked for removal, restore it */
				if(p->_state == -1) {
					p->_state = 0;
					for(int j = 0; j < 3; ++j) _rem[j].erase(p);
				}
				delete tmp;
			}
			/* If it is a new point */
			else {
				for(int j = 0; j < 3; ++j) _add[j].insert(p);
				_points[1].insert(p); _points[2].insert(p);
			}
		}
	}
	/* === Add points to the frontier === */


	/* === Remove points from the frontier === */
	public:
	void rem_point (double x1, double x2, double x3) { double p[3] = {x1, x2, x3}; rem_points(1, p); }
	void rem_point (double point[]) { rem_points(1, point); }
	void rem_points (int n, double points[][3]) { rem_points(n, &points[0][0]); }
	void rem_points (int n, double points[])
	{
		Point *tmp, *p;
		for(int i = 0; i < n*3; i += 3) {
			p = *_points[0].find((tmp = new Point(&points[i]))); delete tmp;
			/* If there is no point with the specified coordinates */
			if(p == *_points[0].end()) continue;
			/* If it was yet to be added, remove it */
			if(p->_state == 1) {
				for(int j = 0; j < 3; ++j) { _add[j].erase(p); _points[j].erase(p); }
				delete p;
			} 
			/* If not, mark it for removal */
			else if(p->_state == 0) {
				p->_state = -1;
				for(int j = 0; j < 3; ++j) _rem[j].insert(p);
			}
		}
	}
	/* === Remove points from the frontier === */


	/* === Compute the facets of the frontier === */
	private:
	void verify_limits (int index)
	{
		int i;
		double limits[2] = {DMIN, DMAX}, extreme;
		Point *p;

		/* Get new limits */
		for(_Tree_P::iterator it = _points[index].begin(); !it.is_sentinel(); ++it)
			if((*it)->_state != -1) { limits[_minmax[index]] = (*it)->_x[index]; break; }
		for(_Tree_P::reverse_iterator it = _points[index].rbegin(); !it.is_sentinel(); ++it)
			if((*it)->_state != -1) { limits[_minmax[index]^1] = (*it)->_x[index]; break; }

		/* Compute extremes based on the new limits */
		double range = limits[1] - limits[0];
		if(_minmax[index] == 0) extreme = limits[1] + (range == 0 ? 1 : range * 0.1);
		else                    extreme = limits[0] - (range == 0 ? 1 : range * 0.1);

		/* If the limits have not changed - exit function */
		if(_limits[index][0] == limits[0] && _limits[index][1] == limits[1]) return;

		/* If they did - update surface extremes */
		_Tree_P::reverse_iterator it, tmp_it;
		/* Remove points marked for removal that are beyond the new limits */
		for(it = _points[index].rbegin(); !it.is_sentinel() && (*it)->_state != 0; it = tmp_it) {
			tmp_it = it.next();
			if((*it)->_state == -1) { p = *it;
				/* If next point has same coordinate - maintain removing point to force facet recomputation */
				if(it.has_next() && p->_x[index] == (*it.next())->_x[index] && (*it.next())->_state == 0) break;
				/* Remove triangles created by the point and remove it from points and removal sets */
				// for(i = 0; i < 3; ++i) rem_triangles(index, *it);
				rem_triangles(index, *it);
				_rem[index].erase(*it); _points[index].erase(*it);
				/* If point would be beyond the new extreme - pull it back inside between the extreme and the limit */
				if(!(_oo[index](p->_x[index], extreme))) p->_x[index] = (extreme + limits[_minmax[index]^1]) / 2;
			}
		}

		/* Translate vertices to the new extremes */
		it = _vertices[index].rbegin();
		/* If extremes have extended - just update coordinates */
		if(_oo[index](_extremes[index], extreme))
			for( ; !it.is_sentinel() && (*it)->_x[index] == _extremes[index]; ++it)
				(*it)->_x[index] = extreme;
		/* If shrunk - update coordinates and reinsert vertices in the trees (order may have been afected) */
		else
			for( ; !it.is_sentinel() && (*it)->_x[index] == _extremes[index]; it = _vertices[index].rbegin()) {
				p = *it;                for(i = 0; i < 3; ++i) _vertices[i].erase(p);
				p->_x[index] = extreme; for(i = 0; i < 3; ++i) _vertices[i].insert(p);
			}

		/* Update limits and extreme */
		_limits[index][0] = limits[0]; _limits[index][1] = limits[1]; _extremes[index] = extreme;
	}
	void update_limits (int index)
	{
		double limits[2] = {DMIN, DMAX}, extreme;

		/* Get new limits */
		for(_Tree_P::iterator it = _points[index].begin(); !it.is_sentinel(); ++it)
			if((*it)->_state != -1) { limits[_minmax[index]] = (*it)->_x[index]; break; }
		for(_Tree_P::reverse_iterator it = _points[index].rbegin(); !it.is_sentinel(); ++it)
			if((*it)->_state != -1) { limits[_minmax[index]^1] = (*it)->_x[index]; break; }

		/* Compute extremes based on the new limits */
		double range = limits[1] - limits[0];
		if(_minmax[index] == 0) extreme = limits[1] + (range == 0 ? 1 : range * 0.1);
		else                    extreme = limits[0] - (range == 0 ? 1 : range * 0.1);

		/* If the limits have not changed - exit function */
		if(_limits[index][0] == limits[0] && _limits[index][1] == limits[1]) return;

		/* If they did - update surface extremes */
		_Tree_P::reverse_iterator it = _vertices[index].rbegin();
		/* Translate vertices to the new extremes */
		for( ; !it.is_sentinel() && (*it)->_x[index] == _extremes[index]; ++it)
				(*it)->_x[index] = extreme;

		/* Update limits and extreme */
		_limits[index][0] = limits[0]; _limits[index][1] = limits[1]; _extremes[index] = extreme;
	}

	public:
	void compute (int display_mode = 0)
	{
		int i;
		/* If no action is needed */
		if(_add[0].empty() && _rem[0].empty()) { if(display_mode != 0) display_i(); return; }

		/* If all points from the previous computation are to be removed - clear frontier */
		if(_points[0].size() == _add[0].size() + _rem[0].size() && _rem[0].size() > 0) clear_frontier();
		/* In case all points have been removed - no action is needed */
		if(_points[0].empty()) { if(display_mode != 0) display_i(); return; }
		_display_mode = display_mode;

		/* Update limits and compute extremes (to be used by sentinels) */
		for(i = 0; i < 3; ++i) verify_limits(i);

		/* Call each coordinate sweep */
		_po[0] = 1; _po[1] = 2; _po[2] = 0; sweep(0);
		_po[0] = 2; _po[1] = 0; _po[2] = 1;	sweep(1);
		_po[0] = 0; _po[1] = 1; _po[2] = 2;	sweep(2);

		/* Remove non optimal points */
		for(_Set_P::iterator it = _non_optimal.begin(); it != _non_optimal.end(); it = _non_optimal.begin()) {
			for(i = 0; i < 3; ++i) _points[i].erase(*it);
			if((*it)->_state == 1) for(i = 0; i < 3; ++i) _add[i].erase(*it);
			delete *it; _non_optimal.erase(it); }

		/* Remove points marked for removal and clear removal sets */
		for(i = 0; i < 3; ++i) {
			for(_Tree_P::iterator it = _rem[i].begin(); !it.is_sentinel(); it = _rem[i].begin()) {
				for(int j = 0; j < 3; ++j) _points[j].erase(*it);
				for(int j = 0; j < 2; ++j) _rem[(i+j+1)%3].erase(*it);
				delete *it; _rem[i].erase(it); } }

		/* Update state of points marked for addition and clear addition sets */
		for(_Tree_P::iterator it = _add[0].begin(); !it.is_sentinel(); ++it) (*it)->_state = 0;
		for(i = 0; i < 3; ++i) _add[i].clear();

		/* Update limits and extremes (non-optimal points have no longer influence) */
		for(i = 0; i < 3; ++i) update_limits(i);

		if(display_mode != 0) display_i();
	}

	private:
	void sweep (int index)
	{
		/* Insert sentinels in the projection tree */
		Point *sentinels[2];
		double sentinel_coord[2][3] = {{0, 0, 0}, {0, 0, 0}};
		sentinel_coord[0][_po[0]] = _extremes[_po[0]]; sentinel_coord[1][_po[1]] = _extremes[_po[1]];
		if(_minmax[_po[0]] == 0) sentinel_coord[1][_po[0]] = DMIN; else sentinel_coord[1][_po[0]] = DMAX;
		if(_minmax[_po[1]] == 0) sentinel_coord[0][_po[1]] = DMIN; else sentinel_coord[0][_po[1]] = DMAX;
		_projection.insert(sentinels[0] = new Point(&sentinel_coord[0][0], 1));
		_projection.insert(sentinels[1] = new Point(&sentinel_coord[1][0], 1));

		/* Perform the sweep */
		bool redo, save_break = 0;
		int to_add = _add[index].size(), to_rem = _rem[index].size(), add_in = 0, rem_in = 0;
		Point *saved = nullptr;
		_Tree_P::iterator it = _points[index].begin(), it_add = _add[index].begin(), it_rem = _rem[index].begin();

		for( ; !it.is_sentinel() &&
		   (to_add > 0 || to_rem > 0 || add_in > 0 || rem_in > 0 || saved != nullptr || save_break == 1); ++it) {

			// Se for ponto para remover - apenas por na arvore
			// Se for ponto para adicionar - refazer
			// Se ainda houver pontos adicionados ou removidos na arvore - refazer
			// Se tiver a mesma coordenada de ponto para adicionar - refazer
			// Se tiver a mesma coordenada de ponto para remover - refazer
			// Se for o caso de coordenadas repetidas (saved) - refazer
			// Se for o caso onde um ponto novo corta o caso de coordenadas repetidas (saved) - refazer
			// Nenhum dos anteriores - apenas por na arvore

			if((*it)->_state == -1) redo = 0;
			else if((*it)->_state == 1) redo = 1;
			else if(add_in > 0 || rem_in > 0) redo = 1;
			else if(to_add > 0 && (*it)->_x[_po[2]] == (*it_add)->_x[_po[2]]) redo = 1;
			else if(to_rem > 0 && (*it)->_x[_po[2]] == (*it_rem)->_x[_po[2]]) redo = 1;
			else if(saved != nullptr) redo = 1;
			else if(save_break == 1) { save_break = 0; redo = 1; }
			else redo = 0;

			/* Insert the point in the projection tree */
			if(redo == 0) insert_only(index, *it, rem_in);
			/* (Re)Compute the facet generated by the point */
			else {
				if(facet(index, it, saved, add_in, rem_in, save_break) == 0) _non_optimal.insert(*it);
				if(_display_mode == 2) display_i();
			}

			/* Update iterators and number of points to be added or to be removed */
			     if((*it)->_state ==  1) { ++it_add; --to_add; }
			else if((*it)->_state == -1) { ++it_rem; --to_rem; }
		}
		
		/* Clear projection tree and sentinels */
		_projection.clear();
		delete sentinels[0]; delete sentinels[1];
	}

	private:
	void insert_only (int index, Point *point, int &rem_in)
	{
		/* If point is marked for removal - remove triangles previously created by it */
		if(point->_state == -1) rem_triangles(index, point);

		/* Insert point in the projection tree */
		_Tree_Proj::iterator p_it = _projection.insert(point).first;
		/* If point is not optimal - remove it from the projection tree and exit function */
		if(!(_oo[_po[1]](point->_x[_po[1]], (*p_it.prev())->_x[_po[1]]))) { _projection.erase(p_it); return; } 
		/* If point is optimal and marked for removal - update their number in the projection tree */ 
		else if(point->_state == -1) ++rem_in;

		/* Cycle to process dominated points until one that is not dominated is reached */
		_Tree_Proj::iterator current = p_it.next(), tmp_it;
		for( ; !(_oo[_po[1]]((*current)->_x[_po[1]], point->_x[_po[1]])); current = tmp_it) {
			tmp_it = current.next();
			/* Points marked for removal can only remove other points marked for removal and
			   maintained points can only remove other maintained points */
			if(point->_state == (*current)->_state) {
				/* Remove current point from the projection tree */
				_projection.erase(current);
				/* If it was a point marked for removal - update their number in the projection tree */
				if(point->_state == -1) --rem_in;
			}
		}
	}

	private:
	bool facet (int index, _Tree_P::iterator &p, Point *&saved, int &add_in, int &rem_in, bool &save_break)
	{
		Point *point = *p;

		/* Remove triangles previously created by the point */
		rem_triangles(index, point);

		/* Insert point in the projection tree */
		_Tree_Proj::iterator p_it = _projection.insert(point).first, tmp_it;
		/* Get the predecessor (it must not be marked for removal) */
		for(tmp_it = p_it.prev(); (*tmp_it)->_state == -1; tmp_it = tmp_it.prev()) {}
		/* If point is not optimal - remove it from the projection tree and exit function */
		if(!(_oo[_po[1]](point->_x[_po[1]], (*tmp_it)->_x[_po[1]]))) { _projection.erase(p_it); return 0; } 
		/* If point is optimal and marked for addition - update their number in the projection tree */ 
		else if(point->_state == 1) ++add_in;

		/* Get the intersected point in the projection tree */
		_Tree_Proj::iterator current = p_it.next();
		/* Find the successor (it must not be marked for removal) */
		for( ; (*current)->_x[_po[0]] == point->_x[_po[0]] && (*current)->_state == -1; current = p_it.next()) {
			/* Successor is partially dominated and is marked for removal - remove it from the projection tree
			   and update their number in the projection tree */
			_projection.erase(current); --rem_in; }
		/* If successor does not have equal coordinate - intersected point is the predecessor obtained earlier */
		if((*current)->_x[_po[0]] != point->_x[_po[0]]) current = tmp_it;

		/* Create vertex 1 */
		Point *vertices[3];
		vertices[0] = add_vertex(point->_x[_po[0]], (*current)->_x[_po[1]], point->_x[_po[2]]);

		/* If intersected point is partially dominated - remove it from the representation tree */
		if(current != tmp_it) { // if((*current)->_x[_po[0]] == point->_x[_po[0]]) {
			/* If it was a point marked for addition - update their number in the projection tree */
			if((*current)->_state == 1) --add_in;
			_projection.erase(current);
		}

		/* Cycle to process points until one that is intercected is reached */
		tmp_it = p_it.next();
		while(1) {
			/* Find the successor (it must not be marked for removal) */
			for(current = tmp_it; (*current)->_state == -1; current = tmp_it) {
				tmp_it = current.next();
				/* Successor is marked for removal. If it also is dominated - remove it from the projection
				   tree and update their number in the projection tree */
				if(!(_oo[_po[1]]((*current)->_x[_po[1]], point->_x[_po[1]]))) { _projection.erase(current); --rem_in; }
			}

			/* If the previous point called into this function has the same coordinate as the
			   current one - recover saved vertex 2 */
			if(saved != nullptr) vertices[1] = saved;
			/* If does not - create vertex 2 */
			else vertices[1] = add_vertex((*current)->_x[_po[0]], vertices[0]->_x[_po[1]], point->_x[_po[2]]);

			/* If current point is not fully dominated - leave cycle */
			if(!(_oo[_po[1]](point->_x[_po[1]], (*current)->_x[_po[1]]))) break;

			/* Create vertex 3 */
			vertices[2] = add_vertex((*current)->_x[_po[0]], (*current)->_x[_po[1]], point->_x[_po[2]]);

			/* Add triangles 103 and 123 (being 0 the point called into this function) */
			add_triangle(index, *p_it, vertices[0], *p_it, vertices[2]);
			add_triangle(index, *p_it, vertices[0], vertices[1], vertices[2]);

			/* There is no case of same coordinate, analysed only after cycle, when ending this function */
			saved = nullptr;

			/* Remove current point from the projection tree (it is fully dominated) */
			/* If it was a point marked for addition - update their number in the projection tree */
			if((*current)->_state == 1) --add_in;
			tmp_it = current.next();
			_projection.erase(current);
			/* Vertex 3 is convertex into vertex 1 */
			vertices[0] = vertices[2];
		}

		/* Find the next point to be called into this function (must not be marked for removal) */
		_Tree_P::iterator tmp2_it = p.next();
		for( ; !tmp2_it.is_sentinel() && (*tmp2_it)->_state == -1; tmp2_it = tmp2_it.next()) {}
		/* If it has same coordinate than the one called into this function, it is not dominated by the latter, 
		   and it intersects the latter in non dominated space - the case of same coordinate occurs */
		if(!tmp2_it.is_sentinel() && point->_x[_po[2]] == (*tmp2_it)->_x[_po[2]]
		   && _oo[_po[1]]((*tmp2_it)->_x[_po[1]], point->_x[_po[1]])
		   && _oo[_po[0]]((*tmp2_it)->_x[_po[0]], (*current)->_x[_po[0]])) {
			/* Create vertex 3 */
			vertices[2] = add_vertex((*tmp2_it)->_x[_po[0]], point->_x[_po[1]], point->_x[_po[2]]);
			/* Save vertex 2 for next function */
			saved = vertices[1];
		}
		/* The case of same coordinate does not occur (default case) */
		else {
			/* Create vertex 3 */
			vertices[2] = add_vertex((*current)->_x[_po[0]], point->_x[_po[1]], point->_x[_po[2]]);
			/* There is no case of same coordinate */
			saved = nullptr;	
			/* If intersected point is partially dominated - remove it from the representation tree */
			if((*current)->_x[_po[1]] == point->_x[_po[1]]) {
				/* If intersected point was marked for addition and breaks case of same coord */
				if((*current)->_state == 1 && !tmp2_it.is_sentinel() && point->_x[_po[2]] == (*tmp2_it)->_x[_po[2]]
				   && _oo[_po[1]]((*tmp2_it)->_x[_po[1]], point->_x[_po[1]]) && (*tmp2_it)->_state == 0) save_break = 1;

				/* If it was a point marked for addition - update their number i\n the projection tree */
				if((*current)->_state == 1) --add_in;
				_projection.erase(current);
			}
		}

		/* Add triangles 103 and 123 (being 0 the point called into this function) */
		add_triangle(index, *p_it, vertices[0], *p_it, vertices[2]);
		add_triangle(index, *p_it, vertices[0], vertices[1], vertices[2]);

		return 1;
	}
	/* === Compute the facets of the frontier === */


	/* === Add vertex === */
	private:
	Point *add_vertex(double x1, double x2, double x3)
	{
		double coordinates[3];
		coordinates[_po[0]] = x1; coordinates[_po[1]] = x2; coordinates[_po[2]] = x3;
		Point *point, *tmp;
		point = *_vertices[0].insert(tmp = new Point(coordinates)).first;
		if(point != tmp) delete tmp;
		else { _vertices[1].insert(point); _vertices[2].insert(point); }
		return point;
	}
	/* === Add vertex === */


	/* === Add triangle === */
	private:
	void add_triangle(int index, Point *p, Point *p1, Point *p2, Point *p3)
	{
		_p_to_ts[index].insert({p, _triangles.insert(_triangles.end(), Triangle(index, p1, p2, p3))});
		++p1->_n_tri; ++p2->_n_tri; ++p3->_n_tri;
	}
	/* === Add triangle === */


	/* === Delete triangles */
	private:
	void rem_triangles (int index, Point *point)
	{
		Triangle *t;
		auto range = _p_to_ts[index].equal_range(point);
		for(auto it = range.first; it != range.second; ++it) {
			t = &*it->second;
			for(int i = 0; i < 3; i++)
				/* Delete vertex if it ceases to have associated triangles */
				if(--t->_v[i]->_n_tri == 0 && t->_v[i]->_optimal == 0) {
					for(int j = 0; j < 3; ++j) _vertices[j].erase(t->_v[i]);
					delete t->_v[i]; }
			_triangles.erase((*it).second);
		}
		_p_to_ts[index].erase(point);
	}
	/* === Delete triangles */

	template <typename T>
	void print_tree(int spaces, typename T::traversor tr)
	{
		if(!tr.has_node()) return;
		if(tr.has_right()) print_tree<T>(spaces+4, tr.right());
		printf("%*s", spaces, "");
		printf("%lf %lf %lf\t\t\t\t%d\n", (*tr)->_x[0], (*tr)->_x[1], (*tr)->_x[2], tr._node->_balance);
		if(tr.has_left()) print_tree<T>(spaces+4, tr.left());
	}
};

#endif
