#ifndef _XML_RPC_COMPRESS_H
#define _XML_RPC_COMPRESS_H

#include <XmlRpcValue.H>
#include <string>
#include <sstream>
#include <stdlib.h>

using namespace std;

extern int	compressAString(
				const std::string &inbuf,
				XmlRpc::XmlRpcValue::BinaryData &result);
extern int	decompressAString(
				XmlRpc::XmlRpcValue::BinaryData &inbuf,
				std::string &result);

#endif /* _XML_RPC_COMPRESS_H */
