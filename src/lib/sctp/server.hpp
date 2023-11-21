//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#pragma once

#include <string>
#include <thread>
#include "types.hpp"
namespace sctp
{

class SctpServer
{
  private:
    int sd;
    int clientsd;
    std::thread thread;
    sctp::ISctpHandler* handler;

  public:
    SctpServer(const std::string &address, uint16_t port,sctp::ISctpHandler* newhandler);
    ~SctpServer();
    void loop();
    void start();    
    // TODO: Other functionalities
};

} // namespace sctp