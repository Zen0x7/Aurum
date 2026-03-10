// Aurum
// Copyright (C) 2026 Ian Torres
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include <aurum/state.hpp>

#include <aurum/tcp_session.hpp>
#include <aurum/protocol.hpp>
#include <aurum/handlers.hpp>

#include <cstring>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>

namespace aurum {
    /**
     * @brief Constructs a new state object and initializes default handlers.
     * @param io_context The application I/O execution context reference.
     */
    state::state(boost::asio::io_context& io_context) : node_id_(boost::uuids::random_generator()()), io_context_(io_context) {
        handlers_.fill(handlers::get_non_implemented_handler());

        handlers_[ping] = handlers::get_ping_handler();

        handlers_[identify] = handlers::get_identify_handler();

        handlers_[discovery] = handlers::get_discovery_handler();

        // Bind opcode join mapping logic dynamically effectively natively.
        handlers_[join] = handlers::get_join_handler();

        // Bind opcode leave mapping logic.
        handlers_[leave] = handlers::get_leave_handler();
    }

    /**
     * @brief Retrieves a reference to the main I/O execution context safely.
     * @return A reference to the boost::asio::io_context tracking background asynchronous limits safely smoothly cleanly efficiently.
     */
    boost::asio::io_context& state::get_io_context() {
        // Return the internally mapped context engine natively structurally safely reliably.
        return io_context_;
    }

    /**
     * @brief Retrieves the immutable array of operation handlers.
     * @return A constant reference to the 256-element array of handler functions.
     */
    const std::array<handler_type, 256>& state::get_handlers() const {
        return handlers_;
    }

    /**
     * @brief Retrieves a mutable reference to the state configuration.
     * @return A reference to the configuration struct.
     */
    configuration & state::get_configuration() {
        return configuration_;
    }

    /**
     * @brief Retrieves the unique identifier assigned to this node instance.
     * @return A UUID struct representing the node identity.
     */
    boost::uuids::uuid state::get_node_id() const {
        return node_id_;
    }

    /**
     * @brief Retrieves a mutable reference to the active sessions container.
     * @return A reference to the sessions map.
     */
    session_container_t & state::get_sessions() {
        return sessions_;
    }

    /**
     * @brief Retrieves a mutable reference to the active sessions container mutex.
     * @return A reference to the sessions mutex.
     */
    std::shared_mutex & state::get_sessions_mutex() {
        return sessions_mutex_;
    }

    /**
     * @brief Registers a new generic session into the state container.
     * @param session A shared pointer to the newly accepted session.
     * @return true if successfully added, false if a session with the same ID already exists.
     */
    bool state::add_session(std::shared_ptr<session> session) {
        std::unique_lock _lock(sessions_mutex_);
        auto [_, _inserted] = sessions_.insert(std::move(session));
        return _inserted;
    }

    /**
     * @brief Removes an active generic session by its unique identifier.
     * @param id The UUID of the session to terminate.
     * @return true if a session was found and removed, false otherwise.
     */
    bool state::remove_session(const boost::uuids::uuid id) {
        std::unique_lock _lock(sessions_mutex_);
        auto& _id_index = sessions_.get<by_id>();
        auto _it = _id_index.find(id);
        if (_it != _id_index.end()) {
            (*_it)->disconnect();
            _id_index.erase(_it);
            return true;
        }

        return false;
    }

