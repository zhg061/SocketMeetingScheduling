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

/**
 * Send the users entered by the client to the main server
 *
 * @param sockfd the socket number for the client
 * @param serverMAddr the address for the main server
 * @param usersEntered the users entered by the client
 */
void sendUsersInvolved(int sockfd, sockaddr_in serverMAddr, string usersEntered){ 
  // convert all upper case to lower case
  transform(usersEntered.begin(), usersEntered.end(), usersEntered.begin(),
                 [](unsigned char letter){ return tolower(letter); });
  if (usersEntered == "") usersEntered = "noUserEntered";
  ssize_t sentResult = send(sockfd, usersEntered.c_str(), 1024, 0);
  if (sentResult == -1) cout << "sentResult" << endl;
  else cout << "Client finished sending the usernames to Main Server." << endl;
}

/**
 * Receive from the main server the invalid users
 *
 * @param sockfd the socket number for the client
 * @param serverMAddr the address for the main server
 */
void getInvalidUsers(int sockfd, sockaddr_in serverMAddr){
  char invalidUsers[1024];
  ssize_t recvInt = recv(sockfd, invalidUsers, sizeof(invalidUsers), 0);
  int port = ntohs(serverMAddr.sin_port);
  if (recvInt == -1) cout << "recvInt" << endl; 
  string s(invalidUsers);
  //cout << s << s.length() << endl;
  if (s.substr(0, 5) != "empty") {
    if (s  == "noUserEntered") cout << "Client received the reply from Main Server using TCP over port " << port << ": " << " do not exist." << endl;
    else cout << "Client received the reply from Main Server using TCP over port " << port << ": " << invalidUsers << " do not exist." << endl;
  }
}

/**
 * Receive from the main server the valid users and the their available time intervals
 *
 * @param sockfd the socket number for the client
 * @param serverMAddr the address for the main server
 * @param users the valid users
 * @return times the string of time intervals that are free to choose
 */
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
  //cout << times << endl;
  int port = ntohs(serverMAddr.sin_port);
  if (users != "empty") {
    cout << "Client received the reply from Main Server using TCP over port " << port << ": Time intervals " << times << " works for " << users << "." << endl;
  }
  return times;
}

/**
 * Convert string to list of time intervals
 *
 * @param s the string of time intervals received from the main server
 * @return the nested list of integers
 */
list<list<int>> convertStringToNestList(string s) {
  // remove spaces from the time intervals
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

/**
 * Convert string to time intervals
 *
 * @param s the string of time intervals received from the main server
 * @return the list of integers
 */
list<int> convertStringToList(string& s){
  // remove unwanted spaces
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

/**
 * Check if the time intervals are valid and are contained in the available list
 *
 * @param lists the list of time intervals are are free to choose
 * @param compare the time intervals entered by the client
 * @return bool that indicate whether the time intervals are valid and are contained in the list
 */
bool isListContained(list<list<int>> lists, list<int> compare) {
  // compare must only have a start time and a end time
  if (compare.size() != 2) return false;
  // extract the start and end time
  int start = compare.front();
  int end = compare.back();
  if (end <= start) return false;
  for (auto it = lists.begin(); it != lists.end(); ++it) {
    // found the valid time intervals
    if ((*it).front() <= start && (*it).back() >= end) {
      return true;
    }
  }
  return false;
}

/**
 * Prompt the client to enter time intervals, and if they are valid, send the time intervals to the main server
 *
 * @param sockfd the socket number for the client
 * @param serverMAddr the address for the main server
 * @param times the string of time intervals that are available
 * @param users the users that are valid
 */
void sendTime(int sockfd, sockaddr_in serverMAddr, string times, string users){
  // get the time intervals into nested list structure
  list<list<int>> timesList = convertStringToNestList(times);
  string timeStr;
  // invalidTime determines whether go in to the while loop again
  bool invalidTime = 1;
  // notfirst is used to distinguish if the while loop has been ran before
  bool notFirst = 0;
  // keep prompting the use to enter time intervals until the time interval is valid
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
  // incase there are no time available
  if (timesList.size() == 0) timeStr="[]";
  ssize_t sentResult = send(sockfd, timeStr.c_str(), 1024, 0);
  if (sentResult == -1) cout << "sentResult" << endl;
  if (timesList.size() > 0) {
    cout << "Sent the request to register "<< timeStr <<"  as the meeting time for " << users << "." << endl;
  }
}

/**
 * Receive the confirmation from the main server that the time intervals are booked for all users
 *
 * @param sockfd the socket number for the client
 * @param serverMAddr the address for the main server
 */
void recvNotify(int sockfd, sockaddr_in serverMAddr, string times){
  char notify[1024];
  ssize_t recvInt = recv(sockfd, notify, sizeof(notify), 0);
  //cout << times << endl;
  if (recvInt == -1) cout << "recvInt" << endl; 
  else {
    if (times != "")  cout << "Received the notification that registration has finished." << endl;
  }
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
  
  // Set up the server address and port number
  sockaddr_in serverMAddr;
  serverMAddr.sin_family = AF_INET;
  serverMAddr.sin_port = htons(24542);
  serverMAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  socklen_t serverMAddrLen = sizeof(serverMAddr);

  //connect to the main server
  if (connect(sockfd, (sockaddr*) &serverMAddr, serverMAddrLen) < 0) {
    perror("connection failed");
    exit(EXIT_FAILURE);
  }
  while (true){
    string usersEntered;
    cout << "Please enter the usernames to check schedule availability:"<< endl;
    getline(cin, usersEntered);
    
    // send the users' name to the main server 
    sendUsersInvolved(sockfd, serverMAddr, usersEntered);
  
    // Receive invalid users
    getInvalidUsers(sockfd, serverMAddr);

    // receive valid users and their time overlaps
    string users;
    string times = getTimeUsers(sockfd, serverMAddr, users);

    // select a time interval and send to the main server
    sendTime(sockfd, serverMAddr, times, users);

    // receive confirmation from the server
    recvNotify(sockfd, serverMAddr, times);
    cout << "-----Start a new request-----" << endl;
  }
     
  close(sockfd);
  return 0;
}

