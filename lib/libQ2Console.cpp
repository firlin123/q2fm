#include "libQ2Console.h"

Q2Console::Q2Console() {
	is_empty = true;
}

Q2Console::Q2Console(const Q2Console & q2c) :q2(q2c.q2), bmp(q2c.bmp), fg_color(q2c.fg_color), bg_color(q2c.bg_color), font_width(q2c.font_width), font_height(q2c.font_height), screen_width(q2c.screen_width), screen_height(q2c.screen_height), prev_char(q2c.prev_char){
	if (!(is_empty = q2c.is_empty)) {
		xy_p = new syncXY_t;
		xy_p->X = q2c.xy_p->X;
		xy_p->Y = q2c.xy_p->Y;

		alpha_arr = new RGB[256];
		for (int i = 0; i < 256; i++) {
			alpha_arr[i] = q2c.alpha_arr[i];
		}

		str_s << q2c.str_s.str(); /*copy contents*/
		str_s.copyfmt(q2c.str_s); /*copy flags and formating*/

		for (auto i = q2c.vfont.begin(); i != q2c.vfont.end(); i++)
		{
			std::map<uint8_t, uint8_t**> char_row;
			for (auto j = i->second.begin(); j != i->second.end(); j++) {
				auto character = new uint8_t*[q2c.font_height];
				for (size_t k = 0; k < q2c.font_height; k++) {
					character[k] = new uint8_t[q2c.font_width];
					memcpy(character[k], (*j).second[k], q2c.font_width * sizeof(uint8_t));
				}
				char_row[j->first] = character;
			}
			vfont[i->first] = char_row;
		}
	}
}

Q2Console::Q2Console(Q2 q2_p, BMP24 bmp_p) : q2(q2_p), bmp(bmp_p), prev_char(-1) {
	is_empty = false;
	str_s.str("");

	xy_p = new syncXY_t;
	xy_p->X = 0;
	xy_p->Y = 0;

	alpha_arr = new RGB[256];
	setColor(rgb_c(0, 0, 0), rgb_c(255, 255, 255));

	screen_height = q2.getHeight();
	screen_width = q2.getWidth();
	font_height = bmp.getHeight() / 22;
	font_width = bmp.getWidth() / 64;
	for (size_t i = 0; i < 22; i++) { /*read font bmp file*/
		std::map<uint8_t, uint8_t**> char_row;
		for (size_t j = 0; j < 64; j++) {
			char_row[((i == 0) ? 32 : 128) + j] = font_read_character(j, i);
		}
		if (i == 0) {
			i++;
			for (int j = 0; j < 32; j++) {
				char_row[96 + j] = font_read_character(j, i);
			}
			vfont[0] = char_row;
		}
		else {
			vfont[(195 - 2) + i] = char_row;
		}
	}
}

Q2Console::~Q2Console() {
	if (!is_empty) {
		delete[] alpha_arr; /*free alpha arr memory*/
		for (auto i = vfont.begin(); i != vfont.end(); i++) {/*free font memory*/
			for (auto j = i->second.begin(); j != i->second.end(); j++) {
				for (size_t k = 0; k < font_height; k++) {
					delete[] (*j).second[k];
				}
				delete[](*j).second;
			}
		}
	}
}

uint8_t** Q2Console::font_read_character(size_t xpos, size_t ypos) {
	uint8_t** character = new uint8_t*[font_height];
	for (size_t k = 0; k < font_height; k++) {
		character[k] = new uint8_t[font_width];
		for (size_t l = 0; l < font_width; l++) {
			character[k][l] = (bmp.getB(xpos * font_width + l, ypos * font_height + k));
		}
	}
	return character;
}

RGB Q2Console::getColor(uint8_t i) {
	RGB rgb; /*foreground on top of backround with alpha(i) opacity*/
	rgb.R = alpha_arr[i].R;
	rgb.G = alpha_arr[i].G;
	rgb.B = alpha_arr[i].B;
	return rgb;
}

