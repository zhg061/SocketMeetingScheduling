#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <algorithm>
#include <cctype>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <unordered_map>
using namespace std;


int sendUsersInvolved(int sockfd, sockaddr_in serverMAddr, string usersEntered){ 
  ssize_t sentResult = send(sockfd, usersEntered.c_str(), 1024, 0);
  if (sentResult == -1) cout << "sentResult" << endl;
  else cout << "Client finished sending the usernames to Main Server." << endl;
  return 0;
}
void getInvalidUsers(int sockfd, sockaddr_in serverMAddr){
  char invalidUsers[1024];
  ssize_t recvInt = recv(sockfd, invalidUsers, sizeof(invalidUsers), 0);
  int port = ntohs(serverMAddr.sin_port);
  if (recvInt == -1) cout << "recvInt" << endl; 
  string s(invalidUsers);
  //cout << s << s.length() << endl;
  if (s.substr(0, 5) != "empty") cout << "Client received the reply from Main Server using TCP over port " << port << ": " << invalidUsers << " do not exist." << endl;
}
string getTimeUsers(int sockfd, sockaddr_in serverMAddr, string &users){
  char timeUsers[1024];
  ssize_t recvInt = recv(sockfd, timeUsers, 1024, 0);
  if (recvInt == -1) cout << "recvInt" << endl; 
  // find the position of the semicolon
  size_t pos = string(timeUsers).find(';');
  // get the substring before the semicolon
  users = string(timeUsers).substr(0, pos);
  size_t bracket = string(timeUsers).find_last_of(']');
  string times = string(timeUsers).substr(pos+1, bracket-pos);
  int port = ntohs(serverMAddr.sin_port);
  if (users != "empty") {
    if (times != "[]") cout << "Client received the reply from Main Server using TCP over port " << port << ": Time intervals " << times << " works for " << users << "." << endl;
    else cout << "Client received the reply from Main Server using TCP over port " << port << ": "<< users << " do not have intersections." << endl;
  }
  return times;
}
list<list<int>> convertStringToNestList(string s) {
  s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());
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
list<int> convertStringToList(string& s){
  s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());
  list<int> mylist;
  if (s.length() <= 1)
    return mylist;
  int i = 0;
  if (s[i] == '[') {
    i++;
    int j = i; // j must be a digit
    
    int cnt = 0;
    while (cnt < 2 && j < s.length()) {
      if (!isdigit(s[j])) return mylist;
      while (j < s.length() && isdigit(s[j])) {
        j++; // get the digit range
      }  
      int num = std::stoi(s.substr(i, j - i + 1));
      if (cnt == 1) {
        num = std::stoi(s.substr(i, j - i));
      }        
      mylist.push_back(num);
      j++; // move over the comma or the right bracket
      i = j;
      cnt++;
    }
  } else {
    i++;
  }
  return mylist;
}
bool isListContained(list<list<int>> lists, list<int> compare) {
  if (compare.size() != 2) return false;
  int start = compare.front();
  int end = compare.back();
  if (end <= start) return false;
  for (auto it = lists.begin(); it != lists.end(); ++it) {
    if ((*it).front() <= start && (*it).back() >= end) {
      return true;
    }
  }
  return false;
}
void checkTime(string times, string timesEntered){
  list<list<int>> lists = convertStringToNestList(times);
  list<int> list = convertStringToList(timesEntered);
}
void sendTime(int sockfd, sockaddr_in serverMAddr, string times, string users){
  list<list<int>> timesList = convertStringToNestList(times);
  string timeStr;
  bool invalidTime = 1;
  bool notFirst = 0;
  while (invalidTime == 1 && timesList.size() > 0) {
    if (notFirst == 1) cout << "Time interval "<<  timeStr <<" is not valid. Please enter again:"<< endl;
    else {
      cout << "Please enter the final meeting time to register an meeting:"<< endl;
      notFirst = 1;
    }
    getline(cin, timeStr);
    list<int> time = convertStringToList(timeStr);
    if (isListContained(timesList, time)) invalidTime = 0;
  } 
  if (timesList.size() == 0) timeStr="[]";
  ssize_t sentResult = send(sockfd, timeStr.c_str(), 1024, 0);
  if (sentResult == -1) cout << "sentResult" << endl;
  if (timesList.size() > 0) {
    cout << "Sent the request to register "<< timeStr <<"  as the meeting time for " << users << "." << endl;
    cout << "..." << endl;
  }
}
void recvNotify(int sockfd, sockaddr_in serverMAddr){
  char notify[1024];
  ssize_t recvInt = recv(sockfd, notify, sizeof(notify), 0);
  if (recvInt == -1) cout << "recvInt" << endl; 
  else cout << "Received the notification that registration has finished." << endl;
}
int main(){
  // create a socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  // specify the address and port to bind to
  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(0);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
  cout << "Client is up and running." << endl;
  
  sockaddr_in serverMAddr;
  serverMAddr.sin_family = AF_INET;
  serverMAddr.sin_port = htons(24542);
  serverMAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  socklen_t serverMAddrLen = sizeof(serverMAddr);
  //while (accaptInt == -1){
  if (connect(sockfd, (sockaddr*) &serverMAddr, serverMAddrLen) < 0) {
    perror("connection failed");
    exit(EXIT_FAILURE);
  }
  while (true){
    string usersEntered;
    cout << "Please enter the usernames to check schedule availability:"<< endl;
    getline(cin, usersEntered);
    // Set up the server address and port number
    
    sendUsersInvolved(sockfd, serverMAddr, usersEntered);
  
    // Receive invalid users
    getInvalidUsers(sockfd, serverMAddr);
    //while (true) { }
    string users;
    string times = getTimeUsers(sockfd, serverMAddr, users);
    sendTime(sockfd, serverMAddr, times, users);
    recvNotify(sockfd, serverMAddr);
    cout << "-----Start a new request-----" << endl;
  }
     
  close(sockfd);
  return 0;
}

