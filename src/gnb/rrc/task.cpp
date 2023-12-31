//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "task.hpp"

#include <gnb/nts.hpp>
#include <gnb/rls/task.hpp>
#include <lib/rrc/encode.hpp>

#include <asn/rrc/ASN_RRC_DLInformationTransfer-IEs.h>
#include <asn/rrc/ASN_RRC_DLInformationTransfer.h>

static constexpr const int TIMER_ID_SI_BROADCAST = 1;
static constexpr const int TIMER_PERIOD_SI_BROADCAST = 10'000;

namespace nr::gnb
{

GnbRrcTask::GnbRrcTask(TaskBase *base) : m_base{base}, m_ueCtx{}, m_tidCounter{}
{
    m_logger = base->logBase->makeUniqueLogger("rrc");
    m_config = m_base->config;
}

void GnbRrcTask::onStart()
{
    setTimer(TIMER_ID_SI_BROADCAST, TIMER_PERIOD_SI_BROADCAST);
}

void GnbRrcTask::onQuit()
{
    // todo
}

void GnbRrcTask::onLoop()
{
    auto msg = take();
    if (!msg)
        return;

    switch (msg->msgType)
    {
    case NtsMessageType::GNB_RLS_TO_RRC: {
        handleRlsSapMessage(dynamic_cast<NmGnbRlsToRrc &>(*msg));
        break;
    }
    case NtsMessageType::GNB_NGAP_TO_RRC: {
        auto &w = dynamic_cast<NmGnbNgapToRrc &>(*msg);
        switch (w.present)
        {
        case NmGnbNgapToRrc::RADIO_POWER_ON: {
            m_isBarred = false;
            triggerSysInfoBroadcast();
            break;
        }
        case NmGnbNgapToRrc::NAS_DELIVERY: {
            handleDownlinkNasDelivery(w.ueId, w.pdu);
            break;
        }
        case NmGnbNgapToRrc::AN_RELEASE: {
            releaseConnection(w.ueId);
            break;
        }
        case NmGnbNgapToRrc::PAGING:
            handlePaging(w.uePagingTmsi, w.taiListForPaging);
            break;
        
        case NmGnbNgapToRrc::EXCHANGE_RRC: {
            exchangeRRCConnection(w.ueId);
            break;
        }
        }
        break;
    }
    case NtsMessageType::TIMER_EXPIRED: {
        auto w = dynamic_cast<NmTimerExpired &>(*msg);
        if (w.timerId == TIMER_ID_SI_BROADCAST)
        {
            setTimer(TIMER_ID_SI_BROADCAST, TIMER_PERIOD_SI_BROADCAST);
            onBroadcastTimerExpired();
        }
        break;
    }
    default:
        m_logger->unhandledNts(*msg);
        break;
    }

}

} // namespace nr::gnb
