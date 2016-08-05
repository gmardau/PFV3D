#include <stdio.h>
#include <stdlib.h>
#include "pfv3d.h"
#include <algorithm>

int main(int argc, char const *argv[])
{
	pfv3d a;
	FILE *f = fopen("Input/surf06.dat", "r");
	int i; fscanf(f, "%d", &i);
	double c[i][3];
	int d[i];
	for(int j = 0; j < i; j++) {
		d[j] = j;
		fscanf(f, "%lf %lf %lf", &c[j][0], &c[j][1], &c[j][2]);
	}
	
	std::random_shuffle(&d[0], &d[i]);
	for(int j = 0; j < i; j+=154){
		for(int k = 0; k < 154; k++)
			a.add_point(c[d[j+k]]);
		a.compute(2);
	}
	a.display();

	std::random_shuffle(&d[0], &d[i]);
	for(int j = 0; j < i; j+=154) {
		for(int k = 0; k < 154; k++)
			a.rem_point(c[d[j+k]]);
		a.compute(2);
	}
	a.display();
	
	return 0; 
}
