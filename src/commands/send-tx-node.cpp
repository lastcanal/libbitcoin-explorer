/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin-explorer.
 *
 * libbitcoin-explorer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <bitcoin/explorer/commands/send-tx-node.hpp>

#include <cstddef>
#include <cstdint>
#include <csignal>
#include <iostream>
#include <boost/format.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/explorer/async_client.hpp>
#include <bitcoin/explorer/callback_state.hpp>
#include <bitcoin/explorer/define.hpp>
#include <bitcoin/explorer/primitives/transaction.hpp>
#include <bitcoin/explorer/utility.hpp>

using namespace bc;
using namespace bc::explorer;
using namespace bc::explorer::commands;
using namespace bc::explorer::primitives;

static void handle_signal(int)
{
    // Can't pass args using lambda capture for a simple function pointer.
    // This means there's no way to terminate without using a global variable
    // or process termination. Since the variable would screw with testing all 
    // other methods we opt for process termination here.
    exit(console_result::failure);
}

console_result send_tx_node::invoke(std::ostream& output, std::ostream& error)
{
    // Bound parameters.
    const auto& host = get_host_option();
    const auto& port = get_port_option();
    const tx_type& transaction = get_transaction_argument();

    const auto identifier = get_network_identifier_setting();
    const auto retries = get_network_connect_retries_setting();
    const auto connect = get_network_connect_timeout_seconds_setting();
    const auto handshake = get_network_channel_handshake_seconds_setting();
    const auto& hosts_file = get_network_hosts_file_setting();
    const auto& debug_file = get_network_debug_file_setting();
    const auto& error_file = get_network_error_file_setting();

    // TODO: give option to send errors to console vs. file.
    static const auto header = format("=========== %1% ==========") % symbol();
    bc::ofstream debug_log(debug_file.string(), log_open_mode);
    bind_debug_log(debug_log);
    log_debug(LOG_NETWORK) << header;
    bc::ofstream error_log(error_file.string(), log_open_mode);
    bind_error_log(error_log);
    log_error(LOG_NETWORK) << header;

    // Not listening or peering, no relay/port/inbound/seeds/hosts/outbound.
    static constexpr auto relay = false;
    static constexpr uint16_t listen = 0;
    static constexpr size_t inbound = 0;
    static constexpr size_t host_capacity = 0;
    static constexpr size_t outbound = 0;
    static const config::endpoint::list seeds;
    static const auto self = bc::unspecified_network_address;

    static constexpr size_t threads = 2;
    static const network::timeout timeouts(connect, handshake);

    async_client client(threads);
    network::hosts hosts(client.pool(), hosts_file, host_capacity);
    network::connector net(client.pool(), identifier, timeouts);
    network::p2p proto(client.pool(), hosts, net, listen, relay, outbound,
        inbound, seeds, self, timeouts);

    callback_state state(error, output);

    const auto handle_send = [&state](const code& code)
    {
        if (state.succeeded(code))
            state.output(format(BX_SEND_TX_NODE_OUTPUT) % now());

        --state;
    };

    const auto handle_connect = [&state, &transaction, &handle_send](
        const std::error_code& code, network::channel::ptr node)
    {
        if (state.succeeded(code))
            node->send(transaction, handle_send);
    };

    // One node always specified.
    ++state;

    // Handle each successful connection.
    proto.subscribe_channel(handle_connect);

    // No need to start or stop the protocol since we only use manual.
    // Connect to the one specified host and retry up to the specified limit.
    proto.maintain_connection(host, port, relay, retries);

    // Catch C signals for aborting the program.
    signal(SIGABRT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    client.poll(state.stopped());
    client.stop();

    return state.get_result();
}
