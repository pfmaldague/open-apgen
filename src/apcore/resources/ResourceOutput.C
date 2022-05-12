#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "Rsource.H"

//
// Static method used by (XML)TOL output methods and by FREEZE and
// UNFREEZE requests. The idea is to convert a set of strings into
// a set of concrete resources that the strings represent.
//
// To make the client's life simpler, if a string is the name of a
// resource container, all resources in the container should be
// included in the output list.
//
// If the client is an XMLTOL request, all it needs is a list of
// resources, because the metadata structure is just a list of
// resources. If the client is a TOL request, it makes more sense
// to organize things by container and then list the resources in
// each container.
//
void Rsource::expand_list_of_resource_names(
	    vector<string>					   resource_names,
	    tlist<alpha_string, Cntnr<alpha_string, Rsource*> >&  all_included_res) {
    
    stringtlist	selected_resources;
    Rsource*	resource;
    RCsource*	container;

    for(int vc = 0; vc < resource_names.size(); vc++) {
	Cstring		resname = resource_names[vc].c_str();
 
	//
	// Identify any resource containers in the given list of
	// resources. If there are any, replace them by the individual
	// resources they contain.
	//
	if((container = RCsource::resource_containers().find(resname))) {
	    vector<Rsource*>& vec = 
			  container->payload->Object->array_elements;
	    for(int i = 0; i < vec.size(); i++) {
		resource = vec[i];

		//
		// Forgive duplication
		//
		if(!selected_resources.find(resource->name)) {
		    selected_resources << new emptySymbol(resource->name);
		}
	    }
	} else {

	    //
	    // Forgive duplication
	    //
	    if(!selected_resources.find(resname)) {
		selected_resources << new emptySymbol(resname);
	    }
	}
    }
    stringtlist	copy_of_selected_resources = selected_resources;

    //
    // We need to worry about resources who depend on other resources
    // through their profile. We need to check the resource data members
    // that capture these dependencies.
    //
    tlist<alpha_string, Cntnr<alpha_string, RCsource*> > containers_required_by_dependencies;
    stringtlist					 	  containers_already_checked;


    bool	include_all_res_and_acts = resource_names.size() == 0;

    {
	Rsource::iterator iter;

	while((resource = iter.next())) {
	    emptySymbol*	es = NULL;
	    if(	(include_all_res_and_acts
		|| (es = copy_of_selected_resources.find(resource->name))
		)
	      ) {
	
		if(es) {
		    delete es;
		}
	
		//
		// We may already have included this resource from dependencies
		//
		if(!all_included_res.find(resource->name)) {
		    all_included_res << new Cntnr<alpha_string, Rsource*>(resource->name, resource);
		}
		container = resource->get_container();
	
		if(!include_all_res_and_acts && !containers_already_checked.find(container->get_key())) {
		    containers_already_checked << new emptySymbol(container->get_key());
	
		    //
		    // check that all prerequisites are included
		    //
		    tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& deps(
					container->payload->ptrs_to_containers_used_in_profile);
		    slist<alpha_void, Cntnr<alpha_void, RCsource*> >::iterator it(deps);
		    Cntnr<alpha_void, RCsource*>* cres;
	
		    while((cres = it())) {
			RCsource* prerequisite_container = cres->payload;
			if(!containers_required_by_dependencies.find(prerequisite_container->get_key())) {
	
			    //
			    // This container was not included
			    //
			    containers_required_by_dependencies << new Cntnr<alpha_string, RCsource*>(
							prerequisite_container->get_key(),
							prerequisite_container);
			}
			RESptr*	 rp;
			vector<Rsource*>& vec = prerequisite_container->payload->Object->array_elements;
	
			//
			// Now check that all the resources inside this container are included
			//
			for(int i = 0; i < vec.size(); i++) {
			    Rsource*	dependent_resource = vec[i];
			    if(!all_included_res.find(dependent_resource->name)) {
				all_included_res << new Cntnr<alpha_string, Rsource*>(
					dependent_resource->name,
					dependent_resource);
			    }
			}
		    }
		}
	    }
	}
    }

    //
    // We deleted all the resources we could make sense of.
    // If any are left, they don't exist:
    //
    if(copy_of_selected_resources.get_length() > 0) {
	stringstream		ss;
	stringslist::iterator   sl(copy_of_selected_resources);
	emptySymbol*		es;

	ss << "The following resources requested for XMLTOL output do not exist:\n";
	while((es = sl())) {
	    ss << "\t" << *es->get_key() << "\n";
	}
	throw(eval_error(ss.str().c_str()));
    }
}
