//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "task.hpp"
#include <utils/common.hpp>
#include <sstream>
#include <gnb/rls/task.hpp>
#include <gnb/app/task.hpp>
#include <gnb/sctp/task.hpp>
#include <gnb/gtp/task.hpp>
#include <gnb/sctp/server.hpp>
#include <utils/cJSON.h>
#include <asn/ngap/ASN_NGAP_NGAP-PDU.h>
#include <gnb/ngap/encode.hpp>
#include <asn/ngap/ASN_NGAP_InitiatingMessage.h>
#include <asn/ngap/ASN_NGAP_ProtocolIE-ID.h>
#include <asn/ngap/ASN_NGAP_PathSwitchRequest.h>
#include <asn/ngap/ASN_NGAP_ProtocolIE-Field.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceToBeSwitchedDLItem.h>
#include <asn/ngap/ASN_NGAP_PathSwitchRequestTransfer.h>
#include <asn/ngap/ASN_NGAP_QosFlowSetupRequestItem.h>
#include <asn/ngap/ASN_NGAP_QosFlowSetupRequestList.h>
#include <asn/ngap/ASN_NGAP_QosFlowAcceptedItem.h>
#include <lib/asn/ngap.hpp>
#include <asn/ngap/ASN_NGAP_GTPTunnel.h>
namespace nr::gnb
{

NgapTask::NgapTask(TaskBase *base) : m_base{base}, m_ueNgapIdCounter{}, m_downlinkTeidCounter{}, m_isInitialized{}
{
    m_logger = base->logBase->makeUniqueLogger("ngap");
}

void NgapTask::onStart()
{
    for (auto &amfConfig : m_base->config->amfConfigs)
        createAmfContext(amfConfig);
    if (m_amfCtx.empty())
        m_logger->warn("No AMF configuration is provided");

    for (auto &amfCtx : m_amfCtx)
    {
        auto msg = std::make_unique<NmGnbSctp>(NmGnbSctp::CONNECTION_REQUEST);
        msg->clientId = amfCtx.second->ctxId;
        msg->localAddress = m_base->config->ngapIp;
        msg->localPort = 0;
        msg->remoteAddress = amfCtx.second->address;
        msg->remotePort = amfCtx.second->port;
        msg->ppid = sctp::PayloadProtocolId::NGAP;
        msg->associatedTask = this;
        m_base->sctpTask->push(std::move(msg));
    }
}

void NgapTask::onLoop()
{
    auto msg = take();
    if (!msg)
        return;

    switch (msg->msgType)
    {
    case NtsMessageType::GNB_RRC_TO_NGAP: {
        auto &w = dynamic_cast<NmGnbRrcToNgap &>(*msg);
        switch (w.present)
        {
        case NmGnbRrcToNgap::INITIAL_NAS_DELIVERY: {
            handleInitialNasTransport(w.ueId, w.pdu, w.rrcEstablishmentCause, w.sTmsi);
            break;
        }
        case NmGnbRrcToNgap::UPLINK_NAS_DELIVERY: {
            handleUplinkNasTransport(w.ueId, w.pdu);
            break;
        }
        case NmGnbRrcToNgap::RADIO_LINK_FAILURE: {
            handleRadioLinkFailure(w.ueId);
            break;
        }
        case NmGnbRrcToNgap::SEND_FAKE_PATHSWITCH:{
            m_logger->info("NmGnbRrcToNgap::SEND_FAKE_PATHSWITCH");
            createUeContext(w.ueId);
            auto *ueCtx = findUeContext(w.ueId);
            if (ueCtx == nullptr)
                return;
            auto *amfCtx = findAmfContext(ueCtx->associatedAmfId);
            if (amfCtx == nullptr)
                return;
            amfCtx->nextStream = (amfCtx->nextStream + 1) % amfCtx->association.outStreams;
            if ((amfCtx->nextStream == 0) && (amfCtx->association.outStreams > 1))
                amfCtx->nextStream += 1;
            ueCtx->uplinkStream = amfCtx->nextStream;
            auto w1 = std::make_unique<NmGnbNgapToGtp>(NmGnbNgapToGtp::UE_CONTEXT_UPDATE);
            ueCtx->ueAmbr.ulAmbr = 1ull<<63;
            ueCtx->ueAmbr.dlAmbr = 1ull<<63;
            w1->update = std::make_unique<GtpUeContextUpdate>(false, ueCtx->ctxId, ueCtx->ueAmbr);
            m_base->gtpTask->push(std::move(w1));
            m_logger->info("Prepare store pdu");
            auto* pdu = m_base->sctpServer->storePdu;
            m_logger->info("Store pdu prepared");
            ASN_NGAP_PathSwitchRequest_t  *msg = &pdu->choice.initiatingMessage->value.choice.PathSwitchRequest;
            auto *ieList = asn::ngap::GetProtocolIe(msg, ASN_NGAP_ProtocolIE_ID_id_PDUSessionResourceToBeSwitchedDLList);
            if (ieList)
            {
                auto &list = ieList->PDUSessionResourceToBeSwitchedDLList.list;
                for (int i = 0; i < list.count; i++)
                {
                    auto &item = list.array[i];
                    auto *resource = new PduSessionResource(ueCtx->ctxId, static_cast<int>(item->pDUSessionID));
                    resource->sessionType = PduSessionType::IPv4;
                    auto *transfer = ngap_encode::Decode<ASN_NGAP_PathSwitchRequestTransfer>(
                    asn_DEF_ASN_NGAP_PathSwitchRequestTransfer, item->pathSwitchRequestTransfer);
                    {
                        resource->upTunnel.teid =m_base->sctpServer->ul_teid;
                        resource->upTunnel.address =
                            asn::GetOctetString(transfer->dL_NGU_UP_TNLInformation.choice.gTPTunnel->transportLayerAddress);
                    }
                    for (int i=0;i<resource->upTunnel.address.length();i++){
                        *(resource->upTunnel.address.data()+i) =  m_base->sctpServer->ul_ip[i]; 
                    }
                    auto *ptr = asn::New<ASN_NGAP_QosFlowSetupRequestList>();
                    {
                        ASN_NGAP_QosFlowAcceptedItem * setupitem =  transfer->qosFlowAcceptedList.list.array[0];
                        ASN_NGAP_QosFlowSetupRequestItem * newitem = asn::New<ASN_NGAP_QosFlowSetupRequestItem>();
                        newitem->qosFlowIdentifier = setupitem->qosFlowIdentifier;
                        asn::SequenceAdd(*ptr,newitem);
                    }

                    resource->qosFlows = asn::WrapUnique(ptr, asn_DEF_ASN_NGAP_QosFlowSetupRequestList);

                    setupPduSessionResource(ueCtx, resource);
                }
            }
            pdu->choice.initiatingMessage->procedureCode = 18; //location report
            sendNgapUeAssociated(w.ueId, pdu);
            break;
        }
        case NmGnbRrcToNgap::SEND_PATHSWITCH:{
            createUeContext(w.ueId);
            auto *ueCtx = findUeContext(w.ueId);
            if (ueCtx == nullptr)
                return;
            auto *amfCtx = findAmfContext(ueCtx->associatedAmfId);
            if (amfCtx == nullptr)
                return;
            amfCtx->nextStream = (amfCtx->nextStream + 1) % amfCtx->association.outStreams;
            if ((amfCtx->nextStream == 0) && (amfCtx->association.outStreams > 1))
                amfCtx->nextStream += 1;
            ueCtx->uplinkStream = amfCtx->nextStream;
            auto w1 = std::make_unique<NmGnbNgapToGtp>(NmGnbNgapToGtp::UE_CONTEXT_UPDATE);
            ueCtx->ueAmbr.ulAmbr = 1ull<<63;
            ueCtx->ueAmbr.dlAmbr = 1ull<<63;
            w1->update = std::make_unique<GtpUeContextUpdate>(false, ueCtx->ctxId, ueCtx->ueAmbr);
            m_base->gtpTask->push(std::move(w1));
            auto* pdu = m_base->sctpServer->storePdu;
            ASN_NGAP_PathSwitchRequest_t  *msg = &pdu->choice.initiatingMessage->value.choice.PathSwitchRequest;
            auto *ieList = asn::ngap::GetProtocolIe(msg, ASN_NGAP_ProtocolIE_ID_id_PDUSessionResourceToBeSwitchedDLList);
            if (ieList)
            {
                auto &list = ieList->PDUSessionResourceToBeSwitchedDLList.list;
                for (int i = 0; i < list.count; i++)
                {
                    auto &item = list.array[i];
                    auto *resource = new PduSessionResource(ueCtx->ctxId, static_cast<int>(item->pDUSessionID));
                    resource->sessionType = PduSessionType::IPv4;
                    auto *transfer = ngap_encode::Decode<ASN_NGAP_PathSwitchRequestTransfer>(
                    asn_DEF_ASN_NGAP_PathSwitchRequestTransfer, item->pathSwitchRequestTransfer);
                    {
                        resource->upTunnel.teid =m_base->sctpServer->ul_teid;
                        resource->upTunnel.address =
                            asn::GetOctetString(transfer->dL_NGU_UP_TNLInformation.choice.gTPTunnel->transportLayerAddress);
                    }
                    for (int i=0;i<resource->upTunnel.address.length();i++){
                        *(resource->upTunnel.address.data()+i) =  m_base->sctpServer->ul_ip[i]; 
                    }

                    // *(resource->upTunnel.address.data()) = 192;
                    // *(resource->upTunnel.address.data()+1) = 168;
                    // *(resource->upTunnel.address.data()+2) = 247;
                    // *(resource->upTunnel.address.data()+3) = 154;
                    // address = resource->upTunnel.address.data() ;
                    // for (int i=0;i<resource->upTunnel.address.length();i++){
                    //     m_logger->info("address: %d",*(address+i));
                    // }
                    auto *ptr = asn::New<ASN_NGAP_QosFlowSetupRequestList>();
                    {
                        ASN_NGAP_QosFlowAcceptedItem * setupitem =  transfer->qosFlowAcceptedList.list.array[0];
                        ASN_NGAP_QosFlowSetupRequestItem * newitem = asn::New<ASN_NGAP_QosFlowSetupRequestItem>();
                        newitem->qosFlowIdentifier = setupitem->qosFlowIdentifier;
                        asn::SequenceAdd(*ptr,newitem);
                    }

                    resource->qosFlows = asn::WrapUnique(ptr, asn_DEF_ASN_NGAP_QosFlowSetupRequestList);

                    setupPduSessionResource(ueCtx, resource);
                }
            }
            
            // for (int psi : ue->pduSessions) {
            //     m_logger->debug("psi : %d,%lu",psi,(uint64_t)psi);
            //     auto *sessionItem = asn::New<ASN_NGAP_PDUSessionResourceToBeSwitchedDLItem_t>();
            //     m_logger->debug("we have session_p");
            //     uint64_t sessionInd = MakeSessionResInd(ueId, psi);
            //     std::unique_ptr<nr::gnb::PduSessionResource> & session_p = m_pduSessions[sessionInd]; //这里有问题再看下
            //     m_logger->debug("we have session_p");
            //     m_logger->debug("ueid: %d",session_p->ueId);

            //     sessionItem->pDUSessionID = static_cast<ASN_NGAP_PDUSessionID_t>(psi);
            //     sessionItem->pathSwitchRequestTransfer = OCTET_STRING{};
            //     transfer = &sessionItem->pathSwitchRequestTransfer;
            //     if (transfer == NULL){
            //         m_logger->debug("transfer is null");
            //     } else {
            //         m_logger->debug("transfer is not null");
            //     }
            //     ASN_NGAP_PathSwitchRequestTransfer_t message = ASN_NGAP_PathSwitchRequestTransfer_t{};
            //     ASN_NGAP_GTPTunnel * gTPTunnel = asn::New<ASN_NGAP_GTPTunnel>();
            //     message.dL_NGU_UP_TNLInformation.present = ASN_NGAP_UPTransportLayerInformation_PR_gTPTunnel;
            //     message.dL_NGU_UP_TNLInformation.choice.gTPTunnel = gTPTunnel;
            //     gTPTunnel->transportLayerAddress.size = 4;
            //     gTPTunnel->transportLayerAddress.buf = (uint8_t *)CALLOC(gTPTunnel->transportLayerAddress.size, sizeof(uint8_t));
            //     memcpy(gTPTunnel->transportLayerAddress.buf, &session_p->downTunnel.address, 4);
            //     m_logger->debug("memcpy transportLayerAddress:%s",OctetStringToIpString(session_p.downTunnel.address));

            //     gTPTunnel->gTP_TEID.size = sizeof(uint32_t);
            //     gTPTunnel->gTP_TEID.buf = (uint8_t *)CALLOC(1, sizeof(uint32_t));
            //     memcpy(gTPTunnel->gTP_TEID.buf, &session_p->downTunnel.teid, sizeof(uint32_t));
            //     m_logger->debug("memcpy gTP_TEID");

            //     // auto &qosList = session_p->qosFlows->list;
            //     auto &list = session_p->qosFlows->list;
            //     m_logger->debug("list length:%d",list.count);
            //     for (int iQos = 0; iQos < list.count; iQos++)
            //     {
            //         m_logger->debug("%d",iQos);
            //         ASN_NGAP_QosFlowAcceptedItem *qosFlowItem = asn::New<ASN_NGAP_QosFlowAcceptedItem>();
            //         auto & item = list.array[iQos];
            //         qosFlowItem->qosFlowIdentifier = item->qosFlowIdentifier;
            //         m_logger->debug("%d",iQos);
            //         asn::SequenceAdd(message.qosFlowAcceptedList, qosFlowItem);
            //     }
            //     m_logger->debug("finish list");

            //     OctetString transfer_s = nr::gnb::ngap_encode::EncodeS(asn_DEF_ASN_NGAP_PathSwitchRequestTransfer, &message);
            //     m_logger->debug("transefer_s success");

            //     int size_tmp = transfer_s.length();
            //     m_logger->debug("buffer size:%d",size_tmp);

            //     uint8_t * data = transfer_s.data();
            //     transfer->buf = (uint8_t*)CALLOC(size_tmp,sizeof(uint8_t));  
            //     memcpy(transfer->buf,data,size_tmp * sizeof(uint8_t));
            //     m_logger->debug("memcpy transfer");

            //     transfer->size = size_tmp;
            //     // message.qosFlowAcceptedList.list = session_p->qosFlows->list;
            //     // // ul->qfi = static_cast<int>(session_p->qosFlows->list)

                    
            //     // ogs_asn_ip_to_BIT_STRING(&ip, &gTPTunnel->transportLayerAddress);
            //     // ogs_asn_uint32_to_OCTET_STRING(sess->gnb_n3_teid, &gTPTunnel->gTP_TEID);
            //     asn::SequenceAdd(*PDUSessionResourceToBeSwitchedDLList, sessionItem);
            // }

            sendNgapUeAssociated(w.ueId, pdu);
            break;
        }
        case NmGnbRrcToNgap::HANDOVER_REQUEST: {
            m_logger->info("NGAP receive handover request");// auto  
            OCTET_STRING gnb_ip = {};
            gnb_ip.buf = (uint8_t *)CALLOC(4,1);
            gnb_ip.size = 4;
            for (int i=0;i<gnb_ip.size;i++){
                gnb_ip.buf[i] = m_base->sctpServer->target_ip[i];
                m_logger->info("target_ip : %d",m_base->sctpServer->target_ip[i]);
            }
            auto *pdu = sendPathSwitchRequestwithTargetIp(w.ueId,gnb_ip);
            ssize_t encoded;
            uint8_t *buffer;
            bool flag = ngap_encode::Encode(asn_DEF_ASN_NGAP_NGAP_PDU, pdu, encoded, buffer);
            m_logger->info("length of buffer is : %d",encoded);
            cJSON *json = cJSON_CreateObject();
            cJSON_AddNumberToObject(json, "ueId", w.ueId);
            cJSON_AddNumberToObject(json, "ack", 0);
            cJSON_AddNumberToObject(json, "length", encoded);
            cJSON_AddNumberToObject(json, "upf_teid", m_base->sctpServer->ul_teid);
            cJSON_AddNumberToObject(json, "upf_ip0", m_base->sctpServer->ul_ip[0]);
            cJSON_AddNumberToObject(json, "upf_ip1", m_base->sctpServer->ul_ip[1]);
            cJSON_AddNumberToObject(json, "upf_ip2", m_base->sctpServer->ul_ip[2]);
            cJSON_AddNumberToObject(json, "upf_ip3", m_base->sctpServer->ul_ip[3]);
            auto encodeStr = cJSON_PrintUnformatted(json);
            //char wholeEncode[1000] = {};
            // memcpy(wholeEncode,encodeStr,strlen(encodeStr));
            // memcpy(wholeEncode+strlen(encodeStr),buffer,encoded);
            sendXnapMessage((unsigned char*)encodeStr,strlen(encodeStr));
            sendXnapMessage(buffer,encoded);

            break;
        }
        case NmGnbRrcToNgap::BEFOREHAND_HANDOVER: {
            m_logger->info("NgapTask::handleBeforehandHandoverMessage");// auto  
            OCTET_STRING gnb_ip = {};
            gnb_ip.buf = (uint8_t *)CALLOC(4,1);
            gnb_ip.size = 4;
            for (int i=0;i<gnb_ip.size;i++){
                gnb_ip.buf[i] = m_base->sctpServer->target_ip[i];
                m_logger->info("target_ip : %d",m_base->sctpServer->target_ip[i]);
            }
            auto *pdu = sendPathSwitchRequestwithTargetIp(w.ueId,gnb_ip);
            ssize_t encoded;
            uint8_t *buffer;
            bool flag = ngap_encode::Encode(asn_DEF_ASN_NGAP_NGAP_PDU, pdu, encoded, buffer);
            m_logger->info("length of buffer is : %d",encoded);
            cJSON *json = cJSON_CreateObject();
            
            cJSON_AddNumberToObject(json, "ueId", w.ueId);
            cJSON_AddNumberToObject(json, "ack", 0);
            cJSON_AddNumberToObject(json, "length", encoded);
            cJSON_AddNumberToObject(json, "be", 1);
            auto encodeStr = cJSON_PrintUnformatted(json);
            
            sendXnapMessage((unsigned char*)encodeStr,strlen(encodeStr));
            sendXnapMessage(buffer,encoded);
            break;
        }
        }
        break;
    }
    case NtsMessageType::GNB_SCTP: {
        auto &w = dynamic_cast<NmGnbSctp &>(*msg);
        switch (w.present)
        {
        case NmGnbSctp::ASSOCIATION_SETUP:
            handleAssociationSetup(w.clientId, w.associationId, w.inStreams, w.outStreams);
            break;
        case NmGnbSctp::RECEIVE_MESSAGE:
            handleSctpMessage(w.clientId, w.stream, w.buffer);
            break;
        case NmGnbSctp::ASSOCIATION_SHUTDOWN:
            handleAssociationShutdown(w.clientId);
            break;
        case NmGnbSctp::BEFOREHAND_HANDOVER:
            handleBeforehandHandoverMessage(w.clientId);
            break;
        default:
            m_logger->info("right way");
            m_logger->unhandledNts(*msg);
            break;
        }
        break;
    }
    default: {
        m_logger->info("wrong way");
        m_logger->unhandledNts(*msg);
        break;
    }
    }
}

void NgapTask::onQuit()
{
    for (auto &i : m_ueCtx)
        delete i.second;
    for (auto &i : m_amfCtx)
        delete i.second;
    m_ueCtx.clear();
    m_amfCtx.clear();
}

} // namespace nr::gnb