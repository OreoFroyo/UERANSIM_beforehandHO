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
#include <ue/nts.hpp>
#include <asn/rrc/ASN_RRC_MeasurementReport.h>
#include <asn/rrc/ASN_RRC_MeasurementReport-IEs.h>
#include <asn/rrc/ASN_RRC_DLInformationTransfer-IEs.h>
#include <asn/rrc/ASN_RRC_DLInformationTransfer.h>
#include <asn/rrc/ASN_RRC_ULInformationTransfer-IEs.h>
#include <asn/rrc/ASN_RRC_ULInformationTransfer.h>
#include <asn/rrc/ASN_RRC_MeasResults.h>
#include <asn/rrc/ASN_RRC_MeasResultListEUTRA.h>
#include <asn/rrc/ASN_RRC_MeasResultListNR.h>
#include <asn/rrc/ASN_RRC_MeasResultServMO.h>
#include <asn/rrc/ASN_RRC_MeasResultNR.h>
namespace nr::ue
{

void UeRrcTask::deliverMeasurementPeport(uint32_t pduId, OctetString &&nasPdu)
{
    if (!m_base->shCtx.currentCell.get<bool>([](auto &value) { return value.hasValue(); }))
    {
        m_logger->err("Uplink NAS delivery failed. No active cell");
        return;
    }

    if (nasPdu.length() == 0)
        return;

    if (m_state == ERrcState::RRC_IDLE)
    {
        startConnectionEstablishment(std::move(nasPdu));
        return;
    }
    else if (m_state == ERrcState::RRC_INACTIVE)
    {
        // TODO
        return;
    }
    auto *pdu = asn::New<ASN_RRC_UL_DCCH_Message>();
    pdu->message.present = ASN_RRC_UL_DCCH_MessageType_PR_c1;
    pdu->message.choice.c1 =
        asn::New<ASN_RRC_UL_DCCH_MessageType::ASN_RRC_UL_DCCH_MessageType_u::ASN_RRC_UL_DCCH_MessageType__c1>();
    pdu->message.choice.c1->present = ASN_RRC_UL_DCCH_MessageType__c1_PR_measurementReport;
    pdu->message.choice.c1->choice.measurementReport = asn::New<ASN_RRC_MeasurementReport>();

    auto &c1 = pdu->message.choice.c1->choice.measurementReport->criticalExtensions;
    c1.present = ASN_RRC_MeasurementReport__criticalExtensions_PR_measurementReport;
    c1.choice.measurementReport = asn::New<ASN_RRC_MeasurementReport_IEs>();
    //c1.choice.measurementReport->measResults = asn::New<ASN_RRC_MeasResults_t>();
    c1.choice.measurementReport->measResults.measId = 1;
    c1.choice.measurementReport->measResults.measResultNeighCells = asn::New<ASN_RRC_MeasResults::ASN_RRC_MeasResults__measResultNeighCells>();
    //c1.choice.measurementReport->measResults.measResultNeighCells->choice.measResultListEUTRA = asn::New<ASN_RRC_MeasResultListEUTRA>();
    c1.choice.measurementReport->measResults.measResultNeighCells->choice.measResultListNR = asn::New<ASN_RRC_MeasResultListNR>();
    c1.choice.measurementReport->measResults.measResultNeighCells->present = ASN_RRC_MeasResults__measResultNeighCells_PR_measResultListNR;
    c1.choice.measurementReport->measResults.measResultServingMOList = *asn::New<ASN_RRC_MeasResultServMOList_t>();
    ASN_RRC_MeasResultServMO *measResultServMO1 = asn::New<ASN_RRC_MeasResultServMO>();
    measResultServMO1->servCellId = 2;
    measResultServMO1->measResultServingCell = *asn::New<ASN_RRC_MeasResultNR_t>();
    measResultServMO1->measResultServingCell.measResult = *asn::New<ASN_RRC_MeasResultNR::ASN_RRC_MeasResultNR__measResult>();
    measResultServMO1->measResultServingCell.measResult.cellResults = *asn::New<ASN_RRC_MeasResultNR::ASN_RRC_MeasResultNR__measResult::ASN_RRC_MeasResultNR__measResult__cellResults>();
    measResultServMO1->measResultServingCell.measResult.rsIndexResults = asn::New<ASN_RRC_MeasResultNR::ASN_RRC_MeasResultNR__measResult::ASN_RRC_MeasResultNR__measResult__rsIndexResults>();
    measResultServMO1->measResultServingCell.ext1 = asn::New<ASN_RRC_MeasResultNR::ASN_RRC_MeasResultNR__ext1>();
    ASN_RRC_MeasResultNR_t *nrElement = asn::New<ASN_RRC_MeasResultNR_t>();
    nrElement->measResult = *asn::New<ASN_RRC_MeasResultNR::ASN_RRC_MeasResultNR__measResult>();
    nrElement->measResult.cellResults = *asn::New<ASN_RRC_MeasResultNR::ASN_RRC_MeasResultNR__measResult::ASN_RRC_MeasResultNR__measResult__cellResults>();
    nrElement->measResult.rsIndexResults = asn::New<ASN_RRC_MeasResultNR::ASN_RRC_MeasResultNR__measResult::ASN_RRC_MeasResultNR__measResult__rsIndexResults>();
    nrElement->ext1 = asn::New<ASN_RRC_MeasResultNR::ASN_RRC_MeasResultNR__ext1>();
    asn::SequenceAdd(c1.choice.measurementReport->measResults.measResultServingMOList,measResultServMO1);
    asn::SequenceAdd(*c1.choice.measurementReport->measResults.measResultNeighCells->choice.measResultListNR,nrElement);
    //asn::SetOctetString(*c1.choice.measurementReport->measResults, nasPdu);

    sendRrcMessage(pdu);
    asn::Free(asn_DEF_ASN_RRC_UL_DCCH_Message, pdu);
}

void UeRrcTask::deliverUplinkNas(uint32_t pduId, OctetString &&nasPdu)
{
    if (!m_base->shCtx.currentCell.get<bool>([](auto &value) { return value.hasValue(); }))
    {
        m_logger->err("Uplink NAS delivery failed. No active cell");
        return;
    }

    if (nasPdu.length() == 0)
        return;

    if (m_state == ERrcState::RRC_IDLE)
    {
        startConnectionEstablishment(std::move(nasPdu));
        return;
    }
    else if (m_state == ERrcState::RRC_INACTIVE)
    {
        // TODO
        return;
    }

    auto *pdu = asn::New<ASN_RRC_UL_DCCH_Message>();
    pdu->message.present = ASN_RRC_UL_DCCH_MessageType_PR_c1;
    pdu->message.choice.c1 =
        asn::New<ASN_RRC_UL_DCCH_MessageType::ASN_RRC_UL_DCCH_MessageType_u::ASN_RRC_UL_DCCH_MessageType__c1>();
    pdu->message.choice.c1->present = ASN_RRC_UL_DCCH_MessageType__c1_PR_ulInformationTransfer;
    pdu->message.choice.c1->choice.ulInformationTransfer = asn::New<ASN_RRC_ULInformationTransfer>();

    auto &c1 = pdu->message.choice.c1->choice.ulInformationTransfer->criticalExtensions;
    c1.present = ASN_RRC_ULInformationTransfer__criticalExtensions_PR_ulInformationTransfer;
    c1.choice.ulInformationTransfer = asn::New<ASN_RRC_ULInformationTransfer_IEs>();
    c1.choice.ulInformationTransfer->dedicatedNAS_Message = asn::New<ASN_RRC_DedicatedNAS_Message_t>();

    asn::SetOctetString(*c1.choice.ulInformationTransfer->dedicatedNAS_Message, nasPdu);

    sendRrcMessage(pdu);
    asn::Free(asn_DEF_ASN_RRC_UL_DCCH_Message, pdu);
}

void UeRrcTask::receiveDownlinkInformationTransfer(const ASN_RRC_DLInformationTransfer &msg)
{
    OctetString nasPdu =
        asn::GetOctetString(*msg.criticalExtensions.choice.dlInformationTransfer->dedicatedNAS_Message);

    auto m = std::make_unique<NmUeRrcToNas>(NmUeRrcToNas::NAS_DELIVERY);
    m->nasPdu = std::move(nasPdu);
    m_base->nasTask->push(std::move(m));
}

} // namespace nr::ue