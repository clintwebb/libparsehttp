/*

	libparsehttp
	(c) Copyright Hyper-Active Systems, Australia

	Contact:
		Clinton Webb
		webb.clint@gmail.com

	This library is intended to be used to parse http headers and content so 
	that an application can become a webserver if it wants.  It does not 
	handle any socket or IO, it merely takes chunks 

	It is released under GPL v2 or later license.  Details are included in the LICENSE file.

*/

#ifndef __PARSEHTTP_H
#define __PARSEHTTP_H


#define LIBPARSEHTTP_VERSION 0x00000600
#define LIBPARSEHTTP_VERSION_TEXT "v0.06"



typedef struct {
	char *buffer;
	int length;
	int pos;
	int dataleft;
	
	enum {
		state_request,
		state_headers,
		state_data,
		state_done
	} state;
	
	void (*cb_method)(const char *method, void *arg);
	void (*cb_path)(const char *path, void *arg);
	void (*cb_params)(const char *params, void *arg);
	void (*cb_version)(const char *version, void *arg);
	void (*cb_header)(const char *key, const char *value, void *arg);
	void (*cb_formdata)(const char *key, const char *value, void *arg);
	void (*cb_cookie)(const char *key, const char *value, void *arg);
	void (*cb_host)(const char *host, int port, void *arg);
	void (*cb_datalength)(int length, void *arg);
	void (*cb_data)(const char *data, int length, int leftover, void *arg);
	void (*cb_complete)(void *arg);
	
	void *arg;
	
	short int internally_created;
} parsehttp_t;


// initialize the structure.  if 'parse' parameter is null, then it will 
// allocate memory and return it.  If 'parse' is not null, then it will 
// initialise it as if it contains garbage information.  In otherwords, do not 
// init an already initialised structure.   If you are supplying a parse 
// parameter, you should use parse_version to ensure that the versions match 
// or it is possible that memory bounds can be breached if the structure in 
// the library is different to the header you are compiling against.
parsehttp_t * parse_init(parsehttp_t *parse);

// free the resources used by the instance.  If the object was created 
// internally then it will also free the parse object itself.
void parse_free(parsehttp_t *parse);

// Return the version that this instance was compiled as.  A useful check to 
// ensure that the header library that was compiled into other projects is the 
// same as the one in the library.
unsigned int parse_version(void);


// Reset the parse object so it can be re-used by the same connection.  This is 
// for cases where HTTP v1.1 is being used and multiple requests are received 
// on the same socket connection.
void parse_reset(parsehttp_t *parse);


// supply data that needs to be processed.  Will return 0 if all the expected 
// data has been received.  If the user doesnt have the cb_complete callback 
// set, this can be used to indicate that everything has been received and 
// parsed.
int parse_process(parsehttp_t *parse, char *data, int length);

// set the user argument.
#define parse_setarg(p,a) {assert(p); assert((p)->arg==NULL); (p)->arg=(a);}

// set callbacks for the various parsing options.
#define parse_setcb_method(p,fn)     {assert(p); assert((p)->cb_method==NULL);     (p)->cb_method=(fn);}
#define parse_setcb_path(p,fn)       {assert(p); assert((p)->cb_path==NULL);       (p)->cb_path=(fn);}
#define parse_setcb_params(p,fn)     {assert(p); assert((p)->cb_params==NULL);     (p)->cb_params=(fn);}
#define parse_setcb_version(p,fn)    {assert(p); assert((p)->cb_version==NULL);    (p)->cb_version=(fn);}
#define parse_setcb_header(p,fn)     {assert(p); assert((p)->cb_header==NULL);     (p)->cb_header=(fn);}
#define parse_setcb_formdata(p,fn)   {assert(p); assert((p)->cb_formdata==NULL);   (p)->cb_formdata=(fn);}
#define parse_setcb_cookie(p,fn)     {assert(p); assert((p)->cb_cookie==NULL);     (p)->cb_cookie=(fn);}
#define parse_setcb_host(p,fn)       {assert(p); assert((p)->cb_host==NULL);       (p)->cb_host=(fn);}
#define parse_setcb_datalength(p,fn) {assert(p); assert((p)->cb_datalength==NULL); (p)->cb_datalength=(fn);}
#define parse_setcb_data(p,fn)       {assert(p); assert((p)->cb_data==NULL);       (p)->cb_data=(fn);}
#define parse_setcb_complete(p,fn)   {assert(p); assert((p)->cb_complete==NULL);   (p)->cb_complete=(fn);}


// given a string containing the parameters, this function will split it into key/value pairs and call the callback routine.
void parse_params(char *params, void (*handler)(const char *key, const char *value, void *arg), void *arg);

#endif
