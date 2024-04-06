# gcc Shell_project.c job_control.c -o shell

all: clean compile

compile: Shell_project.c
	@echo "compiling all modules"
	gcc -o shell Shell_project.c job_control.c

clean:
	@echo "removing output files"
	rm -f shell

run:
	@echo "running the shell"
	./shell