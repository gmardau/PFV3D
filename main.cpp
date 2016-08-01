#include <stdio.h>
#include <stdlib.h>
#include "pfv3d.h"

int main(int argc, char const *argv[])
{
	pfv3d a;
	double b[2][3] = {{3, 3, 1}, {1, 1, 3}};
	a.add_points(2,b);
	a.add_point(2, 2, 2);
	a.add_point(3, 3, 3);
	// a.rem_point(2, 1, 2);
	// a.display();
	a.compute();
	return 0; 
}
