#ifndef _DOUBLE_LIST_H_
#define _DOUBLE_LIST_H_

#include <backpointer.H>

// nbase should be a BPnode template class
template <class keyClass, class nbase, class ownerData = short>
class double_list {
public:
	double_list() : pointers(true) {}
	double_list(const double_list& dl) : pointers(true) {
		typename slist<keyClass, nbase>::iterator	iter(dl.content);
		nbase*						obj;

		while((obj = iter())) {
			(*this) << obj->copy(); } }
	virtual ~double_list() {}

private:
	/* we need two tlists: the list that holds nodes with content,
	 * and a list of smart pointers to nodes with content. The
	 * nodes with content (nbase) should be BPnodes, so they can
	 * be pointed to by smart pointers.
	 *
	 * Make these members private; we want to make sure insertion
	 * is always handled via the operator << in _this_ class. */
	tlist<keyClass, nbase, ownerData>						content;
	tlist<alpha_int, BPptr<alpha_int, nbase, intLinker, ownerData>, ownerData >	pointers;
public:

	virtual double_list&	operator << (nbase* node) {
		content << node;
		pointers << new BPptr<alpha_int, nbase, intLinker, ownerData>(
					content.get_length(),
					node);
		return *this;
	}
	nbase*	operator[](const unsigned int i) const {
		BPptr<alpha_int, nbase, intLinker, ownerData>* ptr = pointers[i];
		return ptr ? ptr->BP : NULL;
	}
	nbase*	find(const keyClass& key) const {
		return content.find(key);
	}
	nbase*	find_at_or_before(const keyClass& key) const {
		return content.find_at_or_before(key);
	}
	long	get_index(nbase* node) const {
		typename backptr<nbase>::ptr2p2b*	ptr2BPptr = node->myBP.pointers.first_node();
		BPptr<alpha_int, nbase, intLinker, ownerData>* ptr
				= dynamic_cast<BPptr<alpha_int, nbase, intLinker, ownerData>* >(
					ptr2BPptr->payload);
		return ptr->get_const_linker().get_int_consumption() - 1;
	}
	long	get_length() const {
		return content.get_length();
	}
	nbase*	first_node() const {
		return content.first_node();
	}
	nbase*	last_node() const {
		return content.last_node();
	}
	nbase*	latest_node() const {
		return content.latest_node();
	}
	nbase*	earliest_node() const {
		return content.earliest_node();
	}
	void	clear() {
		content.clear();
	}
	typename slist<keyClass, nbase, ownerData>::iterator	get_iterator() const {
		return typename slist<keyClass, nbase, ownerData>::iterator(content);
	}
};

#endif /* _DOUBLE_LIST_H_ */
