#ifndef _ABSTRACT_RES_H_
#define _ABSTRACT_RES_H_

#include <APdata.H>
#include <BehavingElement.H>
// #include <accessorkey.H>

class Cstack;

//
// Object which supports the use() method of an abstract resource,
// and also the constructor method of that abstract resource.
//
class abs_res_object: public behaving_object {
public:

    //
    // For any class variables:
    //
    behaving_element		parent_constructor_obj;

    //
    // used once for each usage of the resource:
    //
    abs_res_object(
		const Behavior&	   abs_type,	    // the type that defines the abs. res.
		int		   task_index,      // always 1 for 'use' task
		behaving_object*   parent_scope);   // constructor object for class members

    //
    // used to construct abstract resource objects. Unlike
    // activity instances, these objects are transient;
    // they are created whenever a usage occurs. Their
    // main function is to support the constructor task
    // of the abstract resource, which contains any class
    // members defined for the resource. Examples: ID,
    // PARENT, START. See ActivityInstance.H for the
    // first few, which are common with abstract resources.
    //
    abs_res_object(
    		task&		  constructor,
		behaving_element& parent_context); // the calling object via use Foo(...) or Foo.bar(...)

    virtual TypedValue	get_descr(
				const Cstring&,
				bool&) const;

    abs_res_object(const abs_res_object& EC) = delete;

    ~abs_res_object() {
    }

    //
    // Recall that this method is used by the code
    // that executes a 'use Foo(...)' instruction. The
    // code will execute the 'usage task' of object
    // Foo, whose symbols are contained in behaving
    // object B. The code in question needs to find
    // the timing information for the usage statement,
    // and in particular the location of symbols 'start'
    // and 'finish'. This method tells the code whether
    // to look in B itself (level 2) or in B's parent
    // object (level 1). It is never the case that the
    // code should look in global symbols, which would
    // be level 0.
    //
    // Concretely speaking, the only two possible objects
    // able to use an abstract resource are
    //
    //   - activity instances
    //
    //   - abstract resources
    //
    // In both cases, the usage instruction is part of
    // the object's modeling task (which can be of the
    // 'resource usage' or 'modeling' type).
    //
    // An activity instance has a constructor, which is
    // a level 1 task; that constructor has a symbol table
    // which includes all attributes, including start and
    // finish. Thus, for an activity instance, the method
    // returns 1.
    //
    // An abstract resource has a constructor in which
    // attributes such as subsystem and description can
    // be listed; there is only one instance of the
    // corresponding object, held in the
    // abstract_resources() list. The symbols that are
    // relevant to a usage instruction are attached to
    // the modeling task, which is a level 2 task; that
    // is where start and finish symbols are defined
    // (finish is only defined if the resource is used
    // from ... to). Thus, for a call to an abstract
    // resource, which generates an abs_res_object,
    // the method returns 2.
    //
    // Note that this method is not needed if the usage
    // instruction contains explicit timing information
    // via a 'at' or 'from to' clause. It is only needed
    // when the default timing applies because no such
    // clause is provided.
    //
    int			level_where_start_is_defined() const override {
	return 1;
    }

    virtual Cstring	get_id() const override {
		return operator[](ID).get_string();
    }

    virtual bool	print(
				aoString&,
				const char* prefix) const override;

    static long&	currentID();
};

#endif /* _ABSTRACT_RES_H_ */
