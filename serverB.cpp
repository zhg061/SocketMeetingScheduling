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
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <unordered_map>
using namespace std;
string server = "B";
int port = 22452;
string file="b.txt";
/**
 * Calculates the overlap between two users' time intervals.
 *
 * @param user1 the list of start and end times for user1
 * @param user2 the list of start and end times for user2
 * @return the list of start and end times both user1 and user2 are available at
 */
list<list<int>> overlapTwoUsers(list<list<int>> user1, list<list<int>> user2) {
  list<list<int>> timeOverlaps;
  // iterate through user1
  for (list<int> time1 : user1) {
    int front1 = time1.front();
    int back1 = time1.back();
    for (list<int> time2 : user2) {
      int front2 = time2.front();
      int back2 = time2.back();
      // there is an overlap
      if (front1 < back2 && back1 > front2) {
        list<int> timeOverlap;
        timeOverlap.push_back(max(front1, front2));
        timeOverlap.push_back(min(back1, back2));
        // add the new time interval to the list
        timeOverlaps.push_back(timeOverlap);
      }
    }
  }
  return timeOverlaps;
}
/**
 * Convert the nested list of integers to a string
 *
 * @param overlap the nested list of integers 
 * @return string version of overlap
 */
string nestedListToString(list<list<int>> overlap){
  stringstream ss;
  ss << "[";
  // outside bracket
  for (auto j = overlap.begin(); j != overlap.end(); ++j) {
    // inner bracket
    ss << "[";
    for (auto i = (*j).begin(); i != (*j).end(); ++i) {
      // add time
      ss << *i;
      // add comma if haven't reached to the end of list
      if (next(i) != (*j).end()) ss << ",";
    }
    ss << "]";
    if (next(j) != overlap.end()) ss << ",";
  }
  ss << "]";
  return ss.str();
}
/**
 * Get the overlap for time intervals for a group by users
 *
 * @param users the list of users received from the main server
 * @param myMap the hashmap that contains user name as the key and time intervals as the values
 * @param overlap the resulting time intervals that all users are available at
 * @return string version of the overlap list
 */
string checkOverlap(char* users, unordered_map<string, list<list<int>>> myMap, list<list<int>>& overlap){
  if (strlen(users)==0) return "empty";
  string str(users);
  if (str == "empty") return "empty";
  stringstream ss(users);
  string user;
  while (getline(ss, user, ',')) {
    // remove leading and trailing spaces
    user.erase(0, user.find_first_not_of(" "));
    user.erase(user.find_last_not_of(" ") + 1);
    // if this is the first comparison, just compare the two same lists
    if (overlap.size() == 0) overlap = overlapTwoUsers(myMap[user], myMap[user]);
    else overlap = overlapTwoUsers(overlap, myMap[user]);
  }
  // get the string version of the overlap
  string overlapString = nestedListToString(overlap);
  if (overlapString != "[]") cout << "Found the intersection result: " << overlapString << " for " << users << "." << endl;
  else cout << users  << " do not have intersections."<< endl;
  return overlapString;
}
/**
 * Send the time overlap to the main server.
 *
 * @param overlapString the string version of the list that will be sent to the main server
 * @param serverMAddr the address of the main server
 * @param the socket number for the current server
 */
void sendTimeOverlap(string overlapString, sockaddr_in serverMAddr, int sockfd){
  socklen_t serverMAddrLen = sizeof(serverMAddr);  
  if (overlapString.size() == 0) overlapString="empty";
  ssize_t sendResult = sendto(sockfd, overlapString.c_str(), 1024, 0, (sockaddr*)&serverMAddr, serverMAddrLen);
  if (sendResult == -1) cout << "recvResult" << endl;
  else {
    if (overlapString != "empty" && overlapString != "[]") {
      cout << "Server "<< server <<" finished sending the response to Main Server" << endl;
    }
  }
}
/**
 * Notify the main server that registration has finished.
 *
 * @param serverMAddr the address of the main server
 * @param the socket number for the current server
 */
