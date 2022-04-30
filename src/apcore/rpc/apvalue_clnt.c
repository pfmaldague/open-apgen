/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "apvalue.h"
 #include <unistd.h>
 #include <string.h>

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

void *
print_nothing_0(argp, clnt)
	void *argp;
	CLIENT *clnt;
{
	static char clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call(clnt, PRINT_NOTHING, xdr_void, argp, xdr_void, &clnt_res, TIMEOUT) != RPC_SUCCESS)
		return (NULL);
	return ((void *)&clnt_res);
}

char **
print_value_1(argp, clnt)
	apvalue *argp;
	CLIENT *clnt;
{
	static char *clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call(clnt, PRINT_VALUE, xdr_apvalue, argp, xdr_wrapstring, &clnt_res, TIMEOUT) != RPC_SUCCESS)
		return (NULL);
	return (&clnt_res);
}

char **
print_array_1(argp, clnt)
	aparray *argp;
	CLIENT *clnt;
{
	static char *clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call(clnt, PRINT_ARRAY, xdr_aparray, argp, xdr_wrapstring, &clnt_res, TIMEOUT) != RPC_SUCCESS)
		return (NULL);
	return (&clnt_res);
}
