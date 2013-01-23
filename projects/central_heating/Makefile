
TOOLCHAIN_PREFIX = ../../openwrt/staging_dir/toolchain-i386_gcc-4.5-linaro_uClibc-0.9.32/bin/i486-openwrt-linux


CPP = $(TOOLCHAIN_PREFIX)-g++


all : chmon


clean:
	rm -f chmon main.o message therm *~
	


chmon: main.cpp
	$(CPP) -Wall -static -o $@ $<
	ln -s chmon message
	ln -s chmon therm