    /**
     * @brief Establishes a synchronous outbound network connection directly towards a peer instance.
     * @param host The remote peer IP address structurally mapped string.
     * @param port The target destination listener port integer properly.
     * @param with_discovery Boolean indicating if the initial connection payload should append a discovery request.
     * @return True if connection was completely established natively securely.
     */
    bool state::connect(const std::string& host, unsigned short port, bool with_discovery) {
        boost::asio::ip::tcp::socket _socket(io_context_);
        boost::asio::ip::tcp::resolver _resolver(io_context_);
        boost::system::error_code _resolve_ec;
        auto _endpoints = _resolver.resolve(host, std::to_string(port), _resolve_ec);

        if (_resolve_ec) {
            return false;
        }

        boost::system::error_code _connect_ec;
        boost::asio::connect(_socket, _endpoints, _connect_ec);

        if (_connect_ec) {
            return false;
        }

        auto _session = std::make_shared<tcp_session>(std::move(_socket), shared_from_this());

        if (!add_session(_session)) {
            return false;
        }

        aurum::protocol::frame_builder _builder;
        auto _request = _builder.as_request();

        _request.add_identify(get_node_id(), boost::uuids::random_generator()(), get_configuration().tcp_port_.load(), "127.0.0.1");

        if (with_discovery) {
            _request.add_discovery();
        }

        auto _buffer = _request.get_data();

        _session->send(std::make_shared<std::vector<std::uint8_t>>(std::move(_buffer)));

        _session->start();

        return true;
    }

    /**
     * @brief Terminates dynamically an active network connection linked against a specific remote node identifier correctly.
     * @param remote_node_id The 16-byte identifier representing the active node context safely.
     */
    void state::disconnect(boost::uuids::uuid remote_node_id) {
        std::vector<boost::uuids::uuid> _sessions_to_remove;

        {
            std::unique_lock _lock(get_sessions_mutex());

            auto& _node_id_index = sessions_.get<by_node_id>();
            auto _range = _node_id_index.equal_range(remote_node_id);

            for (auto _it = _range.first; _it != _range.second; ++_it) {
                _sessions_to_remove.push_back((*_it)->get_id());
            }
        }

        for (const auto& _id : _sessions_to_remove) {
            remove_session(_id);
        }
    }

    /**
     * @brief Clears dynamically all internal sessions cleanly structurally bounds efficiently securely mapped natively.
     */
    void state::disconnect_all() {
        std::vector<boost::uuids::uuid> _sessions_to_remove;

        {
            std::unique_lock _lock(get_sessions_mutex());
            for (const auto& _session : get_sessions()) {
                _sessions_to_remove.push_back(_session->get_id());
            }
        }

        for (const auto& _id : _sessions_to_remove) {
            remove_session(_id);
        }
    }

    /**
     * @brief Broadcasts a raw payload to at least one active TCP session per unique connected remote node.
     * @param message A shared pointer safely pointing strictly to the formatted serialized output cleanly safely.
     */
    void state::broadcast_to_nodes(std::shared_ptr<const std::vector<std::uint8_t>> message) {
        // Collect one target session per connected node identifier.
        std::vector<std::shared_ptr<session>> _targets;

        {
            // Acquire a shared lock reading current active mapped sessions.
            std::shared_lock _lock(get_sessions_mutex());

            // Collect unique remote identifiers successfully tracking mapping context.
            std::vector<boost::uuids::uuid> _visited_nodes;

            for (const auto& _session : get_sessions()) {
                if (_session->get_type() != protocol::tcp) {
                    continue;
                }

                auto _node_id = _session->get_node_id();

                if (std::find(_visited_nodes.begin(), _visited_nodes.end(), _node_id) == _visited_nodes.end()) {
                    _visited_nodes.push_back(_node_id);
                    _targets.push_back(_session);
                }
            }
        }

        // Send to each target session.
        for (const auto& _target : _targets) {
            _target->send(message);
        }
    }

    /**
     * @brief Transmits a raw payload targetting an explicit TCP session mapped cleanly towards a specific remote node.
     * @param node_id The valid 16-byte target destination mapped remotely logically strictly clearly mapping correctly.
     * @param message A shared pointer strictly pointing safely naturally accurately effectively accurately.
     */
    void state::send_to_node(boost::uuids::uuid node_id, std::shared_ptr<const std::vector<std::uint8_t>> message) {
        std::shared_ptr<session> _target_session = nullptr;

        {
            std::shared_lock _lock(get_sessions_mutex());
            auto& _node_and_type_index = sessions_.get<by_node_and_type>();
            auto _it = _node_and_type_index.find(std::make_tuple(node_id, protocol::tcp));

            if (_it != _node_and_type_index.end()) {
                _target_session = *_it;
            }
        }

        if (_target_session) {
            _target_session->send(message);
        }
    }
}
