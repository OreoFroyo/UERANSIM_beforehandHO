//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "task.hpp"

#include <gnb/ngap/task.hpp>
#include <lib/rrc/encode.hpp>

#include <asn/ngap/ASN_NGAP_FiveG-S-TMSI.h>
#include <asn/rrc/ASN_RRC_BCCH-BCH-Message.h>
#include <asn/rrc/ASN_RRC_BCCH-DL-SCH-Message.h>
#include <asn/rrc/ASN_RRC_CellGroupConfig.h>
#include <asn/rrc/ASN_RRC_DL-CCCH-Message.h>
#include <asn/rrc/ASN_RRC_DL-DCCH-Message.h>
#include <asn/rrc/ASN_RRC_DLInformationTransfer-IEs.h>
#include <asn/rrc/ASN_RRC_DLInformationTransfer.h>
#include <asn/rrc/ASN_RRC_PCCH-Message.h>
#include <asn/rrc/ASN_RRC_Paging.h>
#include <asn/rrc/ASN_RRC_PagingRecord.h>
#include <asn/rrc/ASN_RRC_PagingRecordList.h>
#include <asn/rrc/ASN_RRC_RRCRelease-IEs.h>
#include <asn/rrc/ASN_RRC_RRCRelease.h>
#include <asn/rrc/ASN_RRC_RRCSetup-IEs.h>
#include <asn/rrc/ASN_RRC_RRCSetup.h>
#include <asn/rrc/ASN_RRC_RRCSetupComplete-IEs.h>
#include <asn/rrc/ASN_RRC_RRCSetupComplete.h>
#include <asn/rrc/ASN_RRC_RRCSetupRequest.h>
#include <asn/rrc/ASN_RRC_UL-CCCH-Message.h>
#include <asn/rrc/ASN_RRC_UL-CCCH1-Message.h>
#include <asn/rrc/ASN_RRC_UL-DCCH-Message.h>
#include <asn/rrc/ASN_RRC_ULInformationTransfer-IEs.h>
#include <asn/rrc/ASN_RRC_ULInformationTransfer.h>
#include <asn/rrc/ASN_RRC_MeasurementReport.h>
#include <asn/rrc/ASN_RRC_MeasurementReport-IEs.h>
#include <asn/rrc/ASN_RRC_RRCReconfiguration.h>
#include <asn/rrc/ASN_RRC_RRCReconfiguration-IEs.h>
#include <asn/rrc/ASN_RRC_MobilityFromNRCommand.h>
#include <asn/rrc/ASN_RRC_MobilityFromNRCommand-IEs.h>

