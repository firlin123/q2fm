#include <iostream>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include "colors.h"
#include "lib/libQ2.h"
#include "lib/libMyBitmap24.h"
#include "lib/libQ2Console.h"

#ifdef WIN32
#define ROOT_PATH string("C:/")
#define FONT_PATH "font.bmp"
#include "winonly/dirent.h"
#else
#define FILE_MODE 666
#define ROOT_PATH string("/")
#define FONT_PATH "/mnt/usb/font.bmp"
#include <dirent.h>
#endif

#define MainMenu_Selected_BG 64,  64,  64
#define MainMenu_Selected_FG 96,  184, 110
#define MainMenu_Main_BG     51,  51,  51
#define MainMenu_Main_FG     255, 255, 255


#define SubMenu_Selected_BG  64,  64,  64
#define SubMenu_Selected_FG  96,  184, 110
#define SubMenu_Main_BG      76,  106, 80
#define SubMenu_Main_FG      255, 255, 255

#define X_SCALE 30.0
#define Y_SCALE 20.0

typedef enum {
	D_ENT_ERROR=0,
	D_ENT_DIR,
	D_ENT_FILE,
	D_ENT_OTHER,
}d_ent_type;


int rmdirSystem(std::string path) {
#ifdef WIN32
	std::string s = "rmdir /Q \"" + path + "\"";
	std::transform(s.begin()+11, s.end(), s.begin()+11, [](char c) { return (c == '/' ? '\\' : c); });
#else
	std::string s = "rm -rf \'" + path+"\'";
#endif
	system(s.c_str());
	DIR* dir = opendir(path.c_str());
	if (dir) {
		closedir(dir);
		return 1;
	}
	else return 0;
}

int move_file(std::string path_from, std::string path_to) {
#ifdef WIN32
	std::string s = "move \"" + path_from + "\" \"" + path_to + "\"";
	std::transform(s.begin() + 7, s.end(), s.begin() + 7, [](char c) { return (c == '/' ? '\\' : c); });
#else
	std::string s = "mv \'" + path_from + "\' \'" + path_to + "\'";
#endif
	system(s.c_str());
	FILE *file = NULL;
	if (file = fopen(path_from.c_str(), "r")) {
		fclose(file);
		return 1;
	}
	else return 0;
}

int move_dir(std::string path_from, std::string path_to)
{
	struct dirent *entry = NULL;
	DIR *dir = NULL;
	dir = opendir(path_from.c_str());
	int result = mkdir(path_to.c_str(),FILE_MODE);
	while (entry = readdir(dir)) {
		DIR *sub_dir = NULL;
		std::string abs_path_to;
		std::string abs_path_from;
		if (!((entry->d_name[0] == '.' && entry->d_name[1] == '\0') || (entry->d_name[0] == '.' && entry->d_name[1] == '.'))) {
			abs_path_from = path_from + "/" + entry->d_name;
			abs_path_to = path_to + "/" + entry->d_name;
			if (sub_dir = opendir(abs_path_from.c_str())) {
				closedir(sub_dir);
				result = (move_dir(abs_path_from, abs_path_to) == 0 ? result : 1);
			}
			else result = (move_file(abs_path_from, abs_path_to) == 0 ? result : 1);
		}
	}
	return ((rmdirSystem(path_from) == 0) ? result : 1);
}

int remove_file(std::string path) {
	FILE *file = NULL;
	if (file = fopen(path.c_str(), "r")) {
		fclose(file);
		return remove(path.c_str());
	}
	else return 1;
}

int remove_dir(std::string path)
{
	struct dirent *entry = NULL;
	DIR *dir = NULL;
	dir = opendir(path.c_str());
	int result = 0;
	while (entry = readdir(dir)) {
		DIR *sub_dir = NULL;
		std::string abs_path;
		if (!((entry->d_name[0] == '.' && entry->d_name[1] == '\0') || (entry->d_name[0] == '.' && entry->d_name[1] == '.'))) {
			abs_path = path + "/" + entry->d_name;
			if (sub_dir = opendir(abs_path.c_str())) {
				closedir(sub_dir);
				remove_dir(abs_path);
			}
			else result = ((remove_file(abs_path) == 0) ? result : 1);
		}
	}
	return ((rmdirSystem(path) == 0) ? result : 1);
}

