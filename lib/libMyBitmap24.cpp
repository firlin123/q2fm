#include "libMyBitmap24.h"



BMP24::BMP24() :width(0), height(0), aln(0) {
	is_empty = true;
	data = NULL;
}



BMP24::BMP24(const BMP24 & bmp) :width(bmp.width), height(bmp.height), aln(bmp.aln) {
	if (!(is_empty=bmp.is_empty)) {
		size_t size = height * (3 * width + aln);
		data = new uint8_t[size];
		memcpy(data, bmp.data, size);
	}
}



BMP24::BMP24(char * filename) {
	is_empty = false;
	FILE* file = fopen(filename, "rb");
	uint8_t info[54];
	fread(info, sizeof(uint8_t), 54, file); /* read the 54-byte header*/
	width = *(int*)&info[18]; /*extract width and height*/
	height = *(int*)&info[22];
	aln = width - size_t(width / 4) * 4; /*calculate after line nulls count*/
	size_t size = height * (3 * width + aln); /* allocate 3 bytes per pixel + aln at the end of each line */
	data = new uint8_t[size];
	fread(data, sizeof(uint8_t), size, file); /* read the rest of the data at once*/
	fclose(file);
}



BMP24::~BMP24() {
	if (!is_empty) {
		delete[] data;
	}
}



size_t BMP24::getWidth() {
	if (!is_empty) {
		return width;
	}
	else return size_t(0);
}



size_t BMP24::getHeight() {
	if (!is_empty) {
		return height;
	}
	else return size_t(0);
}



uint8_t BMP24::getR(size_t x, size_t y) {
	return getP(x, y, 2);
}



uint8_t BMP24::getG(size_t x, size_t y) {
	return getP(x, y, 1);
}



uint8_t BMP24::getB(size_t x, size_t y) {
	return getP(x, y, 0);
}



uint8_t BMP24::getP(size_t x, size_t y, size_t p) {
	if (!is_empty) {
		if (y >= height || x >= width) uint8_t(255);
		y = height - 1 - y;
		return data[3 * (y * width + x) + p + y * aln];
	}
	else return uint8_t(0);
}