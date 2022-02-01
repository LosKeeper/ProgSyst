#
# Ce Makefile contient les cibles suivantes :
#
# all   : compile le programme
# clean : supprime exe et fichiers temp

EXEC_FILE = monshell
OBJECTS = monshell.o chaine.o

CFLAGS = -c -g -Wall -Wextra -Werror # obligatoires

.PHONY: all clean

all: $(EXEC_FILE)

$(OBJECTS): %.o: %.c
	$(CC) $< $(CFLAGS)

$(EXEC_FILE): $(OBJECTS)
	$(CC) $^ -o $@

clean:
	rm -f $(EXEC_FILE) *.o
	rm -f *.aux *.log *.out *.pdf
	rm -f moodle.tgz

