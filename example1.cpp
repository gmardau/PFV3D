#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "pfv3d"

int main(int argc, char const *argv[])
{
	int n_points, i;
	double *points;
	pfv3d visualiser;
	FILE *f = fopen("Input/surf05.dat", "r");

	fscanf(f, "%d", &n_points);
	points = (double *) malloc(n_points * sizeof(double) * 3);

	for(i = 0; i < n_points*3; i+=3) 
		fscanf(f, "%lf %lf %lf", &points[i], &points[i+1], &points[i+2]);
	
	fclose(f);

	visualiser.add_points(n_points, points);
	visualiser.compute();
	visualiser.display();
	visualiser.export_file("output");

	free(points);
	
	return 0; 
}
