/*

	libparsehttp
	(c) Copyright Hyper-Active Systems, Australia

	Contact:
		Clinton Webb
		webb.clint@gmail.com

*/


#include "parsehttp.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#if (LIBPARSEHTTP_VERSION != 0x00000500)
	#error "This version certified for v0.05 only"
#endif



//-----------------------------------------------------------------------------
// initialize the structure.  if 'parse' parameter is null, then it will 
// allocate memory and return it.  If 'parse' is not null, then it will 
// initialise it as if it contains garbage information.  In otherwords, do not 
// init an already initialised structure.   If you are supplying a parse 
// parameter, you should use parse_version to ensure that the versions match 
// or it is possible that memory bounds can be breached if the structure in 
// the library is different to the header you are compiling against.
parsehttp_t * parse_init(parsehttp_t *parse)
{
	parsehttp_t *p;
	
	if (parse == NULL) {
		p = (parsehttp_t*) malloc(sizeof(parsehttp_t));
		p->internally_created = 1;
	}
	else {
		p = parse;
		p->internally_created = 0;
	}
	
	p->buffer = NULL;
	p->length = 0;
	p->pos = 0;
	p->dataleft = 0;
	p->state = state_request;
	p->cb_method = NULL;
	p->cb_path = NULL;
	p->cb_params = NULL;
	p->cb_version = NULL;
	p->cb_header = NULL;
	p->cb_formdata = NULL;
	p->cb_cookie = NULL;
	p->cb_host = NULL;
	p->cb_datalength = NULL;
	p->cb_data = NULL;
	p->cb_complete = NULL;
	p->arg = NULL;
	
	return(p);
}


//-----------------------------------------------------------------------------
// Reset the parse object so it can be re-used by the same connection.  This is 
// for cases where HTTP v1.1 is being used and multiple requests are received 
// on the same socket connection.
void parse_reset(parsehttp_t *parse)
{
	if (parse->buffer) {
		free(parse->buffer);
		parse->buffer = NULL;
	}
	parse->length = 0;
	parse->pos = 0;
	parse->dataleft = 0;
	parse->state = state_request;	
}

//-----------------------------------------------------------------------------
// Free the resources allocated.  If the object was created by the library then 
// it will be freed here too.
void parse_free(parsehttp_t *parse)
{
	assert(parse);
	
	if (parse->buffer) {
		free(parse->buffer);
	}

	assert(parse->internally_created == 0 || parse->internally_created == 1);
	if (parse->internally_created > 0) {
		free(parse);
	}
}


//-----------------------------------------------------------------------------
// Return the version that this instance was compiled as.  A useful check to 
// ensure that the header library that was compiled into other projects is the 
// same as the one in the library.
unsigned int parse_version(void)
{
	return(LIBPARSEHTTP_VERSION);
}


