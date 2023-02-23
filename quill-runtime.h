#include <functional>
#ifndef quill-runtime
#define quill-runtime
bool wake_up_cond(); 
void add_first(int id); 
void delete_first(int id); 
void* thread_func(void* data);
#endif