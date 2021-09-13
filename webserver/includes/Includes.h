#ifndef INCLUDES_H
#define INCLUDES_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <netdb.h>
#include <fcntl.h>
#include <wait.h>
#include <string.h>
#include <unistd.h>

//Assign 4kB as the buffersize
#define BUFSIZE 4096


#endif