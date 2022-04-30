#ifndef _GMSEC_API_H_
#define _GMSEC_API_H_

extern bool gmsec_messaging_available;
extern "C" {
extern void *(*create_gmsec_thread)(const char *, int, char *[]);
extern void (*gmsec_thread_request)(void *, const char *, void *, void *);
} /* extern "C" */

#endif /* _GMSEC_API_H_ */
