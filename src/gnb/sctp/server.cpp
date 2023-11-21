
#include "server.hpp"


namespace nr::gnb
{

    GnbSctpServer::GnbSctpServer(TaskBase *base, const std::string &address, uint16_t port){
        m_base = base;
        sctp::ISctpHandler*  handler = new mySctpHandler(this,10);
        sctpserver = new sctp::SctpServer(address,port,handler);
    }

    void GnbSctpServer::start(){
        this->sctpserver->start();
    }
}