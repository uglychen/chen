MAKEPARA=-fPIC -W -Wall -Werror -Wno-unused-parameter -O3 -pipe -D_REENTRANT -m64 -g
LIBRARY=-ldl -lrt -lnsl -lpthread
OBJECTLIST=MediaAttempterSink.o MediaUdpAttempterSink.o Logger.o TransAVDataEngine.o TransDataBuffer.o transsvr.o StreamInfo.o
LIBARY_DEP=-L../mxBase -l_mxbase
INCLUDE_DEP=-I../mxBase/globaldef\
			-I../mxBase/cluster\
			-I../mxBase/log\
			-I../mxBase/buffer\
			-I../mxBase/mysqldb\
			-I../mxBase/redis-client\
			-I../mxBase/tcpnetkernelV1.0/tcpnetkernel
OBJECTNAME=mediasvr
all: libobj
libobj: $(OBJECTLIST)
	g++ $(MAKEPARA) -o $(OBJECTNAME) $(OBJECTLIST) $(OTHEROBJECT) $(LIBARY_DEP) $(LIBRARY)
	
%.o:%.cpp
	g++ $(INCLUDE_DEP) -c $(MAKEPARA) -o $@ $<
	
clean:
	rm -rf *.o
	rm -rf $(OBJECTNAME)
