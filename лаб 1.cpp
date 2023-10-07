#include "bmp.h"
#include <iostream>

int main() {
	BMP bmp("salim.bmp");
	BMP rotate = bmp;
	bmp.rotateImageClockwise(bmp);
	bmp.write("rotateImageClockwise.bmp");
	rotate.rotateImageCounterClockwise(rotate);
	rotate.write("rotateImageCounterClockwise.bmp");
	rotate.applyGaussianFilter(rotate);
	rotate.write("applyGaussianFilter.bmp");
}