//-----------------------------------------------------------------------------
// supply data that needs to be processed.  Will return 0 if all the expected 
// data has been received.  If the user doesnt have the cb_complete callback 
// set, this can be used to indicate that everything has been received and 
// parsed.
int parse_process(parsehttp_t *parse, char *data, int length)
{
	int expecting = 0;
	
	assert(parse);
	assert(data);
	assert(length > 0);

	assert(parse->state != state_done);

	
// 	printf("parse_process: start. length=%d\n", length);

	// we are processing data, and do not have a formdata callback, then we dont need to process it.
	
	if (parse->state == state_data && parse->cb_formdata == NULL) {
		
		printf("parse_process: data mode, no callback. length=%d, left\n", length, parse->dataleft);
		
		// we call the cb_data callback with the new data after we determine how much data we have.
		assert(length <= parse->dataleft);
		parse->dataleft -= length;
		assert(parse->dataleft >= 0);
		parse->cb_data(data, length, parse->dataleft, parse->arg);
		parse->pos += length;
		assert(parse->pos > 0);
		
		assert(0);
		
		// determine if we are expecting any more data...
		if (parse->dataleft == 0) {
			if (parse->cb_complete) { parse->cb_complete(parse->arg); }
			assert(expecting == 0);
		}
		else {
			expecting = 1;	// we are expecting more data.
		}
	}
	else {


		
// 		printf("parse_process: Adding to existing buffer(%d). length=%d\n", parse->length, length);
		
		// we are still processing headers, then we need to just add the data and then try to process.
		// add the new data to the existing buffer.
		assert((parse->buffer == NULL && parse->length == 0) || (parse->buffer && parse->length > 0));
		assert(length > 0);
		parse->buffer = (char *) realloc(parse->buffer, parse->length + length + 1);
		assert(parse->buffer);
		memcpy(parse->buffer + parse->length, data, length);
		parse->length += length;
		parse->buffer[parse->length] = 0;

		char *current;
		char *next;
		int len;
		
		assert(parse->pos >= 0 && parse->pos < length);
		current = parse->buffer + parse->pos;
		next = index(current, '\n');
		while (next != NULL) {
		
			// get the length of the line we've found.
			assert(next != current);
			len = next - current;
			assert(len > 0);

			// next is pointing to the '\n' char... move it to the next one.
			next ++;
			
			// if we found a line, we are going to process it, therefore we can increase our 'pos' value now.
			parse->pos = next - parse->buffer;
			assert(parse->pos <= parse->length);
			
			// turn the \n into a NULL to terminate the string.
			current[len-1] = 0;
			
// 			printf("parse_process: loop. current='%s':%d, pos=%d, length=%d\n", current, len, parse->pos, parse->length);
			
			// trim the dos line feeds at the end if there are any.
			while (len > 0 && current[len-1] == '\r') {
				current[len-1] = 0;
				len --;
			}
			
			// trim the dos line feeds at the start if there are any.
			while (current < next && *current == '\r') {
				current ++;
				len --;
			}

// 			printf("parse_process: after trim. current='%s':%d, pos=%d, length=%d, state=%d\n", current, len, parse->pos, parse->length, parse->state);

			if (parse->state == state_request) {
				printf("parse_process: processing request. current='%s':%d, pos=%d, length=%d\n", current, len, parse->pos, parse->length);
	
				char *word;
				char *rest;
				char *params;
				
				rest = current;					// start off at the begining of the string.

				// get the METHOD (GET, POST, HEAD, etc).
				word = strsep(&rest, " ");		// pull out the first word.
				assert(word);
				if (parse->cb_method) { parse->cb_method(word, parse->arg); }
					
				// get the PATH
				assert(rest);
				word = strsep(&rest, " ");		// pull out the next word.
				assert(word);
				
				// we have the path, but first we need to check that there are no params added to it.
				params = index(word, '?');
				if (params) {
					*params = 0;
					if (parse->cb_path) { parse->cb_path(word, parse->arg); }
					if (parse->cb_params) { parse->cb_params(params, parse->arg); }
					if (parse->cb_formdata) {
						// need to seperate each form value.
						assert(0);
					}
				}
				else {
					if (parse->cb_path) { parse->cb_path(word, parse->arg); }
				}

				// get the version
				word = strsep(&rest, " ");		// pull out the first word.
				assert(word);
				if (parse->cb_version) { parse->cb_version(word, parse->arg); }
				
				// change the state.
				parse->state = state_headers;
			}
			else if (parse->state == state_headers && *current == 0) {
				// we've reached the end of the headers.  
				
				printf("parse_process: end of headers.  length=%d, pos=%d, dataleft=%d\n", parse->length, parse->pos, parse->dataleft);
				
				// If we have content-length, then we need to set mode to Data, otherwise, we are done, and need to call the 'complete' callback.
				if (parse->dataleft > 0) {
					parse->state = state_data;
					if (parse->cb_data) {
						int l = parse->length - parse->pos;
						assert(l <= parse->dataleft);
						parse->cb_data(current, l, parse->dataleft - l, parse->arg);
						parse->dataleft -= l;
						printf("parse_process: AAA.  length=%d, pos=%d, dataleft=%d\n", parse->length, parse->pos, parse->dataleft);
					}
					else {
						printf("no callback function cb_data set\n");
					}
					
					// if cb_formdata is set, and we now have all the data, then we need to parse the data and get the formvalues out of it.
					if (parse->cb_formdata) {
						assert(0);
					}
					
					assert(parse->dataleft >= 0);
					if (parse->dataleft == 0) {
						if (parse->cb_complete) { parse->cb_complete(parse->arg); }
						parse->state = state_done;
						expecting = 0;
						printf("parse_process: BBB.  length=%d, pos=%d, dataleft=%d\n", parse->length, parse->pos, parse->dataleft);
					}
				}
				else {
					assert(parse->pos == parse->length);
					if (parse->cb_complete) parse->cb_complete(parse->arg);
					parse->state = state_done;
					expecting = 0;
					
					printf("parse_process: CCC.  length=%d, pos=%d, dataleft=%d\n", parse->length, parse->pos, parse->dataleft);
					
				}
			}
			else if (parse->state == state_headers) {
				
				char *key;
				char *value;
				
				key = current;
// 				printf("header: len=%d\n", strlen(key));
				
				// first we clear off any spaces before the first :
				// even though we are modifying 'value', we are actually affecting the end of 'key'.
				value = index(current, ':');
				value --;
				while (value > key && (*value == ' ' || *value == '\t')) { *value = 0; value --; }
				
				value = index(current, ':');
				*value = 0;
				value++;
				while (*value == ' ' || *value == '\t') { value ++; }
				
				printf("header: key='%s', value='%s'\n", key, value);
				
				// call the callback routine
				if (parse->cb_header) { parse->cb_header(key, value, parse->arg); }
					
				// ... look for the content-length and set the dataleft and call the callback if it is set.
				if (strcasecmp(key, "content-length") == 0) {
					assert(parse->dataleft == 0);
					parse->dataleft = atoi(value);
					if (parse->cb_datalength) { parse->cb_datalength(parse->dataleft, parse->arg); }
				}
				else if (parse->cb_host && strcasecmp(key, "host") == 0) {
					// the host value can contain the port also... so we need to look for that.
					char *port = index(value, ':');
					int portnum = 0;
					
					if (port) {
						*port = 0;
						port++;
						portnum = atoi(port);
					}

					parse->cb_host(value, portnum, parse->arg);
				}
			}
			else if (parse->state < state_data) {
				// look at our state to see what we are looking for.
				assert(0);
				
			}
			else {
				// if we are in 'data' mode, then we need to load in all the data and then parse it, calling callback routines.
				assert(0);
			}
			
			// find the next '\n' in the stream.
			if (parse->pos >= parse->length) {
				next = NULL;
			}
			else {
// 				assert(0);
				current = next;
				next = index(current, '\n');
			}
		}

		// if the content-length was zero, and we have finished with headers, then we are not expecting any more.
	}
	
	assert((expecting == 0 && parse->state == state_done) || (expecting == 1 && parse->state < state_done));
	return(expecting);
}


//-----------------------------------------------------------------------------
// given a string containing the parameters, this function will split it into 
// key/value pairs and call the callback routine.
void parse_params(char *params, void (*handler)(const char *key, const char *value, void *arg), void *arg)
{
	char *next;
	char *key;
	char *value;
	
	assert(params);
	assert(handler);
	
	next = params;
	while (next) {
		key = next;
		value = index(next, '=');
		if (value) {
			*value = 0;
			value++;
			next = index(value, '&');
			if (next) {
				*next = 0;
				next++;
			}
			
			handler(key, value, arg);
		}
		else { next = NULL; }
	}
}


