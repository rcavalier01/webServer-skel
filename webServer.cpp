// **************************************************************************************
// * webServer (webServer.cpp)
// * - Implements a very limited subset of HTTP/1.0, use -v to enable verbose debugging output.
// * - Port number 1701 is the default, if in use random number is selected.
// *
// * - GET requests are processed, all other metods result in 400.
// *     All header gracefully ignored
// *     Files will only be served from cwd and must have format file\d.html or image\d.jpg
// *
// * - Response to a valid get for a legal filename
// *     status line (i.e., response method)
// *     Cotent-Length:
// *     Content-Type:
// *     \r\n
// *     requested file.
// *
// * - Response to a GET that contains a filename that does not exist or is not allowed
// *     statu line w/code 404 (not found)
// *
// * - CSCI 471 - All other requests return 400
// * - CSCI 598 - HEAD and POST must also be processed.
// *
// * - Program is terminated with SIGINT (ctrl-C)
// **************************************************************************************
//rachel cavalier
//9/28/2025
#include "webServer.h"
#include <signal.h>

// **************************************************************************************
// * Signal Handler.
// * - Display the signal and exit (returning 0 to OS indicating normal shutdown)
// * - Optional for 471, required for 598
// **************************************************************************************
// void sig_handler(int signo) {}
void sig_handler(int signo) {
  DEBUG << "Caught signal #" << signo << ENDL;
  DEBUG << "CLosing file descriptors 3-31." << ENDL;
  closefrom(3);
  exit(1);
}

// **************************************************************************************
// * processRequest,
//   - Return HTTP code to be sent back
//   - Set filename if appropriate. Filename syntax is valided but existance is not verified.
// **************************************************************************************
int readHeader(int sockFd,std::string &filename, std::string &request, int &contentLength, std::string &bodyStart) {
  int code = 400;
  std::string container;
  char buffer[10];
  bool endHeader = false;
  while(!endHeader){
    ssize_t byteRead = read(sockFd, buffer, 10);
    container.append(buffer, byteRead);
    size_t endHeaderPos = container.find("\r\n\r\n");
    if(endHeaderPos != std::string::npos){
      endHeader = true;
      if(container.size() > endHeaderPos + 4){
        bodyStart = container.substr(endHeaderPos + 4);
      }
    }
  }
  
  //first line look for GET (and later post/head)
  size_t lineEnd = container.find("\r\n");
  std::string getLine = container.substr(0,lineEnd);
  DEBUG << "getline is " << getLine << ENDL;
  size_t endGet = getLine.find(' ');
  std::string get = getLine.substr(0, endGet);
  size_t endAddr = getLine.find(' ', endGet +1);
  std::string addr = getLine.substr(endGet+1, endAddr - endGet -1);
  //check if valid
  if(get == "GET" || get == "HEAD" || get == "POST"){
    request = get;
  }else{
    std::cout << "NOT a valid Request" << std::endl;
    return code;
  }
  //get content length so post body read doesnt hang terminal
  if(request == "POST"){
    size_t contentStart = container.find("Content-Length:");
    if(contentStart != std::string::npos){
      size_t contentEnd = container.find("\r\n", contentStart);
      std::string lenSeg = container.substr(contentStart + 15, contentEnd - contentStart- 15);
      contentLength = std::stoi(lenSeg);
    }
  }
  filename = "data/" + addr.substr(1);
  DEBUG << "filename is " << filename << ENDL;
  //check filename validity
  //regular expression
  std::regex filenameExpression("(image[0-4]\\.jpg|file[0-9]\\.html)");
  bool accepted = std::regex_search(filename, filenameExpression);
  if(accepted){
    code = 200;
    DEBUG << "Accepted" << ENDL;
  }else if(!accepted){
    code = 404;
    DEBUG << "NOT Accepted" << ENDL;
  }
  
  return code;
}


