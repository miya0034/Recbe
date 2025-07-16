/***************************************
*                                      *
* Siglent SDG6032X function generator  *
* Control software                     *
*                                      *
* 2023/09/10 Siyuan Sun                *
* use standard c++ compiler to compile *
*                                      *
***************************************/

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

char const* host_ip("192.168.10.2");
int const portNum = 5025;

int const SocketConnect();
void SocketQuery(int const socketOsc, std::string const& cmd);
void SendCommand(int const socketOsc, std::string const& cmd);
std::string TranslateCommand(int const ch, std::string const& mode, std::string const& cmd);
std::string TranslateCommand(int const ch, std::string const& mode, std::string const& cmd, std::string const& parameter);
