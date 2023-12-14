
#include "server.hpp"
#include <gnb/rls/task.hpp>


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

    
    void mySctpHandler::onAssociationSetup(int associationId, int inStreams, int outStreams){
        printf("printf onAssociationSetup:instream:%d,outstream:%d\n",inStreams,outStreams);
        auto w = std::make_unique<NmGnbSctp>(NmGnbSctp::ASSOCIATION_SETUP);
        // w->clientId = clientId;
        // w->associationId = associationId;
        // w->inStreams = inStreams;
        // w->outStreams = outStreams;

        // sctpTask->push(std::move(w));
    }

    void mySctpHandler::onAssociationShutdown(){
        printf("printf onAssociationShutdown:");
        // auto w = std::make_unique<NmGnbSctp>(NmGnbSctp::ASSOCIATION_SHUTDOWN);
        // w->clientId = clientId;
        // sctpTask->push(std::move(w));
    }

    void mySctpHandler::onMessage(const uint8_t *buffer, size_t length, uint16_t stream){
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
                cJSON* uesti1 = cJSON_GetObjectItem(json, "uesti1");
                cJSON* uesti2 = cJSON_GetObjectItem(json, "uesti2");
                cJSON* uesti3 = cJSON_GetObjectItem(json, "uesti3");
                cJSON* uesti4 = cJSON_GetObjectItem(json, "uesti4");
                uint64_t uesti;
                uint16_t uesti1c,uesti2c,uesti3c,uesti4c;
                if (uesti1!=NULL && uesti2 !=NULL) {
                    
                    uesti1c = uesti1->valueint;
                    uesti2c = uesti2->valueint;
                    uesti3c = uesti3->valueint;
                    uesti4c = uesti4->valueint;
                    printf("uesti_c:%u,%u,%u,%u",uesti1c,uesti2c,uesti3c,uesti4c);
                    uesti = ((unsigned long long)uesti1c << 48) + ((unsigned long long)uesti2c <<32) + ((unsigned long long)uesti3c <<16) +(unsigned long long)uesti4c;
                    printf("uesti: %llu",uesti);
                } else {
                    printf("canot read uesti!!!\n");
                    return ;
                }  
                auto w1 = std::make_unique<NmGnbRrcToNgap>(NmGnbRrcToNgap::SEND_FAKE_PATHSWITCH);
                w1->ueId = server->m_base->rlsTask->getudp()->findRlsPdu(uesti);
                server->m_base->ngapTask->push(std::move(w1));
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
                uint64_t uesti = m_base->rlsTask->getudp()->findUeSti(w.ueId);
                server->m_base->rrcTask->exchangeRRCConnectionWithSti(ue->ctxId,uesti);
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

    void mySctpHandler::onUnhandledNotification(){
        printf("printf UnhandledNotification:");
    }

    void mySctpHandler::onConnectionReset()
    {
        printf("printf onConnectionReset:");
    }

}