// **************************************************************************
// * Send one line (including the line terminator <LF><CR>)
// * - Assumes the terminator is not included, so it is appended.
// **************************************************************************
void sendLine(int socketFd, std::string &stringToSend) {
  size_t stringLength = stringToSend.length();
  char line[stringLength+2];
  stringToSend.copy(line, stringLength);
  //add cr lf
  line[stringLength+1] = '\n';
  line[stringLength] = '\r';
  write(socketFd, line, (stringLength + 2));
  return;
}

// **************************************************************************
// * Send the entire 404 response, header and body.
// **************************************************************************
void send404(int sockFd) {
  std::string string404 = "HTTP/1.0 404 Not Found";
  std::string content = "Content-type: text/html";
  std::string blankLine = "";
  std::string helpful = "The requested document does not exist on this server."; 
  sendLine(sockFd, string404);
  sendLine(sockFd, content);
  sendLine(sockFd, blankLine);
  sendLine(sockFd, helpful);
  sendLine(sockFd, blankLine);
  return;
}

// **************************************************************************
// * Send the entire 400 response, header and body.
// **************************************************************************
void send400(int sockFd) {
  std::string string400 = "HTTP/1.0 400 Bad Request";
  std::string blankLine = "";
  sendLine(sockFd, string400);
  sendLine(sockFd, blankLine);
  return;
}
// **************************************************************************
// * Send just the Header
// **************************************************************************
void sendHead(int sockFd, std::string filename){
  DEBUG << "in send header" << ENDL;
  std::string string200 = "HTTP/1.0 200 OK";
  std::string blankLine = "";
  std::string contentImage = "Content-Type: image/jpeg";
  std::string contentFile = "Content-Type: text/html";
  std::string contentLength = "Content-Length: ";
  struct stat filenameStat;
  if(stat(filename.c_str(), &filenameStat) == -1){
    //read permission lacking or DNE
    DEBUG << "Stat return -1" << ENDL;
    send404(sockFd);
    return;
  }
  int sizeFile = filenameStat.st_size;
  sendLine(sockFd, string200);
  if(filename.find("image") != std::string::npos){
    sendLine(sockFd, contentImage);
  }else if(filename.find("file") != std::string::npos){
    sendLine(sockFd, contentFile);
  }
  contentLength.append(std::to_string(sizeFile));
  sendLine(sockFd, contentLength);
  sendLine(sockFd, blankLine);
  return;
}
// **************************************************************************
// * Save the file through overwite or creation
// **************************************************************************
void saveFile(int sockFd, std::string filename, int contentLength, std::string bodyStart){
  //OTRUNC is overwrite and OCREAT is create if DNE
  DEBUG << "In saveFile" << ENDL;
  DEBUG << "contentLength is '" << contentLength << "'" << ENDL;
  struct stat filenameStat;
  std::string created = "HTTP/1.0 201 Created";
  std::string overwrite = "HTTP/1.0 200 OK";
  std::string blankLine = "";
  bool exists = false;
  char buffer[10];
  ssize_t bytesRead;
  int total = 0;
  bool endBody = false;
  //check if file exists already
  if(stat(filename.c_str(), &filenameStat) == 0){
    exists = true;
  }
  //open file and add body contents
  int post = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if(post == -1){
    DEBUG << "ERROR in open for POST" << ENDL; 
    send400(sockFd);
    return;
  }
  /*
  my read was eating the first 2 characters so save them then resume with writing body
  */
  if(!bodyStart.empty()){
    write(post, bodyStart.data(), bodyStart.size());
    total += bodyStart.size();
  }
  while(total != contentLength){
    ssize_t readSize = 10;
    //expected til end
    if(contentLength-total < 10){
      readSize = contentLength - total;
    }
    bytesRead = read(sockFd, buffer, readSize);
    if(bytesRead <= 0){
      //eof or problem
      break;
    }  
    write(post, buffer, bytesRead);
    total += bytesRead;
  }
  DEBUG << "About to send overwrite or create" << ENDL;
  if(exists){
    sendLine(sockFd, overwrite);
  }else if(!exists){
    sendLine(sockFd, created);
  }
  sendLine(sockFd, blankLine);
  close(post);
  return;
}
// **************************************************************************************
// * sendFile
// * -- Send a file back to the browser.
// **************************************************************************************

