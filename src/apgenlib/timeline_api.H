#ifndef _TIMELINE_API_H_
#define _TIMELINE_API_H_

#include <XmlRpcValue.H>

/* assumes a bare bones timeline made of "StringEpisodes"
 * containing very little info. Must be used in conjunction
 * with addOpaqueDataToActValueAPI below. */
bool xml2plan(	const std::string &xmlContents,
		XmlRpc::XmlRpcValue &thePlan,
		XmlRpc::XmlRpcValue &theIds);
/* assumes a timeline of items containing Opaque data in the form
 * of a serialized XmlRpc structure describing the activity in sv
 * format.
 *
 * Does not require the use of addOpaqueDataToActValueAPI. */
bool xml2plan_v2(	const std::string &xmlContents,
		XmlRpc::XmlRpcValue &thePlan,
		XmlRpc::XmlRpcValue &theIds);
bool xml2states(const char *xmlContents, XmlRpc::XmlRpcValue &thePlan);
bool xml2predicts(const char *xmlContents, XmlRpc::XmlRpcValue &thePlan);
bool addOpaqueDataToActValueAPI(
		XmlRpc::XmlRpcValue &opaqueData,
		const std::string &xmlContent);
void dumpTelemetry(XmlRpc::XmlRpcValue &telem, const string &channel);

#endif /* _TIMELINE_API_H_ */
