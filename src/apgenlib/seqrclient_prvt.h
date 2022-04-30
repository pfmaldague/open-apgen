/*
 * seqrclient_prvt.h
 */
#ifndef SEQRCLIENT_PRVT_H_
#define SEQRCLIENT_PRVT_H_
#include <curl/curl.h>
#include <assert.h>

#define curl_ok(x) assert(x == CURLE_OK)

extern void auth_init(char * (*)(void));
extern void auth_setup(CURL *);
extern void auth_oper(char *);
extern int auth_cookie(void);
extern int auth_success(void);

#endif

