#ifndef _PFV3D_H_
#define _PFV3D_H_

#include <unistd.h>
#include <limits>
#include <list>
#include <set>
#include <unordered_map>
#include "../../Libraries/CppTreeLib/tree.h"

struct Point {
	mutable int _did, _state, _n_tri;
	double _x[3];
	Point (double x1, double x2, double x3) : _state(1), _n_tri(0), _x{x1, x2, x3} {}
	Point (double *x) : _state(1), _n_tri(0), _x{x[0], x[1], x[2]} {}
	bool operator<(const Point &p) const
	{
		if(_x[0] < p._x[0]) return 1;; if(_x[0] > p._x[0]) return 0;
		if(_x[1] < p._x[1]) return 1;; if(_x[1] > p._x[1]) return 0;
		if(_x[2] < p._x[2]) return 1;; return 0;
	}
	// bool operator==(const Point p)
	// { return _x[0] == p._x[0] && _x[1] == p._x[1] && _x[2] == p._x[2]; }
	// bool operator!=(const Point p)
	// { return _x[0] != p._x[0] || _x[1] != p._x[1] || _x[2] != p._x[2]; }
};
/* === Point compare function === */
template<int x1, int x2, int x3> struct point_comp
{
	bool operator()(const Point *p1, const Point *p2)
	{
		if(p1->_x[x1] < p2->_x[x1]) return 1;; if(p1->_x[x1] > p2->_x[x1]) return 0;
		if(p1->_x[x2] < p2->_x[x2]) return 1;; if(p1->_x[x2] > p2->_x[x2]) return 0;
		if(p1->_x[x3] < p2->_x[x3]) return 1;; return 0;
	}
};
/* === Point compare function === */
/* === Projection compare function === */
int _po[3];
bool proj_comp(const Point* p1, const Point* p2)
{
	if(p1->_x[_po[0]] < p2->_x[_po[0]]) return 1;; if(p1->_x[_po[0]] > p2->_x[_po[0]]) return 0;
	if(p1->_x[_po[1]] < p2->_x[_po[1]]) return 1;; if(p1->_x[_po[1]] > p2->_x[_po[1]]) return 0;
	if(p1->_x[_po[2]] < p2->_x[_po[2]]) return 1;; return 0;
}
/* === Projection compare function === */

struct Triangle {
	Point *_v[3];
	public:
	Triangle (Point *v1, Point *v2, Point *v3) : _v{v1, v2, v3} {}
};

#include "pfv3d_display.h"

class pfv3d
{
	#define DMAX std::numeric_limits<double>::max()
	#define DMIN std::numeric_limits<double>::lowest()
	typedef std::set<Point *, point_comp<0,1,2>> _Set_P0;
	typedef std::set<Point *, point_comp<1,2,0>> _Set_P1;
	typedef std::set<Point *, point_comp<2,0,1>> _Set_P2;
	typedef tree<tree_avl, Point *, decltype(&proj_comp)> _Tree_Proj;
	typedef std::set<Point> _Set_V;
	typedef std::list<Triangle> _List_T;
	typedef std::unordered_multimap<Point *, std::list<Triangle>::iterator> _Map_PTs;

	/* === Variables === */
	bool _minmax;
	double _limits[3][2];
	_Set_P0 _points_0, _add_0, _rem_0;
	_Set_P1 _points_1, _add_1, _rem_1;
	_Set_P2 _points_2, _add_2, _rem_2;
	_Tree_Proj _projection = _Tree_Proj(&proj_comp);
	_Set_P0 _non_optimal;
	_Set_V _vertices;
	_List_T _triangles;
	_Map_PTs _p_to_ts;
	pfv3d_display _display;
	/* === Variables === */

