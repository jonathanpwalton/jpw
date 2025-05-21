jpw: src/main.cc Makefile
	g++ -std=c++20 -Wall -Wextra -pedantic -O2 -s -o $@ $< -lcurl -larchive -lssl -lcrypto