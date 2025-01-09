//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "task.hpp"

#include <utils/common.hpp>

namespace nr::gnb
{

NgapAmfContext *NgapTask::findAmfContext(int ctxId)
{
    NgapAmfContext *ctx = nullptr;
    if (m_amfCtx.count(ctxId))
        ctx = m_amfCtx[ctxId];
    if (ctx == nullptr)
        m_logger->err("AMF context not found with id: %d", ctxId);
    return ctx;
}

void NgapTask::createAmfContext(const GnbAmfConfig &conf)
{
    auto *ctx = new NgapAmfContext();
    ctx->ctxId = utils::NextId();
    ctx->state = EAmfState::NOT_CONNECTED;
    ctx->address = conf.address;
    ctx->port = conf.port;
    m_amfCtx[ctx->ctxId] = ctx;
}

void NgapTask::createUeContext(int ueId)
{
    auto *ctx = new NgapUeContext(ueId);
    ctx->amfUeNgapId = -1;
    ctx->ranUeNgapId = ++m_ueNgapIdCounter;

    m_ueCtx[ctx->ctxId] = ctx;

    // Perform AMF selection
    auto *amf = selectAmf(ueId);
    if (amf == nullptr)
        m_logger->err("AMF selection for UE[%d] failed. Could not find a suitable AMF.", ueId);
    else
        ctx->associatedAmfId = amf->ctxId;
}

NgapUeContext *NgapTask::findUeContext(int ctxId)
{
    NgapUeContext *ctx = nullptr;
    if (m_ueCtx.count(ctxId))
        ctx = m_ueCtx[ctxId];
    if (ctx == nullptr)
        m_logger->err("UE context not found with id: %d", ctxId);
    return ctx;
}

NgapUeContext *NgapTask::findUeByRanId(int64_t ranUeNgapId)
{
    if (ranUeNgapId <= 0)
        return nullptr;
    // TODO: optimize
    for (auto &ue : m_ueCtx)
        if (ue.second->ranUeNgapId == ranUeNgapId)
            return ue.second;
    return nullptr;
}

NgapUeContext *NgapTask::findUeByAmfId(int64_t amfUeNgapId)
{
    if (amfUeNgapId <= 0)
        return nullptr;
    // TODO: optimize
    for (auto &ue : m_ueCtx)
        if (ue.second->amfUeNgapId == amfUeNgapId)
            return ue.second;
    return nullptr;
}

NgapUeContext *NgapTask::findUeByNgapIdPair(int amfCtxId, const NgapIdPair &idPair)
{
    auto &amfId = idPair.amfUeNgapId;
    auto &ranId = idPair.ranUeNgapId;
    m_logger->info("amf info:%d",amfCtxId);
    if (!amfId.has_value() && !ranId.has_value())
    {
        sendErrorIndication(amfCtxId, NgapCause::Protocol_abstract_syntax_error_falsely_constructed_message);
        return nullptr;
    }
m_logger->info("amf info2:%d",amfCtxId);
    if (!amfId.has_value())
    {
        auto ue = findUeByRanId(ranId.value());
        if (ue == nullptr)
        {
            sendErrorIndication(amfCtxId, NgapCause::RadioNetwork_unknown_local_UE_NGAP_ID);
            return nullptr;
        }

        return ue;
    }
m_logger->info("amf info3:%d",amfCtxId);
    if (!ranId.has_value())
    {
        auto ue = findUeByAmfId(amfId.value());
        if (ue == nullptr)
        {
            sendErrorIndication(amfCtxId, NgapCause::RadioNetwork_inconsistent_remote_UE_NGAP_ID);
            return nullptr;
        }

        return ue;
    }
m_logger->info("amf info4:%d",amfCtxId);
    auto ue = findUeByRanId(ranId.value());
    if (ue == nullptr)
    {
        sendErrorIndication(amfCtxId, NgapCause::RadioNetwork_unknown_local_UE_NGAP_ID);
        return nullptr;
    }
m_logger->info("amf info5:%d",amfCtxId);
    if (ue->amfUeNgapId == -1)
        ue->amfUeNgapId = amfId.value();
    else if (ue->amfUeNgapId != amfId.value())
    {
        sendErrorIndication(amfCtxId, NgapCause::RadioNetwork_inconsistent_remote_UE_NGAP_ID);
        return nullptr;
    }

    return ue;
}

void NgapTask::deleteUeContext(int ueId)
{
    auto *ue = m_ueCtx[ueId];
    if (ue)
    {
	m_logger->debug("UE delete find :%d",ueId);
        delete ue;
        m_ueCtx.erase(ueId);
    }
    else{
	m_logger->debug("UE delete not find :%d",ueId);
    }
}

void NgapTask::deleteAmfContext(int amfId)
{
    auto *amf = m_amfCtx[amfId];
    if (amf)
    {
        delete amf;
        m_amfCtx.erase(amfId);
    }
}

} // namespace nr::gnb
