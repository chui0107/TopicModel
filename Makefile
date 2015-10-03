# (C) Copyright 2004, David M. Blei (blei [at] cs [dot] cmu [dot] edu)

# This file is part of LDA-C.

# LDA-C is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your
# option) any later version.

# LDA-C is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA

.SUFFIXES: .cpp .h
CC= g++
CFLAGS= -std=c++11 -Wall -g
LDFLAGS= -lm

LOBJECTS= lda-data.o TopicModel.o lda-model.o utils.o cokus.o lda-alpha.o

LSOURCE= lda-data.cpp TopicModel.cpp lda-model.cpp utils.cpp cokus.cpp lda-alpha.cpp

lda:	$(LOBJECTS)
	$(CC) $(CFLAGS) $(LOBJECTS) -o lda $(LDFLAGS)

clean:
	-rm -f *.o
