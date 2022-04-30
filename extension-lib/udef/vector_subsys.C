#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include <unistd.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <memory>
#include <stdlib.h>


#include "AP_exp_eval.H"
#include "APdata.H"
#include "C_string.H"
#include "C_list.H"
#include "RES_eval.H"
#include "apDEBUG.H"

#include "ActivityInstance.H"
#include "spiceIntfc.H"

#define ARG_POINTER args.size()
#define ARG_POP args.pop_front()
#define ARG_STACK slst<TypedValue*>& args
#define WHERE_TO_STOP 0

#define list_array_type 1
#define struct_array_type 2
#define SET_SYMBOL( x , y , z ) x->set_typed_val(y);

using namespace apgen;
extern void	add_to_Func_list( const char * , generic_function P ) ;

extern void qmult(double A[4], double B[4], double *C);
extern void vecrot( double q1to2[4], double r1[3], double *r2);
extern void qposcosine( double qIn[4], double *qOut);
extern void qnorm( double qIn[4], double *qOut);
extern void MatrixMultiply(double *m1, double *m2, int nrow1, int ncol1, int ncol2, double *mout);
extern void Vector_Cross_Product(double A[3], double B[3], double *C);
extern void Vector_Scale(double A[3], double B, double *C);
extern void Vector_Subtract(double A[3], double B[3], double *C);
extern double Vector_Dot_Product(double A[3], double B[3]);
extern double Compute_Angle(double A[3], double B[3]);
extern double Vector_Magnitude(double A[3]);
extern void Unit_Vector(double A[3], double *B);

extern apgen::RETURN_STATUS
get_double_array(Cstring& errs, const TypedValue& given, double returned[], int returned_size);

extern TypedValue create_float_list(int n, double* d);

extern TypedValue create_float_struct(int n, const char* indices[], double* d);

RETURN_STATUS SetVectorClientParams_ap(Cstring&, TypedValue* result, ARG_STACK) {

	//
	// Dummy function. In the client implementation of
	// the library, this method can be used to specify
	// the server parameters (host and port).
	//
	*result = 1L;
	return RETURN_STATUS::SUCCESS;
}

