#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "pfv3d"

int main(int argc, char const *argv[])
{
	int n_points, i;
	double **points;
	pfv3d visualiser(0);
	FILE *f = fopen("Input/surf05.dat", "r");

	fscanf(f, "%d", &n_points);
	points = (double **) malloc(n_points * sizeof(double *));

	for(i = 0; i < n_points; ++i) {
		points[i] = (double *) malloc(3 * sizeof(double));
		fscanf(f, "%lf %lf %lf", &points[i][0], &points[i][1], &points[i][2]);
	}

	std::random_shuffle(&points[0], &points[n_points]);

	for(i = 0; i < n_points; ++i)
		visualiser.add_point(points[i]);
	visualiser.compute(0);
	visualiser.display();
	visualiser.display();

	for(i = 0; i < n_points; ++i)
		free(points[i]);
	free(points);
	
	return 0; 
}