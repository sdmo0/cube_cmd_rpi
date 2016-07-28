CODENAME=cube_cc

GPP_OPTS= -g -Os -Wall
GCC_OPTS= -g -Os -Wall
GCC_LINKOPTS= -Wl,--gc-sections -L. -lm

build:
	arm-linux-gnueabihf-g++-4.9 $(GPP_OPTS) -o$(CODENAME) $(CODENAME).cpp

clean:
	rm -f *.o *.hex *.eep libcore.a *~
