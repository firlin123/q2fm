#ifndef LIBMYBITMAP24_H
#define LIBMYBITMAP24_H

#include <iostream>
#include <string.h>

class BMP24 {
protected:
	bool is_empty;
	size_t width;
	size_t height;
	size_t aln; /*after line nulls*/
	uint8_t * data;
	uint8_t getP(size_t x, size_t y, size_t p);
public:
	BMP24();
	BMP24(const BMP24& bmp);
	BMP24(char* filename);
	~BMP24();
	size_t getWidth();
	size_t getHeight();
	uint8_t getR(size_t x, size_t y);
	uint8_t getG(size_t x, size_t y);
	uint8_t getB(size_t x, size_t y);
};

#endif