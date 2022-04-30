
#include "planning-time.H"

cppTime operator+(const cppTime& a, const cppDuration& b) {
	return cppTime(a.T + b.T); }

cppTime operator-(const cppTime& a, const cppTime& b) {
	return cppTime(a.T - b.T); }

cppTime operator-(const cppTime& a, const cppDuration& b) {
	return cppTime(a.T - b.T); }

cppDuration operator+(const cppDuration& a, const cppDuration& b) {
	return cppDuration(a.T + b.T); }

cppDuration operator-(const cppDuration& a, const cppDuration& b) {
	return cppDuration(a.T - b.T); }

cppDuration operator*(double b, const cppDuration& a) {
	return cppDuration(b * a.T); }

cppDuration operator*(const cppDuration& a, double b) {
	return cppDuration(b * a.T); }

cppDuration operator/(const cppDuration& a, double b) {
	return cppDuration(a.T / b); }

double operator/(const cppDuration& a, const cppDuration& b) {
	return a.T / b.T; }

bool operator<(const cppDuration& a, const cppDuration& b) {
	return a.T < b.T; }

bool operator<(const cppTime& a, const cppTime& b) {
	return a.T < b.T; }

bool operator==(const cppTime& a, const cppTime& b) {
	return a.T == b.T; }

cppTime::operator std::string() const {
	return std::string(*T.to_string()); }

cppDuration::operator std::string() const {
	return std::string(*T.to_string()); }

#ifdef OVERDOING_IT
cppTime::operator long int() const {
	return seconds * 1000 + milliseconds; }

cppDuration::operator long int() const {
	return seconds * 1000 + milliseconds; }
#endif /* OVERDOING_IT */
