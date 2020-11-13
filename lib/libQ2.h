#ifndef LIBQ2_H
#define LIBQ2_H

#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <queue>   

#ifndef WIN32

#include <pthread.h> 
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <termios.h>

#else

#include "../winonly/Windows_emul.h"

#endif // WIN32

/* Input support */
#define LINUX_KDGKBMODE     0x4B44
#define LINUX_KDSKBMODE     0x4B45
/* Key values */
#define K_MEDIUMRAW         2

typedef struct {
	uint8_t R;
	uint8_t G;
	uint8_t B;
} RGB;

typedef enum {
	SHIFT_UP,
	SHIFT_DOWN,

} shiftDirection;

typedef enum {
	PWR_OFF = 0,
	PWR_ON,
	PWR_IDLE,
	PWR_MP3,
	PWR_MP3_DNSE,
	PWR_WMA,
	PWR_WMA_DNSE,
	PWR_OGG,
	PWR_OGG_DNSE,
	PWR_FM,
	PWR_FM_DNSE,
	PWR_RECORDING,
	PWR_AVI,
	PWR_AVI_DNSE,
	PWR_VMW,
	PWR_VMW_DNSE,
	PWR_MAX_CPU,
	PWR_MAX_PERF,
} pmProfile;

typedef enum {
	KB_MENU = 1,
	KB_BACK = 14,
	KB_OK = 28,
	KB_POWER = 87,
	KB_USER = 88,
	KB_UP = 103,
	KB_LEFT = 105,
	KB_RIGHT = 106,
	KB_DOWN = 108,
} inpKey;

class Q2 {
protected:
	bool is_copy;
	bool is_empty;
	clock_t inputHoldTimeout = clock_t(1.0 * double(CLOCKS_PER_SEC)); /*how long do you need to hold key until it starts repeating itself*/
	clock_t inputRepeatTimeout = clock_t(0.02 * double(CLOCKS_PER_SEC)); /*time between repeats*/
	size_t height = 320; /* 320 pixels*/
	size_t width = 240;  /* 240 pixels*/
	int fbfd = 0;  /*framebuffer file descriptor*/
	int ttyfd = 0; /*TTY file descriptor*/
	int ledfd = 0; /*LED file descriptor*/
	uint16_t *fb_main; /*wo framebufer*/
	uint16_t *fb_temp; /*rw framebuffer*/
	std::queue<uint8_t> * inputQueue_ptr;
	pthread_t inputQueueThread_id;
	bool * inputQueueThreadRunning_ptr;
	void inputQueueInit();
	static void* inputQueueInit_static(void* p);
public:
	Q2();
	Q2(const Q2 & q2);
	Q2(size_t h, size_t w);
	~Q2();
	void ledPower(bool state);
	void ledLevelSet(size_t level);
	void pwSetProfile(pmProfile selected);
	void videoBrightnessSet(int level);
	void videoClear();
	void videoFlip();
	void videoPixelSet(size_t x, size_t y, RGB pixel);
	void videoShiftFb(size_t lines, shiftDirection sd);
	uint8_t inputRead();
	size_t getHeight();
	size_t getWidth();
	template<typename T>
	Q2& operator >> (T &t);
};

RGB rgb_c(uint8_t r, uint8_t g, uint8_t b);

template<typename T>
inline Q2 & Q2::operator >> (T & t) {
	Q2 &q(*this);
	t = T(inputRead());
	return q;
}


#endif