void notifyServer(sockaddr_in serverMAddr, int sockfd, string selectedUsers){
  socklen_t serverMAddrLen = sizeof(serverMAddr);  
  string notify = "finished";
  ssize_t sendResult = sendto(sockfd, notify.c_str(), 1024, 0, (sockaddr*)&serverMAddr, serverMAddrLen);
  if (sendResult == -1) cout << "recvResult" << endl;
  else {
    string s(selectedUsers);
    if (s != "empty")  cout << "Notified Main Server that registration has finished." << endl;
  }
}
/**
 * Convert string to list of time intervals
 *
 * @param s the string of time intervals received from the main server
 * @return the nested list of integers
 */
list<list<int>> convertStringToNestedList(string s) {
  list<list<int>> result;
  int i = 0;
  while (i < s.length()) {
    if (s[i] == '[') {
      list<int> mylist;
      i++;
      int j = i; //j must be a digit
      int cnt = 0;
      while (cnt < 2){
        while (j < s.length() && isdigit(s[j])) {
          j++; //get the digit range
        }
        int num = std::stoi(s.substr(i, j-i+1));
        if (cnt == 1) num = std::stoi(s.substr(i, j-i));
        mylist.push_back(num);
        j++; // move over the comma or the right bracket
        i=j;
        cnt++;
      }
      result.push_back(mylist);
    } 
    else {
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
list<int> convertStringToList(string s){
  s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());
  list<int> mylist;
  if (s == "[]") return mylist;
  if (s.length() <= 1)
    return mylist;
  int i = 0;
  if (s[i] == '[') {
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
  } else {
    i++;
  }
  return mylist;
}
/**
 * Read in the file info, save the users, and time intervals into a string and a hashmap 
 *
 * @param s the string of users that are from the file
 * @param myMap the hashmap that contains users as keys and time intervals as values
 * @return an integer that indicate the file reading successful or not
 */
int readFile(string& serializedUsers, unordered_map<string, list<list<int>>>& myMap) {
  ifstream inputFile(file); 
  if (!inputFile.is_open()) { 
    cout << "Error opening file\n";
    return 1;
  }  
  string line;
  while (getline(inputFile, line)) { 
    // find the position of the semicolon
    size_t pos = line.find(';');
    // get the substring before the semicolon
    string user = line.substr(0, pos);
    // remove leading and trailing spaces
    user.erase(0, user.find_first_not_of(" "));
    user.erase(user.find_last_not_of(" ") + 1);
    
    // get the time intervals
    size_t bracket = line.find_last_of(']');
    string times = line.substr(pos+2, bracket-pos+1);

    // remove unwanted space
    times.erase(remove_if(times.begin(), times.end(), ::isspace), times.end());
    myMap[user] = convertStringToNestedList(times);    
    serializedUsers += user + ' ';
  }
  inputFile.close(); // close the file
  return 0;
}
/**
 * Convert the list of users from the file to the main server
 *
 * @param sockfd the socket number of the current server
 * @param serverMAddr the address of the main server
 * @param serializedUsers the string of users separated by comma
 */
void sendUsers(int sockfd, sockaddr_in& serverMAddr, string serializedUsers){
  serverMAddr.sin_family = AF_INET;
  serverMAddr.sin_port = htons(23542); //main server
  serverMAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  socklen_t serverMAddrLen = sizeof(serverMAddr);

  // send users to server M
  ssize_t sentResult = sendto(sockfd, serializedUsers.c_str(), 1024, 0, (sockaddr*)&serverMAddr, serverMAddrLen);
  if (sentResult == -1) cout << "sentResult" << endl;
  else cout << "Server "<< server <<" finished sending a list of usernames to Main Server." << endl;
}
/**
 * Receive the users from the main server
 *
 * @param sockfd the socket number of the current server
 * @param serverMAddr the address of the main server
 * @param selectedUsers the string of users separated by comma
 */
void receiveUsers(int sockfd, sockaddr_in serverMAddr, char *selectedUsers){
  socklen_t serverMAddrLen = sizeof(serverMAddr);
  // send users to server M
  ssize_t recvResult = recvfrom(sockfd, selectedUsers, 1024, 0, (sockaddr*)&serverMAddr, &serverMAddrLen);
  if (recvResult == -1) cout << "sentResult" << endl;
  string str(selectedUsers);
  if (str != "empty") cout << "Server "<< server <<" received the usernames from Main Server using UDP over port "<< port <<"." << endl;
}
/**
 * Receive the scheduled time from the main server
 *
 * @param sockfd the socket number of the current server
 * @param serverMAddr the address of the main server
 * @param selectedTime the string of time intervals from the main server
 */
void receiveTime(int sockfd, sockaddr_in serverMAddr, char *selectedTime){
  socklen_t serverMAddrLen = sizeof(serverMAddr);
  // receive time from server M
  ssize_t recvResult = recvfrom(sockfd, selectedTime, 1024, 0, (sockaddr*)&serverMAddr, &serverMAddrLen);
  if (recvResult == -1) cout << "sentResult" << endl;
}
/**
 * Remove the time interval from the list, and replace it with new time intervals
 *
 * @param times the list of time intervals for a user
 * @param time the time interval needed to be removed
 * @return the updated list of time intervals
 */
list<list<int>> replaceList(list<list<int>> times, list<int> time){
  int start = time.front();
  int end = time.back();
  for (auto it = times.begin(); it != times.end(); ++it) {
    // within the range for *it
    if ((*it).front() <= start && (*it).back() >= end) {
      // get the start and end time of (*it)
      int itS = (*it).front();
      int itE = (*it).back();
      // remove the current time interval
      it = times.erase(it);
      // replace it with new time interval/s
      if (itS < start && itE == end) {
        times.insert(it, {itS, start});
      }
      else if (itS == start && itE > end) {
        times.insert(it, {end, itE});
      }
      else if (itS < start && itE > end) {
        times.insert(it, {itS, start});
        times.insert(it, {end, itE});
      }
      // exit once found the range
      break;
    }
  }
  return times;
}
/**
 * Update the hashmap for the selected users
 *
 * @param myMap the hashmap that contains user name as the key and time intervals as the values
 * @param selectedTime the time interval needed to be removed
 * @param selectedUsers the users whose time intervals needed to be updated
 */
void UpdateMap(unordered_map<string, list<list<int>>>& myMap, char *selectedTime, char *selectedUsers){
  string str(selectedTime);
  // get the time interval selected by the client
  list<int> time = convertStringToList(str);
  if (str == "[]") return;
  string myString(selectedUsers);
  if (myString == "empty") return;
  stringstream ss(selectedUsers); 
  // each user from the selected users 
  string user;
  cout << "Register a meeting at "<< selectedTime <<" and update the availability for the following users:" << endl;
  while (getline(ss, user, ',')) {
    // remove leading and trailing spaces
    user.erase(0, user.find_first_not_of(" "));
    user.erase(user.find_last_not_of(" ") + 1);
    list<list<int>> times = myMap[user];
    myMap[user] = replaceList(times, time);
    cout << user << ": updated from "<< nestedListToString(times) << " to " << nestedListToString(myMap[user]) << endl;
  } 
  
}
int main(){
  // read in file
  string serializedUsers;
  unordered_map<string, list<list<int>>> myMap;
  readFile(serializedUsers, myMap);

  // create a socket
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd == -1) cout << "sockfd" << endl;
  
  // specify the address and port to bind to
  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  // bind the socket to the specified address and port
  int bindInt = bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)); // 0
  if (bindInt == -1) cout << "bindInt" << endl;

  // listen for incoming connections
  listen(sockfd, 5);
  cout << "Server "<< server <<" is up and running using UDP on port "<< port <<"." << endl;
  
  sockaddr_in serverMAddr;
  sendUsers(sockfd, serverMAddr, serializedUsers);
  while (true){
    // receive users from the main server
    char selectedUsers[1024];
    receiveUsers(sockfd, serverMAddr, selectedUsers);
    list<list<int>> overlap = {};

    // calculate the time overlaps of these users
    string overlapString = checkOverlap(selectedUsers, myMap, overlap);
  
    // send the time overlap to main server
    sendTimeOverlap(overlapString, serverMAddr, sockfd);

    // receive the selected time interval from the main server
    char selectedTime[1024];
    receiveTime(sockfd, serverMAddr, selectedTime);

    // update the hashmap by removing the timeinterval for the selected users
    UpdateMap(myMap, selectedTime, selectedUsers);

    // notify the server the update is complete
    notifyServer(serverMAddr, sockfd, selectedUsers);
  }
  close(sockfd);
  return 0;
}