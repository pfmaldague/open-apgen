# README - Activity Creation

## Introduction

This README answers the following questions:

1. where in the code does activity creation occur?

2. what happens when an activity gets created?

## Where in the code does activity creation occur?

Activity creation occurs in the following places in the code:

- In apcore/activities/ACT-helpers.C, method PR-manager::put-a-copy-of-me-into-clipboard(). This method is used when cutting and pasting activities; the code that does that is in ACT-exec.C.

- In apcore/activities/DB.C, method apgenDB::CreateActivity(). This method is invoked by altLanguageReader.C when reading XMLTOL's, and also by apgenDB::CreateActivityInPlan() which is invoked in action-request.C when processing action requests that result in new activities.

- In apcore/modeler/Decomp.C, method Decomp::execute-one-decomp(). This method is invoked when executing the decomposition or expansion section of an activity type definition.

- In apcore/parser-support/ActInstance-consolidate.C, method ActInstance::consolidate(). This method is used when an activity instance definition is read from an APF.


## What happens when an activity gets created?

This README is being written as APGen is being refactored into APGen X. Many
things have been streamlined and clarified in APGen X, but one thing that remains
to be done is to simplify the handling of state changes in activity instances.

The clearest example of what needs to be done during activity creation is in
apgenDB::CreateActivityInPlan(). That method proceeds in four steps:

1. CreateActivity() is invoked. This call does the following:

   * it creates parameters, either from supplied values or by executing parameter declarations

   * it creates an ActivityInstance

   * it executes the activity type constructor, which takes care of any class variables and any attributes

2. instance_state_changer is invoked to instantiate the activity

3. the creation section of the activity type, if any, is executed

4. if modeling is ongoing, the modeling section, if any, is executed


