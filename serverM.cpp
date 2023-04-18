#include <arpa/inet.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <list>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
using namespace std;

list<int> sockets;
// close all connection when CTRL+C
void closeAllConnection(int s){
  for (int socket: sockets) {
    close(socket);
  }
  exit(0);
}
// get the overlap of times between two users
list<list<int>> overlapTwoUsers(list<list<int>> user1, list<list<int>> user2,
                                string toA, string toB) {
  if (toA.size() == 0)
    return user2;
  else if (toB.size() == 0)
    return user1;
  list<list<int>> timeOverlaps;
  for (list<int> time1 : user1) {
    int front1 = time1.front();
    int back1 = time1.back();
    for (list<int> time2 : user2) {
      int front2 = time2.front();
      int back2 = time2.back();
      if (front1 < back2 && back1 > front2) {
        list<int> timeOverlap;
        timeOverlap.push_back(max(front1, front2));
        timeOverlap.push_back(min(back1, back2));
        timeOverlaps.push_back(timeOverlap);
      }
    }
  }
  return timeOverlaps;
}
// convert time intervals to a string for messaging
string listToString(list<list<int>> overlap) {
  stringstream ss;
  ss << "[";
  for (auto j = overlap.begin(); j != overlap.end(); ++j) {
    ss << "[";
    for (auto i = (*j).begin(); i != (*j).end(); ++i) {
      ss << *i;
      if (next(i) != (*j).end())
        ss << ", ";
    }
    ss << "]";
    if (next(j) != overlap.end())
      ss << ", ";
  }
  ss << "]";
  return ss.str();
}
// receive the list of users from the backend server after botting up
void getUsersServer(string server, unordered_map<string, string> &userToServer,
                    int sockUDP, sockaddr_in &serverAddr) {
  // receive users from server A/B
  char users[1024];
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  int port;
  if (server == "A")
    port = 21542;
  else
    port = 22542;
  serverAddr.sin_port = htons(port);
  socklen_t serverAddrLen = sizeof(serverAddr);
  ssize_t recvResult = recvfrom(sockUDP, users, sizeof(users), 0,
                                (sockaddr *)&serverAddr, &serverAddrLen);
  if (recvResult == -1)
    cout << "recvResult" << endl;
  // save users into a hashmap
  stringstream ss(users);
  string user;
  while (getline(ss, user, ' ')) {
    userToServer[user] = server;
  }
  cout << "Main Server received the username list from server " << server
       << " using UDP over port " << port << "." << endl;
}
// convert string to list 
list<list<int>> convertStringToList(string s) {
  list<list<int>> result;
  if (s.length() <= 1)
    return result;
  size_t bracket = s.find_last_of(']');
  s = s.substr(1, bracket); // remove first [ and last ]
  int i = 0;
  while (i < s.length()) {
    if (s[i] == '[') {
      list<int> mylist;
      i++;
      int j = i; // j must be a digit
      int cnt = 0;
      while (cnt < 2) {
        while (j < s.length() && isdigit(s[j])) {
          j++; // get the digit range
        }
        int num = std::stoi(s.substr(i, j - i + 1));
        if (cnt == 1)
          num = std::stoi(s.substr(i, j - i));
        mylist.push_back(num);
        j++; // move over the comma or the right bracket
        i = j;
        cnt++;
      }
      result.push_back(mylist);
    } else {
      i++;
    }
  }
  return result;
}
// receive overlap from the users at server A
list<list<int>> receiveOverlapsA(string server, int sockUDP,
                                 sockaddr_in serverAddr) {
  char overlapStringA[1024];
  socklen_t serverAddrLen = sizeof(serverAddr);
  ssize_t recvResult = recvfrom(sockUDP, overlapStringA, sizeof(overlapStringA),
                                0, (sockaddr *)&serverAddr, &serverAddrLen);
  int port = ntohs(serverAddr.sin_port);
  if (recvResult == -1)
    cout << "recvResult" << endl;
  else {
    string s(overlapStringA);
    //cout << "overlapStringA" << overlapStringA << endl;
    if (s != "empty") {
       if (s != "[]") cout << "Main Server received from server " << server << " the intersection result using UDP over port " << port << ": " << overlapStringA << "." << endl;
       else cout << "Found no intersection from server " << server << " using UDP over port " << port << "." << endl;
     }
  }
  string str(overlapStringA);
  return convertStringToList(str);
}
list<list<int>> receiveOverlapsB(string server, int sockUDP,
                                 sockaddr_in serverAddr) {
  char overlapStringB[1024];
  socklen_t serverAddrLen = sizeof(serverAddr);
  ssize_t recvResult = recvfrom(sockUDP, overlapStringB, sizeof(overlapStringB),
                                0, (sockaddr *)&serverAddr, &serverAddrLen);
  int port = ntohs(serverAddr.sin_port);
  if (recvResult == -1)
    cout << "recvResult" << endl;
   else {     
     string s(overlapStringB); // convert char* to string
     if (s != "empty") {
       if (s != "[]") cout << "Main Server received from server " << server << " the intersection result using UDP over port " << port << ": " << overlapStringB << "." << endl;
       else cout << "Found no intersection from server " << server << " using UDP over port " << port << "." << endl;
     }
  }
  string str(overlapStringB);
  return convertStringToList(str);
}
void sendUsersServer(string server, int sockUDP,
                                sockaddr_in serverAddr, string users) {
  if (users.length() > 0)
    cout << "Found " << users << " located at Server " << server
         << ". Send to Server " << server << "." << endl;
  else users = "empty";
  socklen_t serverAddrLen = sizeof(serverAddr);
  ssize_t sendResult = sendto(sockUDP, users.c_str(), 1024, 0,
                              (sockaddr *)&serverAddr, serverAddrLen);
  if (sendResult == -1)
    cout << "recvResult" << endl;  
}

