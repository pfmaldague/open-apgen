#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <zlib.h>
#include <assert.h>
#include <stdlib.h> // for malloc
#include <XmlRpcCompress.H>


#define CHUNK 65536

int decompressAString(XmlRpc::XmlRpcValue::BinaryData &inbuf, string &result) {
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char *in = (unsigned char *) malloc(inbuf.size());
	unsigned char out[CHUNK];
	vector<char>::iterator it;
	int c = 0, i;
	ostringstream os;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;

	ret = inflateInit(&strm);
	if(ret != Z_OK) {

		//
		// Added by Jon Castello in the MGSS
		// version of APGen:
		//
		free(in);
		return -1;
	}

	for(it = inbuf.begin(); it != inbuf.end(); it++) {
		in[c++] = *it;
	}

	strm.avail_in = inbuf.size();
	strm.next_in = in;
	do {
		strm.avail_out = CHUNK;
		strm.next_out = out;
		ret = inflate(&strm, Z_NO_FLUSH);
		assert(ret != Z_STREAM_ERROR);
		switch(ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				inflateEnd(&strm);

				//
				// Added by Jon Castello in the MGSS
				// version of APGen:
				//
				free(in);
				return -1;
		}
		have = CHUNK - strm.avail_out;
		for(i = 0; i < have; i++) {
			os << out[i];
		}
	} while(strm.avail_out == 0);
	inflateEnd(&strm);
	free(in);
	result = os.str();
	// should probably do someting friendlier
	// assert(ret == Z_STREAM_END);
	if(ret == Z_STREAM_END) {
		return -1;
	}
	return 0;
}

int compressAString(const string &inbuf, XmlRpc::XmlRpcValue::BinaryData &result) {
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char out[CHUNK];
	unsigned char *in = (unsigned char *) inbuf.c_str();
	int i;

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
	if (ret != Z_OK)
		return ret;

	/* compress until end of file */
	strm.avail_in = inbuf.size();
	strm.next_in = in;

	/* run deflate() on input until output buffer not full, finish
	   compression if all of source has been read in */
	do {
		strm.avail_out = CHUNK;
		strm.next_out = out;
		ret = deflate(&strm, Z_FINISH);    /* no bad return value */
		assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
		have = CHUNK - strm.avail_out;
		for(i = 0; i < have; i++) {
			result.push_back(out[i]); }
		} while (strm.avail_out == 0);
	assert(strm.avail_in == 0);     /* all input will be used */
	assert(ret == Z_STREAM_END);        /* stream will be complete */

	/* clean up and return */
	(void)deflateEnd(&strm);
	return Z_OK; }

