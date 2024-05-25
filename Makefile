# gcc Shell_project.c job_control.c -o shell

all: clean compile

compile: Shell_project.c
# assignment requires the output file to be named a.out
	@echo "compiling all modules"
	gcc -o a.out Shell_project.c job_control.c

clean:
	@echo "removing output files"
	rm -f shell a.out

run:
	@echo "running the shell"
	./shell