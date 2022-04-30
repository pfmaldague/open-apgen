#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <assert.h>

#include "RES_def.H"
#include "action_request.H"
#include "AbstractResource.H"
#include "ActivityInstance.H"
#include "RES_eval.H"
#include "EventImpl.H"
#include "apcoreWaiter.H"
#include "xmlrpc_api.H"

#ifdef have_xml_reader
extern "C" {
#	include <dom_chunky_xml_intfc.h>
} // extern "C"
#endif /* have_xml_reader */

using namespace std;

long int RCsource::get_index_count() const {
	return payload->Object->array_elements[0]->indices.size();
}

Rcontainer::Rcontainer(
		smart_ptr<res_container_object>&	obj,
		Behavior&				T,
		apgen::RES_CLASS			c,
		apgen::RES_CONTAINER_TYPE		rct,
		apgen::DATA_TYPE			dt)
	: resContPLD(obj, T, c, rct, dt)
		{}

Rcontainer::~Rcontainer() {
	Object.dereference();
}

res_container_object::res_container_object(
		RCsource*	rc,
		Cstring		the_integrand)
	: res(rc),
		behaving_object(*rc->payload->Type.tasks[0]) {

    //
    // let's define all simple resources here
    //
    Rsource*		new_resource;
    apgen::RES_CONTAINER_TYPE container_type = rc->payload->cardinality;
    int			i, initial_size = 0;
    bool			integrand_is_consumable = false;

    int	n_to_create = rc->payload->Type.SubclassTypes.size();

    // Loop to create each member of Arrayed Resource, OR single simple Res.
    for (i = 0; i < n_to_create; i++) {
	Behavior*	subtype = res->payload->Type.SubclassTypes[i];
	Cstring		index_name = subtype->name;
	vector<Cstring>& index_sequence = res->payload->Type.SubclassIndices[i];

	Rsource*			rs1;
	aafReader::precomp_container*	pc;

	switch(res->get_class()) {
	    case apgen::RES_CLASS::CONSUMABLE:
		new_resource = new RES_consumable(
				*res,
				*subtype,
				index_name);
		break;
	    case apgen::RES_CLASS::NONCONSUMABLE:
		new_resource = new RES_nonconsumable(
				*res,
				*subtype,
				index_name);
		break;
	    case apgen::RES_CLASS::STATE:
		new_resource = new RES_state(
				*res,
				*subtype,
				index_name);
		break;
	    case apgen::RES_CLASS::SETTABLE:

		//
		// We should create a precomputed resource
		// if necessary. To find out, we consult the
		// list in aafReader:
		//
		if((pc = aafReader::precomp_containers().find(rc->get_key()))) {
		    new_resource = new RES_precomputed(
				*res,
				*subtype,
				index_name,
				pc);
		} else {
		    new_resource = new RES_settable(
				*res,
				*subtype,
				index_name);
		}
		break;
	    case apgen::RES_CLASS::EXTERNAL:
		new_resource = NULL;
		break;
	    case apgen::RES_CLASS::INTEGRAL:
	    case apgen::RES_CLASS::ASSOCIATIVE:
	    case apgen::RES_CLASS::ASSOCIATIVE_CONSUMABLE:
	    default:	//should not get here -- weeded out earlier
		new_resource = NULL;
		break;
	}

	//Have valid RES_resource, so create linked RES_container/RES_resource:
	assert(new_resource);
	array_elements.push_back(new_resource);
    }
}


void	Rcontainer::clear_profiles() {
	Rsource*	theResource;

	for(int i = 0; i < Object->array_elements.size(); i++) {
		theResource = Object->array_elements[i];
		// make sure no infinite loop occurs later
		theResource->getTheProfile().clear();
	}
}
