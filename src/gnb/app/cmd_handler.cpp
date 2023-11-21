//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "cmd_handler.hpp"

#include <gnb/app/task.hpp>
#include <gnb/gtp/task.hpp>
#include <gnb/ngap/task.hpp>
#include <gnb/rls/task.hpp>
#include <gnb/rrc/task.hpp>
#include <gnb/sctp/task.hpp>
#include <gnb/sctp/server.hpp>
#include <utils/common.hpp>
#include <utils/printer.hpp>

#define PAUSE_CONFIRM_TIMEOUT 3000
#define PAUSE_POLLING 10

namespace nr::gnb
{

void GnbCmdHandler::sendResult(const InetAddress &address, const std::string &output)
{
    m_base->cliCallbackTask->push(std::make_unique<app::NwCliSendResponse>(address, output, false));
}

void GnbCmdHandler::sendError(const InetAddress &address, const std::string &output)
{
    m_base->cliCallbackTask->push(std::make_unique<app::NwCliSendResponse>(address, output, true));
}

void GnbCmdHandler::pauseTasks()
{
    m_base->gtpTask->requestPause();
    m_base->rlsTask->requestPause();
    m_base->ngapTask->requestPause();
    m_base->rrcTask->requestPause();
    m_base->sctpTask->requestPause();
}

void GnbCmdHandler::unpauseTasks()
{
    m_base->gtpTask->requestUnpause();
    m_base->rlsTask->requestUnpause();
    m_base->ngapTask->requestUnpause();
    m_base->rrcTask->requestUnpause();
    m_base->sctpTask->requestUnpause();
}

bool GnbCmdHandler::isAllPaused()
{
    if (!m_base->gtpTask->isPauseConfirmed())
        return false;
    if (!m_base->rlsTask->isPauseConfirmed())
        return false;
    if (!m_base->ngapTask->isPauseConfirmed())
        return false;
    if (!m_base->rrcTask->isPauseConfirmed())
        return false;
    if (!m_base->sctpTask->isPauseConfirmed())
        return false;
    return true;
}

void GnbCmdHandler::handleCmd(NmGnbCliCommand &msg)
{
    pauseTasks();

    uint64_t currentTime = utils::CurrentTimeMillis();
    uint64_t endTime = currentTime + PAUSE_CONFIRM_TIMEOUT;

    bool isPaused = false;
    while (currentTime < endTime)
    {
        currentTime = utils::CurrentTimeMillis();
        if (isAllPaused())
        {
            isPaused = true;
            break;
        }
        utils::Sleep(PAUSE_POLLING);
    }

    if (!isPaused)
    {
        sendError(msg.address, "gNB is unable process command due to pausing timeout");
    }
    else
    {
        handleCmdImpl(msg);
    }

    unpauseTasks();
}

void GnbCmdHandler::handleCmdImpl(NmGnbCliCommand &msg)
{
    switch (msg.cmd->present)
    {
    case app::GnbCliCommand::STATUS: {
        sendResult(msg.address, ToJson(m_base->appTask->m_statusInfo).dumpYaml());
        break;
    }
    case app::GnbCliCommand::INFO: {
        sendResult(msg.address, ToJson(*m_base->config).dumpYaml());
        break;
    }
    case app::GnbCliCommand::AMF_LIST: {
        Json json = Json::Arr({});
        for (auto &amf : m_base->ngapTask->m_amfCtx)
            json.push(Json::Obj({{"id", amf.first}}));
        sendResult(msg.address, json.dumpYaml());
        break;
    }
    case app::GnbCliCommand::AMF_INFO: {
        if (m_base->ngapTask->m_amfCtx.count(msg.cmd->amfId) == 0)
            sendError(msg.address, "AMF not found with given ID");
        else
        {
            auto amf = m_base->ngapTask->m_amfCtx[msg.cmd->amfId];
            sendResult(msg.address, ToJson(*amf).dumpYaml());
        }
        break;
    }
    case app::GnbCliCommand::UE_LIST: {
        Json json = Json::Arr({});
        for (auto &ue : m_base->ngapTask->m_ueCtx)
        {
            json.push(Json::Obj({
                {"ue-id", ue.first},
                {"ran-ngap-id", ue.second->ranUeNgapId},
                {"amf-ngap-id", ue.second->amfUeNgapId},
            }));
        }
        sendResult(msg.address, json.dumpYaml());
        break;
    }
    case app::GnbCliCommand::UE_COUNT: {
        sendResult(msg.address, std::to_string(m_base->ngapTask->m_ueCtx.size()));
        break;
    }
    case app::GnbCliCommand::UE_RELEASE_REQ: {
        if (m_base->ngapTask->m_ueCtx.count(msg.cmd->ueId) == 0)
            sendError(msg.address, "UE not found with given ID");
        else
        {
            auto ue = m_base->ngapTask->m_ueCtx[msg.cmd->ueId];
            m_base->ngapTask->sendContextRelease(ue->ctxId, NgapCause::RadioNetwork_unspecified);
            sendResult(msg.address, "Requesting UE context release");
        }
        break;
    }
    case app::GnbCliCommand::XNAP_CONNECTION: {
        auto msg1 = std::make_unique<NmGnbSctp>(NmGnbSctp::GNB_CONNECTION_REQUEST);
        msg1->clientId = 10;//amfCtx.second->ctxId; 
        msg1->localAddress = m_base->config->ngapIp;
        msg1->localPort = 0;
        msg1->remoteAddress = msg.cmd->gnbIp;
        std::string octet;
        int octetValue = 0;

        for (int i = 0; i < 4; i++) {
            octet.clear();
            size_t pos = msg1->remoteAddress.find('.');
            
            if (pos != std::string::npos) {
                octet = msg1->remoteAddress.substr(0, pos);
                msg1->remoteAddress = msg1->remoteAddress.substr(pos + 1);
            } else {
                octet = msg1->remoteAddress;
            }
            
            // Convert the octet string to an integer and store it in target_ip
            octetValue = std::stoi(octet);
            m_base->sctpServer->target_ip[i] = octetValue;
        }

        msg1->remoteAddress = msg.cmd->gnbIp;

        msg1->remotePort = std::atoi(msg.cmd->port.c_str());
        msg1->ppid = sctp::PayloadProtocolId::NGAP;
        msg1->associatedTask = m_base->ngapTask; 
        m_base->sctpTask->push(std::move(msg1));
        sendResult(msg.address,"XnAP Connection Setup Successfully");
        break;
    }
    case app::GnbCliCommand::PATH_SWITCH_REQ: {
        if (m_base->ngapTask->m_ueCtx.count(msg.cmd->ueId) == 0)
            sendError(msg.address, "UE not found with given ID");
        else
        {
            auto ue = m_base->ngapTask->m_ueCtx[msg.cmd->ueId];
            auto w = std::make_unique<NmGnbNgapToRrc>(NmGnbNgapToRrc::EXCHANGE_RRC);
            w->ueId = ue->ctxId;
            //m_base->rrcTask->push(std::move(w));
            m_base->ngapTask->sendPathSwitchRequest(ue->ctxId);
            sendResult(msg.address, "Requesting Path Switch Request");
            //sendResult(msg.address, "Send rrc reconfiguration to ue");
        }
        break;
    }
    case app::GnbCliCommand::BEFOREHAND_HANDOVER: {
        auto w = std::make_unique<NmGnbRrcToNgap>(NmGnbRrcToNgap::BEFOREHAND_HANDOVER);
        auto ue = m_base->ngapTask->m_ueCtx[msg.cmd->ueId];
        w->ueId = ue->ctxId;
        m_base->ngapTask->push(std::move(w));
        sendResult(msg.address,"Beforehand Handover Message Send Successfully");
        break;
    }
    }
}

} // namespace nr::gnb