CXXFLAGS = -std=c++11
all: serverM serverA serverB client

serverM: serverM.cpp
	g++ $(CXXFLAGS) -g serverM.cpp -o serverM

serverA: serverA.cpp
	g++ $(CXXFLAGS) -g serverA.cpp -o serverA

serverB: serverB.cpp
	g++ $(CXXFLAGS) -g serverB.cpp -o serverB

client: client.cpp
	g++ $(CXXFLAGS) -g client.cpp -o client

clean:
	rm -f serverM serverA serverB client