void sesendFile(int sockFd,std::string filename) {
  DEBUG << "in send file" << ENDL;
  std::string string200 = "HTTP/1.0 200 OK";
  std::string blankLine = "";
  std::string contentImage = "Content-Type: image/jpeg";
  std::string contentFile = "Content-Type: text/html";
  std::string contentLength = "Content-Length: ";
  struct stat filenameStat;

  if(stat(filename.c_str(), &filenameStat) == -1){
    //read permission lacking or DNE
    DEBUG << "Stat return -1" << ENDL;
    send404(sockFd);
    return;
  }
  int sizeFile = filenameStat.st_size;
  sendLine(sockFd, string200);
  if(filename.find("image") != std::string::npos){
    sendLine(sockFd, contentImage);
  }else if(filename.find("file") != std::string::npos){
    sendLine(sockFd, contentFile);
  }
  contentLength.append(std::to_string(sizeFile));
  sendLine(sockFd, contentLength);
  sendLine(sockFd, blankLine);
  //send file with these
  //open file
  int fileRead = open(filename.c_str(), O_RDONLY);
  if(fileRead == -1){
    DEBUG << "File open failure" << ENDL;
    send404(sockFd);
    return;
  }
  //allocate 10 bytes of mem with malloc() or new[]
  char *memBuffer = new char[10];
  size_t byteSent = 0;
  //while # of bytes sent != size of file
  while(byteSent != sizeFile){
    //clear out the memory with bzero or something
    bzero(memBuffer, 10);
    //read() up to 10 bytes from the file into your memory buffer
    ssize_t byteRead = read(fileRead, memBuffer, 10);
    if(byteRead <= 0){
      break;
    }
    //write() the number of bytes you read
    ssize_t currBytes = write(sockFd, memBuffer, byteRead);
    byteSent +=  currBytes;
  }

  close(fileRead);
  delete[] memBuffer;
  return;
}


// **************************************************************************************
// * processConnection
// * -- process one connection/request.
// **************************************************************************************
int processConnection(int sockFd) {
  std::string container;
  char buffer[10];
  bool closeFound = false;
  bool terminatorFound = false;
  std::string filename;
  std::string request;
  int contentLength = 0;
  std::string bodyStart;
  //Call readHeader()
  int headerReturn = readHeader(sockFd, filename, request, contentLength, bodyStart);
  if(headerReturn == 400){
  // If read header returned 400, send 400
   send400(sockFd);
   return 0;
  }else if(headerReturn == 404){
   // If read header returned 404, call send404
    send404(sockFd);
   return 0;
  }else if(headerReturn == 200 && request == "GET"){
    // 471: If read header returned 200, call sendFile
    // - If the header was valid and the method was GET, call sendFile()
    sesendFile(sockFd, filename);
  }else if(headerReturn == 200 && request == "HEAD"){
    // - If the header was valid and the method was HEAD, call a function to send back the header.
    sendHead(sockFd, filename);
  }else if(headerReturn == 200 && request == "POST"){
    // - If the header was valid and the method was POST, call a function to save the file to dis.
    saveFile(sockFd, filename, contentLength, bodyStart);
  }

  return 0;
}
    

