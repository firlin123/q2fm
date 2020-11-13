#ifndef LIBQ2CONSOLE_H
#define LIBQ2CONSLE_H
#include <string>
#include <sstream>  
#include <map>
#include <iomanip> 
#include "libQ2.h"
#include "libMyBitmap24.h"

namespace Q2ops
{
	typedef enum {
		tendl,
		vflip,
		vclear,
	} ops;
	typedef enum {
		p_setbg,
		p_setfg,
		p_setxy,
	}op_set_vars_params;
	typedef struct {
		op_set_vars_params param_type;
		int64_t var1;
	}op_set_1_vars;
	typedef struct {
		op_set_vars_params param_type;
		int64_t var1;
		int64_t var2;
	} op_set_2_vars;
	op_set_2_vars setxy(uint8_t x, uint8_t y);
	op_set_1_vars setbg(uint8_t r, uint8_t g, uint8_t b);
	op_set_1_vars setfg(uint8_t r, uint8_t g, uint8_t b);
}

typedef std::map<size_t, size_t> rel_map;

bool c16_is_valid_first(uint8_t c);
bool c16_is_valid_second(uint8_t c);
int ln_create_rel_map(std::string s, rel_map& rm);
std::string ln_substr(std::string s, size_t pos, size_t len);
size_t ln_strlen(std::string s);

typedef struct {
	size_t X;
	size_t Y;
}syncXY_t;

class Q2Console
{
	bool is_empty;
	Q2 q2;
	BMP24 bmp;
	RGB fg_color; /*foreground(text) color*/
	RGB bg_color; /*background color*/
	std::map<uint8_t, std::map<uint8_t, uint8_t**>> vfont; /*font array*/
	RGB * alpha_arr; /*array of alpha for current colors*/
	size_t font_width; /*in pixels*/
	size_t font_height; /*in pixels*/
	size_t screen_width; /*in pixels*/
	size_t screen_height; /*in pixels*/
	int prev_char; /*first 8 bit of a 16 bit characters*/
	syncXY_t * xy_p = nullptr; /*pointer to synced or not synced x and y*/
	std::ostringstream str_s; /*string stream for << operator*/
	RGB getColor(uint8_t i); /*get color from i(alpha), backround and foreground*/
	void calculateAlpha(); /*calculate alpha array*/
	void draw(uint8_t first, uint8_t second); /*draws character(s) vfont[first][second]*/
	uint8_t** font_read_character(size_t xpos, size_t ypos); /*reads character from bmp file*/
	void print(std::string s); /*print string of text*/;
public:
	Q2Console();
	Q2Console(const Q2Console& q2c); /*init copy*/
	Q2Console(Q2 q2_p, BMP24 bmp_p); /*init*/
	~Q2Console();
	void clear();
	void setColor(RGB text, RGB background); /*set text and background color*/
	void SetXY(size_t X, size_t Y); /*set text cursor*/
	size_t getWidth();
	size_t getHeight();
	size_t getX();
	size_t getY();
	syncXY_t * syncXY(syncXY_t * s = nullptr);
	void unsyncXY();
	uint8_t inputRead();
	Q2Console& operator<<(Q2ops::ops x); /*control through << operator*/
	Q2Console& operator<<(Q2ops::op_set_1_vars x); /*set 1 parameter for << operator*/
	Q2Console& operator<<(Q2ops::op_set_2_vars x); /*set 2 parameters for << operator*/
	template<typename T>
	Q2Console& operator<<(T x); /*ostream-like output*/
	template<typename T>
	Q2Console& operator>> (T &t); /*input*/
};

template<typename T>
inline Q2Console & Q2Console::operator<<(T x) {
	Q2Console &q(*this);
	if (!is_empty) {
		str_s << x;
		if (str_s.str() != "") {
			print(str_s.str());
			str_s.str("");
		}
	}
	return q;
}

template<typename T>
inline Q2Console & Q2Console::operator >> (T & t) {
	Q2Console &q(*this);
	q2 >> t;
	return q;
}

#endif