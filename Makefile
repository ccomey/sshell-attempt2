all: sshell

sshell: sshell.c
	gcc -Wall -Wextra -Werror -g -o sshell sshell.c

clean:
	rm -f sshell