	/* === Constructor/Destructor === */
	public:
	pfv3d () : pfv3d(0) {}
	pfv3d (bool minmax) : _minmax(minmax), _limits{{DMAX, DMIN}, {DMAX, DMIN}, {DMAX, DMIN}} {}
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
		_limits[0][0] = _limits[1][0] = _limits[2][0] = DMAX;
		_limits[0][1] = _limits[1][1] = _limits[2][1] = DMIN;
	}
	void maximise ()
	{
		if(_minmax == 1) return;
		_minmax = 0;
		_limits[0][0] = _limits[1][0] = _limits[2][0] = DMAX;
		_limits[0][1] = _limits[1][1] = _limits[2][1] = DMIN;
	}
	/* === Define the optimisation orientation === */

	/* === Call the visualiser === */
	public:
	void display () { _display.display_frontier(0, _points_0, _vertices, _triangles); }
	private:
	void display_i () { _display.display_frontier(1, _points_0, _vertices, _triangles); usleep(100); }
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
			p = *_points_0.insert(tmp = new Point(&points[i])).first;
			/* If there is already a point with the specified coordinates */
			if(p != tmp) {
				/* If it was marked for removal, restore it */
				if(p->_state == -1) {
					p->_state = 0;
					_rem_0.erase(p);
					_rem_1.erase(p);
					_rem_2.erase(p);
				}
				delete tmp;
			}
			/* If it is a new point */
			else {
				_add_0.insert(p);
				_add_1.insert(p); _points_1.insert(p);
				_add_2.insert(p); _points_2.insert(p);
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
				_add_0.erase(p); _points_0.erase(p); 
				_add_1.erase(p); _points_1.erase(p); 
				_add_2.erase(p); _points_2.erase(p);
				delete p;
			} 
			/* If not, mark it for removal */
			else if(p->_state == 0) {
				p->_state = -1;
				_rem_0.insert(p);
				_rem_1.insert(p);
				_rem_2.insert(p);
			}
		}
	}
	/* === Remove points from the frontier === */

	/* === Compute the facets of the frontier === */
	public:
	void compute (bool display = 0)
	{
		/* If no action is needed */
		if(_add_0.empty() && _rem_0.empty()) { if(display) this->display(); return; }
		
		/* Get the new limits */
		double limits[3][2] = {{DMAX, DMIN}, {DMAX, DMIN}, {DMAX, DMIN}};
		for(_Set_P0::iterator it = _points_0.begin(); it != _points_0.end(); ++it)
			if((*it)->_state != -1) { limits[0][0] = (*it)->_x[0]; break; }
		for(_Set_P0::reverse_iterator it = _points_0.rbegin(); it != _points_0.rend(); ++it)
			if((*it)->_state != -1) { limits[0][1] = (*it)->_x[0]; break; }
		for(_Set_P1::iterator it = _points_1.begin(); it != _points_1.end(); ++it)
			if((*it)->_state != -1) { limits[1][0] = (*it)->_x[1]; break; }
		for(_Set_P1::reverse_iterator it = _points_1.rbegin(); it != _points_1.rend(); ++it)
			if((*it)->_state != -1) { limits[1][1] = (*it)->_x[1]; break; }
		for(_Set_P2::iterator it = _points_2.begin(); it != _points_2.end(); ++it)
			if((*it)->_state != -1) { limits[2][0] = (*it)->_x[2]; break; }
		for(_Set_P2::reverse_iterator it = _points_2.rbegin(); it != _points_2.rend(); ++it)
			if((*it)->_state != -1) { limits[2][1] = (*it)->_x[2]; break; }
		/* Verify if the limits have changed */
		bool new_limits = 0;
		if(_limits[0][0] != limits[0][0]) { _limits[0][0] = limits[0][0]; new_limits = 1; }
		if(_limits[0][1] != limits[0][1]) { _limits[0][1] = limits[0][1]; new_limits = 1; }
		if(_limits[1][0] != limits[1][0]) { _limits[1][0] = limits[1][0]; new_limits = 1; }
		if(_limits[1][1] != limits[1][1]) { _limits[1][1] = limits[1][1]; new_limits = 1; }
		if(_limits[2][0] != limits[2][0]) { _limits[2][0] = limits[2][0]; new_limits = 1; }
		if(_limits[2][1] != limits[2][1]) { _limits[2][1] = limits[2][1]; new_limits = 1; }
		/* If yes, redo the whole frontier */
		Point *p;
		if(new_limits == 1) {
			for(_Set_P0::iterator it = _points_0.begin(); it != _points_0.end(); ) {
				p = (*it); ++it;
				if(p->_state == 0) {
					p->_state = 1;
					_add_0.insert(p); _add_1.insert(p); _add_2.insert(p);
				} else if(p->_state == -1) {
					_points_0.erase(p); _points_1.erase(p); _points_2.erase(p);
					delete p;
				}
			}
			_rem_0.clear(); _rem_1.clear(); _rem_2.clear();
		}

		/* Call each coordinate sweep */
		// _po[0] = 1; _po[1] = 2; _po[2] = 0;
		// sweep<_Set_P0>(_points_0, _add_0, _rem_0);

		// _po[0] = 2; _po[1] = 0; _po[2] = 1;
		// sweep<_Set_P1>(_points_1, _add_1, _rem_1);

		_po[0] = 0; _po[1] = 1; _po[2] = 2;
		sweep<_Set_P2>(_points_2, _add_2, _rem_2);

		/* Remove non optimal points */
		for(_Set_P0::iterator it = _non_optimal.begin(); it != _non_optimal.end(); ++it) {
			_points_0.erase(*it);
			_points_1.erase(*it);
			_points_2.erase(*it);
			delete *it;
		}
		_non_optimal.clear();
	}

	private: template<typename T>
	void sweep (T points, T add, T rem)
	{
		/* Insert sentinels in the projection tree */
		double sentinel_coord[2][3] = {{0, 0, 0}, {0, 0, 0}};
		Point *sentinels[2];
		double borders[3] = {0, 0, 0};
		borders[_po[0]] = (_limits[_po[0]][1] - _limits[_po[0]][0]) * 0.1;
		borders[_po[1]] = (_limits[_po[1]][1] - _limits[_po[1]][0]) * 0.1;
		if(_minmax == 0) {			
			sentinel_coord[0][_po[0]] = _limits[_po[0]][1] + borders[_po[0]]; sentinel_coord[0][_po[1]] = DMIN;
			sentinel_coord[1][_po[1]] = _limits[_po[1]][1] + borders[_po[1]]; sentinel_coord[1][_po[0]] = DMIN;
		} else {
			sentinel_coord[0][_po[0]] = _limits[_po[0]][0] - borders[_po[0]]; sentinel_coord[0][_po[1]] = DMAX;
			sentinel_coord[1][_po[1]] = _limits[_po[1]][0] - borders[_po[1]]; sentinel_coord[1][_po[0]] = DMAX;
		}
		_projection.insert(sentinels[0] = new Point(&sentinel_coord[0][0]));
		_projection.insert(sentinels[1] = new Point(&sentinel_coord[1][0]));
		sentinels[0]->_state = sentinels[1]->_state = 0; // TALVEZ NAO SEJA PRECISO

		/* Perform the sweep */
		bool redo, result;
		int to_add = add.size(), to_rem = rem.size(), add_in = 0, rem_in = 0;
		Point *same_coord = nullptr;
		typename T::iterator it_add = add.begin(), it_rem = rem.begin();

		for(typename T::iterator it = points.begin(); it != points.end() &&
		   (to_add > 0 || to_rem > 0 || add_in > 0 || rem_in > 0); ++it) {

			// Se for ponto para adicionar ou se tiver a mesma coordenada de ponto para adicionar - refazer
			// Se for ponto para remover - apenas por na arvore
			// Se tiver mesma coordenada de ponto para remover - refazer
			// Se ainda houver pontos adicionados ou removidos na arvore - refazer
			// Se same_coord nao estiver a null - refazer
			// Nenhum dos anteriores - apenas por na arvore

			if((*it)->_state == -1) redo = 0;
			else if((*it)->_x[_po[2]] == (*it_add)->_x[_po[2]] || (*it)->_x[_po[2]] == (*it_rem)->_x[_po[2]] ||
			   add_in > 0 || rem_in > 0 || same_coord != nullptr) redo = 1;
			else redo = 0;

			if(_minmax == 0) {
				if(redo == 0) result = 0;
				else          result = facet_min(*it, same_coord, add_in, rem_in);
			} else {
				if(redo == 0) result = 0;
				else          result = facet_max(*it, same_coord, add_in, rem_in);
			}

		   	/* Save the point for elimination if it is non optimal */
			if(result == 0) _non_optimal.insert(*it);

			/* Update points to be added or to be removed */
			     if((*it)->_state ==  1) { ++it_add; --to_add; }
			else if((*it)->_state == -1) { ++it_rem; --to_rem; }
		}
		
		/* Clear projection tree and sentinels */
		_projection.clear();
		delete sentinels[0]; delete sentinels[1];

		/* Clear 'add' and 'rem' and remove removing points from 'points' */
		add.clear();
		for(typename T::iterator it = rem.begin(); it != rem.end(); ++it) points.erase(it);
		rem.clear();
		/* Ver isto, que é preciso fzr delete aos pontos no rem, mas que podem estar também no non_optimal */
	}
	private:
	bool facet_min (Point *point, Point *&same_coord, int &add_in, int &rem_in)
	{
		/* Remove triangles previously created by the point */
		_p_to_ts.erase(point);

		/* Insert point in the projection tree */
		_Tree_Proj::iterator p_it = _projection.insert(point).first, tmp_it;
		/* Get the predecessor (it must not be marked for removal) */
		for(tmp_it = p_it.prev(); (*tmp_it)->_state == -1; tmp_it = tmp_it.prev()) {}
		/* If point is not optimal - remove it from the projection tree and exit function */
		if((*tmp_it)->_x[_po[1]] <= point->_x[_po[1]]) {
			_projection.erase(p_it);
			return 0;
		} 
		/* If point is optimal and marked for addition - update their number in the projection tree */ 
		else if(point->_state == 1) ++add_in;

		/* Get the intersected point in the projection tree */
		_Tree_Proj::iterator current = p_it.next();
		/* Find the successor (it must not be marked for removal) */
		for(; (*current)->_x[_po[0]] == point->_x[_po[0]] && (*current)->_state == -1; current = p_it.next()) {
			/* Successor has equal coordinate and is marked for removal - remove it from the projection tree */
			_projection.erase(current);
			/* Point is marked for removal - update their number in the projection tree */
			--rem_in;
		}
		/* If successor does not have equal coordinate - intersected point is the predecessor obtained earlier */
		if((*current)->_x[_po[0]] != point->_x[_po[0]]) current = tmp_it;

		/* Add vertex 1 */

		/* If intersected point has equal coordinate - remove it from the representation tree */
		if((*current)->_x[_po[0]] == point->_x[_po[0]]) {
			/* If it was a point marked for addition - update their number in the projection tree */
			if((*current)->_state == 1) --add_in;
			_projection.erase(current);
		}

		return 1;
	}

	private:
	bool facet_max (Point *point, Point *&same_coord, int &add_in, int &rem_in)
	{
		return 1;
	}
	/* === Compute the facets of the frontier === */

	// /* If point is not an optimal solution */
	// _Tree_Proj::iterator p_it = _projection.insert(*point).first;
	// if((_minmax == 0 && (*p_it.prev())->_x[_po[1]] <= (*point)->_x[_po[1]]) ||
	//    (_minmax == 1 && (*p_it.next())->_x[_po[1]] >= (*point)->_x[_po[1]])) {
	// 	_projection.erase(p_it);
	// 	return 0;
	// }

	void print_tree(int spaces, _Tree_Proj::traversor tr)
	{
		if(!tr.has_node()) return;
		if(tr.has_right()) print_tree(spaces+4, tr.right());
		printf("%*s", spaces, "");
		printf("%g %g %g\t\t\t\t%d\n", (*tr)->_x[0], (*tr)->_x[1], (*tr)->_x[2], tr._node->_balance);
		if(tr.has_left()) print_tree(spaces+4, tr.left());
	}
};

#endif