int copy_file(std::string path_from, std::string path_to) {
	char buf[BUFSIZ];
	size_t size;
	FILE* source = fopen(path_from.c_str(), "rb");
	FILE* dest = fopen(path_to.c_str(), "wb");
	if (!source || !dest) return 1;
	while (size = fread(buf, 1, BUFSIZ, source)) {
		fwrite(buf, 1, size, dest);
	} 
	fclose(source); fclose(dest);
	return 0;
}

int copy_dir(std::string path_from, std::string path_to)
{
	struct dirent *entry = NULL;
	DIR *dir = NULL;
	dir = opendir(path_from.c_str());
	int result = mkdir(path_to.c_str(),FILE_MODE);
	while (entry = readdir(dir)) {
		DIR *sub_dir = NULL;
		std::string abs_path_to;
		std::string abs_path_from;
		if (!((entry->d_name[0] == '.' && entry->d_name[1] == '\0') || (entry->d_name[0] == '.' && entry->d_name[1] == '.'))) {
			abs_path_from = path_from + "/" + entry->d_name;
			abs_path_to = path_to + "/" + entry->d_name;
			if (sub_dir = opendir(abs_path_from.c_str())) {
				closedir(sub_dir);
				result = (copy_dir(abs_path_from,abs_path_to) == 0 ? result : 1);
			}
			else result = (copy_file(abs_path_from,abs_path_to) == 0 ? result : 1);
		}
	}
	return result;
}

using namespace Q2ops;
using namespace std;

struct my_dir_entry {
	string name;
	d_ent_type dir_entr_t;
	my_dir_entry& operator=(string& s){
		name = s; dir_entr_t = D_ENT_ERROR;
		return *this;
	}
};

d_ent_type checkPatch(string path) {
	struct stat s;
	if (stat(path.c_str(), &s) == 0)
		if (s.st_mode & S_IFDIR) return D_ENT_DIR;
		else if (s.st_mode & S_IFREG) return D_ENT_FILE;
		else return D_ENT_OTHER;
	else return D_ENT_ERROR;
}





struct menuState {
	size_t from_x;
	size_t from_y;
	size_t to_x;
	size_t to_y;
	size_t selected_element = 0;
	int selected_line = 0;
	int add_i = 0;
};

string makeStr(my_dir_entry mde, size_t size) {
	int len = ln_strlen(mde.name);
	string s = "                                                                ";
	if ((len + 1) > int(size)) {
		s = ln_substr(mde.name, 0, int(size) - (3 + (mde.dir_entr_t == D_ENT_DIR ? 1 : 0))) + "..." + (mde.dir_entr_t == D_ENT_DIR ? "/" : "");
	}
	else {
		s = mde.name + (mde.dir_entr_t == D_ENT_DIR ? "/" : "") + s.substr(0, int(size) - len - 1 - (mde.dir_entr_t == D_ENT_DIR ? 1 : 0));
	}
	return s;
}

