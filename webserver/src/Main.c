#include <sys/types.h>
#include <winsock2.h>
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
int sizeHandler(int sz)
{
	if(BUFSIZE < sz)
	{
		return sz;
	}
	else
	{
		return BUFSIZE;
	}
}

int main(int argc, char *argv[]) 
{
	struct sockaddr_in server_addr;			//socket for server
	struct sockaddr_in client_addr; 		//socket for client

	socklen_t sin_len = sizeof(client_addr);

	int pid;
	int web_server;						//File descriptor for the webserver
	int fd_client;						//File descriptor for the client connections

	int portnumber;
	char buffer[BUFSIZE]; 				//Buffer to take requests and send responses

	long outstream;						//Reads file content to be sent to web page

	bool running = true;

	//If # of arguments isnt the expected # of argments
	if(argc != 2 && argc != 3)
	{
		printf("Usage: %s portnumber, or %s -p portnumber\n\n", argv[0], argv[0]);
		exit(1);
	}

	//If user enters 2 arguments, we expect either a portnumber, or the help guide (-h)
	if(argc == 2)
	{
		if(*argv[1] == *(char* )("-h"))
		{
			printf("Usage: %s portnumber, or %s -p portnumber\n\n", argv[0], argv[0]);
			exit(0);
		}
		if(atoi(argv[1]))
		{
			portnumber = atoi(argv[1]);
		}
		else
		{
			printf("Usage: %s portnumber, or %s -p portnumber\n\n", argv[0], argv[0]);
			exit(1);
		}
	}

	//If user enters 3 arguments, we expect -p followed by a port number
	if(argc == 3)
	{
		if(*argv[1] == *(char *)("-p"))
		{
			if(atoi(argv[2]))
			{
				portnumber = atoi(argv[2]);
			}
			else
			{
				printf("Usage: %s portnumber, or %s -p portnumber\n\n", argv[0], argv[0]);
				exit(1);
			}
		}
		else
		{
			printf("Usage: %s portnumber, or %s -p portnumber\n\n", argv[0], argv[0]);
			exit(1);
		}
	}


	if(portnumber < 0 || portnumber > 65535)
	{
		printf("Invalid port number, please use a port number in the range 1-65534\n");
		exit(1);
	}

	//Normal initialization for the webservers socket
	web_server = socket(AF_INET, SOCK_STREAM, 0); 

	if (web_server < 0) 
	{
		printf("Socket intialization error\n");
		exit(1);	
	}

	//Allows us to reuse ports without delay
	if(setsockopt(web_server, SOL_SOCKET, SO_REUSEADDR, (char*)&running, sizeof(true)) < 0)
	{
		printf("syscall error: setsockopt");
		exit(1);
	}

	//Address family
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(portnumber);
	
	if (bind(web_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
	{
		printf("syscall error: bind\n");
		close(web_server);
		exit(1);
	}

	if (listen(web_server, SOMAXCONN) < 0)
	{
		printf("syscall error: listen\n");
		close(web_server);
		exit(1);
	}	
	
	//Main loop for every connection
	while(running) 
	{
		//Accept incomming connections
		fd_client = accept(web_server, (struct sockaddr *) &client_addr, &sin_len);
		if (fd_client == 1)
		{
			printf("syscall error: listen\n");
			continue;
		}

		printf("Client Connected.\n\n");

		if((pid = fork()) < 0) 
		{
			printf("syscall error: fork\n");
		}
		else 
		{
			if(pid == 0) 
			{
				int file_fd;

				//Close server socket for the request instance
				close(web_server);

				//Reset/zero-fill the buffer
				memset(buffer, 0, BUFSIZE);

				//If we cant recieve the request correctly
				if((recv(fd_client, buffer, sizeHandler(strlen(buffer)), MSG_DONTWAIT)) < 0 && (read(fd_client,buffer,BUFSIZE) < 1))
				{
					memset(buffer, 0, BUFSIZE);

					printf("400 Bad Request\n");

					file_fd = open("../www/400.html", O_RDONLY);

					(void)snprintf(buffer, 69, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");
					(void)write(fd_client, buffer, strnlen(buffer, BUFSIZE));

					while((outstream = read(file_fd, buffer, BUFSIZE)) > 0)
					{
						(void)write(fd_client, buffer, outstream);
					}

					sleep(1);
					(void)close(fd_client);
					exit(1);
				}

				//Checks if request isn't GET or HEAD (If its a valid request send 501, if its invalid, send a 400)
				if(strncmp(buffer, "GET ", 4) && strncmp(buffer, "get ", 4) && strncmp(buffer, "HEAD ", 5) && strncmp(buffer, "head ", 5))
				{
					//Sends 501 if request isnt GET or HEAD but still a valid http request
					//Valid HTTP requests implemented here are: POST, PUT, DELETE, CONNECT, OPTIONS, TRACE och PATCH
					if(!strncmp(buffer, "POST ", 5) || !strncmp(buffer, "post ", 5) || !strncmp(buffer, "PUT ", 4) || !strncmp(buffer, "put ", 4) || !strncmp(buffer, "DELETE ", 7) || !strncmp(buffer, "delete  ", 7) || !strncmp(buffer, "CONNECT ", 8) || !strncmp(buffer, "connect ", 8) || !strncmp(buffer, "OPTIONS ", 8) || !strncmp(buffer, "options  ", 8) || !strncmp(buffer, "TRACE ", 6) || !strncmp(buffer, "trace ", 6) || !strncmp(buffer, "PATCH ", 6) || !strncmp(buffer, "patch  ", 6))
					{
						memset(buffer, 0, BUFSIZE);

						printf("501 Not implemented\n");

						file_fd = open("../www/501.html", O_RDONLY);

						(void)snprintf(buffer, 73, "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");
						(void)write(fd_client, buffer, strnlen(buffer, BUFSIZE));

						while((outstream = read(file_fd, buffer, BUFSIZE)) > 0)
						{
							(void)write(fd_client, buffer, outstream);
						}

						sleep(1);
						(void)close(fd_client);
						exit(1);
					}
					//If the request isnt a valid HTML request, mark it as a bad request
					else
					{
						memset(buffer, 0, BUFSIZE);

						printf("400 Bad request\n");

						file_fd = open("../www/400.html", O_RDONLY);

						(void)snprintf(buffer, 69, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");
						(void)write(fd_client, buffer, strnlen(buffer, BUFSIZE));

						while((outstream = read(file_fd, buffer, BUFSIZE)) > 0)
						{
							(void)write(fd_client, buffer, outstream);
						}

						sleep(1);
						(void)close(fd_client);
						exit(1);
					}
				}

				//Nullterminates the string after second space (only allowed space is after GET/HEAD request)
				if(!strncmp(buffer, "GET ", 4) || !strncmp(buffer, "get ", 4))
				{
					for(long i = 4; i < BUFSIZE; i++)
					{
						if(buffer[i] == ' ')
						{
							buffer[i] = '\0';
							break;
						}
					}
				}
				else
				{
					for(long i = 5; i < BUFSIZE; i++)
					{
						if(buffer[i] == ' ')
						{
							buffer[i] = '\0';
							break;
						}
					}
				}

				//Makes no requested page default to index page
				if(!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6) || !strncmp(&buffer[0], "HEAD /\0", 7) || !strncmp(&buffer[0], "head /\0", 7))
				{
					//If request is a GET request
					if(!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6))
					{
						memset(buffer, 0, BUFSIZE);

						printf("200 OK\n");

						file_fd = open("../www/index.html", O_RDONLY);

						(void)snprintf(buffer, 60, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");
						(void)write(fd_client, buffer, strnlen(buffer, BUFSIZE));

						while((outstream = read(file_fd, buffer, BUFSIZE)) > 0)
						{
							(void)write(fd_client, buffer, outstream);
						}

						sleep(1);
						(void)close(fd_client);
						exit(1);
					}
					//If request is a HEAD request
					else
					{
						memset(buffer, 0, BUFSIZE);

						printf("200 OK\n");

						(void)snprintf(buffer, 60, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");
						(void)write(fd_client, buffer, strnlen(buffer, BUFSIZE));

						sleep(1);
						(void)close(fd_client);
						exit(1);
					}
				}

				//Checks if request tries to access another directory (using ..) and returns a 403 if its the case
				if(!strncmp(buffer, "GET ", 4) || !strncmp(buffer, "get ", 4))
				{
					for(int i = 4; i < BUFSIZE; i++)
					{
						if(buffer[i] == '.' && buffer[i+1] == '.')
						{
							memset(buffer, 0, BUFSIZE);

							printf("403 Forbidden Request\n");

							file_fd = open("../www/403.html", O_RDONLY);

							(void)snprintf(buffer, 67, "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");
							(void)write(fd_client, buffer, strnlen(buffer, BUFSIZE));

							while((outstream = read(file_fd, buffer, BUFSIZE)) > 0)
							{
								(void)write(fd_client, buffer, outstream);
							}

							sleep(1);
							(void)close(fd_client);
							exit(1);
						}
					}
				}
				else
				{
					for(int i = 5; i < BUFSIZE; i++)
					{
						if(buffer[i] == '.' && buffer[i+1] == '.')
						{
							memset(buffer, 0, BUFSIZE);

							printf("403 Forbidden Request\n");

							(void)snprintf(buffer, 67, "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");
							(void)write(fd_client, buffer, strnlen(buffer, BUFSIZE));

							sleep(1);
							(void)close(fd_client);
							exit(1);
						}
					}
				}

				//webDir will search for the request in the assigned directory (in this case the www folder) (The length of this buffer only needs to be the length of the text)
				char* webDir = "../www/";

				/*
				reqHandler will cat together the directory folder with the requested resource to be able to pass it back accordingly
				We can assume to only have a GET request with a requested resource if we get to this point, since the previous code above
				is responsible for handleing any faulty/non-GET requests (or HEAD requests)
				I chose the length of PATH_MAX
				Also initialize to 0 so we have a nullterminator by default
				*/
				char reqHandler[PATH_MAX] = {0};

				/*
				This buffer will hold the filetype, since we mostly will work only with html, css and ico files
				We wont need to think about longer filetypes, meaning we can let this buffer have a size of 8
				*/
				char filetype[8] = {0};

				/*
				contentType is as the name implies the content type. 
				This string will be passed to the HTML header.
				We can assume this wont ever be longer than 73 characters as the longest possible 
				content type is "application/vnd.openxmlformats-officedocument.presentationml.presentation"
				However, for all intents and purpouses of this server we're sticking to smaller files meaning 
				the content type will likely be either text/html, text/css or image/ico
				so an appropriate size will be 16 since I'm trying to work in powers of 2, and 32 is excessive in this scenario
				*/
				char contentType[16] = {0};

				//Makes request go to desired directory (../www/), we can use strlen here as we already know webDir will be nullterminated by initialization
				strncat(reqHandler, webDir, strlen(webDir));

				//If the request fills the entire 4kB  buffer it can be assumed to not be a legitimate request, so we send a 400 error
				if((PATH_MAX - (strlen(reqHandler) -1)) < strlen(buffer))
				{
					if(!strncmp(buffer, "GET ", 4) || !strncmp(buffer, "get ", 4))
					{
						memset(buffer, 0, BUFSIZE);

						printf("400 Bad request\n");

						file_fd = open("../www/400.html", O_RDONLY);

						(void)snprintf(buffer, 69, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");
						(void)write(fd_client, buffer, BUFSIZE);

						while((outstream = read(file_fd, buffer, BUFSIZE)) > 0)
						{
							(void)write(fd_client, buffer, outstream);
						}

						sleep(1);
						(void)close(fd_client);
						exit(1);
					}
					else
					{
						memset(buffer, 0, BUFSIZE);

						printf("400 Bad request\n");

						(void)snprintf(buffer, 69, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");
						(void)write(fd_client, buffer, BUFSIZE);

						sleep(1);
						(void)close(fd_client);
						exit(1);
					}
				}
				//Otherwise we add the requested resource to the request handler
				else
				{
					//Extra safety measure, if strlen of the buffer is less than 4096 we can assume it's nullterminated
					if(strlen(buffer) < BUFSIZE)
					{
						//Print the requested accept to the webserver
						printf("Accepted request: %s\n\n", buffer);
					}

					if(!strncmp(buffer, "GET ", 4) || !strncmp(buffer, "get ", 4))
					{
						strncat(reqHandler, &buffer[5], strnlen(buffer, PATH_MAX));
					}
					else
					{
						strncat(reqHandler, &buffer[6], strnlen(buffer, PATH_MAX));
					}
				}

				//Sends a 404 if the file isnt found in the www directory
				if((file_fd = open(reqHandler, O_RDONLY)) < 0)
				{
					if(!strncmp(buffer, "GET ", 4) || !strncmp(buffer, "get ", 4))
					{
						memset(buffer, 0, BUFSIZE);

						printf("404 File not found\n");

						file_fd = open("../www/404.html", O_RDONLY);

						(void)snprintf(buffer, 67, "HTTP/1.1 404 Not Found\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");

						(void)write(fd_client, buffer, strnlen(buffer, BUFSIZE));

						while((outstream = read(file_fd, buffer, BUFSIZE)) > 0)
						{
							(void)write(fd_client, buffer, outstream);
						}

						sleep(1);
						(void)close(fd_client);
						exit(1);
					}
					else
					{
						memset(buffer, 0, BUFSIZE);

						printf("404 File not found\n");

						(void)snprintf(buffer, 67, "HTTP/1.1 404 Not Found\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");
						(void)write(fd_client, buffer, strnlen(buffer, BUFSIZE));

						sleep(1);
						(void)close(fd_client);
						exit(1);
					}
				}

				/*Start from content written after webDir. Since we're only working with one directory,
				we can assume request made to always have their filetype after their . (such as .html, .css etc...)
				(The reason we start at i = 7 is to bypass ../www/)
				*/
				for(int i = 7; i < strnlen(reqHandler, PATH_MAX); i++)
				{
					if(reqHandler[i] == '.')
					{
						//Move forwards one step to avoid the . in for example .html, .css etc..., the reason for size=8 is explained at the declaration of filetype
						strncat(filetype, &reqHandler[i + 1], 8);
					}
				}

				/*If the file isnt one of the expected filetypes, we cannot process the request, 
				so the server will respond with a 501 since handeling for those filetypes are not implemented*/
				if(strncmp(&filetype[0], "css", 3) && strncmp(&filetype[0], "html", 4) && strncmp(&filetype[0], "png", 3))
				{
					if(!strncmp(buffer, "GET ", 4) || !strncmp(buffer, "get ", 4))
					{
						memset(buffer, 0, BUFSIZE);

						printf("501 Not Implemented");

						file_fd = open("../www/501.html", O_RDONLY);

						(void)snprintf(buffer, 73, "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");
						(void)write(fd_client, buffer, strnlen(buffer, BUFSIZE));

						while((outstream = read(file_fd, buffer, BUFSIZE)) > 0)
						{
							(void)write(fd_client, buffer, outstream);
						}

						sleep(1);
						(void)close(fd_client);
						exit(1);
					}
					else
					{
						memset(buffer, 0, BUFSIZE);

						printf("501 Not Implemented");

						(void)snprintf(buffer, 73, "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n");

						(void)write(fd_client, buffer, strnlen(buffer, BUFSIZE));

						sleep(1);
						(void)close(fd_client);
						exit(1);
					}
				}

				if(!strncmp(&filetype[0], "css", 3))
				{
					strncat(contentType, "text/css", 9);
				}
				if(!strncmp(&filetype[0], "html", 4))
				{
					strncat(contentType, "text/html", 10);
				}

				//If requested file is favicon.png we use sendfile rather than the normal operation
				if(!strncmp(&filetype[0], "png", 3))
				{
					if(!strncmp(buffer, "GET ", 4) || !strncmp(buffer, "get ", 4))
					{
						memset(buffer, 0, BUFSIZE);

						printf("200 OK\n");

						//If the request is for the icon, we want to use sendfile() rather than write()
						strncat(contentType, "image/png", 10);

						//Get the length of the file with lseek
						long img_size = (long)lseek(file_fd, (off_t)0, SEEK_END);

						//lseek to the beginning of the file to prepare for sendfile
						(void)lseek(file_fd, (off_t)0, SEEK_SET);

						//Send header, Here we have to use sprintf, since we do not know what the image size or content type will be before sending
						(void)sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n", img_size, contentType);
						(void)write(fd_client, buffer, sizeof(buffer));

						sendfile(fd_client, file_fd, NULL, img_size);

						sleep(1);
						(void)close(fd_client);
						exit(1);
					}
					else
					{
						memset(buffer, 0, BUFSIZE);

						printf("200 OK\n");

						strncat(contentType, "image/png", 10);

						//Get the length of the file with lseek
						long img_size = (long)lseek(file_fd, (off_t)0, SEEK_END);

						//Send header, Here we have to use sprintf, since we do not know what the image size or content type will be before sending, but we know the header string wont exceed the buffersize (4kB)
						(void)sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n", img_size, contentType);

						sleep(1);
						(void)close(fd_client);
						exit(1);
					}
				}

				if(!strncmp(buffer, "GET ", 4) || !strncmp(buffer, "get ", 4))
				{
					memset(buffer, 0, BUFSIZE);

					printf("200 OK\n");

					//Just like in the image header, we do not know if the content type here will be css or html and for this reason we use sprintf instead of snprintf, and we still know the header string wont exceed the buffersize (4kB)
					(void)sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: %s; charset=UTF-8\r\n\r\n", contentType);
					(void)write(fd_client, buffer, strnlen(buffer, BUFSIZE));

					while ((outstream = read(file_fd, buffer, BUFSIZE)) > 0)
					{
						(void)write(fd_client, buffer, outstream);
					}

					sleep(1);
					(void)close(fd_client);
					exit(1);
				}
				else
				{
					memset(buffer, 0, BUFSIZE);

					printf("200 OK\n");

					(void)sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: %s; charset=UTF-8\r\n\r\n", contentType);

					(void)write(fd_client, buffer, strnlen(buffer, BUFSIZE));

					sleep(1);
					(void)close(fd_client);
					exit(1);
				}
			}
			else
			{
				wait(NULL);
				sleep(1);
				(void)close(fd_client);
			}
		}
	}
	wait(NULL);
	return 0;
}