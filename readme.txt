a. Your Full Name as given in the class list: 
  - Zhaoyi Guo

b. Your Student ID: 
  - 8043646542  

c. What you have done in the assignment, if you have completed the optional part (suffix). If it’s not mentioned, it will not be considered.
  - All phases plus extra credit

d. What your code files are and what each one of them does. (Please do not repeat the project description, just name your code files and briefly mention what they do).
  - serverM.cpp: connect to client through TCP and to backend servers through UDP to transfer users and time intervals
  - serverA.cpp: store users' information and connect to main server through UDP to transfer users and time intervals
  - serverB.cpp: same as serverA.cpp but store different set of users' information
  - client.cpp: schedule time with users by communicating with the main server

e. The format of all the messages exchanged, e.g., usernames are concatenated and delimited by a comma, etc.
  - usernames are concatenated and delimited by a comma
  - time intervals are converted to char* and are in the format of "[[a, b], [c, d]]"

g. Any idiosyncrasy of your project. It should say under what conditions the project fails, if any.
  - N/A

h. Reused Code: Did you use code from anywhere for your project? If not, say so. If so, say what functions and where they're from. (Also identify this with a comment in the source code). Reusing functions which are directly obtained from a source on the
internet without or with few modifications is considered plagiarism (Except code from the Beej’s Guide). Whenever you are referring to an online resource, make sure to only look at the source, understand it, close it and then write the code by yourself. The TAs will perform plagiarism checks on your code so make sure to follow this step rigorously for every piece of code which will be submitted.
	- Beej's guide
