//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "encode.hpp"
#include "task.hpp"
#include "utils.hpp"

#include <gnb/gtp/task.hpp>
#include <gnb/rrc/task.hpp>

#include <asn/ngap/ASN_NGAP_AMF-UE-NGAP-ID.h>
#include <asn/ngap/ASN_NGAP_AssociatedQosFlowItem.h>
#include <asn/ngap/ASN_NGAP_AssociatedQosFlowList.h>
#include <asn/ngap/ASN_NGAP_GTPTunnel.h>
#include <asn/ngap/ASN_NGAP_InitialContextSetupRequest.h>
#include <asn/ngap/ASN_NGAP_InitialContextSetupResponse.h>
#include <asn/ngap/ASN_NGAP_NGAP-PDU.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceFailedToSetupItemCxtRes.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceFailedToSetupItemSURes.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceItemCxtRelReq.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceReleaseCommand.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceReleaseResponse.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceReleaseResponseTransfer.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceReleasedItemRelRes.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceSetupItemCxtReq.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceSetupItemCxtRes.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceSetupItemSUReq.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceSetupItemSURes.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceSetupListCxtReq.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceSetupRequest.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceSetupRequestTransfer.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceSetupResponse.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceSetupResponseTransfer.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceSetupUnsuccessfulTransfer.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceToReleaseItemRelCmd.h>
#include <asn/ngap/ASN_NGAP_ProtocolIE-Field.h>
#include <asn/ngap/ASN_NGAP_QosFlowPerTNLInformationItem.h>
#include <asn/ngap/ASN_NGAP_QosFlowPerTNLInformationList.h>
#include <asn/ngap/ASN_NGAP_QosFlowSetupRequestItem.h>
#include <asn/ngap/ASN_NGAP_QosFlowSetupRequestList.h>
#include <asn/ngap/ASN_NGAP_SuccessfulOutcome.h>
#include <asn/ngap/ASN_NGAP_UE-NGAP-ID-pair.h>
#include <asn/ngap/ASN_NGAP_UE-NGAP-IDs.h>
#include <asn/ngap/ASN_NGAP_UEAggregateMaximumBitRate.h>
#include <asn/ngap/ASN_NGAP_UEContextModificationRequest.h>
#include <asn/ngap/ASN_NGAP_UEContextModificationResponse.h>
#include <asn/ngap/ASN_NGAP_UEContextReleaseCommand.h>
#include <asn/ngap/ASN_NGAP_UEContextReleaseComplete.h>
#include <asn/ngap/ASN_NGAP_UEContextReleaseRequest.h>
#include <gnb/rls/task.hpp>
#include <asn/ngap/ASN_NGAP_PathSwitchRequestTransfer.h>
#include <asn/ngap/ASN_NGAP_QosFlowAcceptedItem.h>
#include <asn/ngap/ASN_NGAP_PDUSessionResourceToBeSwitchedDLItem.h>
#include <utils/network.cpp>
#include <gnb/sctp/server.hpp>
namespace nr::gnb
{

void NgapTask::receiveInitialContextSetup(int amfId, ASN_NGAP_InitialContextSetupRequest *msg)
{
    m_logger->debug("Initial Context Setup Request received");

    auto *ue = findUeByNgapIdPair(amfId, ngap_utils::FindNgapIdPair(msg));
    if (ue == nullptr)
        return;

    auto w = std::make_unique<NmGnbNgapToGtp>(NmGnbNgapToGtp::UE_CONTEXT_UPDATE);
    w->update = std::make_unique<GtpUeContextUpdate>(true, ue->ctxId, ue->ueAmbr);
    m_base->gtpTask->push(std::move(w));

    auto *reqIe = asn::ngap::GetProtocolIe(msg, ASN_NGAP_ProtocolIE_ID_id_UEAggregateMaximumBitRate);
    if (reqIe)
    {
        ue->ueAmbr.dlAmbr = asn::GetUnsigned64(reqIe->UEAggregateMaximumBitRate.uEAggregateMaximumBitRateDL) / 8ull;
        ue->ueAmbr.ulAmbr = asn::GetUnsigned64(reqIe->UEAggregateMaximumBitRate.uEAggregateMaximumBitRateUL) / 8ull;
    }

    std::vector<ASN_NGAP_PDUSessionResourceSetupItemCxtRes *> successList;
    std::vector<ASN_NGAP_PDUSessionResourceFailedToSetupItemCxtRes *> failedList;

    reqIe = asn::ngap::GetProtocolIe(msg, ASN_NGAP_ProtocolIE_ID_id_PDUSessionResourceSetupListCxtReq);
    if (reqIe)
    {
        auto &list = reqIe->PDUSessionResourceSetupListCxtReq.list;
        for (int i = 0; i < list.count; i++)
        {
            auto &item = list.array[i];
            auto *transfer = ngap_encode::Decode<ASN_NGAP_PDUSessionResourceSetupRequestTransfer>(
                asn_DEF_ASN_NGAP_PDUSessionResourceSetupRequestTransfer, item->pDUSessionResourceSetupRequestTransfer);
            if (transfer == nullptr)
            {
                m_logger->err(
                    "Unable to decode a PDU session resource setup request transfer. Ignoring the relevant item");
                asn::Free(asn_DEF_ASN_NGAP_PDUSessionResourceSetupRequestTransfer, transfer);
                continue;
            }

            auto *resource = new PduSessionResource(ue->ctxId, static_cast<int>(item->pDUSessionID));

            auto *ie = asn::ngap::GetProtocolIe(transfer, ASN_NGAP_ProtocolIE_ID_id_PDUSessionAggregateMaximumBitRate);
            if (ie)
            {
                resource->sessionAmbr.dlAmbr =
                    asn::GetUnsigned64(ie->PDUSessionAggregateMaximumBitRate.pDUSessionAggregateMaximumBitRateDL) /
                    8ull;
                resource->sessionAmbr.ulAmbr =
                    asn::GetUnsigned64(ie->PDUSessionAggregateMaximumBitRate.pDUSessionAggregateMaximumBitRateUL) /
                    8ull;
            }

            ie = asn::ngap::GetProtocolIe(transfer, ASN_NGAP_ProtocolIE_ID_id_DataForwardingNotPossible);
            if (ie)
                resource->dataForwardingNotPossible = true;

            ie = asn::ngap::GetProtocolIe(transfer, ASN_NGAP_ProtocolIE_ID_id_PDUSessionType);
            if (ie)
                resource->sessionType = ngap_utils::PduSessionTypeFromAsn(ie->PDUSessionType);

            ie = asn::ngap::GetProtocolIe(transfer, ASN_NGAP_ProtocolIE_ID_id_UL_NGU_UP_TNLInformation);
            if (ie)
            {
                resource->upTunnel.teid =
                    (uint32_t)asn::GetOctet4(ie->UPTransportLayerInformation.choice.gTPTunnel->gTP_TEID);

                resource->upTunnel.address =
                    asn::GetOctetString(ie->UPTransportLayerInformation.choice.gTPTunnel->transportLayerAddress);
            }

            ie = asn::ngap::GetProtocolIe(transfer, ASN_NGAP_ProtocolIE_ID_id_QosFlowSetupRequestList);
            if (ie)
            {
                auto *ptr = asn::New<ASN_NGAP_QosFlowSetupRequestList>();
                asn::DeepCopy(asn_DEF_ASN_NGAP_QosFlowSetupRequestList, ie->QosFlowSetupRequestList, ptr);

                resource->qosFlows = asn::WrapUnique(ptr, asn_DEF_ASN_NGAP_QosFlowSetupRequestList);
            }

            auto error = setupPduSessionResource(ue, resource);
            if (error.has_value())
            {
                auto *tr = asn::New<ASN_NGAP_PDUSessionResourceSetupUnsuccessfulTransfer>();
                ngap_utils::ToCauseAsn_Ref(error.value(), tr->cause);

                OctetString encodedTr =
                    ngap_encode::EncodeS(asn_DEF_ASN_NGAP_PDUSessionResourceSetupUnsuccessfulTransfer, tr);

                if (encodedTr.length() == 0)
                    throw std::runtime_error("PDUSessionResourceSetupUnsuccessfulTransfer encoding failed");

                asn::Free(asn_DEF_ASN_NGAP_PDUSessionResourceSetupUnsuccessfulTransfer, tr);

                auto *res = asn::New<ASN_NGAP_PDUSessionResourceFailedToSetupItemCxtRes>();
                res->pDUSessionID = resource->psi;
                asn::SetOctetString(res->pDUSessionResourceSetupUnsuccessfulTransfer, encodedTr);

                failedList.push_back(res);
            }
            else
            {
                auto *tr = asn::New<ASN_NGAP_PDUSessionResourceSetupResponseTransfer>();

                auto &qosList = resource->qosFlows->list;
                for (int iQos = 0; iQos < qosList.count; iQos++)
                {
                    auto *associatedQosFlowItem = asn::New<ASN_NGAP_AssociatedQosFlowItem>();
                    associatedQosFlowItem->qosFlowIdentifier = qosList.array[iQos]->qosFlowIdentifier;
                    asn::SequenceAdd(tr->dLQosFlowPerTNLInformation.associatedQosFlowList, associatedQosFlowItem);
                }

                auto &upInfo = tr->dLQosFlowPerTNLInformation.uPTransportLayerInformation;
                upInfo.present = ASN_NGAP_UPTransportLayerInformation_PR_gTPTunnel;
                upInfo.choice.gTPTunnel = asn::New<ASN_NGAP_GTPTunnel>();
                asn::SetBitString(upInfo.choice.gTPTunnel->transportLayerAddress, resource->downTunnel.address);
                asn::SetOctetString4(upInfo.choice.gTPTunnel->gTP_TEID, (octet4)resource->downTunnel.teid);

                OctetString encodedTr =
                    ngap_encode::EncodeS(asn_DEF_ASN_NGAP_PDUSessionResourceSetupResponseTransfer, tr);

                if (encodedTr.length() == 0)
                    throw std::runtime_error("PDUSessionResourceSetupResponseTransfer encoding failed");

                asn::Free(asn_DEF_ASN_NGAP_PDUSessionResourceSetupResponseTransfer, tr);

                auto *res = asn::New<ASN_NGAP_PDUSessionResourceSetupItemCxtRes>();
                res->pDUSessionID = resource->psi;
                asn::SetOctetString(res->pDUSessionResourceSetupResponseTransfer, encodedTr);

                successList.push_back(res);
            }

            asn::Free(asn_DEF_ASN_NGAP_PDUSessionResourceSetupRequestTransfer, transfer);
        }
    }

    reqIe = asn::ngap::GetProtocolIe(msg, ASN_NGAP_ProtocolIE_ID_id_NAS_PDU);
    if (reqIe)
        deliverDownlinkNas(ue->ctxId, asn::GetOctetString(reqIe->NAS_PDU));

    std::vector<ASN_NGAP_InitialContextSetupResponseIEs *> responseIes;

    if (!successList.empty())
    {
        auto *ie = asn::New<ASN_NGAP_InitialContextSetupResponseIEs>();
        ie->id = ASN_NGAP_ProtocolIE_ID_id_PDUSessionResourceSetupListCxtRes;
        ie->criticality = ASN_NGAP_Criticality_ignore;
        ie->value.present = ASN_NGAP_InitialContextSetupResponseIEs__value_PR_PDUSessionResourceSetupListCxtRes;

        for (auto &item : successList)
            asn::SequenceAdd(ie->value.choice.PDUSessionResourceSetupListCxtRes, item);

        responseIes.push_back(ie);
    }

    if (!failedList.empty())
    {
        auto *ie = asn::New<ASN_NGAP_InitialContextSetupResponseIEs>();
        ie->id = ASN_NGAP_ProtocolIE_ID_id_PDUSessionResourceFailedToSetupListCxtRes;
        ie->criticality = ASN_NGAP_Criticality_ignore;
        ie->value.present = ASN_NGAP_InitialContextSetupResponseIEs__value_PR_PDUSessionResourceFailedToSetupListCxtRes;

        for (auto &item : failedList)
            asn::SequenceAdd(ie->value.choice.PDUSessionResourceFailedToSetupListCxtRes, item);

        responseIes.push_back(ie);
    }

    auto *response = asn::ngap::NewMessagePdu<ASN_NGAP_InitialContextSetupResponse>(responseIes);
    sendNgapUeAssociated(ue->ctxId, response);
}

void NgapTask::receiveContextRelease(int amfId, ASN_NGAP_UEContextReleaseCommand *msg)
{
    m_logger->debug("UE Context Release Command received");

    auto *ue = findUeByNgapIdPair(amfId, ngap_utils::FindNgapIdPairFromUeNgapIds(msg));
    if (ue == nullptr)
        return;

    // Notify RRC task
    auto w1 = std::make_unique<NmGnbNgapToRrc>(NmGnbNgapToRrc::AN_RELEASE);
    w1->ueId = ue->ctxId;
    m_base->rrcTask->push(std::move(w1));

    // Notify GTP task
    auto w2 = std::make_unique<NmGnbNgapToGtp>(NmGnbNgapToGtp::UE_CONTEXT_RELEASE);
    w2->ueId = ue->ctxId;
    m_base->gtpTask->push(std::move(w2));

    auto *response = asn::ngap::NewMessagePdu<ASN_NGAP_UEContextReleaseComplete>({});
    sendNgapUeAssociated(ue->ctxId, response);

    deleteUeContext(ue->ctxId);
}

void NgapTask::receiveContextModification(int amfId, ASN_NGAP_UEContextModificationRequest *msg)
{
    m_logger->debug("UE Context Modification Request received");

    auto *ue = findUeByNgapIdPair(amfId, ngap_utils::FindNgapIdPair(msg));
    if (ue == nullptr)
        return;

    auto *ie = asn::ngap::GetProtocolIe(msg, ASN_NGAP_ProtocolIE_ID_id_UEAggregateMaximumBitRate);
    if (ie)
    {
        ue->ueAmbr.dlAmbr = asn::GetUnsigned64(ie->UEAggregateMaximumBitRate.uEAggregateMaximumBitRateDL);
        ue->ueAmbr.ulAmbr = asn::GetUnsigned64(ie->UEAggregateMaximumBitRate.uEAggregateMaximumBitRateUL);
    }

    ie = asn::ngap::GetProtocolIe(msg, ASN_NGAP_ProtocolIE_ID_id_NewAMF_UE_NGAP_ID);
    if (ie)
    {
        int64_t old = ue->amfUeNgapId;
        ue->amfUeNgapId = asn::GetSigned64(ie->AMF_UE_NGAP_ID_1);
        m_logger->debug("AMF-UE-NGAP-ID changed from %ld to %ld", old, ue->amfUeNgapId);
    }

    auto *response = asn::ngap::NewMessagePdu<ASN_NGAP_UEContextModificationResponse>({});
    sendNgapUeAssociated(ue->ctxId, response);

    auto w = std::make_unique<NmGnbNgapToGtp>(NmGnbNgapToGtp::UE_CONTEXT_UPDATE);
    w->update = std::make_unique<GtpUeContextUpdate>(false, ue->ctxId, ue->ueAmbr);
    m_base->gtpTask->push(std::move(w));
}

void NgapTask::sendContextRelease(int ueId, NgapCause cause)
{
    m_logger->debug("Sending UE Context release request (NG-RAN node initiated)");

    auto *ue = findUeContext(ueId);
    if (ue == nullptr)
        return;

    std::vector<ASN_NGAP_UEContextReleaseRequest_IEs *> ies;

    if (!ue->pduSessions.empty())
    {
        auto *ieSessionList = asn::New<ASN_NGAP_UEContextReleaseRequest_IEs>();
        ieSessionList->id = ASN_NGAP_ProtocolIE_ID_id_PDUSessionResourceListCxtRelReq;
        ieSessionList->criticality = ASN_NGAP_Criticality_reject;
        ieSessionList->value.present = ASN_NGAP_UEContextReleaseRequest_IEs__value_PR_PDUSessionResourceListCxtRelReq;

        for (int psi : ue->pduSessions)
        {
            auto *sessionItem = asn::New<ASN_NGAP_PDUSessionResourceItemCxtRelReq>();
            sessionItem->pDUSessionID = static_cast<ASN_NGAP_PDUSessionID_t>(psi);
            asn::SequenceAdd(ieSessionList->value.choice.PDUSessionResourceListCxtRelReq, sessionItem);
        }

        ies.push_back(ieSessionList);
    }

    auto *ieCause = asn::New<ASN_NGAP_UEContextReleaseRequest_IEs>();
    ieCause->id = ASN_NGAP_ProtocolIE_ID_id_Cause;
    ieCause->criticality = ASN_NGAP_Criticality_ignore;
    ieCause->value.present = ASN_NGAP_UEContextReleaseRequest_IEs__value_PR_Cause;
    ngap_utils::ToCauseAsn_Ref(cause, ieCause->value.choice.Cause);
    ies.push_back(ieCause);

    auto *pdu = asn::ngap::NewMessagePdu<ASN_NGAP_UEContextReleaseRequest>(ies);
    sendNgapUeAssociated(ueId, pdu);
}

void NgapTask::UeHandover(uint64_t sti){
    int ueId = m_base->rlsTask->getudp()->findRlsPdu(sti);
    createUeContext(ueId);
    
    auto *ueCtx = findUeContext(ueId);
    if (ueCtx == nullptr)
        return;
    auto *amfCtx = findAmfContext(ueCtx->associatedAmfId);
    if (amfCtx == nullptr)
        return;

    if (amfCtx->state != EAmfState::CONNECTED)
    {
        m_logger->err("Initial UE Handover failure. AMF is not in connected state.");
        return;
    }

    amfCtx->nextStream = (amfCtx->nextStream + 1) % amfCtx->association.outStreams;
    if ((amfCtx->nextStream == 0) && (amfCtx->association.outStreams > 1))
        amfCtx->nextStream += 1;
    ueCtx->uplinkStream = amfCtx->nextStream;
    auto w1 = std::make_unique<NmGnbNgapToRrc>(NmGnbNgapToRrc::EXCHANGE_RRC);
    w1->ueId = ueId;
    m_base->rrcTask->push(std::move(w1));
}

ASN_NGAP_NGAP_PDU* NgapTask::sendPathSwitchRequestwithTargetIp(int ueId, OCTET_STRING target_ip){
    ASN_NGAP_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
    ASN_NGAP_UESecurityCapabilities_t *UESecurityCapabilities = NULL;
    ASN_NGAP_PDUSessionResourceToBeSwitchedDLList_t
        *PDUSessionResourceToBeSwitchedDLList = NULL;
    ASN_NGAP_PDUSessionResourceToBeSwitchedDLItem_t *PDUSessionItem = NULL;
    // NGAP_PathSwitchRequestTransfer_t 
    OCTET_STRING_t *transfer = NULL;

    m_logger->debug("Sending Path Switch request");

    auto *ue = findUeContext(ueId);
    if (ue == nullptr)
        m_logger->info("ue is null");

    std::vector<ASN_NGAP_PathSwitchRequestIEs *> ies;


    //auto *respPdu = asn::ngap::NewMessagePdu<ASN_NGAP_PDUSessionResourceSetupResponse>(responseIes);


    auto *ie = asn::New<ASN_NGAP_PathSwitchRequestIEs>();
    ie->id = ASN_NGAP_ProtocolIE_ID_id_SourceAMF_UE_NGAP_ID;
    ie->criticality = ASN_NGAP_Criticality_reject;
    ie->value.present = ASN_NGAP_PathSwitchRequestIEs__value_PR_AMF_UE_NGAP_ID;
    // auto *id = asn::New<ASN_NGAP_RAN_UE_NGAP_ID_t>();
    AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
    m_logger->debug("%lld",ue->amfUeNgapId);
    asn::SetSigned64(ue->amfUeNgapId, *AMF_UE_NGAP_ID);
    // auto *id = asn::New<ASN_NGAP_RAN_UE_NGAP_ID_t>();
    // *id = ue->amfUeNgapId;
    m_logger->debug("SourceAMF_UE_NGAP_ID :size %llu",AMF_UE_NGAP_ID->size);

    for (int i=0;i<AMF_UE_NGAP_ID->size;i++){
        m_logger->debug("%d",AMF_UE_NGAP_ID->buf[i]);
    }
    m_logger->debug("SourceAMF_UE_NGAP_ID :show true size %lld",ie->value.choice.AMF_UE_NGAP_ID.size);

    // ie->value.choice.RAN_UE_NGAP_ID = *id;
    ies.push_back(ie);
    // auto msgType = static_cast<asn::ngap::NgapMessageType>(asn::ngap::NgapMessageTypeToEnum<ASN_NGAP_PathSwitchRequest>::V);

    // void *pDescription = nullptr;
    // void *pMessage = asn::ngap::NewDescFromMessageType(msgType, pDescription);
    // for (auto &ie : ies){
    //     asn::ngap::AddProtocolIe(*reinterpret_cast<ASN_NGAP_PathSwitchRequest *>(pMessage), ie);
    // }
    // auto *pdu =  asn::ngap::NgapPduFromPduDescription(reinterpret_cast<ASN_NGAP_InitiatingMessage *>(pDescription));
    ie = asn::New<ASN_NGAP_PathSwitchRequestIEs>();
    ie->id = ASN_NGAP_ProtocolIE_ID_id_UESecurityCapabilities;
    ie->criticality = ASN_NGAP_Criticality_ignore;
    ie->value.present =
        ASN_NGAP_PathSwitchRequestIEs__value_PR_UESecurityCapabilities;

    UESecurityCapabilities = &ie->value.choice.UESecurityCapabilities;
    ies.push_back(ie);

    UESecurityCapabilities->nRencryptionAlgorithms.size = 2;
    UESecurityCapabilities->nRencryptionAlgorithms.buf =
        (uint8_t *)CALLOC(UESecurityCapabilities->
                    nRencryptionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->nRencryptionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->nRencryptionAlgorithms.buf[0] =
        (0 << 1);

    UESecurityCapabilities->nRintegrityProtectionAlgorithms.size = 2;
    UESecurityCapabilities->nRintegrityProtectionAlgorithms.buf =
        (uint8_t *)CALLOC(UESecurityCapabilities->
                    nRintegrityProtectionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->nRintegrityProtectionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->nRintegrityProtectionAlgorithms.buf[0] =
        (0 << 1);

    UESecurityCapabilities->eUTRAencryptionAlgorithms.size = 2;
    UESecurityCapabilities->eUTRAencryptionAlgorithms.buf =
        (uint8_t *)CALLOC(UESecurityCapabilities->
                    eUTRAencryptionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->eUTRAencryptionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->eUTRAencryptionAlgorithms.buf[0] =
        (0 << 1);

    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.size = 2;
    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.buf =
        (uint8_t *)CALLOC(UESecurityCapabilities->
                    eUTRAintegrityProtectionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.buf[0] =
        (0 << 1);
    m_logger->debug("Security Capability finish");
    ie = asn::New<ASN_NGAP_PathSwitchRequestIEs>();

    ie->id = ASN_NGAP_ProtocolIE_ID_id_PDUSessionResourceToBeSwitchedDLList;
    ie->criticality = ASN_NGAP_Criticality_ignore;
    ie->value.present = ASN_NGAP_PathSwitchRequestIEs__value_PR_PDUSessionResourceToBeSwitchedDLList;
    // std::vector<uint64_t> sessions{};
    // PduSessionTree.enumerateByUe(ueId, sessions);
    PDUSessionResourceToBeSwitchedDLList =
        &ie->value.choice.PDUSessionResourceToBeSwitchedDLList;
    m_logger->debug("PDUSessionResourceToBeSwitchedDLList 1");

    auto & m_pduSessions = m_base->gtpTask->GetPduSessions();
    auto iter = m_pduSessions.begin();//auto自动识别为迭代器类型unordered_map<int,string>::iterator
    while (iter!= m_pduSessions.end())
    {  
        m_logger->debug("%d,%lu\n",iter->first,iter->first);
        ++iter;  
    }  
    m_logger->debug("all pdu in m_pdusessions out\n");

    
    for (int psi : ue->pduSessions) {
        m_logger->debug("psi : %d,%lu",psi,(uint64_t)psi);
        auto *sessionItem = asn::New<ASN_NGAP_PDUSessionResourceToBeSwitchedDLItem_t>();
        m_logger->debug("we have session_p");
        uint64_t sessionInd = MakeSessionResInd(ueId, psi);
        std::unique_ptr<nr::gnb::PduSessionResource> & session_p = m_pduSessions[sessionInd]; //这里有问题再看下
        m_logger->debug("we have session_p");
        m_logger->debug("ueid: %d",session_p->ueId);

        sessionItem->pDUSessionID = static_cast<ASN_NGAP_PDUSessionID_t>(psi);
        sessionItem->pathSwitchRequestTransfer = OCTET_STRING{};
        transfer = &sessionItem->pathSwitchRequestTransfer;
        if (transfer == NULL){
            m_logger->debug("transfer is null");
        } else {
            m_logger->debug("transfer is not null");
        }
        ASN_NGAP_PathSwitchRequestTransfer_t message = ASN_NGAP_PathSwitchRequestTransfer_t{};
        ASN_NGAP_GTPTunnel * gTPTunnel = asn::New<ASN_NGAP_GTPTunnel>();
        message.dL_NGU_UP_TNLInformation.present = ASN_NGAP_UPTransportLayerInformation_PR_gTPTunnel;
        message.dL_NGU_UP_TNLInformation.choice.gTPTunnel = gTPTunnel;
        gTPTunnel->transportLayerAddress.size = 4;
        gTPTunnel->transportLayerAddress.buf = (uint8_t *)CALLOC(gTPTunnel->transportLayerAddress.size, sizeof(uint8_t));
        memcpy(gTPTunnel->transportLayerAddress.buf, target_ip.buf, 4);//critical:TODO:CHANGE HERE 
        m_logger->debug("transportlayeraddress as following");
        auto * address = gTPTunnel->transportLayerAddress.buf;
        for (int i=0;i<4;i++){
            m_logger->info("address: %d",*(address+i));
        }   
        m_logger->debug("store uptunnel address to sctp as following");
        address = session_p->upTunnel.address.data();
        for (int i=0;i<4;i++){
            m_base->sctpServer->ul_ip[i] = *(address+i);
            m_logger->info("address: %d",*(address+i));
        }
        // address = gTPTunnel->transportLayerAddress.buf;
        // for (int i=0;i<4;i++){
        //     m_logger->info("address: %d",*(address+i));
        // }   
        asn::SetOctetString4(gTPTunnel->gTP_TEID, (octet4)session_p->downTunnel.teid);
        //gTPTunnel->gTP_TEID.size = sizeof(uint32_t);
        
        //gTPTunnel->gTP_TEID.buf = (uint8_t *)CALLOC(1, sizeof(uint32_t));
        //memcpy(gTPTunnel->gTP_TEID.buf, &session_p->downTunnel.teid, sizeof(uint32_t));
        m_logger->debug("memcpy gTP_TEID");
        m_logger->info("Teid up:%lu",session_p->upTunnel.teid);
        m_logger->info("Teid down:%lu",session_p->downTunnel.teid);
        m_base->sctpServer->ul_teid = session_p->upTunnel.teid;
        
        // auto &qosList = session_p->qosFlows->list;
        auto &list = session_p->qosFlows->list;
        m_logger->debug("list length:%d",list.count);
        for (int iQos = 0; iQos < list.count; iQos++)
        {
            m_logger->debug("%d",iQos);
            ASN_NGAP_QosFlowAcceptedItem *qosFlowItem = asn::New<ASN_NGAP_QosFlowAcceptedItem>();
            auto & item = list.array[iQos];
            qosFlowItem->qosFlowIdentifier = item->qosFlowIdentifier;
            m_logger->debug("%d",iQos);
            asn::SequenceAdd(message.qosFlowAcceptedList, qosFlowItem);
        }
        m_logger->debug("finish list");

        OctetString transfer_s = nr::gnb::ngap_encode::EncodeS(asn_DEF_ASN_NGAP_PathSwitchRequestTransfer, &message);
        m_logger->debug("transefer_s success");

        int size_tmp = transfer_s.length();
        m_logger->debug("buffer size:%d",size_tmp);

        uint8_t * data = transfer_s.data();
        transfer->buf = (uint8_t*)CALLOC(size_tmp,sizeof(uint8_t));  
        memcpy(transfer->buf,data,size_tmp * sizeof(uint8_t));
        m_logger->debug("memcpy transfer");

        transfer->size = size_tmp;
        // message.qosFlowAcceptedList.list = session_p->qosFlows->list;
        // // ul->qfi = static_cast<int>(session_p->qosFlows->list)

            
        // ogs_asn_ip_to_BIT_STRING(&ip, &gTPTunnel->transportLayerAddress);
        // ogs_asn_uint32_to_OCTET_STRING(sess->gnb_n3_teid, &gTPTunnel->gTP_TEID);
        asn::SequenceAdd(*PDUSessionResourceToBeSwitchedDLList, sessionItem);
    }
    // for (auto &session : sessions){
    //     PDUSessionItem =
    //         asn::New<ASN_NGAP_PDUSessionResourceToBeSwitchedDLItem_t>();
    //     ASN_SEQUENCE_ADD(
    //         &PDUSessionResourceToBeSwitchedDLList->list, PDUSessionItem);

    //     PDUSessionItem->pDUSessionID = session ;

    //     // n2smbuf = testngap_build_path_switch_request_trasfer(sess);
    //     // transfer = &PDUSessionItem->pathSwitchRequestTransfer;

    //     // transfer->size = n2smbuf->len;
    //     // transfer->buf = CALLOC(transfer->size, sizeof(uint8_t));
    //     // memcpy(transfer->buf, n2smbuf->data, transfer->size);
    // }
    ies.push_back(ie);
    m_logger->debug("PDUSessionResourceToBeSwitchedDLList finish");

    ASN_NGAP_NGAP_PDU *pdu = asn::ngap::NewMessagePdu<ASN_NGAP_PathSwitchRequest>(ies);
    // m_logger->debug("attention ! the code add in 20230613 should be modify ! ");
    // sendNgapUeAssociated(ueId, pdu); //TODO:attention ! you have to modify it 
    return pdu;
}


ASN_NGAP_NGAP_PDU* NgapTask::sendPathSwitchRequestwithTargetIp_BH(int ueId, OCTET_STRING target_ip){//Before Handover
    ASN_NGAP_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
    ASN_NGAP_UESecurityCapabilities_t *UESecurityCapabilities = NULL;
    ASN_NGAP_PDUSessionResourceToBeSwitchedDLList_t
        *PDUSessionResourceToBeSwitchedDLList = NULL;
    ASN_NGAP_PDUSessionResourceToBeSwitchedDLItem_t *PDUSessionItem = NULL;
    // NGAP_PathSwitchRequestTransfer_t 
    OCTET_STRING_t *transfer = NULL;

    m_logger->debug("Sending Path Switch request");

    auto *ue = findUeContext(ueId);
    if (ue == nullptr)
        m_logger->info("ue is null");

    std::vector<ASN_NGAP_PathSwitchRequestIEs *> ies;


    //auto *respPdu = asn::ngap::NewMessagePdu<ASN_NGAP_PDUSessionResourceSetupResponse>(responseIes);

    
    auto *ie = asn::New<ASN_NGAP_PathSwitchRequestIEs>();
    ie->id = ASN_NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
    ie->criticality = ASN_NGAP_Criticality_reject;
    ie->value.present = ASN_NGAP_PathSwitchRequestIEs__value_PR_AMF_UE_NGAP_ID;
    // auto *id = asn::New<ASN_NGAP_RAN_UE_NGAP_ID_t>();
    AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
    m_logger->debug("%lld",ue->amfUeNgapId);
    asn::SetSigned64(ue->amfUeNgapId, *AMF_UE_NGAP_ID);
    // auto *id = asn::New<ASN_NGAP_RAN_UE_NGAP_ID_t>();
    // *id = ue->amfUeNgapId;
    m_logger->debug("SourceAMF_UE_NGAP_ID :size %llu",AMF_UE_NGAP_ID->size);



    ie->id = ASN_NGAP_ProtocolIE_ID_id_SourceAMF_UE_NGAP_ID;
    ie->criticality = ASN_NGAP_Criticality_reject;
    ie->value.present = ASN_NGAP_PathSwitchRequestIEs__value_PR_AMF_UE_NGAP_ID;
    // auto *id = asn::New<ASN_NGAP_RAN_UE_NGAP_ID_t>();
    AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
    m_logger->debug("%lld",ue->amfUeNgapId);
    asn::SetSigned64(ue->amfUeNgapId, *AMF_UE_NGAP_ID);
    // auto *id = asn::New<ASN_NGAP_RAN_UE_NGAP_ID_t>();
    // *id = ue->amfUeNgapId;
    m_logger->debug("SourceAMF_UE_NGAP_ID :size %llu",AMF_UE_NGAP_ID->size);

    for (int i=0;i<int(AMF_UE_NGAP_ID->size);i++){
        m_logger->debug("%d",AMF_UE_NGAP_ID->buf[i]);
    }
    m_logger->debug("SourceAMF_UE_NGAP_ID :show true size %lld",ie->value.choice.AMF_UE_NGAP_ID.size);

    // ie->value.choice.RAN_UE_NGAP_ID = *id;
    ies.push_back(ie);
    // auto msgType = static_cast<asn::ngap::NgapMessageType>(asn::ngap::NgapMessageTypeToEnum<ASN_NGAP_PathSwitchRequest>::V);

    // void *pDescription = nullptr;
    // void *pMessage = asn::ngap::NewDescFromMessageType(msgType, pDescription);
    // for (auto &ie : ies){
    //     asn::ngap::AddProtocolIe(*reinterpret_cast<ASN_NGAP_PathSwitchRequest *>(pMessage), ie);
    // }
    // auto *pdu =  asn::ngap::NgapPduFromPduDescription(reinterpret_cast<ASN_NGAP_InitiatingMessage *>(pDescription));
    ie = asn::New<ASN_NGAP_PathSwitchRequestIEs>();
    ie->id = ASN_NGAP_ProtocolIE_ID_id_UESecurityCapabilities;
    ie->criticality = ASN_NGAP_Criticality_ignore;
    ie->value.present =
        ASN_NGAP_PathSwitchRequestIEs__value_PR_UESecurityCapabilities;

    UESecurityCapabilities = &ie->value.choice.UESecurityCapabilities;
    ies.push_back(ie);

    UESecurityCapabilities->nRencryptionAlgorithms.size = 2;
    UESecurityCapabilities->nRencryptionAlgorithms.buf =
        (uint8_t *)CALLOC(UESecurityCapabilities->
                    nRencryptionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->nRencryptionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->nRencryptionAlgorithms.buf[0] =
        (0 << 1);

    UESecurityCapabilities->nRintegrityProtectionAlgorithms.size = 2;
    UESecurityCapabilities->nRintegrityProtectionAlgorithms.buf =
        (uint8_t *)CALLOC(UESecurityCapabilities->
                    nRintegrityProtectionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->nRintegrityProtectionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->nRintegrityProtectionAlgorithms.buf[0] =
        (0 << 1);

    UESecurityCapabilities->eUTRAencryptionAlgorithms.size = 2;
    UESecurityCapabilities->eUTRAencryptionAlgorithms.buf =
        (uint8_t *)CALLOC(UESecurityCapabilities->
                    eUTRAencryptionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->eUTRAencryptionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->eUTRAencryptionAlgorithms.buf[0] =
        (0 << 1);

    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.size = 2;
    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.buf =
        (uint8_t *)CALLOC(UESecurityCapabilities->
                    eUTRAintegrityProtectionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.buf[0] =
        (0 << 1);
    m_logger->debug("Security Capability finish");
    ie = asn::New<ASN_NGAP_PathSwitchRequestIEs>();

    ie->id = ASN_NGAP_ProtocolIE_ID_id_PDUSessionResourceToBeSwitchedDLList;
    ie->criticality = ASN_NGAP_Criticality_ignore;
    ie->value.present = ASN_NGAP_PathSwitchRequestIEs__value_PR_PDUSessionResourceToBeSwitchedDLList;
    // std::vector<uint64_t> sessions{};
    // PduSessionTree.enumerateByUe(ueId, sessions);
    PDUSessionResourceToBeSwitchedDLList =
        &ie->value.choice.PDUSessionResourceToBeSwitchedDLList;
    m_logger->debug("PDUSessionResourceToBeSwitchedDLList 1");

    auto & m_pduSessions = m_base->gtpTask->GetPduSessions();
    auto iter = m_pduSessions.begin();//auto自动识别为迭代器类型unordered_map<int,string>::iterator
    while (iter!= m_pduSessions.end())
    {  
        m_logger->debug("%d,%lu\n",iter->first,iter->first);
        ++iter;  
    }  
    m_logger->debug("all pdu in m_pdusessions out\n");

    
    for (int psi : ue->pduSessions) {
        m_logger->debug("psi : %d,%lu",psi,(uint64_t)psi);
        auto *sessionItem = asn::New<ASN_NGAP_PDUSessionResourceToBeSwitchedDLItem_t>();
        m_logger->debug("we have session_p");
        uint64_t sessionInd = MakeSessionResInd(ueId, psi);
        std::unique_ptr<nr::gnb::PduSessionResource> & session_p = m_pduSessions[sessionInd]; //这里有问题再看下
        m_logger->debug("we have session_p");
        m_logger->debug("ueid: %d",session_p->ueId);

        sessionItem->pDUSessionID = static_cast<ASN_NGAP_PDUSessionID_t>(psi);
        sessionItem->pathSwitchRequestTransfer = OCTET_STRING{};
        transfer = &sessionItem->pathSwitchRequestTransfer;
        if (transfer == NULL){
            m_logger->debug("transfer is null");
        } else {
            m_logger->debug("transfer is not null");
        }
        ASN_NGAP_PathSwitchRequestTransfer_t message = ASN_NGAP_PathSwitchRequestTransfer_t{};
        ASN_NGAP_GTPTunnel * gTPTunnel = asn::New<ASN_NGAP_GTPTunnel>();
        message.dL_NGU_UP_TNLInformation.present = ASN_NGAP_UPTransportLayerInformation_PR_gTPTunnel;
        message.dL_NGU_UP_TNLInformation.choice.gTPTunnel = gTPTunnel;
        gTPTunnel->transportLayerAddress.size = 4;
        gTPTunnel->transportLayerAddress.buf = (uint8_t *)CALLOC(gTPTunnel->transportLayerAddress.size, sizeof(uint8_t));
        memcpy(gTPTunnel->transportLayerAddress.buf, target_ip.buf, 4);//critical:TODO:CHANGE HERE 
        m_logger->debug("transportlayeraddress as following");
        auto * address = gTPTunnel->transportLayerAddress.buf;
        for (int i=0;i<4;i++){
            m_logger->info("address: %d",*(address+i));
        }   
        m_logger->debug("store uptunnel address to sctp as following");
        address = session_p->upTunnel.address.data();
        for (int i=0;i<4;i++){
            m_base->sctpServer->ul_ip[i] = *(address+i);
            m_logger->info("address: %d",*(address+i));
        }
        // address = gTPTunnel->transportLayerAddress.buf;
        // for (int i=0;i<4;i++){
        //     m_logger->info("address: %d",*(address+i));
        // }   
        asn::SetOctetString4(gTPTunnel->gTP_TEID, (octet4)session_p->downTunnel.teid);
        //gTPTunnel->gTP_TEID.size = sizeof(uint32_t);
        
        //gTPTunnel->gTP_TEID.buf = (uint8_t *)CALLOC(1, sizeof(uint32_t));
        //memcpy(gTPTunnel->gTP_TEID.buf, &session_p->downTunnel.teid, sizeof(uint32_t));
        m_logger->debug("memcpy gTP_TEID");
        m_logger->info("Teid up:%lu",session_p->upTunnel.teid);
        m_logger->info("Teid down:%lu",session_p->downTunnel.teid);
        m_base->sctpServer->ul_teid = session_p->upTunnel.teid;
        
        // auto &qosList = session_p->qosFlows->list;
        auto &list = session_p->qosFlows->list;
        m_logger->debug("list length:%d",list.count);
        for (int iQos = 0; iQos < list.count; iQos++)
        {
            m_logger->debug("%d",iQos);
            ASN_NGAP_QosFlowAcceptedItem *qosFlowItem = asn::New<ASN_NGAP_QosFlowAcceptedItem>();
            auto & item = list.array[iQos];
            qosFlowItem->qosFlowIdentifier = item->qosFlowIdentifier;
            m_logger->debug("%d",iQos);
            asn::SequenceAdd(message.qosFlowAcceptedList, qosFlowItem);
        }
        m_logger->debug("finish list");

        OctetString transfer_s = nr::gnb::ngap_encode::EncodeS(asn_DEF_ASN_NGAP_PathSwitchRequestTransfer, &message);
        m_logger->debug("transefer_s success");

        int size_tmp = transfer_s.length();
        m_logger->debug("buffer size:%d",size_tmp);

        uint8_t * data = transfer_s.data();
        transfer->buf = (uint8_t*)CALLOC(size_tmp,sizeof(uint8_t));  
        memcpy(transfer->buf,data,size_tmp * sizeof(uint8_t));
        m_logger->debug("memcpy transfer");

        transfer->size = size_tmp;
        // message.qosFlowAcceptedList.list = session_p->qosFlows->list;
        // // ul->qfi = static_cast<int>(session_p->qosFlows->list)

            
        // ogs_asn_ip_to_BIT_STRING(&ip, &gTPTunnel->transportLayerAddress);
        // ogs_asn_uint32_to_OCTET_STRING(sess->gnb_n3_teid, &gTPTunnel->gTP_TEID);
        asn::SequenceAdd(*PDUSessionResourceToBeSwitchedDLList, sessionItem);
    }
    // for (auto &session : sessions){
    //     PDUSessionItem =
    //         asn::New<ASN_NGAP_PDUSessionResourceToBeSwitchedDLItem_t>();
    //     ASN_SEQUENCE_ADD(
    //         &PDUSessionResourceToBeSwitchedDLList->list, PDUSessionItem);

    //     PDUSessionItem->pDUSessionID = session ;

    //     // n2smbuf = testngap_build_path_switch_request_trasfer(sess);
    //     // transfer = &PDUSessionItem->pathSwitchRequestTransfer;

    //     // transfer->size = n2smbuf->len;
    //     // transfer->buf = CALLOC(transfer->size, sizeof(uint8_t));
    //     // memcpy(transfer->buf, n2smbuf->data, transfer->size);
    // }
    ies.push_back(ie);
    m_logger->debug("PDUSessionResourceToBeSwitchedDLList finish");

    ASN_NGAP_NGAP_PDU *pdu = asn::ngap::NewMessagePdu<ASN_NGAP_PathSwitchRequest>(ies);
    // m_logger->debug("attention ! the code add in 20230613 should be modify ! ");
    // sendNgapUeAssociated(ueId, pdu); //TODO:attention ! you have to modify it 
    return pdu;
}



ASN_NGAP_NGAP_PDU* NgapTask::sendPathSwitchRequest(int ueId)
{
    ASN_NGAP_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
    ASN_NGAP_UESecurityCapabilities_t *UESecurityCapabilities = NULL;
    ASN_NGAP_PDUSessionResourceToBeSwitchedDLList_t
        *PDUSessionResourceToBeSwitchedDLList = NULL;
    ASN_NGAP_PDUSessionResourceToBeSwitchedDLItem_t *PDUSessionItem = NULL;
    // NGAP_PathSwitchRequestTransfer_t 
    OCTET_STRING_t *transfer = NULL;

    m_logger->debug("Sending Path Switch request");

    auto *ue = findUeContext(ueId);
    if (ue == nullptr)
        m_logger->info("ue is null");

    std::vector<ASN_NGAP_PathSwitchRequestIEs *> ies;


    //auto *respPdu = asn::ngap::NewMessagePdu<ASN_NGAP_PDUSessionResourceSetupResponse>(responseIes);
    auto *ie = asn::New<ASN_NGAP_PathSwitchRequestIEs>();
    ie->id = ASN_NGAP_ProtocolIE_ID_id_SourceAMF_UE_NGAP_ID;
    ie->criticality = ASN_NGAP_Criticality_reject;
    ie->value.present = ASN_NGAP_PathSwitchRequestIEs__value_PR_AMF_UE_NGAP_ID;
    // auto *id = asn::New<ASN_NGAP_RAN_UE_NGAP_ID_t>();
    AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
    m_logger->debug("%d",ue->amfUeNgapId);
    m_logger->debug("%d",ie->value.choice.AMF_UE_NGAP_ID);

    asn::SetSigned64(ue->amfUeNgapId, *AMF_UE_NGAP_ID);
    // auto *id = asn::New<ASN_NGAP_RAN_UE_NGAP_ID_t>();
    // *id = ue->amfUeNgapId;
    m_logger->debug("%d",*ie->value.choice.AMF_UE_NGAP_ID.buf);

    // ie->value.choice.RAN_UE_NGAP_ID = *id;
    ies.push_back(ie);
    // auto msgType = static_cast<asn::ngap::NgapMessageType>(asn::ngap::NgapMessageTypeToEnum<ASN_NGAP_PathSwitchRequest>::V);

    // void *pDescription = nullptr;
    // void *pMessage = asn::ngap::NewDescFromMessageType(msgType, pDescription);
    // for (auto &ie : ies){
    //     asn::ngap::AddProtocolIe(*reinterpret_cast<ASN_NGAP_PathSwitchRequest *>(pMessage), ie);
    // }
    // auto *pdu =  asn::ngap::NgapPduFromPduDescription(reinterpret_cast<ASN_NGAP_InitiatingMessage *>(pDescription));
    ie = asn::New<ASN_NGAP_PathSwitchRequestIEs>();
    ie->id = ASN_NGAP_ProtocolIE_ID_id_UESecurityCapabilities;
    ie->criticality = ASN_NGAP_Criticality_ignore;
    ie->value.present =
        ASN_NGAP_PathSwitchRequestIEs__value_PR_UESecurityCapabilities;

    UESecurityCapabilities = &ie->value.choice.UESecurityCapabilities;
    ies.push_back(ie);

    UESecurityCapabilities->nRencryptionAlgorithms.size = 2;
    UESecurityCapabilities->nRencryptionAlgorithms.buf =
        (uint8_t *)CALLOC(UESecurityCapabilities->
                    nRencryptionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->nRencryptionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->nRencryptionAlgorithms.buf[0] =
        (0 << 1);

    UESecurityCapabilities->nRintegrityProtectionAlgorithms.size = 2;
    UESecurityCapabilities->nRintegrityProtectionAlgorithms.buf =
        (uint8_t *)CALLOC(UESecurityCapabilities->
                    nRintegrityProtectionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->nRintegrityProtectionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->nRintegrityProtectionAlgorithms.buf[0] =
        (0 << 1);

    UESecurityCapabilities->eUTRAencryptionAlgorithms.size = 2;
    UESecurityCapabilities->eUTRAencryptionAlgorithms.buf =
        (uint8_t *)CALLOC(UESecurityCapabilities->
                    eUTRAencryptionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->eUTRAencryptionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->eUTRAencryptionAlgorithms.buf[0] =
        (0 << 1);

    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.size = 2;
    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.buf =
        (uint8_t *)CALLOC(UESecurityCapabilities->
                    eUTRAintegrityProtectionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.buf[0] =
        (0 << 1);
    m_logger->debug("Security Capability finish");
    ie = asn::New<ASN_NGAP_PathSwitchRequestIEs>();

    ie->id = ASN_NGAP_ProtocolIE_ID_id_PDUSessionResourceToBeSwitchedDLList;
    ie->criticality = ASN_NGAP_Criticality_ignore;
    ie->value.present = ASN_NGAP_PathSwitchRequestIEs__value_PR_PDUSessionResourceToBeSwitchedDLList;
    // std::vector<uint64_t> sessions{};
    // PduSessionTree.enumerateByUe(ueId, sessions);
    PDUSessionResourceToBeSwitchedDLList =
        &ie->value.choice.PDUSessionResourceToBeSwitchedDLList;
    m_logger->debug("PDUSessionResourceToBeSwitchedDLList 1");

    auto & m_pduSessions = m_base->gtpTask->GetPduSessions();
    auto iter = m_pduSessions.begin();//auto自动识别为迭代器类型unordered_map<int,string>::iterator
    while (iter!= m_pduSessions.end())
    {  
        m_logger->debug("%d,%lu\n",iter->first,iter->first);
        ++iter;  
    }  
    m_logger->debug("all pdu in m_pdusessions out\n");

    
    for (int psi : ue->pduSessions) {
        m_logger->debug("psi : %d,%lu",psi,(uint64_t)psi);
        auto *sessionItem = asn::New<ASN_NGAP_PDUSessionResourceToBeSwitchedDLItem_t>();
        m_logger->debug("we have session_p");
        uint64_t sessionInd = MakeSessionResInd(ueId, psi);
        std::unique_ptr<nr::gnb::PduSessionResource> & session_p = m_pduSessions[sessionInd]; //这里有问题再看下
        m_logger->debug("we have session_p");
        m_logger->debug("ueid: %d",session_p->ueId);

        sessionItem->pDUSessionID = static_cast<ASN_NGAP_PDUSessionID_t>(psi);
        sessionItem->pathSwitchRequestTransfer = OCTET_STRING{};
        transfer = &sessionItem->pathSwitchRequestTransfer;
        if (transfer == NULL){
            m_logger->debug("transfer is null");
        } else {
            m_logger->debug("transfer is not null");
        }
        ASN_NGAP_PathSwitchRequestTransfer_t message = ASN_NGAP_PathSwitchRequestTransfer_t{};
        ASN_NGAP_GTPTunnel * gTPTunnel = asn::New<ASN_NGAP_GTPTunnel>();
        message.dL_NGU_UP_TNLInformation.present = ASN_NGAP_UPTransportLayerInformation_PR_gTPTunnel;
        message.dL_NGU_UP_TNLInformation.choice.gTPTunnel = gTPTunnel;
        gTPTunnel->transportLayerAddress.size = 4;
        gTPTunnel->transportLayerAddress.buf = (uint8_t *)CALLOC(gTPTunnel->transportLayerAddress.size, sizeof(uint8_t));
        memcpy(gTPTunnel->transportLayerAddress.buf, session_p->upTunnel.address.data(), 4);//critical:TODO:CHANGE HERE 
        m_logger->debug("memcpy transportLayerAddress:%s",OctetStringToIpString(session_p->upTunnel.address));
        auto address = session_p->upTunnel.address.data() ;
        for (int i=0;i<session_p->upTunnel.address.length();i++){
            m_logger->info("address: %d",*(address+i));
        }
        // address = gTPTunnel->transportLayerAddress.buf;
        // for (int i=0;i<4;i++){
        //     m_logger->info("address: %d",*(address+i));
        // }   
        gTPTunnel->gTP_TEID.size = sizeof(uint32_t);
        gTPTunnel->gTP_TEID.buf = (uint8_t *)CALLOC(1, sizeof(uint32_t));
        memcpy(gTPTunnel->gTP_TEID.buf, &session_p->upTunnel.teid, sizeof(uint32_t));
        m_logger->debug("memcpy gTP_TEID");
        m_logger->info("Teid:%lu",session_p->upTunnel.teid);
        m_logger->info("Teid:%lu",session_p->downTunnel.teid);

        // auto &qosList = session_p->qosFlows->list;
        auto &list = session_p->qosFlows->list;
        m_logger->debug("list length:%d",list.count);
        for (int iQos = 0; iQos < list.count; iQos++)
        {
            m_logger->debug("%d",iQos);
            ASN_NGAP_QosFlowAcceptedItem *qosFlowItem = asn::New<ASN_NGAP_QosFlowAcceptedItem>();
            auto & item = list.array[iQos];
            qosFlowItem->qosFlowIdentifier = item->qosFlowIdentifier;
            m_logger->debug("%d",iQos);
            asn::SequenceAdd(message.qosFlowAcceptedList, qosFlowItem);
        }
        m_logger->debug("finish list");

        OctetString transfer_s = nr::gnb::ngap_encode::EncodeS(asn_DEF_ASN_NGAP_PathSwitchRequestTransfer, &message);
        m_logger->debug("transefer_s success");

        int size_tmp = transfer_s.length();
        m_logger->debug("buffer size:%d",size_tmp);

        uint8_t * data = transfer_s.data();
        transfer->buf = (uint8_t*)CALLOC(size_tmp,sizeof(uint8_t));  
        memcpy(transfer->buf,data,size_tmp * sizeof(uint8_t));
        m_logger->debug("memcpy transfer");

        transfer->size = size_tmp;
        // message.qosFlowAcceptedList.list = session_p->qosFlows->list;
        // // ul->qfi = static_cast<int>(session_p->qosFlows->list)

            
        // ogs_asn_ip_to_BIT_STRING(&ip, &gTPTunnel->transportLayerAddress);
        // ogs_asn_uint32_to_OCTET_STRING(sess->gnb_n3_teid, &gTPTunnel->gTP_TEID);
        asn::SequenceAdd(*PDUSessionResourceToBeSwitchedDLList, sessionItem);
    }
    // for (auto &session : sessions){
    //     PDUSessionItem =
    //         asn::New<ASN_NGAP_PDUSessionResourceToBeSwitchedDLItem_t>();
    //     ASN_SEQUENCE_ADD(
    //         &PDUSessionResourceToBeSwitchedDLList->list, PDUSessionItem);

    //     PDUSessionItem->pDUSessionID = session ;

    //     // n2smbuf = testngap_build_path_switch_request_trasfer(sess);
    //     // transfer = &PDUSessionItem->pathSwitchRequestTransfer;

    //     // transfer->size = n2smbuf->len;
    //     // transfer->buf = CALLOC(transfer->size, sizeof(uint8_t));
    //     // memcpy(transfer->buf, n2smbuf->data, transfer->size);
    // }
    ies.push_back(ie);
    m_logger->debug("PDUSessionResourceToBeSwitchedDLList finish");

    ASN_NGAP_NGAP_PDU *pdu = asn::ngap::NewMessagePdu<ASN_NGAP_PathSwitchRequest>(ies);
    // m_logger->debug("attention ! the code add in 20230613 should be modify ! ");
    // sendNgapUeAssociated(ueId, pdu); //TODO:attention ! you have to modify it 
    return pdu;
}



} // namespace nr::gnb