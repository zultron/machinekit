#ifndef CALLSTACKTRACKER_HH
#define CALLSTACKTRACKER_HH

#include <string>
#include <vector>
#include "string.h"
#include "stdint.h"

namespace interp_shared {
using std::vector;
using std::string;

struct CallStackFrame
{
    CallStackFrame() = default;
    CallStackFrame(string const &call_name, string const &src_file, int src_line);
    string call_name; //!< stack of calls for error reporting
    string src_file; //!< file where error occurred
    int src_line; //!< line where error occurred
};

class CallStackTracker
{
    vector<CallStackFrame> stack;
public:
    CallStackTracker();
    void clear();
    void append_stack_info(const char * function_name, const char *file_name, int line);
    vector<CallStackFrame> const &get_stack() const { return stack; }
    size_t frame_count() const { return stack.size(); }
    char *stack_name(size_t stack_index, char *function_name, size_t max_size);
    const char *call_name(size_t stack_index);
};

inline CallStackTracker::CallStackTracker()
{

}

inline void CallStackTracker::clear()
{
    stack.clear();
    stack.reserve(16);
}

inline void CallStackTracker::append_stack_info(const char * function_name, const char * file_name, int line)
{
    stack.emplace_back(
        function_name,
        file_name,
        line);
}

inline char *CallStackTracker::stack_name(size_t stack_index, char * function_name, size_t max_size)
{
    if (function_name && stack_index < stack.size()) {
        strncpy(function_name, stack[stack_index].call_name.c_str(), max_size);
    }
    return function_name;
}

inline const char * CallStackTracker::call_name(size_t stack_index)
{
    if (stack_index < stack.size()) {
        return stack[stack_index].call_name.c_str();
    }
    return nullptr;
}

inline CallStackFrame::CallStackFrame(const std::string & call_name, const std::string & src_file, int src_line) :
    call_name(call_name),
    src_file(src_file),
    src_line(src_line)
{
}

} //namespace

#endif // CALLSTACKTRACKER_HH
