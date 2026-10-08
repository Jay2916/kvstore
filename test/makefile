test_codec:	test_codec.cpp ../src/codec.cpp ../include/codec.hpp ../src/helper.cpp
	g++ -g3 -std=c++20 ../src/codec.cpp ../src/helper.cpp test_codec.cpp ../src/ringbuffer.cpp -Wall -o test_codec
test_dispatch: test_dispatch.cpp ../src/RequestDispatcher.cpp 
	g++ -g3 -std=c++20 ../src/RequestDispatcher.cpp ../src/helper.cpp test_dispatch.cpp ../src/HashTableStorage.cpp ../src/hashtable.cpp ../src/ringbuffer.cpp -Wall -o test_dispatch