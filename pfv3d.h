#ifndef _PFV3D_H_
#define _PFV3D_H_

#include <unistd.h>
#include <limits>
#include <list>
#include <set>
#include <unordered_map>
#include "../../Libraries/CppTreeLib/tree.h"

struct Point
{
	bool _optimal;
	int _state, _n_tri;
	double _x[3];
	Point (double x1, double x2, double x3, bool optimal = 0) : _optimal(optimal), _state(1), _n_tri(0), _x{x1, x2, x3} {}
	Point (double *x, bool optimal = 0) : _optimal(optimal), _state(1), _n_tri(0), _x{x[0], x[1], x[2]} {}
};
/* === Point compare function === */
template<int x1, int x2, int x3> struct point_comp {
	bool operator()(const Point *p1, const Point *p2) {
		if(p1->_x[x1] < p2->_x[x1]) return 1;; if(p1->_x[x1] > p2->_x[x1]) return 0;
		if(p1->_x[x2] < p2->_x[x2]) return 1;; if(p1->_x[x2] > p2->_x[x2]) return 0;
		if(p1->_x[x3] < p2->_x[x3]) return 1;; return 0; } };
/* === Point compare function === */
/* === Projection compare function === */
int _po[3];
bool proj_comp(const Point* p1, const Point* p2) {
	if(p1->_x[_po[0]] < p2->_x[_po[0]]) return 1;; if(p1->_x[_po[0]] > p2->_x[_po[0]]) return 0;
	if(p1->_x[_po[1]] < p2->_x[_po[1]]) return 1;; if(p1->_x[_po[1]] > p2->_x[_po[1]]) return 0;
	if(p1->_x[_po[2]] < p2->_x[_po[2]]) return 1;; return 0; }
/* === Projection compare function === */

struct Triangle
{
	Point *_v[3];
	Triangle (Point *v1, Point *v2, Point *v3) : _v{v1, v2, v3} {}
	~Triangle () {}
};

#include "pfv3d_display.h"

struct pfv3d
{
	#define DMAX std::numeric_limits<double>::max()
	#define DMIN std::numeric_limits<double>::lowest()
	typedef tree<tree_avl, Point *, point_comp<0,1,2>> _Tree_P0;
	typedef tree<tree_avl, Point *, point_comp<1,2,0>> _Tree_P1;
	typedef tree<tree_avl, Point *, point_comp<2,0,1>> _Tree_P2;
	typedef tree<tree_avl, Point *, decltype(&proj_comp)> _Tree_Proj;
	typedef std::list<Point *> _List_P;
	typedef std::list<Triangle> _List_T;
	typedef std::unordered_multimap<Point *, _List_T::iterator> _Map_PTs;

	/* === Variables === */
	bool _minmax;
	int _display_mode;
	double _limits[3][2];
	_Tree_P0 _points_0, _add_0, _rem_0, _vertices_0;
	_Tree_P1 _points_1, _add_1, _rem_1, _vertices_1;
	_Tree_P2 _points_2, _add_2, _rem_2, _vertices_2;
	_Tree_Proj _projection = _Tree_Proj(&proj_comp);
	_List_P _non_optimal;
	_List_T _triangles;
	_Map_PTs _p_to_ts_0, _p_to_ts_1, _p_to_ts_2;
	pfv3d_display _display;
	/* === Variables === */

	/* === Constructor/Destructor === */
	public:
	pfv3d () : pfv3d(0) {}
	pfv3d (bool minmax) : _minmax(minmax), _limits{{DMIN, DMAX}, {DMIN, DMAX}, {DMIN, DMAX}} {}
	~pfv3d ()
	{

	}
	/* === Constructor/Destructor === */

	/* === Define the optimisation orientation === */
	public:
	void minimise ()
	{
		if(_minmax == 0) return;
		_minmax = 1;
		_limits[0][0] = _limits[1][0] = _limits[2][0] = DMIN;
		_limits[0][1] = _limits[1][1] = _limits[2][1] = DMAX;
	}
	void maximise ()
	{
		if(_minmax == 1) return;
		_minmax = 0;
		_limits[0][0] = _limits[1][0] = _limits[2][0] = DMIN;
		_limits[0][1] = _limits[1][1] = _limits[2][1] = DMAX;
	}
	/* === Define the optimisation orientation === */

