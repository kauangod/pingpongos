CFLAGS = -Wall
dep = ppos-core-aux.c ppos-all.o queue.o

.PHONY: all preempcao dispatcher prio scheduler

all:
	@echo "Passe 'preempcao', 'dispatcher', 'prio', 'scheduler'" 

preempcao: pingpong-preempcao.c 
	gcc $(CFLAGS) -o $@ $< $(dep)
	./$@
	rm -f $@

dispatcher: pingpong-dispatcher.c
	gcc $(CFLAGS) -o $@ $< $(dep)
	./$@
	rm -f $@

prio: pingpong-contab-prio.c
	gcc $(CFLAGS) -o $@ $< $(dep)
	./$@
	rm -f $@
	
scheduler: pingpong-scheduler.c
	gcc $(CFLAGS) -o $@ $< $(dep)
	./$@
	rm -f $@

clean:
	rm -f preempcao dispatcher prio scheduler
