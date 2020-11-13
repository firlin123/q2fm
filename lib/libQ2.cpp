#include "libQ2.h"
RGB rgb_c(uint8_t r, uint8_t g, uint8_t b) {
	RGB rgb = { r,g,b };
	return rgb;
}

uint16_t RGB888_to_RGB565(RGB rgb) {
	/*uint16_t rr = uint16_t((double(rgb.R)*31.0) / 255.0 + 0.5) << 11;
	uint16_t gg = uint16_t((double(rgb.G)*63.0) / 255.0 + 0.5) << 5;
	uint16_t bb = uint16_t((double(rgb.B)*31.0) / 255.0 + 0.5);
	*/
	uint16_t bb = (rgb.B >> 3) & 0x1f;
	uint16_t gg = ((rgb.G >> 2) & 0x3f) << 5;
	uint16_t rr = ((rgb.R >> 3) & 0x1f) << 11;
	return (uint16_t)(rr | gg | bb);
}

uint8_t Q2::inputRead() {
	if (!is_empty) {
		while (inputQueue_ptr->empty()); /*wait untill queue is not empty*/
		auto key = inputQueue_ptr->front(); /*pop and return key*/
		inputQueue_ptr->pop();
		return key;
	}
	else return uint8_t(0);
}

size_t Q2::getHeight() {
	if (!is_empty) {
		return height;
	}
	else return size_t(0);
}

size_t Q2::getWidth() {
	if (!is_empty) {
		return width;
	}
	else return size_t(0);
}

void Q2::inputQueueInit() {
	uint8_t keys;
	inputQueue_ptr = new std::queue<uint8_t>;
	while ((*inputQueueThreadRunning_ptr)) {
		if (read(ttyfd, &keys, 1)) {                                 /*if there's a key*/
			if (keys < 128) {                                          /*if key is pressed down*/
				inputQueue_ptr->push(keys);                                   /*push key in queue*/
				uint8_t tmpkey = keys;                                   /*save the key to tmp*/
				clock_t end_time = clock() + inputHoldTimeout;           /*set timeout to inputHoldTimeout*/
				bool fl = true;                                          /*key is held down true*/
				while ((*inputQueueThreadRunning_ptr) && fl) {                  /*while thread is running and key is held down*/
					if (read(ttyfd, &keys, 1)) {                           /*if there's a key*/
						if (keys == tmpkey + 128) fl = false;                /*if a key code tmp + pressed up flag then key is held down false*/
						else if (clock() >= end_time) {                      /*else if timeout*/
							end_time = clock() + inputRepeatTimeout;           /*set timeout to inputRepeatTimeout*/
							inputQueue_ptr->push(tmpkey);                           /*repeat key into queue*/
						}
					}
					else if (clock() >= end_time) {                        /*else if timeout*/
						end_time = clock() + inputRepeatTimeout;             /*set timeout to inputRepeatTimeout timeout*/
						inputQueue_ptr->push(tmpkey);                             /*repeat key into queue*/
					}
				}
			}
		}
	}
}

void * Q2::inputQueueInit_static(void * p) {
	static_cast<Q2*>(p)->inputQueueInit();
	return NULL;
}

