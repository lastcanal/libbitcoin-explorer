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
#include <bitcoin/explorer/commands/help.hpp>

#include <iostream>
#include <map>
#include <bitcoin/explorer/prop_tree.hpp>
#include <bitcoin/explorer/utility.hpp>

using namespace bc::explorer;
using namespace bc::explorer::commands;
using namespace bc::explorer::primitives;
using namespace pt;

console_result settings::invoke(std::ostream& output, std::ostream& error)
{
    // bound parameters
    const auto& encoding = get_format_option();

    // TODO: look into serializer object quoting.
    // TODO: load from metadata into settings list.

    // This must be updated for any settings metadata change.
    settings_list list;

    // [wallet]
    list["wallet.wif_version"] =
        serialize(get_wallet_wif_version_setting());
    list["wallet.hd_public_version"] =
        serialize(get_wallet_hd_public_version_setting());
    list["wallet.hd_secret_version"] =
        serialize(get_wallet_hd_secret_version_setting());
    list["wallet.pay_to_public_key_hash_version"] =
        serialize(get_wallet_pay_to_public_key_hash_version_setting());
    list["wallet.pay_to_script_hash_version"] =
        serialize(get_wallet_pay_to_script_hash_version_setting());
    list["wallet.transaction_version"] =
        serialize(get_wallet_transaction_version_setting());

    // [network]
    list["network.identifier"] =
        serialize(get_network_identifier_setting());
    list["network.connect_retries"] =
        serialize(get_network_connect_retries_setting());
    list["network.connect_timeout_seconds"] = 
        serialize(get_network_connect_timeout_seconds_setting());
    list["network.channel_handshake_seconds"] =
        serialize(get_network_channel_handshake_seconds_setting());
    list["network.hosts_file"] =
        get_network_hosts_file_setting().string();
    list["network.debug_file"] = 
        get_network_debug_file_setting().string();
    list["network.error_file"] = 
        get_network_error_file_setting().string();

    const auto& nodes = get_network_seeds_setting();
    const auto& seeds = nodes.empty() ? network::session_seed::mainnet : nodes;

    std::vector<std::string> buffer;
    for (const auto& seed: seeds)
    {
        buffer.push_back(seed.to_string());
    }

    list["network.seeds"] = join(buffer, ",");

    // [server]
    list["server.url"] =
        get_server_url_setting().to_string();
    list["server.connect_retries"] =
        serialize(get_server_connect_retries_setting());
    list["server.connect_timeout_seconds"] =
        serialize(get_server_connect_timeout_seconds_setting());
    list["server.server_cert_key"] =
        get_server_server_cert_key_setting().get_base85();
    list["server.cert_file"] = 
        get_server_cert_file_setting().string();

    write_stream(output, prop_tree(list), encoding);
    return console_result::okay;
}
