#
# Ce Makefile contient les cibles suivantes :
#
# all   : compile le programme
# clean : supprime fichiers temporaires
CC = gcc

PROG = ordonnanceur
CHRONO = chrono

CFLAGS = -g -Wall -Wextra -Werror # obligatoires

.PHONY: all clean

all: $(PROG) $(CHRONO)

clean:
	rm -f $(PROG) $(CHRONO) *.o
	rm -f *.aux *.log *.out *.pdf
	rm -f moodle.tgz

