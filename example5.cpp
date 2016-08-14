#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "pfv3d"

int main(int argc, char const *argv[])
{
	int n_points, i;
	double **points;
	pfv3d visualiser(0, 0, 0);
	FILE *f = fopen("Input/surf06-500.dat", "r");

	fscanf(f, "%d", &n_points);
	points = (double **) malloc(n_points * sizeof(double *));

	for(i = 0; i < n_points; ++i) {
		points[i] = (double *) malloc(3 * sizeof(double));
		fscanf(f, "%lf %lf %lf", &points[i][0], &points[i][1], &points[i][2]);
	}
	
	fclose(f);

	std::random_shuffle(&points[0], &points[n_points]);

	for(i = 0; i < n_points; ++i) {
		if(i % 1000 == 0)     visualiser.orientation(0, 0, 0);
		else if(i % 500 == 0) visualiser.orientation(1, 1, 1);
		visualiser.add_point(points[i]);
		visualiser.compute(3);
	}
	visualiser.display();

	std::random_shuffle(&points[0], &points[n_points]);

	for(i = 0; i < n_points; ++i) {
		if(i % 1000 == 0)     visualiser.orientation(0, 0, 0);
		else if(i % 500 == 0) visualiser.orientation(1, 1, 1);
		visualiser.rem_point(points[i]);
		visualiser.compute(3);
	}
	visualiser.display();

	for(i = 0; i < n_points; ++i)
		free(points[i]);
	free(points);
	
	return 0; 
}
