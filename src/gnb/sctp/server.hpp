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
        mySctpHandler(GnbSctpServer *const server, int clientId) : server(server), clientId(clientId)
    {
    }
        
    private:
        void onAssociationSetup(int associationId, int inStreams, int outStreams) override
    {
        printf("printf onAssociationSetup:instream:%d,outstream:%d\n",inStreams,outStreams);
        auto w = std::make_unique<NmGnbSctp>(NmGnbSctp::ASSOCIATION_SETUP);
        // w->clientId = clientId;
        // w->associationId = associationId;
        // w->inStreams = inStreams;
        // w->outStreams = outStreams;

        // sctpTask->push(std::move(w));
    }

    void onAssociationShutdown() override
    {
        printf("printf onAssociationShutdown:");
        // auto w = std::make_unique<NmGnbSctp>(NmGnbSctp::ASSOCIATION_SHUTDOWN);
        // w->clientId = clientId;
        // sctpTask->push(std::move(w));
    }

    void onMessage(const uint8_t *buffer, size_t length, uint16_t stream) override
    {
        // auto *data1 = new uint8_t[length];
        // std::memcpy(data1, buffer, length);
        // const char* data = (const char*)(char*)data1;
        // printf("data:%s\n",data);
        // cJSON* json = cJSON_Parse(data);
        // cJSON* item = cJSON_GetObjectItem(json,"ueSti");
        // const char* ueStiStr = item->valuestring;
        // printf("ueStiStr:%s",ueStiStr);
        // uint64_t ueSti = strtoull(ueStiStr, NULL, 0);
        // printf("printf onMessage:");
        // printf("ueSti:%ld\n",ueSti);
        // this->server->m_base->ngapTask->UeHandover(ueSti);
        

        auto *data1 = new uint8_t[length];
        std::memcpy(data1, buffer, length);
        const char* data = (const char*)(char*)data1;
        printf("data:%s\n",data);
        cJSON* json = cJSON_Parse(data);
        if (json){
            cJSON* item = cJSON_GetObjectItem(json,"ack");
            int ack = item->valueint;
            if(ack == 0){
                cJSON* ueIdC = cJSON_GetObjectItem(json, "ueId");
                int ueId = ueIdC->valueint;
                printf("UeId:%d\n",ueId);
                // cJSON* buffer1 = cJSON_GetObjectItem(json, "buffer");
                // unsigned char* bufferStr = (unsigned char*)buffer1->valuestring;
                // printf("buffer is : %s\n",bufferStr);
                cJSON* length1 = cJSON_GetObjectItem(json, "length");
                int lengthStr = length1->valueint;
                printf("length is : %d\n",lengthStr);
                
                cJSON *json = cJSON_CreateObject();
                cJSON_AddNumberToObject(json, "ack", 1);
                cJSON_AddNumberToObject(json, "ueId", ueId);
                unsigned char* encodeStr = (unsigned char* )cJSON_PrintUnformatted(json);
                auto msg = std::make_unique<NmGnbSctp>(NmGnbSctp::SEND_MESSAGE);
                msg->clientId = 10;
                msg->stream = 1;
                msg->buffer = UniqueBuffer{encodeStr, strlen((char *)encodeStr)};
                printf("send buffer: %s\n",(char *)msg->buffer.data());
                server->m_base->sctpTask->push(std::move(msg));
            }
            else if(ack == 1){
                cJSON* ueIdC = cJSON_GetObjectItem(json, "ueId");
                int ueId = ueIdC->valueint;
                auto ue = server->m_base->ngapTask->getUectx(ueId);
                auto w = std::make_unique<NmGnbNgapToRrc>(NmGnbNgapToRrc::EXCHANGE_RRC);
                w->ueId = ue->ctxId;
                server->m_base->rrcTask->push(std::move(w));
            }
        } else {
            printf("hh I'm in pdu store mode.\n");
            auto *pdu = ngap_encode::Decode<ASN_NGAP_NGAP_PDU>(asn_DEF_ASN_NGAP_NGAP_PDU, data1, length);
            server->storePdu = pdu;
            if(!pdu){
                printf("decode failed\n");
            }
            else{
                printf("decode success\n");
            }
        }
        
        
        

    }

    void onUnhandledNotification() override
    {
        printf("printf UnhandledNotification:");
    }

    void onConnectionReset() override
    {
        printf("printf onConnectionReset:");
    }


    // void onMessage(const uint8_t *buffer, size_t length, uint16_t stream) override
    // {
    //     auto *data = new uint8_t[length];
    //     std::memcpy(data, buffer, length);

    //     auto w = std::make_unique<NmGnbSctp>(NmGnbSctp::RECEIVE_MESSAGE);
    //     w->clientId = clientId;
    //     w->buffer = UniqueBuffer{data, length};
    //     w->stream = stream;
    //     sctpTask->push(std::move(w));
    // }

    // void onUnhandledNotification() override
    // {
    //     auto w = std::make_unique<NmGnbSctp>(NmGnbSctp::UNHANDLED_NOTIFICATION);
    //     w->clientId = clientId;
    //     sctpTask->push(std::move(w));
    // }

    // void onConnectionReset() override
    // {
    //     auto w = std::make_unique<NmGnbSctp>(NmGnbSctp::UNHANDLED_NOTIFICATION);
    //     w->clientId = clientId;
    //     sctpTask->push(std::move(w));
    // }
    
};
}