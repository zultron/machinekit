#include "callstacktracker.hh"
#include "string.h"
#include "stdint.h"

CallStackTracker::CallStackTracker()
{

}

void CallStackTracker::clear()
{
    stack.clear();
    stack.reserve(16);
}

void CallStackTracker::append_stack_info(const char * function_name, const char * file_name, int line)
{
    stack.emplace_back(
        function_name,
        file_name,
        line);
}

char *CallStackTracker::stack_name(size_t stack_index, char * function_name, size_t max_size)
{
    if (function_name && stack_index < stack.size()) {
        strncpy(function_name, stack[stack_index].call_name.c_str(), max_size);
    }
    return function_name;
}

const char * CallStackTracker::call_name(size_t stack_index)
{
    if (stack_index < stack.size()) {
        return stack[stack_index].call_name.c_str();
    }
    return nullptr;
}

CallStackFrame::CallStackFrame(const std::string & call_name, const std::string & src_file, int src_line) :
    call_name(call_name),
    src_file(src_file),
    src_line(src_line)
{
}
