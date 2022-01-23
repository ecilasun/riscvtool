#include <inttypes.h>

struct STaskContext {
    void *m_registers;
    uint32_t m_flags;
};

typedef void(*taskfunc)(uint32_t _flags, void * _userdata);

void AddTask(taskfunc _task);
