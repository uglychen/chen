#!/bin/sh
rm -f ./bin/*.so
rm -f ./bin/mediasvr
rm -f ./bin/log/*
rm -f ./bin1/*.so
rm -f ./bin1/mediasvr
rm -f ./bin1/log/*
rm -f ./bin2/*.so
rm -f ./bin2/mediasvr
rm -f ./bin2/log/*
rm -f ./bin3/*.so
rm -f ./bin3/mediasvr
rm -f ./bin3/log/*
rm -f ./bin4/*.so
rm -f ./bin4/mediasvr
rm -f ./bin4/log/*
rm -f ./bin5/*.so
rm -f ./bin5/mediasvr
rm -f ./bin5/log/*
rm -f ./bin6/*.so
rm -f ./bin6/mediasvr
rm -f ./bin6/log/*
rm -f ./bin7/*.so
rm -f ./bin7/mediasvr
rm -f ./bin7/log/*
rm -f ./bin8/*.so
rm -f ./bin8/mediasvr
rm -f ./bin8/log/*
cp ../source/lib/* ./lib/
cp ../source/mediasvr/mediasvr ./
cp ./lib/*.so ./bin/
cp ./mediasvr ./bin/mediasvr
cp ./mediasvr ./bin1/mediasvr
cp ./lib/*.so ./bin1/
cp ./mediasvr ./bin2/mediasvr
cp ./lib/*.so ./bin2/
cp ./mediasvr ./bin3/mediasvr
cp ./lib/*.so ./bin3/
cp ./mediasvr ./bin4/mediasvr
cp ./lib/*.so ./bin4/
cp ./mediasvr ./bin5/mediasvr
cp ./lib/*.so ./bin5/
cp ./mediasvr ./bin6/mediasvr
cp ./lib/*.so ./bin6/
cp ./mediasvr ./bin6/mediasvr
cp ./lib/*.so ./bin6/
cp ./mediasvr ./bin7/mediasvr
cp ./lib/*.so ./bin7/
cp ./mediasvr ./bin8/mediasvr
cp ./lib/*.so ./bin8/
rm -f ./bin/libMysqlDB.so
rm -f ./bin/libTcpSocketClient_c.so
rm -f ./bin1/libMysqlDB.so
rm -f ./bin1/libTcpSocketClient_c.so
rm -f ./bin2/libMysqlDB.so
rm -f ./bin2/libTcpSocketClient_c.so
rm -f ./bin3/libMysqlDB.so
rm -f ./bin3/libTcpSocketClient_c.so
rm -f ./bin4/libMysqlDB.so
rm -f ./bin4/libTcpSocketClient_c.so
rm -f ./bin5/libMysqlDB.so
rm -f ./bin5/libTcpSocketClient_c.so
rm -f ./bin6/libMysqlDB.so
rm -f ./bin6/libTcpSocketClient_c.so
rm -f ./bin7/libMysqlDB.so
rm -f ./bin7/libTcpSocketClient_c.so
rm -f ./bin8/libMysqlDB.so
rm -f ./bin8/libTcpSocketClient_c.so

