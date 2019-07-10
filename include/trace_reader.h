#ifndef _TRACE_READER_H_
#define _TRACE_READER_H_

#include "global.h"
#include "subset.h"

using namespace std;

class TraceReader {
private:
	string trace_file;
	ifstream file_stream;
	string trace_line;
public:
	TraceReader(string tracefile);
	~TraceReader();
	bool HasNext();
	chunk Next();

};

#endif
