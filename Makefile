all: serverM serverA serverB client

serverM: serverM.cpp
	g++ -g serverM.cpp -o serverM

serverA: serverA.cpp
	g++ -g serverA.cpp -o serverA

serverB: serverB.cpp
	g++ -g serverB.cpp -o serverB

client: client.cpp
	g++ -g client.cpp -o client

clean:
	rm -f serverM serverA serverB client
