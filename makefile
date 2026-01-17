# Do not edit the contents of this file.
warnings_BAD: *.c
	gcc -Werror -Wall -g -std=gnu99 -o Assignment_1 *.c -lrt
	
clean: *.c
	rm Assignment_1