namespace nr::gnb
{

void GnbRrcTask::handleDownlinkNasDelivery(int ueId, const OctetString &nasPdu)
{
    auto *pdu = asn::New<ASN_RRC_DL_DCCH_Message>();
    pdu->message.present = ASN_RRC_DL_DCCH_MessageType_PR_c1;
    pdu->message.choice.c1 =
        asn::New<ASN_RRC_DL_DCCH_MessageType_t::ASN_RRC_DL_DCCH_MessageType_u::ASN_RRC_DL_DCCH_MessageType__c1>();
    pdu->message.choice.c1->present = ASN_RRC_DL_DCCH_MessageType__c1_PR_dlInformationTransfer;
    pdu->message.choice.c1->choice.dlInformationTransfer = asn::New<ASN_RRC_DLInformationTransfer>();

    auto &c1 = pdu->message.choice.c1->choice.dlInformationTransfer->criticalExtensions;
    c1.present = ASN_RRC_DLInformationTransfer__criticalExtensions_PR_dlInformationTransfer;
    c1.choice.dlInformationTransfer = asn::New<ASN_RRC_DLInformationTransfer_IEs>();
    c1.choice.dlInformationTransfer->dedicatedNAS_Message = asn::New<ASN_RRC_DedicatedNAS_Message_t>();
    asn::SetOctetString(*c1.choice.dlInformationTransfer->dedicatedNAS_Message, nasPdu);

    sendRrcMessage(ueId, pdu);
    asn::Free(asn_DEF_ASN_RRC_DL_DCCH_Message, pdu);
}

void GnbRrcTask::deliverUplinkNas(int ueId, OctetString &&nasPdu)
{
    auto w = std::make_unique<NmGnbRrcToNgap>(NmGnbRrcToNgap::UPLINK_NAS_DELIVERY);
    w->ueId = ueId;
    w->pdu = std::move(nasPdu);
    m_base->ngapTask->push(std::move(w));
}

void GnbRrcTask::receiveUplinkInformationTransfer(int ueId, const ASN_RRC_ULInformationTransfer &msg)
{
    if (msg.criticalExtensions.present == ASN_RRC_ULInformationTransfer__criticalExtensions_PR_ulInformationTransfer)
        deliverUplinkNas(
            ueId, asn::GetOctetString(*msg.criticalExtensions.choice.ulInformationTransfer->dedicatedNAS_Message));
}

void GnbRrcTask::releaseConnection(int ueId)
{
    m_logger->info("Releasing RRC connection for UE[%d]", ueId);

    // Send RRC Release message
    auto *pdu = asn::New<ASN_RRC_DL_DCCH_Message>();
    pdu->message.present = ASN_RRC_DL_DCCH_MessageType_PR_c1;
    pdu->message.choice.c1 = asn::NewFor(pdu->message.choice.c1);
    pdu->message.choice.c1->present = ASN_RRC_DL_DCCH_MessageType__c1_PR_rrcRelease;
    auto &rrcRelease = pdu->message.choice.c1->choice.rrcRelease = asn::New<ASN_RRC_RRCRelease>();
    rrcRelease->rrc_TransactionIdentifier = getNextTid();
    rrcRelease->criticalExtensions.present = ASN_RRC_RRCRelease__criticalExtensions_PR_rrcRelease;
    rrcRelease->criticalExtensions.choice.rrcRelease = asn::New<ASN_RRC_RRCRelease_IEs>();

    sendRrcMessage(ueId, pdu);
    asn::Free(asn_DEF_ASN_RRC_DL_DCCH_Message, pdu);

    // Delete UE RRC context
    m_ueCtx.erase(ueId);
}



void GnbRrcTask::exchangeRRCConnectionWithSti(int ueId,uint64_t sti)
{
    m_logger->info("Exchange RRC connection by use reconfigure for UE[%d]", ueId);

    // Send RRC Release message
    auto *pdu = asn::New<ASN_RRC_DL_DCCH_Message>();
    pdu->message.present = ASN_RRC_DL_DCCH_MessageType_PR_c1;
    pdu->message.choice.c1 = asn::NewFor(pdu->message.choice.c1);
    pdu->message.choice.c1->present = ASN_RRC_DL_DCCH_MessageType__c1_PR_rrcReconfiguration;
    auto &rrcReconfiguration = pdu->message.choice.c1->choice.rrcReconfiguration = asn::New<ASN_RRC_RRCReconfiguration>();
    rrcReconfiguration->rrc_TransactionIdentifier = getNextTid();
    rrcReconfiguration->criticalExtensions.present = ASN_RRC_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration;
    rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration = asn::New<ASN_RRC_RRCReconfiguration_IEs>();
    auto &rrcReconfiguration_IEs = rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration = asn::New<ASN_RRC_RRCReconfiguration_IEs>();
    auto *gnb_ip_s = rrcReconfiguration_IEs->secondaryCellGroup = new OCTET_STRING();
    gnb_ip_s->buf = (uint8_t *)CALLOC(8,1);
    gnb_ip_s->size = 8;

    for (int i=0;i<gnb_ip_s->size;i++){
        gnb_ip_s->buf[i] = sti << (8*i) >> 56;
        m_logger->info("target_gnb_sti : %d",gnb_ip_s->buf[i]);
    }
    sendRrcMessage(ueId, pdu);
    asn::Free(asn_DEF_ASN_RRC_DL_DCCH_Message, pdu);

}


void GnbRrcTask::exchangeRRCConnection(int ueId)
{
    m_logger->info("Exchange RRC connection by use reconfigure for UE[%d]", ueId);

    // Send RRC Release message
    auto *pdu = asn::New<ASN_RRC_DL_DCCH_Message>();
    pdu->message.present = ASN_RRC_DL_DCCH_MessageType_PR_c1;
    pdu->message.choice.c1 = asn::NewFor(pdu->message.choice.c1);
    pdu->message.choice.c1->present = ASN_RRC_DL_DCCH_MessageType__c1_PR_rrcReconfiguration;
    auto &rrcReconfiguration = pdu->message.choice.c1->choice.rrcReconfiguration = asn::New<ASN_RRC_RRCReconfiguration>();
    rrcReconfiguration->rrc_TransactionIdentifier = getNextTid();
    rrcReconfiguration->criticalExtensions.present = ASN_RRC_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration;
    rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration = asn::New<ASN_RRC_RRCReconfiguration_IEs>();
    // auto &rrcReconfiguration_IEs = rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration = asn::New<ASN_RRC_RRCReconfiguration_IEs>();
    // auto &gnb_ip = rrcReconfiguration_IEs->secondaryCellGroup = new OCTET_STRING();
    // gnb_ip.buf = (uint8_t *)CALLOC(4,1);
    // gnb_ip.size = 4;
    // for (int i=0;i<gnb_ip.size;i++){
    //     gnb_ip.buf[i] = m_base->sctpServer->target_ip[i];
    //     m_logger->info("target_ip : %d",m_base->sctpServer->target_ip[i]);
    // }
    sendRrcMessage(ueId, pdu);
    asn::Free(asn_DEF_ASN_RRC_DL_DCCH_Message, pdu);

}


void GnbRrcTask::handleRadioLinkFailure(int ueId)
{
    // Notify NGAP task
    auto w = std::make_unique<NmGnbRrcToNgap>(NmGnbRrcToNgap::RADIO_LINK_FAILURE);
    w->ueId = ueId;
    m_base->ngapTask->push(std::move(w));
  
    // Delete UE RRC context
    m_ueCtx.erase(ueId);
}

void GnbRrcTask::receiveMeasurementReport(int ueId , const ASN_RRC_MeasurementReport &msg)
{
    if(msg.criticalExtensions.choice.measurementReport->measResults.measId == 7){ //beforehand-HO
        auto w = std::make_unique<NmGnbRrcToNgap>(NmGnbRrcToNgap::BEFOREHAND_HO_FROM_UE);
        w->ueId = ueId;
        // OctetString nasPdu = asn::GetOctetString(*msg.criticalExtensions.choice.measurementReport->measResults);
        // w->pdu = std::move(nasPdu);
        m_logger->info("RRC receive beforehand-HO signal");
        m_base->ngapTask->push(std::move(w));
    }
    else{
        auto w = std::make_unique<NmGnbRrcToNgap>(NmGnbRrcToNgap::HANDOVER_REQUEST);
        w->ueId = ueId;
        // OctetString nasPdu = asn::GetOctetString(*msg.criticalExtensions.choice.measurementReport->measResults);
        // w->pdu = std::move(nasPdu);
        m_logger->info("RRC receive measurement report");
        m_base->ngapTask->push(std::move(w));
    }  
}

void GnbRrcTask::handlePaging(const asn::Unique<ASN_NGAP_FiveG_S_TMSI> &tmsi,
                              const asn::Unique<ASN_NGAP_TAIListForPaging> &taiList)
{
    // Construct and send a Paging message
    auto *pdu = asn::New<ASN_RRC_PCCH_Message>();
    pdu->message.present = ASN_RRC_PCCH_MessageType_PR_c1;
    pdu->message.choice.c1 = asn::NewFor(pdu->message.choice.c1);
    pdu->message.choice.c1->present = ASN_RRC_PCCH_MessageType__c1_PR_paging;
    auto &paging = pdu->message.choice.c1->choice.paging = asn::New<ASN_RRC_Paging>();

    auto *record = asn::New<ASN_RRC_PagingRecord>();
    record->ue_Identity.present = ASN_RRC_PagingUE_Identity_PR_ng_5G_S_TMSI;

    OctetString tmsiOctets{};
    tmsiOctets.appendOctet2(bits::Ranged16({
        {10, asn::GetBitStringInt<10>(tmsi->aMFSetID)},
        {6, asn::GetBitStringInt<10>(tmsi->aMFPointer)},
    }));
    tmsiOctets.append(asn::GetOctetString(tmsi->fiveG_TMSI));

    asn::SetBitString(record->ue_Identity.choice.ng_5G_S_TMSI, tmsiOctets);

    paging->pagingRecordList = asn::NewFor(paging->pagingRecordList);
    asn::SequenceAdd(*paging->pagingRecordList, record);

    sendRrcMessage(pdu);
    asn::Free(asn_DEF_ASN_RRC_PCCH_Message, pdu);
}

} // namespace nr::gnb