RETURN_STATUS Vector_Cross_Product_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue  * n1,  // "A"
	  * n2;  // "B"

	double		C[3];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 2 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function "
		     << "Vector_Cross_Product; expected 2, got "
		     << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(!n1->is_array()) {
		errs << " error: Vector_Cross_Product first argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	ListOVal*	given_matrix = &n1->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "Vector_Cross_Product error: first argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement*	given_matrix_element;


	double      A[3];
	for(int i = 0; i < given_matrix->get_length(); i++) {
		given_matrix_element = (*given_matrix)[i];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "Vector_Cross_Product: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		A[i] = given_matrix_element->Val().get_double();
	}

	if(!n2->is_array()) {
		errs << "Vector_Cross_Product error: second argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	given_matrix = &n2->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "Vector_Cross_Product error: second argument expected given "
		     << "array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
	}		
	double      B[3];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "Vector_Cross_Product: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		B[index] = given_matrix_element->Val().get_double();
	}


	Vector_Cross_Product(A,B,C);

	*result = create_float_list(3, C);
	return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS MatrixMultiply_ap ( Cstring & errs , TypedValue * result , ARG_STACK ) {
	TypedValue  * n1,  // "m1"
	  * n2, // "m2"
	  * n3, // "nrow1"
	  * n4, // "nrow2"
	  * n5;  // "mcol2"


	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 5 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function MatrixMultiply_ap; expected 5, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;

        int nrow1;
        int ncol1;
	int ncol2;
        nrow1 = n3->get_int();
	ncol1 = n4->get_int();
	ncol2 = n5->get_int();

	if( n3->get_type() != DATA_TYPE::INTEGER ) {
		errs << "MatrixMultiply_api: third argument is not an integer \n";
		return RETURN_STATUS::FAIL; }
	if( n4->get_type() != DATA_TYPE::INTEGER ) {
		errs << "MatrixMultiply_api: fourth argument is not an integer \n";
		return RETURN_STATUS::FAIL; }
	if( n5->get_type() != DATA_TYPE::INTEGER ) {
		errs << "MatrixMultiply_api: fifth argument is not an integer \n";
		return RETURN_STATUS::FAIL; }
	double m1[nrow1][ncol1];
	double m2[ncol1][ncol2];

	////
	if(!n1->is_array()) {
		errs << "MatrixMultiply_api error: expected argument m1 to be an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	ListOVal*	given_matrix1 = &n1->get_array();
	if(given_matrix1->get_length() != nrow1) {
	  errs << "MatrixMultiply_api error: expected given array m1 to have" << nrow1 << " items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement*					ae1;

	for(int outer_index = 0; outer_index < given_matrix1->get_length(); outer_index++) {
		ae1 = (*given_matrix1)[outer_index];
		if(!ae1->Val().is_array()) {
			errs << "MatrixMultiply_api error: expected given array m1 to contain arrays\n";
			return RETURN_STATUS::FAIL;
		}
		ListOVal&	given_sub_array1(ae1->Val().get_array());

		if(given_sub_array1.get_length() != ncol1) {
		  errs << "MatrixMultiply_api error: expected given array m1 to contain arrays of " << ncol1 << " elements each\n";
			return RETURN_STATUS::FAIL;
		}

		ArrayElement*					given_matrix_element1;

		for(int inner_index = 0; inner_index < given_sub_array1.get_length(); inner_index++) {
			given_matrix_element1 = given_sub_array1[inner_index];
			if(!given_matrix_element1->Val().is_numeric()) {
				errs << "MatrixMultiply_api error: expected argument m1 to be a matrix of floats\n";
				return RETURN_STATUS::FAIL;
			}

			double		value1 = given_matrix_element1->Val().get_double();
			m1[outer_index][inner_index] = value1;
		}
	}

	////
	if(!n2->is_array()) {
		errs << "MatrixMultiply_api error: expected argument m2 to be an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	ListOVal*	given_matrix2 = &n2->get_array();
	if(given_matrix2->get_length() != ncol1) {
	  errs << "MatrixMultiply_api error: expected given array m2 to have" << ncol1 << " items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement*					ae2;

	for(int outer_index2 = 0; outer_index2 < given_matrix2->get_length(); outer_index2++) {
		ae2 = (*given_matrix2)[outer_index2];
		if(!ae2->Val().is_array()) {
			errs << "MatrixMultiply_api error: expected given array m2to contain arrays\n";
			return RETURN_STATUS::FAIL;
		}
		ListOVal&	given_sub_array2(ae2->Val().get_array());

		if(given_sub_array2.get_length() != ncol2) {
			errs << "MatrixMultiply_api error: expected given array m2 to contain arrays of " << ncol2 << " elements each\n";
			return RETURN_STATUS::FAIL;
		}

		ArrayElement*					given_matrix_element2;

		for(int inner_index2 = 0; inner_index2 < given_sub_array2.get_length(); inner_index2++) {
			given_matrix_element2 = given_sub_array2[inner_index2];
			if(!given_matrix_element2->Val().is_numeric()) {
				errs << "MatrixMultiply_api error: expected argument m2 to be a : "
					<< nrow1 << " by " << ncol2 << "  matrix of floats\n";
				return RETURN_STATUS::FAIL; 
			}

			double		value2 = given_matrix_element2->Val().get_double();
			m2[outer_index2][inner_index2] = value2;
		}

	}

	////

        double mout[nrow1][ncol2];
	MatrixMultiply((double *)m1, (double *)m2, nrow1, ncol1, ncol2, (double *)mout);

	ListOVal*	returned_matrix = new ListOVal;
	
	for(int outer_index = 0; outer_index < nrow1; outer_index++) {

	    ListOVal*	returned_sub_array_list = new ListOVal;
	    ArrayElement*	returned_sub_array_element = new ArrayElement(outer_index);
	
	    for (int inner_index = 0; inner_index < ncol2; inner_index++) {
	        ArrayElement*	returned_matrix_element = new ArrayElement(inner_index);
	        TypedValue	returned_matrix_element_value;
	        double		value = mout[outer_index][inner_index];
	        returned_matrix_element_value = value;
		SET_SYMBOL (returned_matrix_element, returned_matrix_element_value, errs);
		(*returned_sub_array_list) << returned_matrix_element;
	    }

	    TypedValue	new_sub_array_value;
	    new_sub_array_value = *returned_sub_array_list;
	    SET_SYMBOL (returned_sub_array_element, new_sub_array_value, errs);
	    (*returned_matrix) << returned_sub_array_element;
	}

	*result = *returned_matrix;
	return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS vecrot_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue  * n1,  // "A"
	  * n2;  // "B"

	double		r2[3];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 2 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function vecrot_ap; expected 2, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(!n1->is_array()) {
		errs << " error: vecrot first argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL; }
	ListOVal*	given_matrix = &n1->get_array();
	if(given_matrix->get_length() != 4) {
		errs << "vecrot_api error: first argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL; }
	ArrayElement*	given_matrix_element;
	double      q1to2[4];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "vecrot_api: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value = given_matrix_element->Val().get_double();
		q1to2[index] = value;
	}

	if(!n2->is_array()) {
		errs << "vecrot error: second argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL; }
	given_matrix = &n2->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "vecrot_api error: second argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL; }		
	double      r1[3];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "vecrot_api: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value2 = given_matrix_element->Val().get_double();
		r1[index] = value2;
	}


	vecrot(q1to2,r1,r2);
	*result = create_float_list(3, r2);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS qmult_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue  * n1,  // "A"
	  * n2;  // "B"

	double		C[4];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 2 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function qmult_ap; expected 2, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(!n1->is_array()) {
		errs << " error: qmult first argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL; }
	ListOVal*	given_matrix = &n1->get_array();
	if(given_matrix->get_length() != 4) {
		errs << "qmult error: first argument expected given array to have 4 items in it\n";
		return RETURN_STATUS::FAIL; }
	ArrayElement*	given_matrix_element;
	double      A[4];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "qmult: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value = given_matrix_element->Val().get_double();
		A[index] = value;
	}

	if(!n2->is_array()) {
		errs << "qmult error: second argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL; }
	given_matrix = &n2->get_array();
	if(given_matrix->get_length() != 4) {
		errs << "qmult error: second argument expected given array to have 4 items in it\n";
		return RETURN_STATUS::FAIL; }		
	double      B[4];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "vprjp_api vprjp_api: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value2 = given_matrix_element->Val().get_double();
		B[index] = value2;
	}


	qmult(A,B,C);
	*result = create_float_list(4, C);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS Vector_Scale_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue  * n1,  // "A"
	            * n2;  // "B"

	double		C[3];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 2 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Vector_Scale_ap; expected 2, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(!n1->is_array()) {
		errs << " error: Vector_Scale first argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL; }
	ListOVal*	given_matrix = &n1->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "vprjp_api error: first argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL; }
	ArrayElement*	given_matrix_element;
	double      A[3];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "Vector_Scale: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value = given_matrix_element->Val().get_double();
		A[index] = value;
	}
	if( !n2->is_numeric() )
	{
	errs << "Vector_Scale: second argument is not a double\n" ;
	return RETURN_STATUS::FAIL ;
	}

	Vector_Scale(A,n2->get_double(),C);
	*result = create_float_list(3, C);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS Vector_Subtract_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue  * n1,  // "A"
	  * n2;  // "B"

	double		C[3];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 2 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Vector_Subtract_ap; expected 2, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(!n1->is_array()) {
		errs << " error: Vector_Subtract first argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL; }
	ListOVal*	given_matrix = &n1->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "vprjp_api error: first argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL; }
	ArrayElement*	given_matrix_element;
	double      A[3];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "vprjp_api: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value = given_matrix_element->Val().get_double();
		A[index] = value;
	}

	if(!n2->is_array()) {
		errs << "Vector_Subtract error: second argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL; }
	given_matrix = &n2->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "vprjp_api error: second argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL; }		
	double      B[3];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "vprjp_api: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value2 = given_matrix_element->Val().get_double();
		B[index] = value2;
	}


	Vector_Subtract(A,B,C);
	*result = create_float_list(3, C);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS Unit_Vector_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	  TypedValue  * n1;  // "A"

	double		B[3];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Unit_Vector_ap; expected 2, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;
	if(!n1->is_array()) {
		errs << " error: Unit_Vector first argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL; }
	ListOVal*	given_matrix = &n1->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "vprjp_api error: first argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL; }
	ArrayElement*	given_matrix_element;
	double      A[3];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "vprjp_api vprjp_api: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value = given_matrix_element->Val().get_double();
		A[index] = value;
	}

	Unit_Vector(A,B);
	*result = create_float_list(3, B);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS qposcosine_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	  TypedValue  * n1;  // "A"

	double		qOut[4];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function qposcosine_ap; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;
	if(!n1->is_array()) {
		errs << " error: qposcosine first argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
        }
	ListOVal*	given_matrix = &n1->get_array();
	if(given_matrix->get_length() != 4) {
		errs << "vprjp_api error: first argument expected given array to have 4 items in it\n";
		return RETURN_STATUS::FAIL;
        }
	ArrayElement*	given_matrix_element;
	double      qIn[4];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "vprjp_api vprjp_api: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value = given_matrix_element->Val().get_double();
		qIn[index] = value;
	}

	qposcosine(qIn,qOut);
	*result = create_float_list(4, qOut);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS qnorm_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	  TypedValue  * n1;  // "A"

	double		qOut[4];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function qnorm_ap; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;
	if(!n1->is_array()) {
		errs << " error: qnorm first argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
        }
	ListOVal*	given_matrix = &n1->get_array();
	if(given_matrix->get_length() != 4) {
		errs << "vprjp_api error: first argument expected given array to have 4 items in it\n";
		return RETURN_STATUS::FAIL;
        }
	ArrayElement*	given_matrix_element;
	double      qIn[4];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "vprjp_api vprjp_api: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value = given_matrix_element->Val().get_double();
		qIn[index] = value;
	}

	qnorm(qIn,qOut);
	*result = create_float_list(4, qOut);
	return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS Vector_Dot_Product_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue  * n1,  // "A"
	  * n2;  // "B"

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 2 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Vector_Dot_Product_ap; expected 2, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(!n1->is_array()) {
		errs << " error: Vector_Dot_Product first argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
        }
	ListOVal*	given_matrix = &n1->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "Vector_Dot_Product error: first argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
        }
	ArrayElement*	given_matrix_element;
	double      A[3];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "Vector_Dot_Product: A expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value = given_matrix_element->Val().get_double();
		A[index] = value;
	}

	if(!n2->is_array()) {
		errs << "Vector_Dot_Product error: second argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	given_matrix = &n2->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "Vector_Dot_Product error: second argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
	}		
	double      B[3];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "Vector_Dot_Product: B expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value2 = given_matrix_element->Val().get_double();
		B[index] = value2;
	}


	*result = Vector_Dot_Product(A,B);
	return RETURN_STATUS::SUCCESS ;
	}