int menuGetKey(Q2Console &q2c_normal, Q2Console &q2c_selected, vector<my_dir_entry> &l, menuState& mn, bool align_down = false) {
	if (mn.to_x > X_SCALE) mn.to_x = X_SCALE;
	if (mn.to_y > Y_SCALE) mn.to_y = Y_SCALE;
	if (mn.from_x >= mn.to_x - 4) mn.from_y = mn.to_x - 5;
	if (mn.from_y >= mn.to_y) mn.from_y = mn.to_y - 1;
	size_t fX = mn.from_x * q2c_normal.getWidth() / X_SCALE + 0.5,
		fY = mn.from_y * q2c_normal.getHeight() / Y_SCALE + 0.5,
		tX = mn.to_x   * q2c_normal.getWidth() / X_SCALE + 0.5,
		tY = mn.to_y   * q2c_normal.getHeight() / Y_SCALE + 0.5,
		n = l.end() - l.begin(),
		sizeX = tX - fX,
		sizeY = tY - fY;
	if (sizeY > l.end() - l.begin()) {
		if (align_down) fY += sizeY - (l.end() - l.begin());
		else {
			for (int i = fY+(l.end() - l.begin()); i < tY; i++)
				q2c_normal << setxy(fX, i) << makeStr({"  ",D_ENT_ERROR}, sizeX);
			tY -= sizeY - (l.end() - l.begin());
		}
		sizeY = tY - fY;
	}
	uint8_t key;
	bool print_full = true;

	while (1) {
		if (print_full) {
			for (int i = 0; i < sizeY; i++) {
				(i == mn.selected_line ? q2c_selected : q2c_normal) << setxy(fX, fY + i) << makeStr(l[i + mn.add_i], sizeX);
			}
		}

		q2c_selected << vflip;
		q2c_selected >> key;

		if (key == KB_UP) {
			if (mn.selected_element > 0) {
				mn.selected_element--;
				mn.selected_line--;
				if (print_full = (mn.selected_line < 0)) {
					mn.selected_line++; mn.add_i--;
				}
				else {
					q2c_normal << setxy(fX, fY + mn.selected_line + 1) << makeStr(l[(mn.selected_line + 1) + mn.add_i], sizeX);
					q2c_selected << setxy(fX, fY + mn.selected_line) << makeStr(l[mn.selected_line + mn.add_i], sizeX);
				}
			}
		}
		else if (key == KB_DOWN) {
			if (mn.selected_element < (n - 1)) {
				mn.selected_element++;
				mn.selected_line++;
				if (print_full = (mn.selected_line >= sizeY)) {
					mn.selected_line--; mn.add_i++;
				}
				else {
					q2c_normal << setxy(fX, fY + mn.selected_line - 1) << makeStr(l[(mn.selected_line - 1) + mn.add_i], sizeX);
					q2c_selected << setxy(fX, fY + mn.selected_line) << makeStr(l[mn.selected_line + mn.add_i], sizeX);
				}
			}
		}
		else return key;
	}
}




int menuGetKey(Q2Console &q2c_normal, Q2Console &q2c_selected, vector<string> &l, menuState& mn, bool align_down=true) {
	vector<my_dir_entry> v(l.end() - l.begin());
	copy(l.begin(), l.end(), v.begin());
	return menuGetKey(q2c_normal, q2c_selected, v, mn, align_down);
}

menuState menuConstruct(size_t from_x, size_t from_y, size_t to_x, size_t to_y) {
	menuState mn;
	mn.from_x = from_x;
	mn.from_y = from_y;
	mn.to_x = to_x;
	mn.to_y = to_y;
	mn.add_i = 0;
	mn.selected_element = 0;
	mn.selected_line = 0;
	return mn;
}

#include <list>

string makePath(list<my_dir_entry> md) {
	string path = ROOT_PATH;
	while (!md.empty()) {
		path += md.front().name + ((md.front().dir_entr_t == D_ENT_DIR) ? "/" : "");
		md.pop_front();
	}
	return path;
}

int readDir(vector<my_dir_entry> &v, string path) {
	v.clear();
	DIR* Dir;
	struct dirent *DirEntry;
	my_dir_entry mde;
	Dir = opendir(path.c_str());
	if (Dir == NULL) {
		mde.dir_entr_t = D_ENT_DIR;
		mde.name = "..";
		v.push_back(mde);
		return -1;
	}
	while (DirEntry = readdir(Dir))
	{
		mde.dir_entr_t = checkPatch(path + "/" + DirEntry->d_name);
		mde.name = DirEntry->d_name;
		if (mde.name != "." || mde.name == "..") v.push_back(mde);
	}
	return 1;
}

