

#include "mm.hpp"

#include <lib/nas/utils.hpp>
#include <ue/app/task.hpp>
#include <ue/nas/task.hpp>
#include <ue/nas/usim/usim.hpp>
#include <ue/rrc/task.hpp>
#include <utils/common.hpp>

namespace nr::ue
{
    void NasMm::handoverEvent()
    {
        sendNasMessage(nas::Handover{});
        m_logger->debug("handoverEvent running");
    }
} // namespace nr::ue
