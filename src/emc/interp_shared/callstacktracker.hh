#ifndef CALLSTACKTRACKER_HH
#define CALLSTACKTRACKER_HH

#include <string>
#include <vector>
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

#endif // CALLSTACKTRACKER_HH