	/* === Call the visualiser === */
	public:
	void display () { _display.display_frontier(0, _points_0.size(), _triangles); }
	private:
	void display_i () { _display.display_frontier(1, _points_0.size(), _triangles); /*usleep(10000);*/ }
	/* === Call the visualiser === */

	/* === Add points to the frontier === */
	public:
	void add_point (double x1, double x2, double x3) { double p[3] = {x1, x2, x3}; add_points(1, p); }
	void add_point (double point[]) { add_points(1, point); }
	void add_points (int n, double points[][3]) { add_points(n, &points[0][0]); }
	void add_points (int n, double points[])
	{
		Point *tmp, *p;
		for(int i = 0; i < n*3; i += 3) {
			p = *_points_0.insert(tmp = new Point(&points[i], 1)).first;
			/* If there is already a point with the specified coordinates */
			if(p != tmp) {
				/* If it was marked for removal, restore it */
				if(p->_state == -1) {
					p->_state = 0;
					_rem_0.erase(p); _rem_1.erase(p); _rem_2.erase(p);
				}
				delete tmp;
			}
			/* If it is a new point */
			else {
				_add_0.insert(p); _add_1.insert(p); _add_2.insert(p);
				_points_1.insert(p); _points_2.insert(p);
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
			p = *_points_0.find((tmp = new Point(&points[i]))); delete tmp;
			/* If there is no point with the specified coordinates */
			if(p == *_points_0.end()) continue;
			/* If it was yet to be added, remove it */
			if(p->_state == 1) {
				_add_0.erase(p); _add_1.erase(p); _add_2.erase(p);
				_points_0.erase(p); _points_1.erase(p); _points_2.erase(p);
				delete p;
			} 
			/* If not, mark it for removal */
			else if(p->_state == 0) {
				p->_state = -1;
				_rem_0.insert(p); _rem_1.insert(p); _rem_2.insert(p);
			}
		}
	}
	/* === Remove points from the frontier === */

	/* === Compute the facets of the frontier === */
	private: template<typename T>
	void verify_limits (T &points, T &rem, _Map_PTs &p_to_ts, T &vertices, int index, double limits[3][2], double extremes[3])
	{
		/* Get new limits */
		for(typename T::iterator it = points.begin(); it != points.end(); ++it)
			if((*it)->_state != -1) { limits[index][0] = (*it)->_x[index]; break; }
		for(typename T::reverse_iterator it = points.rbegin(); it != points.rend(); ++it)
			if((*it)->_state != -1) { limits[index][1] = (*it)->_x[index]; break; }

		/* Compute extremes based on the new limits */
		double range = limits[index][1] - limits[index][0];
		if(_minmax == 0) extremes[index] = limits[index][1] + (range == 0 ? 1 : range * 0.1);
		else             extremes[index] = limits[index][0] - (range == 0 ? 1 : range * 0.1);

		/* If the limits have not changed - exit function */
		if(_limits[index][0] == limits[index][0] && _limits[index][1] == limits[index][1]) return;
		/* If they did - update surface */
		if(_minmax == 0) {
			typename T::reverse_iterator it;
			/* Remove points marked for removal that are beyond the new limits */
			for(it = points.rbegin(); it != points.rend() && (*it)->_state != 0; ) {
				if((*it)->_state == -1) {
					del_triangles(*it, p_to_ts); rem.erase(*it); points.erase(*it); it = points.rbegin();
				} else ++it; }
			/* Translate vertices that are beyond the old limits to the new extremes */
			for(it = vertices.rbegin(); it != vertices.rend() && (*it)->_x[index] > _limits[index][1]; ++it)
				(*it)->_x[index] = extremes[index];
		} else {

		}

		/* Update limits */
		_limits[index][0] = limits[index][0]; _limits[index][1] = limits[index][1];
	}

	public:
	void compute (int display_mode = 0)
	{
		/* If no action is needed */
		if(_add_0.empty() && _rem_0.empty()) { if(display_mode == 1 || display_mode == 3) display(); return; }
		_display_mode = display_mode;
		
		/* Update limits and compute extremes (to be used by sentinels) */
		double limits[3][2] = {{DMIN, DMAX}, {DMIN, DMAX}, {DMIN, DMAX}}, extremes[2];
		verify_limits<_Tree_P0>(_points_0, _rem_0, _p_to_ts_0, _vertices_0, 0, limits, extremes);
		verify_limits<_Tree_P1>(_points_1, _rem_1, _p_to_ts_1, _vertices_1, 1, limits, extremes);
		verify_limits<_Tree_P2>(_points_2, _rem_2, _p_to_ts_2, _vertices_2, 2, limits, extremes);

		/* In case all points have been removed - clean triangles and vertices */
		if(_points_0.empty()) {
			_triangles.clear();
			for(_Tree_P0::iterator it = _vertices_0.begin(); it != _vertices_0.end(); it = _vertices_0.begin()) {
				delete *it; _vertices_0.erase(it); }
			_vertices_1.clear(); _vertices_2.clear();
			if(display_mode == 1 || display_mode == 3) display();
			return;
		}

		/* Call each coordinate sweep */
		_po[0] = 1; _po[1] = 2; _po[2] = 0; sweep<_Tree_P0>(_points_0, _add_0, _rem_0, _p_to_ts_0, extremes);
		_po[0] = 2; _po[1] = 0; _po[2] = 1;	sweep<_Tree_P1>(_points_1, _add_1, _rem_1, _p_to_ts_1, extremes);
		_po[0] = 0; _po[1] = 1; _po[2] = 2;	sweep<_Tree_P2>(_points_2, _add_2, _rem_2, _p_to_ts_2, extremes);

		/* Remove non optimal points */
		for(_List_P::iterator it = _non_optimal.begin(); it != _non_optimal.end(); it = _non_optimal.begin()) {
			_points_0.erase(*it); _points_1.erase(*it); _points_2.erase(*it);
			_non_optimal.erase(it); delete *it; }

		/* Remove points marked for removal and clear removal sets */
		for(_Tree_P0::iterator it = _rem_0.begin(); it != _rem_0.end(); it = _rem_0.begin()) {
			_points_0.erase(*it); _points_1.erase(*it); _points_2.erase(*it);
			_rem_1.erase(*it); _rem_2.erase(*it); delete *it; _rem_0.erase(it); }
		for(_Tree_P1::iterator it = _rem_1.begin(); it != _rem_1.end(); it = _rem_1.begin()) {
			_points_0.erase(*it); _points_1.erase(*it); _points_2.erase(*it);
			_rem_0.erase(*it); _rem_2.erase(*it); delete *it; _rem_1.erase(it); }
		for(_Tree_P2::iterator it = _rem_2.begin(); it != _rem_2.end(); it = _rem_2.begin()) {
			_points_0.erase(*it); _points_1.erase(*it); _points_2.erase(*it);
			_rem_0.erase(*it); _rem_1.erase(*it); delete *it; _rem_2.erase(it); }

		/* Update state of points marked for addition and clear addition sets */
		for(_Tree_P0::iterator it = _add_0.begin(); it != _add_0.end(); it = _add_0.begin()) {
			(*it)->_state = 0; _add_0.erase(it); }
		_add_1.clear(); _add_2.clear();

		if(display_mode == 1 || display_mode == 3) display();
	}

	private: template<typename T>
	void sweep (T &points, T &add, T &rem, _Map_PTs &p_to_ts, double extremes[3])
	{
		/* Insert sentinels in the projection tree */
		Point *sentinels[2];
		double sentinel_coord[2][3] = {{0, 0, 0}, {0, 0, 0}};
		sentinel_coord[0][_po[0]] = extremes[_po[0]]; sentinel_coord[1][_po[1]] = extremes[_po[1]];
		if(_minmax == 0) sentinel_coord[0][_po[1]] = sentinel_coord[1][_po[0]] = DMIN;
		else             sentinel_coord[0][_po[1]] = sentinel_coord[1][_po[0]] = DMAX;
		_projection.insert(sentinels[0] = new Point(&sentinel_coord[0][0], 1));
		_projection.insert(sentinels[1] = new Point(&sentinel_coord[1][0], 1));
		sentinels[0]->_state = sentinels[1]->_state = 0; // TALVEZ NAO SEJA PRECISO

		/* Perform the sweep */
		bool redo, special = 0, result;
		int to_add = add.size(), to_rem = rem.size(), add_in = 0, rem_in = 0;
		Point *saved = nullptr;
		typename T::iterator it_add = add.begin(), it_rem = rem.begin();

		for(typename T::iterator it = points.begin(); it != points.end() &&
		   (to_add > 0 || to_rem > 0 || add_in > 0 || rem_in > 0 || saved != nullptr || special == 1); ++it) {

			// Se for ponto para remover - apenas por na arvore
			// Se for ponto para adicionar - refazer
			// Se ainda houver pontos adicionados ou removidos na arvore - refazer
			// Se saved nao estiver a null - refazer
			// Se tiver a mesma coordenada de ponto para adicionar - refazer
			// Se tiver mesma coordenada de ponto para remover - refazer
			// Se for o caso especial onde um ponto novo corta o caso de coordenadas repetidas (saved) - refazer
			// Nenhum dos anteriores - apenas por na arvore

			if((*it)->_state == -1) redo = 0;
			else if((*it)->_state == 1) redo = 1;
			else if(add_in > 0 || rem_in > 0 || saved != nullptr) redo = 1;
			else if(to_add > 0 && (*it)->_x[_po[2]] == (*it_add)->_x[_po[2]]) redo = 1;
			else if(to_rem > 0 && (*it)->_x[_po[2]] == (*it_rem)->_x[_po[2]]) redo = 1;
			else if(special == 1) { special = 0; redo = 1; }
			else redo = 0;

			if(redo == 0) {
				if(_minmax == 0) insert_only_min(*it, p_to_ts, rem_in);
				else             insert_only_max(*it, p_to_ts, rem_in);
			} else {
				if(_minmax == 0) result = facet_min<T>(it, p_to_ts, saved, add_in, rem_in, special);
				else	         result = facet_max<T>(it, p_to_ts, saved, add_in, rem_in, special);
			   	/* Save the point for elimination if it is non optimal */
				if(result == 0) _non_optimal.insert(_non_optimal.end(), *it);
				if(_display_mode == 2 || _display_mode == 3) display_i();
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
	void insert_only_min (Point *point, _Map_PTs &p_to_ts, int &rem_in)
	{
		/* If point is marked for removal - remove triangles previously created by it */
		if(point->_state == -1) del_triangles(point, p_to_ts);

		/* Insert point in the projection tree */
		_Tree_Proj::iterator p_it = _projection.insert(point).first;
		/* If point is not optimal - remove it from the projection tree and exit function */
		if((*p_it.prev())->_x[_po[1]] <= point->_x[_po[1]])
			{ _projection.erase(p_it); if(point->_state == 0) printf("ASNEIRA\n"); return; } 
		/* If point is optimal and marked for removal - update their number in the projection tree */ 
		else if(point->_state == -1) ++rem_in;

		/* Cycle to process dominated points until one that is not dominated is reached */
		_Tree_Proj::iterator current = p_it.next(), tmp_it;
		for( ; point->_x[_po[1]] <= (*current)->_x[_po[1]]; current = tmp_it) {
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

	private: template<typename T>
	bool facet_min (typename T::iterator &p, _Map_PTs &p_to_ts, Point *&saved, int &add_in, int &rem_in, bool &special)
	{
		Point *point = *p;

		/* Remove triangles previously created by the point */
		del_triangles(point, p_to_ts);

		/* Insert point in the projection tree */
		_Tree_Proj::iterator p_it = _projection.insert(point).first, tmp_it;
		/* Get the predecessor (it must not be marked for removal) */
		for(tmp_it = p_it.prev(); (*tmp_it)->_state == -1; tmp_it = tmp_it.prev()) {}
		/* If point is not optimal - remove it from the projection tree and exit function */
		if((*tmp_it)->_x[_po[1]] <= point->_x[_po[1]]) { _projection.erase(p_it); return 0; } 
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
				/* If successor is marked for removal and is dominated - remove it from the projection
				   tree and update their number in the projection tree */
				if(point->_x[_po[1]] <= (*current)->_x[_po[1]]) { _projection.erase(current); --rem_in; }
			}

			/* If the previous point called into this function has the same coordinate as the
			   current one - recover saved vertex 2 */
			if(saved != nullptr) vertices[1] = saved;
			/* If does not - create vertex 2 */
			else vertices[1] = add_vertex((*current)->_x[_po[0]], vertices[0]->_x[_po[1]], point->_x[_po[2]]);

			/* If current point is not fully dominated - leave cycle */
			if((*current)->_x[_po[1]] <= point->_x[_po[1]]) break;

			/* Create vertex 3 */
			vertices[2] = add_vertex((*current)->_x[_po[0]], (*current)->_x[_po[1]], point->_x[_po[2]]);

			/* Add triangles 103 and 123 (being 0 the point called into this function) */
			add_triangle(*p_it, vertices[0], vertices[1], vertices[2], p_to_ts);
			add_triangle(*p_it, vertices[0], *p_it, vertices[2], p_to_ts);

			/* There is no case of same coordinate, analysed only after cycle, when ending this function */
			saved = nullptr;

			/* Remove current point from the projection tree (it is fully dominated) */
			tmp_it = current.next();
			/* If it was a point marked for addition - update their number in the projection tree */
			if((*current)->_state == 1) --add_in;
			_projection.erase(current);
			/* Vertex 3 is convertex into vertex 1 */
			vertices[0] = vertices[2];
		}

		/* Find the next point to be called into this function (must not be marked for removal) */
		typename T::iterator tmp2_it = p.next();
		for( ; tmp2_it.info() != nullptr && (*tmp2_it)->_state == -1; tmp2_it = tmp2_it.next()) {}
		/* If it has same coordinate than the one called into this function, it is not dominated by the latter, 
		   and it intersects the latter in non dominated space - the case of same coordinate occurs */
		if(tmp2_it.info() != nullptr && point->_x[_po[2]] == (*tmp2_it)->_x[_po[2]]
		   && (*tmp2_it)->_x[_po[1]] < point->_x[_po[1]] && (*tmp2_it)->_x[_po[0]] < (*current)->_x[_po[0]]) {
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
				if((*current)->_state == 1 && tmp2_it.info() != nullptr && point->_x[_po[2]] == (*tmp2_it)->_x[_po[2]]
				   && (*tmp2_it)->_x[_po[1]] < point->_x[_po[1]]) special = 1;

				/* If it was a point marked for addition - update their number i\n the projection tree */
				if((*current)->_state == 1) --add_in;
				_projection.erase(current);
			}
		}

		/* Add triangles 103 and 123 (being 0 the point called into this function) */
		add_triangle(*p_it, vertices[0], *p_it, vertices[2], p_to_ts);
		add_triangle(*p_it, vertices[0], vertices[1], vertices[2], p_to_ts);

		return 1;
	}

	private:
	void insert_only_max (Point *point, _Map_PTs &p_to_ts, int &rem_in)
	{
		
	}

	private: template<typename T>
	bool facet_max (typename T::iterator &p, _Map_PTs &p_to_ts, Point *&saved, int &add_in, int &rem_in, bool &special)
	{
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
		point = *_vertices_0.insert(tmp = new Point(coordinates)).first;
		if(point != tmp) delete tmp;
		else { _vertices_1.insert(point); _vertices_2.insert(point); }
		return point;
	}
	/* === Add vertex === */

	/* === Add triangle === */
	private:
	void add_triangle(Point *p, Point *p1, Point *p2, Point *p3, _Map_PTs &p_to_ts)
	{
		p_to_ts.insert({p, _triangles.insert(_triangles.end(), Triangle(p1, p2, p3))});
		++p1->_n_tri; ++p2->_n_tri; ++p3->_n_tri;
	}
	/* === Add triangle === */

	/* === Delete triangles */
	private: template<typename T>
	void del_triangles (Point *point, T &p_to_ts)
	{
		Triangle *t;
		auto range = p_to_ts.equal_range(point);
		for(auto it = range.first; it != range.second; ++it) {
			t = &*(*it).second;
			for(int i = 0; i < 3; i++)
				if(--t->_v[i]->_n_tri == 0 && t->_v[i]->_optimal == 0) {
					_vertices_0.erase(t->_v[i]); _vertices_1.erase(t->_v[i]); _vertices_2.erase(t->_v[i]);
					delete t->_v[i]; }
			_triangles.erase((*it).second);
		}
		p_to_ts.erase(point);
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
