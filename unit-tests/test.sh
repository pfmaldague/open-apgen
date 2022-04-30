#!/bin/bash

function test-aaf
{
#
# Sanity check
#
if [ ! -d $1 ]; then
	echo "$1 is not a directory"
	return
fi
to_test=$1
expect_error="no"
if [[ "$to_test" =~ "error" ]]; then
	expect_error="yes"
	# echo "(expecting error in $1)"
fi

echo -n "$1: "
apbatch -read $1/input.aaf	-read input/write_report.aaf	-read input/build.scr -write out.aaf	>& d 
if grep "APGEN Error" apgen.log > /dev/null; then
	if [ "$expect_error" = "yes" ]; then
		echo "OK";
	else
		echo "errors found";
	fi
else
	if [ -f behavior.txt ]; then
		mv behavior.txt results/$1.beh;
	fi
	if [ -f out.aaf	]; then
		mv out.aaf	results/$1.out.aaf;
	fi
	echo "OK";
fi
# mv apgen.log results/$1.log;
}

function test-script
{
if [ ! -d $1 ]; then
	echo "$1 is not a directory"
	return
fi
to_test=$1
expect_error="no"
if [[ "$to_test" =~ "error" ]]; then
	expect_error="yes"
fi
echo -n "$1: "

options=""
if [ $# = 2 ]; then
	options="$2"
fi

apbatch ${options} -read $1/script -read input/write_report.aaf	-read input/build.scr -write out.aaf	>& d
if grep "APGEN Error" apgen.log > /dev/null; then
	if [ "$expect_error" = "yes" ]; then
		echo "OK";
	else
		echo "errors found";
	fi
else
	if [ -f behavior.txt ]; then
		mv behavior.txt results/$1.beh;
	fi
	if [ -f out.aaf	]; then
		mv out.aaf	results/$1.out.aaf;
	fi
	echo "OK";
fi
# mv apgen.log results/$1.log;
}

if [ ! -d results ]; then
	mkdir results
fi

test-aaf	1-global-int
test-aaf	2-global-int
test-aaf	3-global-2-ints
test-aaf	4-global-list
test-aaf	5-global-struct
test-aaf	6-one-function
test-aaf	7-two-functions
test-aaf	8-function-error
test-aaf	9-function-plus-global
test-aaf	10-function-plus-globals
test-aaf	11-func-glob-error
test-aaf	12-small-loop-in-function
test-aaf	13-loop-error
test-aaf	14-loop-many-ints
test-aaf	15-loop-many-globals
test-aaf	16-one-act-type
test-aaf	17-one-act-instance
test-aaf	18-one-act-error
test-aaf	19-one-act-error
test-aaf	20-state-res
test-aaf	21-generic-act
test-aaf	22-act-inst-using-state-res
test-aaf	23-act-inst-many-num-uses
test-aaf	24-many-acts-many-uses
test-aaf	25-act-using-complex-state
test-aaf	26-state-res-array-w-labeled-states
test-aaf	27-state-array-error
test-aaf	28-state-res-multiple-profiles
test-aaf	29-currentval-simple
test-aaf	30-currentval-in-usage
test-aaf	31-currentval-performance
test-aaf	32-string-index-optim
test-aaf	33-string-index-performance
test-aaf	34-one-epoch
test-aaf	35-one-timesystem
test-aaf	36-one-typedef
test-aaf	37-act-instance-w-o-legend
test-aaf	38-one-decomp
test-aaf	39-abs-res-error
test-aaf	40-one-abs-res
test-aaf	41-one-signal
test-aaf	42-regexp-signal
test-aaf	43-res-w-o-params
test-aaf	44-res-usage-calling-modeling
test-aaf	47-act-instance-id
test-aaf	48-custom-attributes
test-aaf	49-func-in-profile
test-aaf	50-this-in-res-attributes
test-aaf	51-decomp-unknown-child-error
test-aaf	52-alt-profile-label-format
test-aaf	53-res-value
test-aaf	54-curval-in-profile
test-aaf	55-profile-dep-pending-def-error
test-script	56-act-editor
test-aaf	57-args-passed-by-value
test-aaf	58-arg-by-value-in-abs-res
test-aaf	59-read-apf
test-aaf	60-int-index-res-array
test-aaf	61-eval-PI
test-aaf	62-arrayed-res-w-computed-list
test-aaf	63-func-returns-array
test-script	64-separate-apf -noremodel
test-script	65-apf-w-children
test-aaf	66-use-abs-res-w-args-by-value
test-aaf	67-act-type-w-class-vars
test-aaf	68-abs-res-w-conditional-called-by-value
test-script	69-act-that-changes-start
test-script	70-global-array-ref-another
test-script	71-write-aaf
test-script	72-state-w-func-in-profile
test-aaf	73-res-min-max
test-aaf	74-res-warning-limit
test-aaf	75-descr-attribute
test-script	76-regen-children
test-aaf	77-immed-use-of-arrayed-res
test-aaf	78-res-array-multi-attributes
test-aaf	79-res-usage-disagrees-w-res-def-error
test-aaf	80-pi
test-aaf	81-func-param-order
test-aaf	82-instance-ptr-mem-leak
test-aaf	83-function-decl
test-aaf	84-act-decl
test-aaf	85-conc-res-out-of-order
test-script	86-multiple-decl
test-aaf	87-assoc-res
test-script	88-res-decl-after-res-def
test-script	89-use-res-when-only-decl-error -noremodel
test-aaf	90-nonexistent-act-type-error
test-script	91-new-act-request-has-nonexistent-type-error
test-script	92-use-res-when-only-decl-error -noremodel
test-script	93-redetail-hidden-act
test-aaf	94-state-res-w-side-effects-error
test-aaf	95-abs-res-invokes-param-in-modeling
test-aaf	96-tight-numerical-loop
test-aaf	97-matrix-mult-loop
test-aaf	98-delete-array-element
test-aaf	99-simple-script
test-aaf	100-directives
test-aaf	101-duration-global
test-aaf	102-act-w-one-child
test-aaf	103-func-and-glob-with-wrong-default-type-in-decl
test-aaf	104-directive-setting-global
test-aaf	105-directive-setting-struct-element
test-aaf	106-func-w-o-return-error
test-aaf	107-directive-setting-a-string
test-aaf	108-act-type-w-attribute-that-depends-on-params
test-aaf	109-arithmetic
test-aaf	110-booleans-etc
test-aaf	111-function
test-aaf	112-act-type-w-sasf
test-aaf	113-directives
test-aaf	114-currentval-in-act-type-error
test-aaf	115-assign-float-to-int
test-aaf	116-directives-w-array-indices-in-lhs
test-aaf	117-func-w-comparison-and-eq-checks
test-aaf	118-bad-function-call-error
test-script	119-save-apf
test-aaf	120-ranges
test-script	121-big-decomp
test-aaf	122-unoptimized-comparison
test-aaf	123-complex-array
test-aaf	124-recursive-decomp
test-aaf	125-unacknowledged-child
test-aaf	126-double-assign-in-func
test-aaf	127-use-simple-noncons
test-aaf	128-use-res-5-args-error
test-aaf	129-abs-res-w-many-uses
test-aaf	130-abs-res-w-modeling
test-aaf	131-optimized-currentval
test-aaf	132-currentval-in-func
test-aaf	133-currentval-in-defaultval
test-aaf	134-currentval-of-abs-res-error
test-script	135-background-points
test-aaf	136-deep-resource-dependencies
test-aaf	137-decomp-every-from-to-error
test-aaf	138-simple-signal
test-aaf	139-cascading-abs-res-usages
test-aaf	140-wait-until-cond
test-aaf	141-wrong-sasf-type
test-aaf	142-func-returns-nothing-error
test-aaf	143-func-from-europa-error
test-aaf	144-arrayed-res-w-indices-in-args
test-aaf	145-apf-many-parents
test-aaf	146-long-consolidation-time
test-aaf	147-instance-exists
test-aaf	148-array-ref-in-array
test-aaf	149-string-addition
test-aaf	150-epsilon
test-aaf	151-fabs-replacement-in-if
test-script	152-get-windows
test-script	153-complex-get-windows
test-script	154-get-windows
test-script	155-simple-get-windows
test-aaf	156-func-call-in-directive
test-aaf	157-comparing-sym-to-array-val
test-aaf	158-xcmd
test-aaf	159-assign-array-to-array-el
test-aaf	160-usage-at-scheduling-start
test-aaf	161-floating-state-res-error
test-script	162-save-partial
test-script	163-ungroup-acts
test-script	164-default-child-start
test-script	165-read-xmltol -noremodel
test-script	166-res-array-in-tol -noremodel
test-script	167-read-xml-tol -noremodel
test-aaf	168-log10-error
test-aaf	169-wrong-brackets-in-res-array-error
test-script	170-call-spawn
test-script	171-creation-section
test-script	172-act-state-changes-error
test-script	173-act-state-changes
test-script	174-regen-children-script
test-script	175-intervals
test-script	176-int-intervals
test-script	177-simple-constraint
test-script	178-immediate-abs-error
test-script	179-max-dur-constraint
test-script	180-eclipses
test-aaf	181-action-programs-error
test-script	182-settable-resources-error-1
test-script	183-settable-resources-2
test-script	184-settable-resources-3
test-script	185-settable-resources-4
test-script	186-duration-settable-res
test-script	187-settable-duration-array
test-aaf	188-settable-time-array
test-script	189-sasf-attribute
test-script	190-sasf-many-times
test-script	191-decomp-in-func-error
test-script	192-bad-layout-error
test-aaf	193-mismatched-quotes-error
test-aaf	194-function-with-curval-logic-error
test-script	195-constraint-querying-array-error
test-script	196-editing-global-1
test-aaf	197-act-w-array-params
test-script	198-nightmare-tol
test-aaf	199-continue
test-script	200-tolcomp-1
test-script	201-zero-dur-act
test-script	202-tol-first-value
test-aaf	203-delete-list-el
test-script	204-history
test-aaf	205-method-1
test-aaf	206-parent-parent-foo
test-aaf	207-highly-qualified
test-script	208-dur-res-w-warnings
test-aaf	209-assign-vs-use-array-el-error
test-script	210-method-2
test-script	211-method-3
test-script	212-method-4
test-script	213-assign-time-to-dur-error
test-aaf	214-invoke-client-udef
test-script	215-expansion-func
test-script	216-assign-legend-while-modeling
test-script	217-test-att-vals-in-instance
test-script	218-get-windows-zero-duration
test-script	219-scheduling-a-cascade-of-activities
test-aaf	220-decomp-cascade-1
test-aaf	221-decomp-function
test-aaf	222-error-wrong-num-indices-arrayed-res
test-aaf	223-copy-array-into-parent
test-aaf	224-interp-res-2-tol
test-script	225-precomputed-res
test-aaf	226-computed-min
test-aaf	227-get-id-of
test-script	228-scheduling-tutorial
