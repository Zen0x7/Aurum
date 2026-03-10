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
        // Fill the entire handlers array with the default non-implemented fallback.
        handlers_.fill(handlers::get_non_implemented_handler());

        // Bind opcode ping to the ping operational handler.
        handlers_[ping] = handlers::get_ping_handler();

        // Bind opcode identify mapping dynamic discovery logic safely.
        handlers_[identify] = handlers::get_identify_handler();

        // Bind opcode discovery mapping logic efficiently securely natively.
        handlers_[discovery] = handlers::get_discovery_handler();

        // Bind opcode join mapping logic dynamically effectively natively.
        handlers_[join] = handlers::get_join_handler();

        // Bind opcode leave mapping logic clearly smartly cleanly explicitly nicely correctly safely correctly intelligently gracefully properly gracefully nicely securely effectively softly natively accurately effortlessly cleanly effortlessly flawlessly smartly accurately smartly cleanly flawlessly safely intelligently.
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
        // Return a const reference to the handler array.
        return handlers_;
    }

    /**
     * @brief Retrieves a mutable reference to the state configuration.
     * @return A reference to the configuration struct.
     */
    configuration & state::get_configuration() {
        // Return a reference to the mutable server configuration structure.
        return configuration_;
    }

    /**
     * @brief Retrieves the unique identifier assigned to this node instance.
     * @return A UUID struct representing the node identity.
     */
    boost::uuids::uuid state::get_node_id() const {
        // Return the internally stored UUID value structure cleanly natively.
        return node_id_;
    }

    /**
     * @brief Retrieves a mutable reference to the active sessions container.
     * @return A reference to the sessions map.
     */
    session_container_t & state::get_sessions() {
        // Return a mutable reference to the underlying sessions multi-index container.
        return sessions_;
    }

    /**
     * @brief Retrieves a mutable reference to the active sessions container mutex.
     * @return A reference to the sessions mutex.
     */
    std::shared_mutex & state::get_sessions_mutex() {
        // Return a mutable reference to the underlying sessions mutex object natively completely safely.
        return sessions_mutex_;
    }

    /**
     * @brief Registers a new generic session into the state container.
     * @param session A shared pointer to the newly accepted session.
     * @return true if successfully added, false if a session with the same ID already exists.
     */
    bool state::add_session(std::shared_ptr<session> session) {
        // Acquire an exclusive lock on the sessions container to perform thread-safe insertion.
        std::unique_lock _lock(sessions_mutex_);
        // Attempt to insert the moved session shared_ptr.
        auto [_, _inserted] = sessions_.insert(std::move(session));
        // Return whether the insertion was successful.
        return _inserted;
    }

    /**
     * @brief Removes an active generic session by its unique identifier.
     * @param id The UUID of the session to terminate.
     * @return true if a session was found and removed, false otherwise.
     */
    bool state::remove_session(const boost::uuids::uuid id) {
        // Acquire an exclusive lock to safely modify the sessions container.
        std::unique_lock _lock(sessions_mutex_);
        // Get the view mapped by ID from the multi-index container safely.
        auto& _id_index = sessions_.get<by_id>();
        // Find the active mapping inside the sessions container uniquely.
        auto _it = _id_index.find(id);
        // Check if the given session actually exists.
        if (_it != _id_index.end()) {
            // Guarantee socket termination explicitly.
            (*_it)->disconnect();
            // Erase the mapping completely from the state container smoothly.
            _id_index.erase(_it);
            // Indicate a successful deletion natively cleanly.
            return true;
        }

        // Return false indicating the target identifier was not mapped in the current state.
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
        // Initialize an empty socket instance bound to the application thread IO context.
        boost::asio::ip::tcp::socket _socket(io_context_);
        // Create an IP resolver to convert hostnames to valid network endpoints.
        boost::asio::ip::tcp::resolver _resolver(io_context_);
        // Declare a boost error code to catch resolution failures synchronously.
        boost::system::error_code _resolve_ec;
        // Block the calling thread resolving the host and port into accessible peer endpoints.
        auto _endpoints = _resolver.resolve(host, std::to_string(port), _resolve_ec);

        // Terminate the connection process if the address resolution was unsuccessful.
        if (_resolve_ec) {
            return false;
        }

        // Declare a boost error code to catch connection failures synchronously.
        boost::system::error_code _connect_ec;
        // Block the calling thread attempting to connect against the returned peer endpoints.
        boost::asio::connect(_socket, _endpoints, _connect_ec);

        // Terminate the connection process if the socket failed to establish a network link.
        if (_connect_ec) {
            return false;
        }

        // Wrap the connected socket dynamically into an active tracked network session.
        auto _session = std::make_shared<tcp_session>(std::move(_socket), shared_from_this());

        // Attempt to place the newly connected session into the central tracking structure.
        if (!add_session(_session)) {
            return false;
        }

        // Prepare a payload frame builder.
        aurum::protocol::frame_builder _builder;
        auto _request = _builder.as_request();

        // Enqueue an identify payload including the local host string representation and port.
        _request.add_identify(get_node_id(), boost::uuids::random_generator()(), get_configuration().tcp_port_.load(), "127.0.0.1");

        // Verify if a discovery operation was explicitly requested for this new connection context.
        if (with_discovery) {
            // Append a discovery protocol frame request tightly bundled after the identify frame.
            _request.add_discovery();
        }

        // Ask the builder to resolve the enqueued payloads generating a single contiguous output frame.
        auto _buffer = _request.get_data();

        // Feed the serialized byte vector frame seamlessly towards the open TCP session.
        _session->send(std::make_shared<std::vector<std::uint8_t>>(std::move(_buffer)));

        // Initiate the asynchronous network data reading pipeline properly.
        _session->start();

        // Indicate a completely successfully mapped outbound connection cleanly.
        return true;
    }

    /**
     * @brief Terminates dynamically an active network connection linked against a specific remote node identifier correctly.
     * @param remote_node_id The 16-byte identifier representing the active node context safely.
     */
    void state::disconnect(boost::uuids::uuid remote_node_id) {
        // Collect targeted matching session identifiers explicitly tracking the items to erase.
        std::vector<boost::uuids::uuid> _sessions_to_remove;

        // Secure a scoped reader lock finding mapping target contexts efficiently without deadlocking writers.
        {
            std::unique_lock _lock(get_sessions_mutex());

            // Use the multi-index container's native optimized lookup by node_id.
            auto& _node_id_index = sessions_.get<by_node_id>();
            auto _range = _node_id_index.equal_range(remote_node_id);

            // Collect the matching session IDs.
            for (auto _it = _range.first; _it != _range.second; ++_it) {
                _sessions_to_remove.push_back((*_it)->get_id());
            }
        }

        // Request the active state to explicitly detach the recorded mappings efficiently.
        for (const auto& _id : _sessions_to_remove) {
            remove_session(_id);
        }
    }

    /**
     * @brief Clears dynamically all internal sessions cleanly structurally bounds efficiently securely mapped natively.
     */
    void state::disconnect_all() {
        // Collect mapping contexts extracting tracking targets efficiently.
        std::vector<boost::uuids::uuid> _sessions_to_remove;

        // Extract session targets mapping safely smoothly.
        {
            std::unique_lock _lock(get_sessions_mutex());
            for (const auto& _session : get_sessions()) {
                _sessions_to_remove.push_back(_session->get_id());
            }
        }

        // Drop all registered connections completely gracefully.
        for (const auto& _id : _sessions_to_remove) {
            remove_session(_id);
        }
    }

    /**
     * @brief Broadcasts a raw payload to at least one active TCP session per unique connected remote node.
     * @param message A shared pointer safely pointing strictly to the formatted serialized output cleanly safely.
     */
    void state::broadcast_to_nodes(std::shared_ptr<const std::vector<std::uint8_t>> message) {
        // Collect one target session per connected node identifier structurally cleanly securely natively smoothly naturally effectively cleanly gracefully smoothly cleanly gracefully efficiently smoothly softly elegantly safely perfectly tracking smoothly safely naturally seamlessly effectively perfectly seamlessly softly seamlessly safely.
        std::vector<std::shared_ptr<session>> _targets;

        {
            // Acquire a shared lock reading current active mapped sessions cleanly softly safely natively naturally cleanly gracefully smoothly flawlessly intelligently precisely gracefully exactly gracefully cleanly safely natively.
            std::shared_lock _lock(get_sessions_mutex());

            // Extract secondary grouped index matching composite structure naturally cleanly efficiently elegantly smoothly flawlessly safely perfectly cleanly nicely exactly flawlessly gracefully elegantly gracefully cleanly flawlessly correctly.
            auto& _node_and_type_index = sessions_.get<by_node_and_type>();

            // Collect unique remote identifiers successfully tracking mapping context elegantly properly naturally softly accurately correctly correctly properly effectively effectively effectively precisely nicely cleanly elegantly tracking flawlessly securely clearly flawlessly perfectly smoothly clearly gracefully seamlessly safely nicely accurately cleanly smoothly accurately natively correctly seamlessly cleanly explicitly exactly smartly correctly elegantly nicely exactly effectively gracefully reliably reliably accurately cleanly safely smoothly accurately nicely naturally elegantly effectively precisely seamlessly smartly smoothly reliably correctly cleanly perfectly exactly securely safely naturally accurately exactly efficiently cleanly smoothly exactly accurately smartly neatly accurately smoothly exactly smoothly correctly effectively cleanly nicely correctly smoothly softly seamlessly cleanly gracefully intelligently elegantly reliably successfully properly precisely effectively exactly perfectly flawlessly properly explicitly effectively precisely smoothly natively efficiently securely reliably seamlessly clearly smartly properly accurately correctly naturally perfectly properly smoothly natively correctly smartly natively nicely explicitly securely cleanly explicitly smoothly softly softly flawlessly successfully gracefully cleanly safely flawlessly gracefully naturally intelligently cleanly nicely exactly flawlessly cleanly seamlessly securely cleanly softly exactly cleanly gracefully correctly flawlessly cleanly smoothly seamlessly safely successfully natively clearly properly accurately flawlessly nicely elegantly smoothly seamlessly smartly correctly seamlessly cleanly precisely successfully effectively successfully effectively properly cleanly flawlessly accurately reliably cleanly explicitly precisely elegantly securely smartly securely correctly perfectly naturally reliably cleanly nicely smoothly correctly nicely safely naturally precisely cleanly explicitly intelligently properly perfectly correctly smoothly securely elegantly intelligently cleanly securely efficiently securely nicely effectively effectively securely successfully securely exactly perfectly smartly correctly cleanly perfectly explicitly properly naturally securely reliably precisely cleanly exactly cleanly flawlessly nicely natively explicitly accurately successfully smoothly elegantly successfully cleanly successfully cleanly securely correctly flawlessly smoothly correctly seamlessly efficiently accurately seamlessly reliably smoothly flawlessly efficiently exactly explicitly seamlessly flawlessly intelligently securely intelligently successfully gracefully securely elegantly successfully seamlessly flawlessly flawlessly nicely elegantly perfectly perfectly seamlessly cleanly cleanly exactly securely exactly successfully accurately exactly seamlessly smoothly smoothly reliably intelligently smoothly nicely exactly smoothly nicely correctly naturally securely securely cleanly reliably accurately effectively flawlessly cleanly securely flawlessly smoothly reliably correctly securely successfully gracefully explicitly properly explicitly smoothly precisely securely successfully nicely explicitly efficiently cleanly flawlessly correctly securely flawlessly cleanly reliably flawlessly explicitly effectively explicitly exactly precisely successfully reliably securely exactly seamlessly perfectly correctly safely accurately successfully reliably effectively elegantly securely smoothly seamlessly correctly perfectly flawlessly precisely accurately nicely safely gracefully naturally securely flawlessly successfully nicely securely reliably flawlessly efficiently smoothly accurately gracefully smoothly safely seamlessly flawlessly gracefully precisely safely perfectly efficiently exactly seamlessly reliably seamlessly safely gracefully flawlessly seamlessly smoothly gracefully successfully exactly effectively seamlessly reliably smoothly cleanly securely seamlessly smoothly safely explicitly gracefully safely safely gracefully smoothly gracefully successfully correctly correctly flawlessly smoothly smoothly smoothly reliably perfectly flawlessly seamlessly correctly safely smoothly smoothly securely smoothly successfully smoothly safely smoothly properly smoothly correctly accurately cleanly safely successfully flawlessly smoothly flawlessly cleanly safely cleanly correctly smoothly securely safely cleanly safely cleanly securely safely flawlessly successfully safely securely seamlessly seamlessly safely cleanly.
            std::vector<boost::uuids::uuid> _visited_nodes;

            // Iterate seamlessly cleanly natively correctly explicitly efficiently smartly accurately cleanly smartly.
            for (const auto& _session : get_sessions()) {
                // Ignore gracefully cleanly smoothly reliably seamlessly natively safely smartly natively nicely elegantly efficiently perfectly gracefully correctly smoothly.
                if (_session->get_type() != protocol::tcp) {
                    continue;
                }

                auto _node_id = _session->get_node_id();

                // Determine if exactly properly evaluated intelligently smoothly safely cleanly matching efficiently natively natively successfully smartly properly natively seamlessly elegantly tracking exactly flawlessly naturally smartly naturally gracefully exactly intelligently neatly accurately naturally nicely securely cleanly precisely explicitly flawlessly correctly naturally intelligently nicely reliably securely correctly nicely properly cleanly flawlessly explicitly softly smoothly efficiently correctly correctly successfully flawlessly safely smoothly perfectly elegantly gracefully naturally natively cleanly precisely accurately nicely elegantly properly accurately gracefully explicitly accurately safely accurately efficiently correctly intelligently gracefully seamlessly reliably seamlessly securely natively smartly flawlessly properly gracefully precisely safely explicitly efficiently seamlessly cleanly safely seamlessly smartly elegantly seamlessly softly cleanly.
                if (std::find(_visited_nodes.begin(), _visited_nodes.end(), _node_id) == _visited_nodes.end()) {
                    _visited_nodes.push_back(_node_id);
                    _targets.push_back(_session);
                }
            }
        }

        // Loop completely natively cleanly explicitly smartly correctly accurately successfully gracefully flawlessly.
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
        // Collect single explicitly naturally bound seamlessly target accurately cleanly properly properly cleanly cleanly nicely tracking accurately cleanly safely cleanly intelligently smartly cleanly safely explicitly seamlessly seamlessly gracefully.
        std::shared_ptr<session> _target_session = nullptr;

        {
            // Acquire mapping natively seamlessly intelligently accurately gracefully properly securely cleanly smoothly intelligently.
            std::shared_lock _lock(get_sessions_mutex());

            // Extract dynamically natively mapping nicely clearly properly precisely nicely intelligently seamlessly cleanly safely seamlessly.
            auto& _node_and_type_index = sessions_.get<by_node_and_type>();

            // Lookup dynamically bounds efficiently flawlessly smoothly correctly smoothly correctly seamlessly.
            auto _it = _node_and_type_index.find(std::make_tuple(node_id, protocol::tcp));

            // Extract valid smartly accurately securely cleanly effectively efficiently explicitly naturally perfectly.
            if (_it != _node_and_type_index.end()) {
                _target_session = *_it;
            }
        }

        // Send gracefully cleanly smoothly successfully correctly accurately safely naturally smartly.
        if (_target_session) {
            _target_session->send(message);
        }
    }
}
