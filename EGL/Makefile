NAME=test4
CXXFLAGS=-Wall -g
INCLUDES=-I/opt/vc/include \
                 -I/opt/vc/include/interface/vcos/pthreads \
                 -I/opt/vc/include/interface/vmcs_host/linux \
                 
LDFLAGS= -L/opt/vc/lib -L/usr/lib -L/usr/lib/arm-linux-gnueabihf -lGLESv2 -lEGL -lm -lX11
SRCS=main5.cpp rpiGLES.c

all: clean $(NAME)

$(NAME):
	$(CXX) -o $@ $(SRCS)  $(INCLUDES) $(LDFLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(NAME)
