# Simple Webserver
This is a very simple webserver written in C.

Currently, it can only handle GET and HEAD requests, but will send a 501 response if 
the sent request is valid.

Other implemented status codes are 400, 403, 404 and 200.

To compile the program, just run the makefile in the webserver folder

Additionally, a great source of knowledge for creating this was nweb23, so Thank you Nigel Griffiths.
