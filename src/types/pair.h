#pragma once

// Basic Pair class template
template <typename T1, typename T2>
class Pair
{
public:
	// Data members to hold the pair values
	T1 first;
	T2 second;

	// Default constructor
	Pair() : first(), second() {}

	// Parameterized constructor
	Pair(const T1 &first, const T2 &second) : first(first), second(second) {}

	// Getter for first element
	T1 getFirst() const { return first; }

	// Getter for second element
	T2 getSecond() const { return second; }

	// Setter for first element
	void setFirst(const T1 &value) { first = value; }

	// Setter for second element
	void setSecond(const T2 &value) { second = value; }

};