void Q2Console::calculateAlpha() {
	for (size_t i = 0; i < 256; i++) {
		alpha_arr[i].R = ((bg_color.R * i) + (fg_color.R * (255 - i))) / 255;
		alpha_arr[i].G = ((bg_color.G * i) + (fg_color.G * (255 - i))) / 255;
		alpha_arr[i].B = ((bg_color.B * i) + (fg_color.B * (255 - i))) / 255;
	}
}

void Q2Console::draw(uint8_t first, uint8_t second) {
	bool flipped = first == 1; /*if flipped flag(for unknown characters)*/
	bool newline = first == 2; /*if newline flag*/
	if (flipped || newline) first = 0;
	if (vfont.count(first)) { /*check if exists in font*/
		if (vfont[first].count(second)) { /*check if exists in font*/
			if ((xy_p->X + 1)*font_width > screen_width || newline) { /*check if not enough space vertically or newline*/
				xy_p->X = 0;
				xy_p->Y++;
				if ((xy_p->Y + 1) *font_height > screen_height) { /*check if not enough space vertically*/
					xy_p->Y--; /*revert inrement*/
					q2.videoShiftFb(font_height, SHIFT_UP); /*shift screen up to make space for new line*/
				}
			}
			if (!newline) { /*if not new line draw character and increment x*/
				auto fnt_chr = vfont[first][second];
				for (size_t i = 0; i < font_width; i++) {
					for (size_t j = 0; j < font_height; j++) {
						q2.videoPixelSet(xy_p->X*font_width + i, xy_p->Y*font_height + (flipped ? font_height - j - 1 : j), getColor(fnt_chr[j][i]));
					}
				}
				xy_p->X++;
			}
		}
		else {
			if (first != 0) draw(1, uint8_t('?')); /*if first part of 16 bit character doesn't match with any second parts in font map this means this is just 2 unknown 8 bit characters*/
			if (second == '\n') draw(2, ' '); /*if newline draw() with newline flag*/
			else if (second == '\t') draw(0, ' '); /*if tab draw space*/
			else draw(1, uint8_t('?')); /*unknown character*/
		}
	}
	else draw(1, uint8_t('?')); /*unknown character*/
}

void Q2Console::print(std::string s) {
	rel_map rm;
	if (prev_char != -1) s = std::string("") + char(prev_char) + s;
	prev_char = ln_create_rel_map(s, rm);
	for (auto i : rm) {
		if (c16_is_valid_first(s[i.second])) {
			if (c16_is_valid_second(s[i.second + 1])) {
				draw(s[i.second], s[i.second + 1]);
			}
			else {
				draw(0, s[i.second]);
			}
		}
		else {
			draw(0, s[i.second]);
		}
	}
}

void Q2Console::clear() {
	if (!is_empty) {
		xy_p->X = 0; xy_p->Y = 0;
		q2.videoClear();
	}
}

void Q2Console::setColor(RGB text, RGB background) {
	if (!is_empty) {
		bg_color = background; fg_color = text;
		calculateAlpha();
	}
}

void Q2Console::SetXY(size_t X, size_t Y) {
	if (!is_empty) {
		if (((X + 1)*font_width) > screen_width) return;
		if (((Y + 1)*font_height) > screen_height) return;
		xy_p->X = X; xy_p->Y = Y;
	}
}

size_t Q2Console::getWidth() {
	if (!is_empty) {
		return (screen_width / font_width);
	}
	else return size_t(0);
}

size_t Q2Console::getHeight() {
	if (!is_empty) {
		return (screen_height / font_height);
	}
	else return size_t(0);
}

size_t Q2Console::getX() {
	return xy_p->X;
}

size_t Q2Console::getY(){
	return xy_p->Y;
}

syncXY_t * Q2Console::syncXY(syncXY_t * s) {
	if (s == nullptr) {
		return xy_p;
	}
	else{
		xy_p = s;
		return nullptr;
	}
}

void Q2Console::unsyncXY() {
	syncXY_t * tmp = new syncXY_t;
	tmp->X = xy_p->X;
	tmp->Y = xy_p->Y;
	xy_p = tmp;
}

