#ifndef _TMS_INTFC_H_
#define _TMS_INTFC_H_

#include <RES_exceptions.H>
#include <libcrest.h>

namespace TMS {
			// in APmain.C
extern TLLibraryAPI*	crestAPI;
			// in access_seqr.C
extern void		publishAllResources() throw(eval_error);

extern char* pwd;

} // namespace TMS


#endif /* _TMS_INTFC_H_ */
