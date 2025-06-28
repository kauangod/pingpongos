CFLAGS = -Wall -lrt
dep = ppos-core-aux.c ppos-all.o ppos-disk-manager.o queue.o disk-driver.o 

.PHONY: all disco1

all:
	@echo "Passe 'disco1'" 

disco1: pingpong-disco1.c
	@cp disk_original.dat disk.dat
	gcc $(CFLAGS) -o $@ $< $(dep)
	./$@
	rm -f $@

clean:
	rm -f disco1
