//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "gnb.hpp"
#include "app/task.hpp"
#include "gtp/task.hpp"
#include "ngap/task.hpp"
#include "rls/task.hpp"
#include "rrc/task.hpp"
#include "sctp/task.hpp"
#include "sctp/server.hpp"
#include <lib/sctp/server.hpp>
#include <lib/app/cli_base.hpp>

namespace nr::gnb
{

GNodeB::GNodeB(GnbConfig *config, app::INodeListener *nodeListener, NtsTask *cliCallbackTask)
{
    auto *base = new TaskBase();
    base->config = config;
    base->logBase = new LogBase("logs/" + config->name + ".log");
    base->nodeListener = nodeListener;
    base->cliCallbackTask = cliCallbackTask;

    base->appTask = new GnbAppTask(base);
    base->sctpTask = new SctpTask(base);
    base->ngapTask = new NgapTask(base);
    base->rrcTask = new GnbRrcTask(base);
    base->gtpTask = new GtpTask(base);
    base->rlsTask = new GnbRlsTask(base);
    
    base->sctpServer = new GnbSctpServer(base,config->ngapIp,1478,10);
    base->sctpServer2 = new GnbSctpServer(base,config->ngapIp,1479,11);
    base->sctpServer3 = new GnbSctpServer(base,config->ngapIp,1480,12);
    base->sctpServer4 = new GnbSctpServer(base,config->ngapIp,1481,13);

    base->sctpServer5 = new GnbSctpServer(base,config->ngapIp,1482,14);
    base->sctpServer6 = new GnbSctpServer(base,config->ngapIp,1483,15);
    base->sctpServer7 = new GnbSctpServer(base,config->ngapIp,1484,16);
    base->sctpServer8 = new GnbSctpServer(base,config->ngapIp,1485,17);
    base->sctpServer9 = new GnbSctpServer(base,config->ngapIp,1486,18);
    taskBase = base;
}

GNodeB::~GNodeB()
{
    taskBase->appTask->quit();
    taskBase->sctpTask->quit();
    taskBase->ngapTask->quit();
    taskBase->rrcTask->quit();
    taskBase->gtpTask->quit();
    taskBase->rlsTask->quit();

    delete taskBase->appTask;
    delete taskBase->sctpTask;
    delete taskBase->ngapTask;
    delete taskBase->rrcTask;
    delete taskBase->gtpTask;
    delete taskBase->rlsTask;

    delete taskBase->logBase;

    delete taskBase;
}

void GNodeB::start()
{
    taskBase->appTask->start();
    taskBase->sctpTask->start();
    taskBase->ngapTask->start();
    taskBase->rrcTask->start();
    taskBase->rlsTask->start();
    taskBase->gtpTask->start();
    taskBase->sctpServer->start();
    taskBase->sctpServer2->start();
    taskBase->sctpServer3->start();
    taskBase->sctpServer4->start();

    taskBase->sctpServer5->start();
    taskBase->sctpServer6->start();
    taskBase->sctpServer7->start();
    taskBase->sctpServer8->start();
    taskBase->sctpServer9->start();
}

void GNodeB::pushCommand(std::unique_ptr<app::GnbCliCommand> cmd, const InetAddress &address)
{
    taskBase->appTask->push(std::make_unique<NmGnbCliCommand>(std::move(cmd), address));
}

} // namespace nr::gnb
