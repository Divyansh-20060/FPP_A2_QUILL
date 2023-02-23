#include <functional>
#ifndef quill
#define quill
void init_runtime(); 
void async(std::function<void()> &&lambda); 
void start_finish(); 
void end_finish();
void finalize_runtime();
#endif

/*

4. Master thread 
5. Arrange the files and rename them
6. Document
*/