RETURN_STATUS Compute_Angle_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue  * n1,  // "A"
	  * n2;  // "B"

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 2 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Compute_Angle_ap; expected 2, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(!n1->is_array()) {
		errs << " error: Compute_Angle first argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	ListOVal*	given_matrix = &n1->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "Compute_Angle error: first argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement*	given_matrix_element;
	double      A[3];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "Compute_Angle: A expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value = given_matrix_element->Val().get_double();
		A[index] = value;
	}

	if(!n2->is_array()) {
		errs << "Compute_Angle error: second argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
        }
	given_matrix = &n2->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "Compute_Angle error: second argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
	}		
	double      B[3];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "Compute_Angle: B expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value2 = given_matrix_element->Val().get_double();
		B[index] = value2;
	}


	try {
	*result = Compute_Angle(A,B);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS Vector_Magnitude_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	  TypedValue  * n1;  // "A"

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Vector_Magnitude_ap; expected 2, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;
	if(!n1->is_array()) {
		errs << " error: Vector_Magnitude first argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
        }
	ListOVal*	given_matrix = &n1->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "Vector_Magnitude error: first argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
        }
	ArrayElement*	given_matrix_element;
	double      A[3];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "Vector_Magnitude: A expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
                }

		double	value = given_matrix_element->Val().get_double();
		A[index] = value;
	}


	try {
	*result = Vector_Magnitude(A);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
}

void	register_vector_subsystem_functions() {
	udef_intfc::add_to_Func_list( "SetVectorClientParams",		SetVectorClientParams_ap, 	apgen::DATA_TYPE::INTEGER);
	udef_intfc::add_to_Func_list( "MatrixMultiply",			MatrixMultiply_ap,		apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "Vector_Magnitude",		Vector_Magnitude_ap,		apgen::DATA_TYPE::FLOATING);
	udef_intfc::add_to_Func_list( "Vector_Scale",			Vector_Scale_ap,		apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "Vector_Subtract",		Vector_Subtract_ap,		apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "Unit_Vector",			Unit_Vector_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "vecrot", 			vecrot_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "qposcosine",			qposcosine_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "qnorm",				qnorm_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "Vector_Cross_Product",		Vector_Cross_Product_ap,	apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "qmult",				qmult_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "Vector_Dot_Product",		Vector_Dot_Product_ap,		apgen::DATA_TYPE::FLOATING);
	udef_intfc::add_to_Func_list( "Compute_Angle",			Compute_Angle_ap,		apgen::DATA_TYPE::FLOATING);

}
