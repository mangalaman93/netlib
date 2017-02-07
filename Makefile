# use -pg for profiling, use -g for debugging or replace
# it with `-ffast-math -O3` for enabling optimizations

IDIR = include
CC = g++
DEBUG = -ffast-math -O3
PROFILE =

SDIR = src
ODIR = bin
LDIR = lib
EDIR = examples
ED25519 = ed25519

CFLAGS = -I$(IDIR) -I$(LDIR) $(DEBUG) $(PROFILE) -std=gnu++11 -Wall
LDFLAGS = -L$(LDIR)
LIBS = -lboost_system -lboost_thread -lpthread -lrt -lcryptopp -l$(ED25519)

_DEPS = tcp_connection.h base_network.h cryptor.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = tcp_connection.o base_network.o cryptor.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_MAIN = cryptor_test client_receive client_send connections eserver lclient lserver tc_rec tc_send sync_connections
MAIN = $(patsubst %,$(ODIR)/%,$(_MAIN))

all: dir $(MAIN)

dir:
	mkdir -p $(ODIR)
ifeq "$(wildcard $(LDIR)/boost_1_55_0.tar.bz2 )" ""
	cd $(LDIR) && wget http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.bz2
	cd $(LDIR) && tar xvf boost_1_55_0.tar.bz2
	cd $(LDIR)/boost_1_55_0/ && ./bootstrap.sh --with-libraries=system --with-libraries=thread --prefix=../../
	cd $(LDIR)/boost_1_55_0/ && ./b2 install runtime-link=static link=static
	cd $(LDIR) && wget http://www.cryptopp.com/cryptopp562.zip
	cd $(LDIR) && unzip cryptopp562.zip -d cryptopp
	cd $(LDIR)/cryptopp && make static && mv *.a ../
	cp -r $(LDIR)/cryptopp $(IDIR)
	cd $(LDIR)/$(ED25519) && make && mv lib$(ED25519).a ../
	cp -r $(LDIR)/$(ED25519) $(IDIR)
endif

$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(ODIR)/%: $(EDIR)/%.cpp $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

distclean: clean
	rm -rf $(LDIR)/boost_1_55_0.tar.bz2
	rm -rf $(LDIR)/boost_1_55_0/
	rm -rf $(LDIR)/cryptopp562.zip
	rm -rf $(LDIR)/cryptopp
	rm -rf $(LDIR)/*.a
	rm -rf $(IDIR)/boost
	rm -rf $(IDIR)/cryptopp

clean:
	rm -rf $(ODIR) *~

rebuild: clean all

.PHONY: clean