int main() {
	cout << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";

	Q2 q2(320, 240);
	BMP24 bmp(FONT_PATH);
	Q2Console q2c(q2, bmp);
	Q2Console q2c_main_main = q2c;;     /*main menu foreground colored console*/
	Q2Console q2c_main_selected = q2c;  /*main menu selected colored console*/
	Q2Console q2c_sub_main = q2c;       /*sub menu foreground colored console*/
	Q2Console q2c_sub_selected = q2c;   /*sub menu selected colored console*/
	uint8_t key;
	menuState ms;
	menuState sub_ms;
	vector<string> submenu_dir = { "Open", "Copy", "Move", "Delete", };
	vector<string> submenu_file = { "Copy", "Move", "Delete",};
	vector<my_dir_entry> dir;
	vector<string> tmpv;
	list<my_dir_entry> path;

	list<my_dir_entry> copied_path;
	list<my_dir_entry> tmp_path;
	bool is_copied_dir = false;
	bool is_copied=false;
	bool is_moved=false;


	q2c_main_main << setfg(MainMenu_Main_FG) << setbg(MainMenu_Main_BG);
	q2c_main_selected << setfg(MainMenu_Selected_FG) << setbg(MainMenu_Selected_BG);
	q2c_sub_main << setfg(SubMenu_Main_FG) << setbg(SubMenu_Main_BG);
	q2c_sub_selected << setfg(SubMenu_Selected_FG) << setbg(SubMenu_Selected_BG);

	readDir(dir, makePath(path));

	bool inf;
	bool inf2;

	while (1) {
		readDir(dir, makePath(path));
		q2c << setxy(0, 0) << makeStr({ makePath(path),D_ENT_ERROR }, q2c.getWidth()) << vflip;
		ms = menuConstruct(1, 1, 30, 20);
		inf = true;
		while (inf) {
			inf = false;
			switch (menuGetKey(q2c_main_main, q2c_main_selected, dir, ms)) {
			case KB_OK:
				if (dir[ms.selected_element].dir_entr_t == D_ENT_DIR&&dir[ms.selected_element].name != "..") {
					path.push_back(dir[ms.selected_element]);
				}
				else if (dir[ms.selected_element].name == "..") {
					if(!path.empty()) path.pop_back();
				}else inf = true;
				break;
			case KB_BACK:
				if (!path.empty()) path.pop_back();
				else inf = true;
				break;
			case KB_POWER:
				/*todo poweroff menu*/
				break;
			case KB_MENU:
				inf2 = true;
				sub_ms = menuConstruct(10, 10, 30, 20);
				while (inf2) {
					inf2 = false;
					if (dir[ms.selected_element].dir_entr_t == D_ENT_DIR) {
						key=menuGetKey(q2c_sub_main, q2c_sub_selected, submenu_dir, sub_ms);
					}
					else if (dir[ms.selected_element].dir_entr_t == D_ENT_FILE) {
						key=menuGetKey(q2c_sub_main, q2c_sub_selected, submenu_file, sub_ms);
					}
					else inf2 = inf = true;
					if(!inf2) switch (key)
					{
					case KB_OK:
						if (dir[ms.selected_element].dir_entr_t == D_ENT_DIR) {
							if (submenu_dir[sub_ms.selected_element] == "Open") {
								if (dir[ms.selected_element].dir_entr_t == D_ENT_DIR&&dir[ms.selected_element].name != "..") {
									path.push_back(dir[ms.selected_element]);
								}
								else if (dir[ms.selected_element].name == "..") {
									if (!path.empty()) path.pop_back();
								}
								else inf = true;
							}
							else {
								if (submenu_dir[sub_ms.selected_element] == "Copy") {
									is_copied = is_copied_dir = true;
									is_moved = false;
									copied_path = path;
									copied_path.push_back(dir[ms.selected_element]);
									if ((*submenu_dir.rbegin()) != "Paste") submenu_dir.push_back("Paste");
									if ((*submenu_file.rbegin()) != "Paste") submenu_file.push_back("Paste");
								}
								else if (submenu_dir[sub_ms.selected_element] == "Move") {
									is_moved = is_copied = is_copied_dir = true;
									copied_path = path;
									copied_path.push_back(dir[ms.selected_element]);
									if ((*submenu_dir.rbegin()) != "Paste") submenu_dir.push_back("Paste");
									if ((*submenu_file.rbegin()) != "Paste") submenu_file.push_back("Paste");
								}
								else if (submenu_dir[sub_ms.selected_element] == "Delete") {
									tmp_path = path;
									tmp_path.push_back(dir[ms.selected_element]);
									if (is_copied && makePath(tmp_path) == makePath(copied_path)) {
										is_copied = is_copied_dir = is_moved = false;
										submenu_dir.pop_back(); submenu_file.pop_back();
									}
									remove_dir(makePath(tmp_path));
									readDir(dir, makePath(path));
									if (ms.selected_element > 0) ms.selected_element--;
									if (ms.selected_line > 0) ms.selected_line--;
									if (ms.add_i > 0) ms.add_i--;
								}
								else if (submenu_dir[sub_ms.selected_element] == "Paste") {
									tmp_path = path;
									tmp_path.push_back(*copied_path.rbegin());
									if (is_copied&&!is_moved) {
										if(is_copied_dir)
											copy_dir( makePath(copied_path), makePath(tmp_path));
										else 
											copy_file( makePath(copied_path), makePath(tmp_path));
									}
									if (is_moved) {
										if (is_copied_dir)
											move_dir(makePath(copied_path), makePath(tmp_path));
										else
											move_file(makePath(copied_path), makePath(tmp_path));
										is_copied = is_moved = is_copied_dir = false;
										submenu_dir.pop_back(); submenu_file.pop_back();
									}
									readDir(dir, makePath(path));
								}
								inf = true;
							}
						}
						else{
							if (submenu_file[sub_ms.selected_element] == "Copy") {
								is_copied = true;
								is_moved = is_copied_dir = false;
								copied_path = path;
								copied_path.push_back(dir[ms.selected_element]);
								if ((*submenu_dir.rbegin()) != "Paste") submenu_dir.push_back("Paste");
								if ((*submenu_file.rbegin()) != "Paste") submenu_file.push_back("Paste");
							}
							else if (submenu_file[sub_ms.selected_element] == "Move") {
								is_moved = is_copied = true;
								is_copied_dir = false;
								copied_path = path;
								copied_path.push_back(dir[ms.selected_element]);
								if ((*submenu_dir.rbegin()) != "Paste") submenu_dir.push_back("Paste");
								if ((*submenu_file.rbegin()) != "Paste") submenu_file.push_back("Paste");
							}
							else if (submenu_file[sub_ms.selected_element] == "Delete") {
								tmp_path = path;
								tmp_path.push_back(dir[ms.selected_element]);
								if (is_copied && makePath(tmp_path) == makePath(copied_path)) {
									is_copied = is_copied_dir = is_moved = false;
									submenu_dir.pop_back(); submenu_file.pop_back();
								}
								remove_file(makePath(tmp_path));
								readDir(dir, makePath(path));
								if (ms.selected_element > 0) ms.selected_element--;
								if (ms.selected_line > 0) ms.selected_line--;
								if (ms.add_i > 0) ms.add_i--;
							}
							else if (submenu_file[sub_ms.selected_element] == "Paste") {
								tmp_path = path;
								tmp_path.push_back(*copied_path.rbegin());
								if (is_copied && !is_moved) {
									if (is_copied_dir)
										copy_dir(makePath(copied_path), makePath(tmp_path));
									else
										copy_file(makePath(copied_path), makePath(tmp_path));
								}
								if (is_moved) {
									if (is_copied_dir)
										move_dir(makePath(copied_path), makePath(tmp_path));
									else
										move_file(makePath(copied_path), makePath(tmp_path));
									is_copied = is_moved = is_copied_dir = false;
									submenu_dir.pop_back(); submenu_file.pop_back();
								}
								readDir(dir, makePath(path));
							}
							inf = true;
						}
						break;
					case KB_MENU:
					case KB_BACK:
						inf = true;
						break;
					case KB_POWER:
						/*todo poweroff menu*/
						break;
					default:
						inf2 = true;
						break;
					}
				}
				break;
			default:
				inf = true;
				break;
			}
		}
	}
	return 0;
}
