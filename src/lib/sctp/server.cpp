//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "server.hpp"
#include "internal.hpp"

sctp::SctpServer::SctpServer(const std::string &address, uint16_t port,sctp::ISctpHandler* newhandler) : sd(0)
{
    handler = newhandler;
    try
    {
        sd = CreateSocket();
        BindSocket(sd, address, port);
        SetInitOptions(sd, 10, 10, 10, 10 * 1000);
        SetEventOptions(sd);
        StartListening(sd);
    }
    catch (const SctpError &e)
    {
        CloseSocket(sd);
        throw;
    }
}



void sctp::SctpServer::loop()
{
    // sctp::ISctpHandler *handler = new nr::gnb::mySctpHandler(nullptr,10);
    ReceiveMessage(clientsd,60,this->handler);

}

sctp::SctpServer::~SctpServer()
{
    CloseSocket(sd);
}

void sctp::SctpServer::start()
{
    printf("---------------start--------------\n");
    thread = std::thread{[this]() {
            printf("thread\n");
            clientsd = MyAccept(sd);
            while (true)
            {
                 this->loop();
            }
        }};
}
