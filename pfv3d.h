#include <limits>
#include <list>
#include <set>
#include <map>
#include <unordered_map>

struct Point {
	int _state, _n_tri;
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
template<int x1, int x2, int x3> struct point_comp
{
	bool operator()(const Point *p1, const Point *p2)
	{
		if(p1->_x[x1] < p2->_x[x1]) return 1;; if(p1->_x[x1] > p2->_x[x1]) return 0;
		if(p1->_x[x2] < p2->_x[x2]) return 1;; if(p1->_x[x2] > p2->_x[x2]) return 0;
		if(p1->_x[x3] < p2->_x[x3]) return 1;; return 0;
	}
};
int pcx[3];
bool proj_comp(const Point* p1, const Point* p2)
{
	if(p1->_x[pcx[0]] < p2->_x[pcx[0]]) return 1;; if(p1->_x[pcx[0]] > p2->_x[pcx[0]]) return 0;
	if(p1->_x[pcx[1]] < p2->_x[pcx[1]]) return 1;; if(p1->_x[pcx[1]] > p2->_x[pcx[1]]) return 0;
	if(p1->_x[pcx[2]] < p2->_x[pcx[2]]) return 1;; return 0;
}

struct Triangle {
	Point *_v[3];
	public:
	Triangle (Point *v1, Point *v2, Point *v3) : _v{v1, v2, v3} {}
};

class pfv3d
{
	#define DMAX std::numeric_limits<double>::max()
	#define DMIN std::numeric_limits<double>::lowest()
	typedef std::set<Point *, point_comp<0,1,2>> _Set_P0;
	typedef std::set<Point *, point_comp<1,2,0>> _Set_P1;
	typedef std::set<Point *, point_comp<2,0,1>> _Set_P2;
	typedef std::set<Point *, decltype(&proj_comp)> _Set_Proj;
	typedef std::set<Point> _Set_V;
	typedef std::list<Triangle> _List_T;
	typedef std::unordered_multimap<Point *, std::list<Triangle>::iterator> _Map_PTs;

	/* === Variables === */
	bool _minmax;
	double _limits[3][2];
	_Set_P0 _points_0, _add_0, _rem_0;
	_Set_P1 _points_1, _add_1, _rem_1;
	_Set_P2 _points_2, _add_2, _rem_2;
	_Set_Proj _projection = _Set_Proj(&proj_comp);
	_Set_V _vertices;
	_List_T _triangles;
	_Map_PTs _p_to_ts;
	/* === Variables === */

	/* === Constructor/Destructor === */
	public:
	pfv3d () : pfv3d(0) {}
	pfv3d (bool minmax) : _minmax(minmax), _limits{{DMAX, DMIN}, {DMAX, DMIN}, {DMAX, DMIN}} /*init display*/ {}
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

	/* === Control the visualiser === */
	public:
	void display () { /*Call display and wait*/ }
	private:
	void display_i () { /*Call display*//*Sleep*/ }
	/* === Control the visualiser === */

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
		if(_add_0.empty() && _rem_0.empty()) { if(display) display(); return; }
		
		/* Verify if the limits have changed */
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

		/* Preparation for each coordinate sweep */
		for(int i = 0; i < 3; i++) {
			pcx[0] = (i+1)%3; pcx[1] = (i+2)%3; pcx[2] = (i+3)%3;


		}
	}

	private:
	template<typename T> void sweep (T points, T add, T rem)
	{

	}
	/* === Compute the facets of the frontier === */
};
