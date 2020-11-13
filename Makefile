TOOL_PREFIX = ../toolchain/bin/arm-none-linux-gnueabi
CC = $(TOOL_PREFIX)-g++ 
STRIP = $(TOOL_PREFIX)-strip

OUTPUT = adv
SRC = main.cpp lib/libQ2.cpp lib/libQ2Console.cpp lib/libMyBitmap24.cpp
DEST_DIR = /media/firlin123/Q2

# -Wno-unused-variable -IlibQ2 -LlibQ2 -lQ2

all:
#	@rm -f $(OUTPUT)
#	cd libQ2 && $(MAKE) && cd ..
	@echo "[COMPILE]"
	@$(CC) -std=c++11 -pthread -Wall -static $(SRC) -o $(OUTPUT)
# 	@echo "[STRIP]"
# 	@$(STRIP) -s $(OUTPUT)
clean:
#	cd libQ2 && $(MAKE) clean && cd ..
	@echo "[CLEAN]"
	@rm -f $(OUTPUT)
copy:
	if [ -f "$(OUTPUT)" ]; then \
		if [ -d "$(DEST_DIR)" ]; then \
			cp $(OUTPUT) myscript.sh $(DEST_DIR)/ ; \
			rm -f  $(DEST_DIR)/adv_2 $(DEST_DIR)/term.log.txt $(DEST_DIR)/term.log_n.txt  ; \
			if [ -f "$(DEST_DIR)/$(OUTPUT)" ]; then \
				notify-send -i codelite "Copy" "Successful"; \
				umount /media/firlin123/689E-68BA ; \
			else \
				notify-send -i codelite "Error" "Copy failed"; \
			fi \
		else \
			notify-send -i codelite "Error" "Device is not connected or mounted" ; \
		fi \
	else \
		notify-send -i codelite "Error" "Compile failed" ; \
	fi