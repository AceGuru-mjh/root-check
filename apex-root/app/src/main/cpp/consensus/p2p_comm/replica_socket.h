#ifndef APEX_ROOT_REPLICA_SOCKET_H
#define APEX_ROOT_REPLICA_SOCKET_H

#include "consensus/replica_manager.h"
#include <cstdint>
#include <cstddef>
#include <vector>

namespace apex {
namespace consensus {

namespace replica_socket {

bool init_socket(ReplicaRole role);
bool send_heartbeat(ReplicaRole role);
bool send_vote(ReplicaRole role, const uint8_t* hash);
bool broadcast_result(const uint8_t* data, size_t len);
std::vector<uint8_t> receive_message();
void close_socket();

} // namespace replica_socket
} // namespace consensus
} // namespace apex

#endif // APEX_ROOT_REPLICA_SOCKET_H
