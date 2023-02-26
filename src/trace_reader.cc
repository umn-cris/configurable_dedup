/*
 ============================================================================
 Name        : trace_reader.cc
 Author      : Zhichao Cao
 Date        : 03/07/2018
 Copyright   : Your copyright notice
 Description : the dedup trace reader and processing
 ============================================================================
 */

#include <iostream>
#include <cassert>

#include "trace_reader.h"
using namespace std;


TraceReader::TraceReader(string tracefile) {
	trace_file = tracefile;
	file_stream.open(trace_file);
	if(file_stream.fail())
	{
		cout<<"open: "<<trace_file<<" failed!\n";
		exit(1);
	}
}


TraceReader::~TraceReader() {
	file_stream.close();
}
bool TraceReader::HasNext() {

    if (!getline(file_stream, trace_line))
        return false;
    else
        return true;
}

chunk TraceReader::Next() {
    vector<string> result;
    string tmp;
    stringstream ss(trace_line);
    while(ss>>tmp)
        result.push_back(tmp);

    chunk ck;
    ck.SetID(result[0]);

    return ck;
}


