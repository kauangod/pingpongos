CFLAGS = -Wall -lrt
dep = ppos-core-aux.c ppos-all.o queue.o disk-driver.o

.PHONY: all disco1 disco2

all:
	@echo "Passe 'disco1' ou 'disco2'"

disco1: pingpong-disco1.c
	@cp disk_original.dat disk.dat
	gcc $(CFLAGS) -o $@ $< $(dep)
	./$@
	rm -f $@

disco2: pingpong-disco2.c
	@cp disk_original.dat disk.dat
	gcc $(CFLAGS) -o $@ $< $(dep)
	./$@ > felipe_lucas.txt
	rm -f $@

clean:
	rm -f disco1 disco2
