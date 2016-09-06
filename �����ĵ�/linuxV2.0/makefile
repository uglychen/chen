BUILD_DIR_ROOT = $(PWD)
BUILD_STRING = `date +%Y%m%d`"build"`cat $(BUILD_DIR_ROOT)/.build`""
NJOBS = `if [ "" == "$(NCPUS)" ]; then echo "";else echo "-j$(NCPUS)"; fi`

DEBUG_OPT = -D_MX_DEBUG_
#SUB_DIRS = mxBase gateway transsvr media_gateway mediasvr
SUB_DIRS = mxBase gateway trans/business trans/ChatBusinessV1.5.0 trans/RoomBusinessV1.5.0 trans/GroupBusinessV1.5.0 trans mediasvr

all:debug
debug:
	@for dir in $(SUB_DIRS);\
		do\
		MAKEPARA='$(DEBUG_OPT) -DBUILD_STRING' $(MAKE) $(NJOBS) -C $$dir || exit 1;\
		done

clean:
	@for dir in $(SUB_DIRS);\
		do\
		MAKEPARA='$(DEBUG_OPT) -DBUILD_STRING' $(MAKE) $(NJOBS) -C $$dir clean || exit 1;\
		done
	@rm -rf ./Publish/Include/*
	@rm -f ./Publish/lib/*


