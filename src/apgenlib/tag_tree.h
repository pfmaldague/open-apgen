#ifndef _TAG_TREE_H
#define _TAG_TREE_H

/* C-callable interface for defining arbitrary trees tagged by strings.
 *
 * In short: a set of C wrappers for C_list.H */

/****************************************************************
 * functions that take neither list, node, not iterator args	*
 ****************************************************************/

/* creates new, or clears if already exists */
extern void*
getAnEmptyListNamed(

		const char* theListName);
/* NULL if the list does not already exist */
extern void*
getListNamed(
		const char* theListName);


/********************************************************
 * functions that take one node argument		*
 ********************************************************/

extern const char*
getNodeKey(
		void* theNode);

/* returns NULL if node is not a ListNode */
extern void*
getListOf(
		void* theNode);

/* returns NULL if the node is not a SymbolNode */
extern const char*
getNodeValue(
		void* theNode);

/********************************************************
 * functions that take one list argument		*
 ********************************************************/

/* NOTES:
 *	- returns NULL if list is NULL
 *	- the iterator will delete itself at the end of the loop!
 */
extern void*
getIteratorForList(
		void* theList);

extern int
getLengthOfList(
		void* theList);

/* does nothing if theList is NULL */
extern void*
insertSymbolNodeNamed(
		const char* theKey,
		const char* theValue,
		void* theList);

/* NULL if not found or if List is NULL */
extern void*
findInList(
		void* theList,
		const char* theKey);

/* does nothing if theList is NULL */
extern void*
insertListNodeNamed(
		const char* theKey,
		void* theList);

/********************************************************
 * functions that take one iterator argument		*
 ********************************************************/

/* NOTE: the iterator will delete itself at the end of the loop! */
extern void*
getNextNode(
		void* theIterator);

#endif /* _TAG_TREEE_H */
