#pragma once
#include <gnb/types.hpp>
#include <lib/sctp/server.hpp>
#include <gnb/nts.hpp>
#include <utils/cJSON.h>
#include <gnb/ngap/task.hpp>
#include <gnb/ngap/encode.hpp>
#include <asn/ngap/ASN_NGAP_NGAP-PDU.h>
#include <gnb/rrc/task.hpp>
#include <gnb/sctp/task.hpp>
// #include "task.hpp"

namespace nr::gnb
{

class GnbSctpServer 
{
    public:
        TaskBase *m_base;
        ASN_NGAP_NGAP_PDU *storePdu;
        char target_ip[4];
        uint32_t ul_teid;
        char ul_ip[4];
    private:
        sctp::SctpServer * sctpserver;
        
    public:
        GnbSctpServer(TaskBase *base, const std::string &address, uint16_t port);
        ~GnbSctpServer();
        void start();  
};


class mySctpHandler : public sctp::ISctpHandler
{
    private:
        int clientId;
        GnbSctpServer *const server;
    
    public:
        extern int B_HO = 0;
        
    public:
        mySctpHandler(GnbSctpServer *const server, int clientId) : server(server), clientId(clientId)
    {
    }
        
    public:

        void onAssociationSetup(int associationId, int inStreams, int outStreams);
        void onAssociationShutdown();
        void onMessage(const uint8_t *buffer, size_t length, uint16_t stream);
        void onUnhandledNotification();
        void onConnectionReset();

    
};
}