//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "task.hpp"

#include <lib/rrc/encode.hpp>
#include <ue/nas/task.hpp>
#include <ue/rls/task.hpp>
#include <ue/nts.hpp>
#include <utils/random.hpp>

#include <asn/rrc/ASN_RRC_RRCSetup-IEs.h>
#include <asn/rrc/ASN_RRC_RRCSetup.h>
#include <asn/rrc/ASN_RRC_RRCSetupComplete-IEs.h>
#include <asn/rrc/ASN_RRC_RRCSetupComplete.h>
#include <asn/rrc/ASN_RRC_RRCSetupRequest-IEs.h>
#include <asn/rrc/ASN_RRC_RRCSetupRequest.h>
#include <asn/rrc/ASN_RRC_RRCReconfigurationComplete.h>
#include <asn/rrc/ASN_RRC_RRCReconfiguration.h>
#include <asn/rrc/ASN_RRC_RRCReconfigurationComplete-IEs.h>

namespace nr::ue
{

static ASN_RRC_UL_CCCH_Message *ConstructSetupRequest(ASN_RRC_InitialUE_Identity_t initialUeId,
                                                      ASN_RRC_EstablishmentCause_t establishmentCause)
{
    auto *pdu = asn::New<ASN_RRC_UL_CCCH_Message>();
    pdu->message.present = ASN_RRC_UL_CCCH_MessageType_PR_c1;
    pdu->message.choice.c1 = asn::NewFor(pdu->message.choice.c1);
    pdu->message.choice.c1->present = ASN_RRC_UL_CCCH_MessageType__c1_PR_rrcSetupRequest;

    auto &r = pdu->message.choice.c1->choice.rrcSetupRequest = asn::New<ASN_RRC_RRCSetupRequest>();
    asn::DeepCopy(asn_DEF_ASN_RRC_InitialUE_Identity, initialUeId, &r->rrcSetupRequest.ue_Identity);
    r->rrcSetupRequest.establishmentCause = establishmentCause;
    asn::SetSpareBits<1>(r->rrcSetupRequest.spare);

    return pdu;
}

void UeRrcTask::startConnectionEstablishment(OctetString &&nasPdu)
{
    /* Check the protocol state */
    if (m_state != ERrcState::RRC_IDLE)
    {
        m_logger->err("RRC establishment could not start, UE not in RRC-IDLE state");
        handleEstablishmentFailure();
        return;
    }

    /* Check the current cell */
    int activeCell = m_base->shCtx.currentCell.get<int>([](auto &item) { return item.cellId; });
    if (activeCell == 0)
    {
        m_logger->err("RRC establishment could not start, no active cell");
        handleEstablishmentFailure();
        return;
    }

    /* Handle Initial UE Identity (S-TMSI or 39-bit random value) */
    std::optional<GutiMobileIdentity> gutiOrTmsi = m_base->shCtx.providedGuti.get();
    if (!gutiOrTmsi)
        gutiOrTmsi = m_base->shCtx.providedTmsi.get();

    if (gutiOrTmsi)
    {
        m_initialId.present = ASN_RRC_InitialUE_Identity_PR_ng_5G_S_TMSI_Part1;
        asn::SetBitStringLong<39>(static_cast<int64_t>(gutiOrTmsi->tmsi) |
                                      (static_cast<int64_t>(gutiOrTmsi->amfPointer & 0b1111111) << 32ull),
                                  m_initialId.choice.ng_5G_S_TMSI_Part1);
    }
    else
    {
        m_initialId.present = ASN_RRC_InitialUE_Identity_PR_randomValue;
        asn::SetBitStringLong<39>(Random::Mixed(m_base->config->getNodeName()).nextL(), m_initialId.choice.randomValue);
    }

    /* Set the Initial NAS PDU */
    m_initialNasPdu = std::move(nasPdu);

    /* Send the message */
    m_logger->debug("Sending RRC Setup Request");

    auto *rrcSetupRequest =
        ConstructSetupRequest(m_initialId, static_cast<ASN_RRC_EstablishmentCause_t>(m_establishmentCause));
    sendRrcMessage(activeCell, rrcSetupRequest);
    asn::Free(asn_DEF_ASN_RRC_UL_CCCH_Message, rrcSetupRequest);
}

void UeRrcTask::receiveRrcSetup(int cellId, const ASN_RRC_RRCSetup &msg)
{
    if (!isActiveCell(cellId))
        return;

    if (m_lastSetupReq != ERrcLastSetupRequest::SETUP_REQUEST)
    {
        // TODO
        return;
    }

    auto *pdu = asn::New<ASN_RRC_UL_DCCH_Message>();
    pdu->message.present = ASN_RRC_UL_DCCH_MessageType_PR_c1;
    pdu->message.choice.c1 = asn::NewFor(pdu->message.choice.c1);
    pdu->message.choice.c1->present = ASN_RRC_UL_DCCH_MessageType__c1_PR_rrcSetupComplete;

    auto &setupComplete = pdu->message.choice.c1->choice.rrcSetupComplete = asn::New<ASN_RRC_RRCSetupComplete>();
    setupComplete->rrc_TransactionIdentifier = msg.rrc_TransactionIdentifier;
    setupComplete->criticalExtensions.present = ASN_RRC_RRCSetupComplete__criticalExtensions_PR_rrcSetupComplete;

    auto &ies = setupComplete->criticalExtensions.choice.rrcSetupComplete = asn::New<ASN_RRC_RRCSetupComplete_IEs>();
    ies->selectedPLMN_Identity = 1;
    asn::SetOctetString(ies->dedicatedNAS_Message, m_initialNasPdu);

    /* Send S-TMSI if available */
    std::optional<GutiMobileIdentity> gutiOrTmsi = m_base->shCtx.providedGuti.get();
    if (!gutiOrTmsi)
        gutiOrTmsi = m_base->shCtx.providedTmsi.get();
    if (gutiOrTmsi)
    {
        auto &sTmsi = setupComplete->criticalExtensions.choice.rrcSetupComplete->ng_5G_S_TMSI_Value =
            asn::New<ASN_RRC_RRCSetupComplete_IEs::ASN_RRC_RRCSetupComplete_IEs__ng_5G_S_TMSI_Value>();
        sTmsi->present = ASN_RRC_RRCSetupComplete_IEs__ng_5G_S_TMSI_Value_PR_ng_5G_S_TMSI;
        asn::SetBitStringLong<48>(gutiOrTmsi->toTmsiValue(), sTmsi->choice.ng_5G_S_TMSI);
    }

    m_initialNasPdu = {};
    sendRrcMessage(pdu);
    asn::Free(asn_DEF_ASN_RRC_UL_DCCH_Message, pdu);

    m_logger->info("RRC connection established");
    switchState(ERrcState::RRC_CONNECTED);
    m_base->nasTask->push(std::make_unique<NmUeRrcToNas>(NmUeRrcToNas::RRC_CONNECTION_SETUP));
}

void UeRrcTask::receiveRrcReject(int cellId, const ASN_RRC_RRCReject &msg)
{
    if (!isActiveCell(cellId))
        return;

    m_logger->err("RRC Reject received");

    handleEstablishmentFailure();
}

void UeRrcTask::receiveRrcRelease(const ASN_RRC_RRCRelease &msg)
{
    m_logger->debug("RRC Release received");
    m_state = ERrcState::RRC_IDLE;
    m_base->nasTask->push(std::make_unique<NmUeRrcToNas>(NmUeRrcToNas::RRC_CONNECTION_RELEASE));
}

void UeRrcTask::receiveRrcReconfiguration(const ASN_RRC_RRCReconfiguration &msg)
{
    m_logger->debug("RRC RrcReconfiguration received");
    OCTET_STRING_t uesti_buf = msg.choice.criticalExtensionsFuture.rrcReconfiguration.secondaryCellGroup;
    //todo:下一步改这里 
    uint64_t uesti;
    if (uesti_buf.size != 4){
        m_logger->info("no enoughlength for ip, size is %d",ip.size);
        return;
    } else {
        uesti = uesti_buf.buf[0]<<24+uesti_buf.buf[1]<<16+uesti_buf.buf[2]<<8+uesti_buf.buf[3];
    }
    // if (uesti1!=NULL && uesti2 !=NULL) {
    //     uesti1c = uesti1->valueint;
    //     uesti2c = uesti2->valueint;
    //     uesti = uesti1c << 32 + uesti2c;
    // } else {
    //     printf("canot read uesti!!!\n");
    //     return 
    // }  
    // auto w1 = std::make_unique<NmGnbRrcToNgap>(NmGnbRrcToNgap::SEND_FAKE_PATHSWITCH);

    int cellId = m_base->rlsTask->getudp()->findCell(uesti);
    auto & lastCell = m_cellDesc[cellId];
    // if (cellId == 1){
    //     cellId = 2;
    // }else{
    //     cellId = 1;
    // }
    ActiveCellInfo cellInfo = {};
    auto & nextcell = m_cellDesc[cellId];
    cellInfo.cellId = cellId;
    cellInfo.plmn = nextcell.sib1.plmn;
    cellInfo.tac = nextcell.sib1.tac;
    cellInfo.category = ECellCategory::SUITABLE_CELL;

    m_base->shCtx.currentCell.set(cellInfo);
    // 更新了当前的cellinfo
    auto w1 = std::make_unique<NmUeRrcToRls>(NmUeRrcToRls::ASSIGN_CURRENT_CELL);
    w1->cellId = cellId;
    m_base->rlsTask->push(std::move(w1));

    auto w2 = std::make_unique<NmUeRrcToNas>(NmUeRrcToNas::ACTIVE_CELL_CHANGED);
    w2->previousTai = Tai{lastCell.sib1.plmn, lastCell.sib1.tac};
    m_base->nasTask->push(std::move(w2));
    //通知了一些人
     // m_state = ERrcState::RRC_IDLE;
    auto *pdu = asn::New<ASN_RRC_UL_DCCH_Message>();
    pdu->message.present = ASN_RRC_UL_DCCH_MessageType_PR_c1;
    pdu->message.choice.c1 = asn::NewFor(pdu->message.choice.c1);
    pdu->message.choice.c1->present = ASN_RRC_UL_DCCH_MessageType__c1_PR_rrcReconfigurationComplete;

    auto &reconfigComplete = pdu->message.choice.c1->choice.rrcReconfigurationComplete = asn::New<ASN_RRC_RRCReconfigurationComplete>();
    reconfigComplete->rrc_TransactionIdentifier = msg.rrc_TransactionIdentifier;
    reconfigComplete->criticalExtensions.present = ASN_RRC_RRCReconfigurationComplete__criticalExtensions_PR_rrcReconfigurationComplete;

    auto &ies = reconfigComplete->criticalExtensions.choice.rrcReconfigurationComplete = asn::New<ASN_RRC_RRCReconfigurationComplete_IEs>();

    sendRrcMessage(pdu);
    //送回去了
    m_base->nasTask->push(std::make_unique<NmUeRrcToNas>(NmUeRrcToNas::RRC_CONNECTION_RECONFIGURATION)); //没处理感觉
}



void UeRrcTask::handleEstablishmentFailure()
{
    m_base->nasTask->push(std::make_unique<NmUeRrcToNas>(NmUeRrcToNas::RRC_ESTABLISHMENT_FAILURE));
}

} // namespace nr::ue