int main (int argc, char *argv[]) {


  // ********************************************************************
  // * Process the command line arguments
  // ********************************************************************
  int opt = 0;
  while ((opt = getopt(argc,argv,"d:")) != -1) {
    
    switch (opt) {
    case 'd':
      LOG_LEVEL = std::stoi(optarg);
      break;
    case ':':
    case '?':
    default:
      std::cout << "useage: " << argv[0] << " -d LOG_LEVEL" << std::endl;
      exit(-1);
    }
  }


  // *******************************************************************
  // * Catch all possible signals
  // ********************************************************************
  DEBUG << "Setting up signal handlers" << ENDL;
  //signal(SIGINT, sig_handler);
  //for loop i 32
  for(int i = 0; i < 32; i++){
    signal(i, sig_handler);
  }
  //signal(i, sig_handler);
  
  // *******************************************************************
  // * Creating the inital socket using the socket() call.
  // ********************************************************************
  int listenFd;
  DEBUG << "Calling Socket() assigned file descriptor " << listenFd << ENDL;
  listenFd = socket(PF_INET, SOCK_STREAM, 0);
  if(listenFd == -1){
    DEBUG << "problem with listen()" << ENDL;
  }
  //check fir errir
  
  // ********************************************************************
  // * The bind() call takes a structure used to spefiy the details of the connection. 
  // *
  // * struct sockaddr_in servaddr;
  // *
  // On a cient it contains the address of the server to connect to. 
  // On the server it specifies which IP address and port to lisen for connections.
  // If you want to listen for connections on any IP address you use the
  // address INADDR_ANY
  // ********************************************************************

  struct sockaddr_in server_addr;
  server_addr.sin_addr.s_addr = INADDR_ANY; //or htonl
  server_addr.sin_family = PF_INET;
  
  
  // ********************************************************************
  // * Binding configures the socket with the parameters we have
  // * specified in the servaddr structure.  This step is implicit in
  // * the connect() call, but must be explicitly listed for servers.
  // *
  // * Don't forget to check to see if bind() fails because the port
  // * you picked is in use, and if the port is in use, pick a different one.
  // ********************************************************************
  uint16_t port;
  bool exitLoop;
  exitLoop = false;
  port = 1701;
  DEBUG << "Calling bind()" << ENDL;
  while(exitLoop == false){
    server_addr.sin_port = htons(port);  //hon()??
    int try_bind = bind(listenFd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(try_bind == -1){
      if(errno == EADDRINUSE){
        //IFF ADRESS IN USE
        DEBUG << "Port in Use, Trying Next" << ENDL;
        port = port+1;
        continue;
      }else{
        //if error exit and debug
        DEBUG << "Bind Failure" << ENDL;
        exit(1);
      }
    }else if(try_bind != -1){
      exitLoop = true;
    }
  }

  std::cout << "Using port: " << port << std::endl;


  // ********************************************************************
  // * Setting the socket to the listening state is the second step
  // * needed to being accepting connections.  This creates a que for
  // * connections and starts the kernel listening for connections.
  // ********************************************************************
  DEBUG << "Calling listen()" << ENDL;
  int listenTry = listen(listenFd, 10);
  //check failure
  if(listenTry == -1){
    DEBUG << "Listen Failure" << ENDL;
  }
  


  // ********************************************************************
  // * The accept call will sleep, waiting for a connection.  When 
  // * a connection request comes in the accept() call creates a NEW
  // * socket with a new fd that will be used for the communication.
  // ********************************************************************
  int quitProgram = 0;
  while (!quitProgram) {
    int connFd = 0;
    DEBUG << "Calling connFd = accept(fd,NULL,NULL)." << ENDL;
    connFd = accept(listenFd, NULL, NULL);
    if(connFd == -1){
      DEBUG << "Accept failure" << ENDL;
    }

    DEBUG << "We have recieved a connection on " << connFd << ". Calling processConnection(" << connFd << ")" << ENDL;
    quitProgram = processConnection(connFd);
    DEBUG << "processConnection returned " << quitProgram << " (should always be 0)" << ENDL;
    DEBUG << "Closing file descriptor " << connFd << ENDL;
    close(connFd);
  }
  

  ERROR << "Program fell through to the end of main. A listening socket may have closed unexpectadly." << ENDL;
  closefrom(3);

}
