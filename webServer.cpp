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
int readHeader(int sockFd,std::string &filename) {
  int code = 400;
  std::string container;
  char buffer[10];
  bool endHeader = false;
  while(!endHeader){
    ssize_t byteRead = read(sockFd, buffer, 10);
    container.append(buffer, byteRead);
    if(container.find("\r\n\r\n") != std::string::npos){
      endHeader = true;
    }
  }
  //first line look for GET
  size_t lineEnd = container.find("\r\n");
  //400?
  std::string getLine = container.substr(0,lineEnd);
  //if valid get get filename
  size_t endGet = getLine.find(' ');
  std::string get = getLine.substr(0, endGet);
  size_t endAddr = getLine.find(' ', endGet +1);
  std::string addr = getLine.substr(endGet+1, endAddr - endGet -1);
  if(get != "GET"){
    DEBUG << "NOT a valid GET" << ENDL;
    return code;
  }
  filename = addr.substr(1);
  //check filename validity
  //imageX.jpg or fileX.html both 10 char
  if(filename.length() != 10){
    DEBUG << "Invalid filename length" << ENDL;
    code = 404;
  }
  if(filename.substr(0,5) == "image"){
    if((filename[5] >= '0') && (filename[5] <= '9')){
      if(filename.substr(6) == ".jpg"){
        code = 200;
      }else{
        DEBUG << "Wrong image type" << ENDL;
        code = 404;
      }
    }else{
      DEBUG << "Incorrect digit extension" << ENDL;
      code = 404;
    }
  }else if(filename.substr(0,4) == "file"){
    if((filename[4] >= '0') && (filename[4] <= '9')){
      if(filename.substr(5) == ".html"){
        code = 200;
      }else{
        DEBUG << "Wrong file type" << ENDL;
        code = 404;
      }
    }else{
      DEBUG << "Incorrect digit extension" << ENDL;
      code = 404;
    }
  }else{
    //something else wrong
    code = 404;
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
  size_t end_position = sizeof(line) - 1;
  //add cr lf
  line[end_position] = '\n';
  line[end_position - 1] = '\r';
  write(socketFd, line, sizeof(line));
  return;
}

// **************************************************************************
// * Send the entire 404 response, header and body.
// **************************************************************************
void send404(int sockFd) {
  return;
}

// **************************************************************************
// * Send the entire 400 response, header and body.
// **************************************************************************
void send400(int sockFd) {
  return;
}


// **************************************************************************************
// * sendFile
// * -- Send a file back to the browser.
// **************************************************************************************
void sesendFile(int sockFd,std::string filename) {
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
  while(!closeFound){

    while(!terminatorFound){
        ssize_t readBytes = read(sockFd,buffer,10);
        container.append(buffer, readBytes);
        if(container.find("\r\n") != std::string::npos){
          terminatorFound = true;
        }
    }
    write(sockFd, container.data(), container.size());
    if(container.find("CLOSE") != std::string::npos){
      closeFound = true;
    }
  }
  // Call readHeader()
  //int headerReturn = readHeader(sockFd);
  //if(headerReturn == 400){
  //  send400();
  //  return 0;
  //}else if(headerReturn == 404){
  //  send404();
  //  return 0;
  //}else if(headerReturn == 200){
  //  send200();
  //}


  // If read header returned 400, send 400

  // If read header returned 404, call send404

  // 471: If read header returned 200, call sendFile
  
  // 598 students
  // - If the header was valid and the method was GET, call sendFile()
  // - If the header was valid and the method was HEAD, call a function to send back the header.
  // - If the header was valid and the method was POST, call a function to save the file to dis.

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
  signal(SIGINT, sig_handler);

  
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
  port = 1029;
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
  listen(listenFd, 10);
  //check failure


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
