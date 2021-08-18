#ifndef _CO_SERVER_H_
#define _CO_SERVER_H_

#include "core/co_cycle.h"
#include "core/co_request.h"
#include "base/co_configparser.h"


namespace coserver
{

class CoServer
{
public:
    CoServer();
    virtual ~CoServer();

    void    add_user_handlers(const std::string &handlerName, CoFuncUserProcess userProcess, CoFuncUserDestroy userDestroy, void* userData = NULL);
   
    int32_t run_server(const std::string &configFilename, int32_t useCurThreadServer = 1);
    int32_t shut_down();

    void run_server_thread(int32_t index);


private:
    CoConfigParser* m_configParser = NULL;

    std::vector<std::thread> m_workerThreads;
    std::vector<CoDispatcher*> m_workerDispatchers;
};

}

#endif //_CO_SERVER_H_