void getUsersClient(int sockTCP, sockaddr_in clientAddr, int sockClient, char *usersInvolved) { 
  ssize_t recvInt = recv(sockClient, usersInvolved, 1024, 0);
  //cout << "usersInvolved" << usersInvolved << endl;
  if (recvInt == -1)
    cout << "recvInt" << endl;
  int port = ntohs(clientAddr.sin_port);
  cout << "Main Server received the request from client using TCP over port "
       << port << "." << endl;
}
void recvSendTimes(int sockTCP, int socketUDP, sockaddr_in serverAAddr, sockaddr_in serverBAddr, sockaddr_in clientAddr, int sockClient, char *timeSelected) { 
  // receive from client
  ssize_t recvInt = recv(sockClient, timeSelected, 1024, 0);
  if (recvInt == -1)
    cout << "recvInt" << endl;
  // send to backend
  socklen_t serverAddrLen = sizeof(serverAAddr);
  ssize_t sendResult = sendto(socketUDP, timeSelected, 1024, 0, (sockaddr *)&serverAAddr, serverAddrLen);
  if (sendResult == -1)
    cout << "recvResult" << endl;
  serverAddrLen = sizeof(serverBAddr);
  sendResult = sendto(socketUDP, timeSelected, 1024, 0, (sockaddr *)&serverBAddr, serverAddrLen);
  if (sendResult == -1)
    cout << "recvResult" << endl;
}
void recvSendNotify(int sockTCP, int sockUDP, sockaddr_in serverAAddr, sockaddr_in serverBAddr, sockaddr_in clientAddr, int sockClient) { 
  // receive from servers
  char notifyA[1024];
  socklen_t serverAddrLen = sizeof(serverAAddr);
  ssize_t recvResult = recvfrom(sockUDP, notifyA, sizeof(notifyA), 0,
                                (sockaddr *)&serverAAddr, &serverAddrLen);
  if (recvResult == -1)
    cout << "recvResult" << endl;
  char notifyB[1024];
  serverAddrLen = sizeof(serverBAddr);
  recvResult = recvfrom(sockUDP, notifyB, sizeof(notifyB), 0,
                                (sockaddr *)&serverBAddr, &serverAddrLen);
  if (recvResult == -1)
    cout << "recvResult" << endl;
  // send to client
  socklen_t clientAddrLen = sizeof(clientAddr);
  // Send users invalid to the client
  ssize_t sentResult = send(sockClient, notifyA, 1024, 0);
  if (sentResult == -1)
    cout << "sentResult" << endl;
  
}
void examineUsers(char *usersInvolved,
                  unordered_map<string, string> userToServer, string &toA,
                  string &toB, string &toC) {
  stringstream ss(usersInvolved);
  string user;
  while (getline(ss, user, ' ')) {
    if (userToServer.find(user) != userToServer.end()) {
      if (userToServer[user] == "A") {
        if (toA.size() == 0)
          toA += user;
        else
          toA += ", " + user;
      } else {
        if (toB.size() == 0)
          toB += user;
        else
          toB += ", " + user;
      }

    } else { // do not exist
      if (toC.size() == 0)
        toC += user;
      else
        toC += ", " + user;
    }
  }
}
void sendInvalidClient(int sockTCP, sockaddr_in clientAddr, string toC,
                       int sockClient) {
  if (toC.size() > 0)
    cout << toC << " do not exist. Send a reply to the client." << endl;
  socklen_t clientAddrLen = sizeof(clientAddr);
  // Send users invalid to the server
  if (toC.size() == 0) toC = "empty";
  ssize_t sentResult = send(sockClient, toC.c_str(), 1024, 0);
  if (sentResult == -1)
    cout << "sentResult" << endl;
}
void sendTimeUsersClient(string toA, string toB, string overlapABStr,
                         int sockTCP, sockaddr_in clientAddr, int sockClient) {
  string result;
  if (toA.size() != 0 && toB.size() != 0)
    result = toA + ", " + toB + "; " + overlapABStr;
  else if (toA.size() != 0)
    result = toA + ";" + overlapABStr;
  else if (toB.size() != 0)
    result = toB + ";" + overlapABStr;
  else
    result = "empty";
  socklen_t clientAddrLen = sizeof(clientAddr);
  // Send users invalid to the server
  
  ssize_t sentResult = send(sockClient, result.c_str(), 1024, 0);
  if (sentResult == -1)
    cout << "sentResult" << endl;
  else
    if (result != "empty") cout << "Main Server sent the result to the client." << endl;
}
int main() {
  signal(SIGINT, closeAllConnection);
  // create a socket
  int sockUDP = socket(AF_INET, SOCK_DGRAM, 0);
  sockets.push_back(sockUDP);
  int sockTCP = socket(AF_INET, SOCK_STREAM, 0);
  sockets.push_back(sockTCP);
  if (sockUDP == -1) {
    cerr << "Error creating UDP socket: " << strerror(errno) << endl;
    return 1;
  }
  if (sockTCP == -1) {
    cerr << "Error creating TCP socket: " << strerror(errno) << endl;
    return 1;
  }

  // specify the address and port to bind to
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  // bind and listen
  addr.sin_port = htons(23542);
  if (bind(sockUDP, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    cerr << "Error binding sockUDP socket: " << strerror(errno) << endl;
    //return 1;
  }
  addr.sin_port = htons(24542);
  if (bind(sockTCP, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    cerr << "Error binding socket sockTCP: " << strerror(errno) << endl;
    //return 1;
  }

  if (listen(sockTCP, 5) == -1) {
    cerr << "Error listening on socket sockTCP: " << strerror(errno) << endl;
    //return 1;
  }
  cout << "Main Server is up and running." << endl;

  // user to server hashmap
  unordered_map<string, string> userToServer;
  sockaddr_in serverAAddr;
  sockaddr_in serverBAddr;
  getUsersServer("A", userToServer, sockUDP, serverAAddr);
  getUsersServer("B", userToServer, sockUDP, serverBAddr);

  // connect to the client
  sockaddr_in clientAddr; 
  int sockClient;
  clientAddr.sin_family = AF_INET;
  clientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  socklen_t clientAddrLen = sizeof(clientAddr);
  sockClient = accept(sockTCP, (struct sockaddr *)&clientAddr, &clientAddrLen);
  sockets.push_back(sockClient);
  if (sockClient == -1)
    cout << "accaptInt" << endl;
  
  while (true){
    // receive user involved front client       
    char usersInvolved[1024]; 
    getUsersClient(sockTCP, clientAddr, sockClient, usersInvolved);  
    string toA;
    string toB;
    string toC;
    // check the users involved: A, B, or invalid
    examineUsers(usersInvolved, userToServer, toA, toB, toC);
    // send invalid users back to client
    sendInvalidClient(sockTCP, clientAddr, toC, sockClient);
    // find which server are the users coming from and send the server the users
    sendUsersServer("A", sockUDP, serverAAddr, toA);
    sendUsersServer("B", sockUDP, serverBAddr, toB);
    list<list<int>> overlapA = receiveOverlapsA("A", sockUDP, serverAAddr);
    list<list<int>> overlapB = receiveOverlapsB("B", sockUDP, serverBAddr);
    string overlapABStr =
        listToString(overlapTwoUsers(overlapA, overlapB, toA, toB));
    if (overlapABStr != "[]" && overlapA.size() > 0 && overlapB.size() > 0) cout << "Found the intersection between the results from server A and B: "
         << overlapABStr << endl;
    sendTimeUsersClient(toA, toB, overlapABStr, sockTCP, clientAddr, sockClient);
    // get the time from client and pass it to the backend servers
    char timeSelected[1024];
    recvSendTimes(sockTCP, sockUDP, serverAAddr, serverBAddr, clientAddr, sockClient, timeSelected);
    recvSendNotify(sockTCP, sockUDP, serverAAddr, serverBAddr, clientAddr, sockClient);
  }
  close(sockClient);
  close(sockTCP);
  close(sockUDP);
  return 0;
}