Q2::Q2(size_t h, size_t w) :height(h), width(w) {
	is_empty = false;
	is_copy = false;
	/*Video Init*/
	fbfd = open("/dev/fb", O_RDWR);
	if (fbfd < 0) {
		std::cerr << "Error: cannot open /dev/fb"; exit(1);
	}
	fb_main = (uint16_t*)mmap(0, height * width * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if (!fb_main) {
		std::cerr << "Error: cannot mmap framebuffer"; exit(1);
	}
	fb_temp = (uint16_t*)malloc(height * width * 2 * sizeof(uint16_t));
	/*Input Init*/
	struct termios newTerm;
	int termMode = 0;
	ttyfd = open("/dev/tty0", O_RDONLY);
	if (ttyfd < 0) {
		std::cerr << "Error: cannot open /dev/tty0"; exit(1);
	}
	if (ioctl(ttyfd, LINUX_KDGKBMODE, &termMode)) {
		std::cerr << "Error: ioctl(LINUX_KDGKBMODE, &termMode) failed"; exit(1);
	}
	tcgetattr(ttyfd, &newTerm);
	newTerm.c_lflag &= ~0xB;
	newTerm.c_iflag &= ~0x15C0;
	newTerm.c_iflag &= ~0x22;
	newTerm.c_cc[VSTOP] = 0;
	newTerm.c_cc[VSUSP] = 0;
	newTerm.c_cc[VMIN] = 0;
	newTerm.c_cc[VTIME] = 0;
	tcsetattr(ttyfd, TCSAFLUSH, &newTerm);
	if (ioctl(ttyfd, LINUX_KDSKBMODE, K_MEDIUMRAW)) {
		std::cerr << "Error: ioctl(LINUX_KDGKBMODE, K_MEDIUMRAW) failed"; exit(1);
	}
	/*LED Init*/
	ledfd = open("/dev/led", O_RDWR);
	if (ledfd < 0) {
		std::cerr << "Error: cannot open /dev/led"; exit(1);
	}
	ledPower(true); /*led power on*/
	ledLevelSet(1); /*set led ??brightness?? to 1*/
					/*CPU Power Profile Set*/
	pwSetProfile(PWR_MAX_CPU);
	/*Video Brighteess Set to 10*/
	videoBrightnessSet(10);
	/*Clear and Sync*/
	videoClear();
	videoFlip();
	/*create input thread*/
	inputQueueThreadRunning_ptr = new bool;
	(*inputQueueThreadRunning_ptr) = true;
	pthread_create(&inputQueueThread_id, NULL, Q2::inputQueueInit_static, this);
}

Q2::Q2() {
	is_empty = true;
}

Q2::Q2(const Q2 & q2) :inputHoldTimeout(q2.inputHoldTimeout), inputRepeatTimeout(q2.inputRepeatTimeout), height(q2.height), width(q2.width), fbfd(q2.fbfd), ttyfd(q2.ttyfd), ledfd(q2.ledfd), fb_main(q2.fb_main), fb_temp(q2.fb_temp), inputQueue_ptr(q2.inputQueue_ptr), inputQueueThreadRunning_ptr(q2.inputQueueThreadRunning_ptr), inputQueueThread_id(q2.inputQueueThread_id) {
	is_copy = true;
	is_empty = false;
}

Q2::~Q2() {
	if (!(is_copy || is_empty)) {
		free(fb_temp);
		(*inputQueueThreadRunning_ptr) = false;
		pthread_join(inputQueueThread_id, NULL);
		inputQueue_ptr->empty();
	}
}

void Q2::videoClear() {
	if (!is_empty) {
		memset(fb_main, 0, height * width * sizeof(uint16_t)); /*clear framebuffer*/
		memset(fb_temp, 0, height * width * sizeof(uint16_t));
	}
}

void Q2::ledPower(bool state) {
	if (!is_empty) {
		size_t level = 0;
		ioctl(ledfd, state ? 0 : 1, level);
	}
}

void Q2::ledLevelSet(size_t level) {
	if (!is_empty) {
		ioctl(ledfd, 2, level);
	}
}

void Q2::pwSetProfile(pmProfile selected) {
	if (!is_empty) {
		int powermanfd;
		int profile = selected;
		if (selected > PWR_MAX_PERF) {
			std::cerr << "Warning: invalid power profile" << std::endl; return;
		}
		powermanfd = open("/dev/misc/pm", O_RDWR);
		if (powermanfd < 0) {
			std::cerr << "Warning: cannot open /dev/misc/pm" << std::endl; return;
		}
		if (ioctl(powermanfd, 0x40046300, profile))
			std::cerr << "Warning: ioctl(powermanfd, 0x40046300, profile) failed" << std::endl; return;
		close(powermanfd);
	}
}

void Q2::videoBrightnessSet(int level) {
	if (!is_empty) {
		char cmd[0x10] = { 0, };
		int lcdcr = open("/proc/lfbctrl", O_RDWR);
		if (lcdcr < 0) {
			std::cerr << "Warning: cannot open /proc/lfbctrl" << std::endl; return;
		}
		if (level < 0) level = 0;
		if (level > 10) level = 10;
		sprintf(cmd, "backlight %d", level);
		write(lcdcr, cmd, strlen(cmd));
		close(lcdcr);
	}
}

void Q2::videoFlip() {
	if (!is_empty) {
		const size_t screen_region[4] = { 0, 0, height, width };
		ioctl(fbfd, 0x4010C10A, screen_region);
	}
}

void Q2::videoPixelSet(size_t x, size_t y, RGB pixel) {
	if (!is_empty) {
		if (y > height) return;
		if (x > width) return;
		uint16_t p = RGB888_to_RGB565(pixel);
		fb_main[y * width + x] = p;
		fb_temp[y* width + x] = p;
	}
}

void Q2::videoShiftFb(size_t lines, shiftDirection sd) {
	if (!is_empty) {
		if (sd == SHIFT_UP) {
			memmove(fb_temp, fb_temp + (lines * width), width * (height - lines) * sizeof(uint16_t));
			memset(fb_temp + (width * (height - lines)), 0, width * (height - lines));
		}
		else {
			memmove(fb_temp + (lines * width), fb_temp, width * (height - lines) * sizeof(uint16_t));
			memset(fb_temp, 0, width * (height - lines));
		}
		memcpy(fb_main, fb_temp, width*height * sizeof(uint16_t));
	}
}