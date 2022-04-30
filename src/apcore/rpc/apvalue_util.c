#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <apvalue.h>
// #include <typed_value.h>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#include <stdlib.h>
#endif

int make_simpleInt(apvalue* s, int64_t h)  {
	s->type = AP_INT;
	s->apvalue_u.asInt = h;
	return 0; }

int make_simpleFloat(apvalue* s, double d) {
	s->type = AP_FLOAT;
	s->apvalue_u.asFloat = d;
	return 0; }

int make_simpleBool(apvalue* s, bool_t b) {
	s->type = AP_BOOL;
	s->apvalue_u.asBool = b;
	return 0; }

int make_simpleTime(apvalue* s, int64_t* t) {
	s->type = AP_TIME;
	s->apvalue_u.asTime = *t;
	return 0; }

int make_simpleDuration(apvalue* s, int64_t* D) {
	s->type = AP_DURATION;
	s->apvalue_u.asDuration = *D;
	return 0; }

int make_simpleString(apvalue* s, const char* str) {
	s->type = AP_STRING;
	s->apvalue_u.asString = strdup(str);
	return 0; }

int make_simpleInstance(apvalue* s, char* inst) {
	s->type = AP_INSTANCE;
	s->apvalue_u.asInstance = strdup(inst);
	return 0; }

int make_simpleArray(apvalue* s) {
	s->type = AP_ARRAY;
	return 0; }

int make_simpleStruct(apvalue* s) {
	s->type = AP_STRUCT;
	return 0; }

void copy_apvalue(apvalue* to, apvalue* from) {
	to->type = from->type;
	switch(from->type) {
		case AP_INT:
			to->apvalue_u.asInt = from->apvalue_u.asInt;
			break;
		case AP_FLOAT:
			to->apvalue_u.asFloat = from->apvalue_u.asFloat;
			break;
		case AP_BOOL:
			to->apvalue_u.asBool = from->apvalue_u.asBool;
			break;
		case AP_TIME:
			to->apvalue_u.asTime = from->apvalue_u.asTime;
			break;
		case AP_DURATION:
			to->apvalue_u.asDuration = from->apvalue_u.asDuration;
			break;
		case AP_STRING:
			to->apvalue_u.asString = strdup(from->apvalue_u.asString);
			break;
		case AP_INSTANCE:
			to->apvalue_u.asInstance = strdup(from->apvalue_u.asInstance);
			break;
		case AP_ARRAY:
			break;
		case AP_STRUCT:
			break;
		default:
			break; } }

void mk_aparrayelement(aparrayelement* apval, apindex* label, int IDtoUse, int parent, apvalue* val) {
	apval->indx.tag = label->tag;
	if(label->tag == AP_STRUCT_STYLE) {
		apval->indx.apindex_u.s = strdup(label->apindex_u.s); }
	else if(label->tag == AP_ARRAY_STYLE) {
		apval->indx.apindex_u.x = label->apindex_u.x; }
	apval->apid = IDtoUse;
	apval->parent = parent;
	copy_apvalue(&apval->content, val); }
