MAKEPARA=-fPIC -W -Werror -Wno-unused-parameter -Wall -O3 -pipe -D_REENTRANT -m64 -g
LIBRARY=-ldl -lrt -lnsl -lpthread
OBJECTLIST=GatewayAttempterSink.o Logger.o gatewaysvr.o
LIBARY_DEP=-L../mxBase -l_mxbase
INCLUDE_DEP=-I../mxBase/globaldef\
			-I../mxBase/cluster\
			-I../mxBase/log\
			-I../mxBase/buffer\
			-I../mxBase/mysqldb\
			-I../mxBase/redis-client\
			-I../mxBase/tcpnetkernelV1.0/tcpnetkernel
OBJECTNAME=gatewaysvr
all: libobj
libobj: $(OBJECTLIST)
	g++ $(MAKEPARA) -o $(OBJECTNAME)  $(OBJECTLIST) $(OTHEROBJECT) $(LIBARY_DEP) $(LIBRARY)
	
%.o:%.cpp
	g++ $(INCLUDE_DEP) -c $(MAKEPARA) -o $@ $<
	
clean:
	rm -rf *.o
	rm -rf $(OBJECTNAME)