uint8_t Q2Console::inputRead()
{
	uint8_t k;
	q2 >> k;
	return k;
}

Q2Console & Q2Console::operator<<(Q2ops::op_set_2_vars x) {
	Q2Console &q(*this);
	if (!is_empty) {
		switch (x.param_type) {
		case Q2ops::p_setxy:
			SetXY(x.var1, x.var2);
			break;
		}
	}
	return q;
}

Q2Console & Q2Console::operator<<(Q2ops::op_set_1_vars x) {
	Q2Console &q(*this);
	if (!is_empty) {
		char* c = (char*)&x.var1;
		switch (x.param_type) {
		case Q2ops::p_setbg:
			bg_color = rgb_c(c[0], c[1], c[2]);
			calculateAlpha();
			break;
		case Q2ops::p_setfg:
			fg_color = rgb_c(c[0], c[1], c[2]);
			calculateAlpha();
			break;
		}
	}
	return q;
}

Q2Console & Q2Console::operator<<(Q2ops::ops x) {
	Q2Console &q(*this);
	if (!is_empty) {
		switch (x)
		{
		case Q2ops::tendl:
			operator<<("\n");
			break;
		case Q2ops::vclear:
			clear();
			break;
		case Q2ops::vflip:
			q2.videoFlip();
			break;
		}
	}
	return q;
}

Q2ops::op_set_2_vars Q2ops::setxy(uint8_t x, uint8_t y) {
	op_set_2_vars res = { p_setxy, x , y };
	return res;
}

Q2ops::op_set_1_vars Q2ops::setbg(uint8_t r, uint8_t g, uint8_t b) {
	int64_t s;
	char* c = (char*)&s;
	c[0] = r; c[1] = g; c[2] = b;
	op_set_1_vars ret = { p_setbg, s };
	return ret;
}

Q2ops::op_set_1_vars Q2ops::setfg(uint8_t r, uint8_t g, uint8_t b) {
	int64_t s;
	char* c = (char*)&s;
	c[0] = r; c[1] = g; c[2] = b;
	op_set_1_vars ret = { p_setfg, s };
	return ret;
}

bool c16_is_valid_first(uint8_t c) {
	return (c >= 195 && c <= 214);
}

bool c16_is_valid_second(uint8_t c) {
	return (c >= 128 && c <= 191);
}

int ln_create_rel_map(std::string s, rel_map & rm) {
	int prev_char = -1;
	int n = s.length();
	for (int i = 0, j = 0; i < n; i++, j++) {
		if (c16_is_valid_first(s[i])) {
			if (i + 1 < n) {
				rm[j] = i;
				if (c16_is_valid_second(s[i + 1])) i++;
				else if (!c16_is_valid_first(s[i + 1])) {
					if (uint8_t(s[i + 1]) == uint8_t('\t')) {
						j++; rm[j] = i + 1;
						j++; rm[j] = i + 1;
					}
					j++; i++;
					rm[j] = i;
				}
			}
			else  prev_char = uint8_t(s[i]);
		}
		else {
			rm[j] = i;
			if (uint8_t(s[i]) == uint8_t('\t')) {
				j++; rm[j] = i;
				j++; rm[j] = i;
			}
		}
	}
	return prev_char;
}

std::string ln_substr(std::string s, size_t pos, size_t len) {
	rel_map rm;
	int k = ln_create_rel_map(s, rm);
	if (k != -1) rm[rm.rbegin()->first + 1] = s.length() - 1;
	k = 0;
	len--;
	if ((rm[pos + len] + 1) < s.length())
		if (c16_is_valid_first(s[rm[pos + len]]) && c16_is_valid_second(s[1 + rm[pos + len]])) k = 1;
	s = s.substr(rm[pos], 1 + ((rm[pos + len] - rm[pos]) + k));
	return s;
}

size_t ln_strlen(std::string s) {
	if (s == "") return 0;
	rel_map rm;
	ln_create_rel_map(s, rm);
	return rm.rbegin()->second;
}