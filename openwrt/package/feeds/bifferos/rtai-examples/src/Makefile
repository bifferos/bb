

all: rtai-led-demo


rtai-led-demo: rtai-led-demo.c
	$(CC) $(LXRT_CFLAGS) -o $@ $< $(LXRT_LDFLAGS) -llxrt

clean:
	rm -f *.o *~ rtai-led-demo

