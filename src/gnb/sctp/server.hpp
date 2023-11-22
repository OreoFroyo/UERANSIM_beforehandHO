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
        printf("length is : %d\n",length);
        int beforehand;
        int ueId;
        auto *data1 = new uint8_t[length];
        std::memcpy(data1, buffer, length);
        const char* data = (const char*)(char*)data1;
        printf("data:%s\n",data);
        cJSON* json = cJSON_Parse(data);
        const char *error_ptr = cJSON_GetErrorPtr();
        if(error_ptr != NULL){
            printf("Error before: %s\n", error_ptr);
        }
        if (json){

            cJSON* item = cJSON_GetObjectItem(json,"ack");
            int ack = item->valueint;

            cJSON* item2 = cJSON_GetObjectItem(json,"be");
            if (item2!=NULL) {
                beforehand = item2->valueint;
            }
            else
                beforehand = 0;
            if(ack == 0 && beforehand == 0){
                cJSON* ueIdC = cJSON_GetObjectItem(json, "ueId");
                ueId = ueIdC->valueint;
                printf("UeId:%d\n",ueId);
                // cJSON* buffer1 = cJSON_GetObjectItem(json, "buffer");
                // unsigned char* bufferStr = (unsigned char*)buffer1->valuestring;
                // printf("buffer is : %s\n",bufferStr);
                cJSON* length1 = cJSON_GetObjectItem(json, "length");
                int lengthStr = length1->valueint;
                printf("length is : %d\n",lengthStr);
                cJSON* teid = cJSON_GetObjectItem(json, "upf_teid");
                server->ul_teid = teid->valueint;
                char * up_adress[4] = {};
                cJSON* ip0 = cJSON_GetObjectItem(json, "upf_ip0");
                cJSON* ip1 = cJSON_GetObjectItem(json, "upf_ip1");
                cJSON* ip2 = cJSON_GetObjectItem(json, "upf_ip2");
                cJSON* ip3 = cJSON_GetObjectItem(json, "upf_ip3");
                server->ul_ip[0] = ip0->valueint;
                server->ul_ip[1] = ip1->valueint;
                server->ul_ip[2] = ip2->valueint;
                server->ul_ip[3] = ip3->valueint;
                
                cJSON *json2 = cJSON_CreateObject();
                cJSON_AddNumberToObject(json2, "ack", 1);
                cJSON_AddNumberToObject(json2, "ueId", ueId);
                unsigned char* encodeStr = (unsigned char* )cJSON_PrintUnformatted(json2);
                auto msg = std::make_unique<NmGnbSctp>(NmGnbSctp::SEND_MESSAGE);
                msg->clientId = 10;
                msg->stream = 1;
                msg->buffer = UniqueBuffer{encodeStr, strlen((char *)encodeStr)};
                printf("send buffer: %s\n",(char *)msg->buffer.data());
                server->m_base->sctpTask->push(std::move(msg));
            }
            else if(beforehand == 1){
                printf("beforehand == 1");
                // cJSON* ueIdc = cJSON_GetObjectItem(json, "ueId");
                // if (ueIdc!=NULL) {
                //     ueId = ueIdc->valueint;
                // } else {
                //     ueId = 0;
                // }  
                // auto ue1 = server->m_base->ngapTask->getUectx(ueId);
                // auto w1 = std::make_unique<NmGnbRrcToNgap>(NmGnbRrcToNgap::SEND_FAKE_PATHSWITCH);
                // w1->ueId = ue1->ctxId;
                // server->m_base->ngapTask->push(std::move(w1));
            }
            else if(ack == 1){
                cJSON* ueIdC = cJSON_GetObjectItem(json, "ueId");
                ueId = ueIdC->valueint;
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