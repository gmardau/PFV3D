#include <stdio.h>
#include <stdlib.h>
#include "pfv3d.h"

int main(int argc, char const *argv[])
{
	pfv3d a;
	FILE *f = fopen("Input/surf05.dat", "r");
	int i; fscanf(f, "%d", &i);
	double c[i][3];
	for(int j = 0; j < i; j++) {
		fscanf(f, "%lf %lf %lf", &c[j][0], &c[j][1], &c[j][2]);
	}
	for(int j = 0; j < i; j++){
		a.add_point(c[j]);
		a.compute(1);}
	// for(int j = 0; j < i; j++) {
	// 	a.add_point(c[rand()%i]);
	// 	a.add_point(c[rand()%i]);
	// 	a.add_point(c[rand()%i]);
	// 	a.add_point(c[rand()%i]);
	// 	a.compute(1);
	// 	// a.display();
	// }
	// for(int j = 0; j < i; j++) {
	// 	a.rem_point(c[rand()%i]);
	// 	a.compute(1);
	// 	// a.display();
	// }
	// a.add_point(1, 1, 3);
	// a.compute();
	// a.display();
	// a.add_point(2, 2, 2);
	// a.compute();
	// a.display();
	// a.add_point(3, 3, 1);
	// a.compute();
	// a.display();
	return 0; 
}
