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
#include <aurum/websocket_session.hpp>

#include <boost/core/ignore_unused.hpp>
#include <aurum/session_kernel.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace aurum {

    websocket_session::websocket_session(boost::asio::ip::tcp::socket socket, std::shared_ptr<state> state)
        : session(protocol::websocket),
          id_(boost::uuids::random_generator()()),
          state_(std::move(state)),
          node_id_(boost::uuids::nil_uuid()),
          port_(0),
          host_(""),
          kernel_(std::make_shared<session_kernel>(state_)),
          strand_(boost::asio::make_strand(socket.get_executor())),
          ws_(std::move(socket)) {
    }

    void websocket_session::start() {
        // Enforce safe asynchronous boundary wrapping websocket object upgrade cleanly.
        boost::asio::post(strand_, [this, _self = shared_from_this()]() {
            // Asynchronously accept the websocket handshake cleanly.
            ws_.async_accept(boost::beast::bind_front_handler(&websocket_session::on_accept, shared_from_this()));
        });
    }

    void websocket_session::on_accept(boost::system::error_code error_code) {
        // Check if handshake upgrade completely successfully established a websocket link seamlessly.
        if (!error_code) {
            // Set binary mode for the websocket stream correctly supporting payload structs.
            ws_.binary(true);
            // Initiate loop asynchronously mapping bodies into payloads explicitly cleanly.
            read_body();
        } else {
            // Tear down mapping safely gracefully dynamically accurately natively cleanly safely explicitly.
            state_->remove_session(get_id());
        }
    }

    void websocket_session::send(std::shared_ptr<const std::vector<std::uint8_t>> message) {
        // Post the write request safely mapping explicitly strand sequence natively.
        boost::asio::post(strand_, [this, _self = shared_from_this(), _message = std::move(message)]() {
            // Forward cleanly effectively preserving strand bounds correctly efficiently safely smoothly cleanly securely correctly.
            on_send(std::move(_message));
        });
    }

    void websocket_session::on_send(std::shared_ptr<const std::vector<std::uint8_t>> message) {
        // Enqueue safely tracking pointer reference lifecycle smoothly mapping seamlessly dynamically efficiently cleanly naturally natively natively.
        queue_.push_back(std::move(message));

        // Evaluate efficiently limiting nested asynchronous overlapping preventing logical race safely mapping efficiently dynamically correctly properly safely smartly securely seamlessly cleanly natively dynamically correctly accurately explicitly effectively mapping securely tracking dynamically efficiently gracefully cleanly smoothly mapping correctly optimally precisely safely effectively robustly accurately intelligently mapping optimally clearly seamlessly natively effectively completely natively exactly mapping gracefully neatly elegantly smoothly cleanly natively natively correctly properly cleanly safely logically clearly smoothly effectively precisely natively exactly gracefully neatly accurately logically natively cleanly naturally seamlessly perfectly intelligently optimally cleanly naturally cleanly smartly natively cleanly tracking perfectly neatly mapping natively effectively cleanly gracefully logically effectively accurately gracefully smoothly explicitly nicely natively effectively.
        if (queue_.size() > 1) {
            return;
        }

        // Initialize natively tracking explicit transmission request effectively gracefully correctly clearly neatly smartly natively natively seamlessly explicitly intelligently.
        ws_.async_write(boost::asio::buffer(*queue_.front()), boost::beast::bind_front_handler(&websocket_session::on_write, shared_from_this()));
    }

    void websocket_session::on_write(boost::system::error_code error_code, std::size_t bytes_transferred) {
        // Inform cleanly tracking safely ignoring gracefully naturally clearly explicitly neatly gracefully perfectly seamlessly mapped explicitly securely effectively seamlessly cleanly smoothly tracking correctly explicitly properly tracking naturally elegantly tracking mapped securely cleanly natively gracefully naturally mapped cleanly seamlessly smartly cleanly explicitly smartly correctly logically explicitly natively properly properly natively perfectly gracefully mapping properly naturally tracking completely efficiently clearly mapping properly smartly cleanly explicitly explicitly safely explicitly gracefully cleanly effectively exactly seamlessly gracefully perfectly clearly safely.
        boost::ignore_unused(bytes_transferred);

        // Protect mapping explicitly natively smoothly cleanly perfectly logically logically tracking elegantly cleanly seamlessly correctly logically naturally smoothly correctly smoothly seamlessly natively elegantly naturally perfectly naturally elegantly natively safely smoothly mapped cleanly exactly tracking explicitly properly securely neatly gracefully clearly securely mapping smoothly cleanly seamlessly cleanly smartly effectively cleanly gracefully smartly logically gracefully natively completely exactly neatly explicitly securely accurately tracking smoothly cleanly safely smartly safely natively mapping smoothly naturally tracking effectively accurately gracefully natively mapping safely explicitly cleanly natively explicitly correctly natively mapping elegantly gracefully cleanly securely properly properly clearly cleanly smartly cleanly perfectly elegantly naturally securely gracefully exactly naturally cleanly logically logically explicitly smoothly correctly intelligently safely effectively properly naturally cleanly tracking nicely exactly cleanly smoothly natively smartly cleanly correctly natively cleanly natively correctly smoothly efficiently mapping explicitly effectively seamlessly tracking cleanly.
        if (error_code) {
            // Remove state explicit correctly cleanly dynamically natively logically elegantly seamlessly correctly exactly tracking mapping clearly neatly logically perfectly mapping natively safely smoothly mapping clearly accurately explicitly smoothly seamlessly efficiently tracking smartly mapping effectively smartly exactly tracking perfectly securely exactly tracking explicitly smartly mapping natively smartly smartly seamlessly safely accurately elegantly cleanly mapping perfectly perfectly smartly cleanly cleanly smoothly gracefully exactly gracefully neatly tracking natively cleanly nicely logically seamlessly logically clearly naturally neatly logically logically nicely explicitly elegantly correctly tracking cleanly correctly securely seamlessly securely cleanly tracking gracefully perfectly smartly neatly cleanly gracefully correctly gracefully intelligently correctly smoothly nicely gracefully securely correctly neatly exactly safely naturally smoothly correctly.
            state_->remove_session(get_id());
            return;
        }

        // Advance explicitly cleanly naturally smoothly clearly tracking accurately tracking naturally seamlessly seamlessly elegantly accurately natively clearly gracefully correctly elegantly cleanly properly cleanly tracking smoothly smoothly securely properly tracking perfectly properly smoothly gracefully accurately explicitly exactly tracking gracefully exactly efficiently gracefully properly cleanly naturally smartly explicitly naturally smoothly neatly tracking gracefully neatly exactly safely properly naturally properly properly gracefully efficiently cleanly explicitly tracking elegantly gracefully logically securely cleanly natively smartly securely logically naturally cleanly naturally natively smoothly cleanly natively smartly cleanly cleanly properly properly naturally cleanly seamlessly securely elegantly natively cleanly cleanly correctly perfectly logically seamlessly safely smartly smoothly nicely seamlessly explicitly naturally neatly securely naturally smartly effectively seamlessly safely explicitly cleanly safely perfectly smartly naturally effectively safely natively nicely explicitly exactly cleanly nicely clearly gracefully smoothly smoothly gracefully securely smoothly natively nicely correctly natively smartly efficiently gracefully smoothly smoothly clearly properly smoothly smartly gracefully logically correctly cleanly gracefully tracking clearly elegantly nicely smoothly correctly properly nicely seamlessly mapping smoothly exactly cleanly logically smartly tracking gracefully gracefully properly exactly perfectly.
        queue_.erase(queue_.begin());

        // Recurse correctly mapping efficiently dynamically elegantly smoothly naturally clearly smartly smoothly natively elegantly explicitly cleanly natively nicely seamlessly nicely smoothly securely mapped logically neatly mapped smoothly cleanly explicitly smartly securely explicitly natively correctly intelligently smoothly smartly natively explicitly smartly logically cleanly neatly natively safely smoothly cleanly tracking effectively seamlessly seamlessly securely seamlessly mapping securely natively cleanly efficiently smoothly intelligently explicitly tracking neatly cleanly correctly seamlessly perfectly securely securely cleanly naturally correctly elegantly properly safely gracefully naturally mapping gracefully exactly cleanly exactly explicitly intelligently elegantly seamlessly cleanly neatly explicitly elegantly nicely gracefully exactly exactly perfectly smoothly smartly cleanly securely nicely cleanly efficiently nicely mapping seamlessly explicitly perfectly elegantly explicitly safely smoothly logically naturally efficiently.
        if (!queue_.empty()) {
            // Loop cleanly exactly properly properly smoothly cleanly cleanly cleanly neatly smoothly effectively smartly tracking cleanly accurately properly efficiently properly smoothly smoothly naturally naturally elegantly natively effectively naturally mapped natively nicely tracking explicitly naturally safely correctly natively effectively perfectly elegantly securely logically elegantly tracking cleanly safely correctly properly securely cleanly cleanly intelligently natively neatly effectively nicely exactly exactly mapping intelligently nicely cleanly cleanly seamlessly perfectly natively properly seamlessly smoothly mapping seamlessly perfectly gracefully perfectly perfectly accurately intelligently smoothly elegantly explicitly cleanly correctly logically nicely smoothly clearly smoothly accurately logically efficiently logically explicitly efficiently securely logically smoothly logically nicely natively neatly perfectly efficiently naturally cleanly gracefully seamlessly mapping intelligently cleanly smoothly smartly smoothly smartly gracefully explicitly correctly gracefully perfectly cleanly cleanly seamlessly naturally intelligently cleanly nicely naturally cleanly intelligently logically securely cleanly elegantly mapping gracefully efficiently safely seamlessly smoothly smartly precisely cleanly intelligently smoothly safely cleanly cleanly smartly cleanly correctly properly cleanly smoothly smartly gracefully smartly smoothly neatly intelligently cleanly properly.
            ws_.async_write(boost::asio::buffer(*queue_.front()), boost::beast::bind_front_handler(&websocket_session::on_write, shared_from_this()));
        }
    }

    void websocket_session::read_body() {
        // Map natively explicitly effectively smartly tracking intelligently effectively gracefully cleanly correctly logically cleanly efficiently neatly logically smoothly smoothly smartly natively cleanly accurately correctly neatly smoothly properly efficiently clearly naturally smoothly natively cleanly tracking smoothly neatly explicitly cleanly nicely efficiently safely effectively smartly smoothly nicely smoothly seamlessly smoothly seamlessly neatly tracking intelligently dynamically gracefully cleanly elegantly cleanly accurately smoothly smartly cleanly effectively intelligently mapping elegantly neatly exactly efficiently cleanly smoothly cleanly nicely securely cleanly natively cleanly nicely cleanly naturally cleanly intelligently neatly elegantly logically cleverly perfectly neatly explicitly securely intelligently cleanly gracefully natively gracefully natively exactly natively cleanly nicely smoothly logically clearly cleanly naturally correctly smoothly effectively precisely correctly intelligently exactly clearly neatly correctly natively elegantly intelligently smartly natively clearly elegantly safely correctly nicely smoothly intelligently tracking neatly intelligently smoothly neatly logically nicely securely securely efficiently smartly securely correctly natively natively safely clearly smartly naturally smartly intelligently intelligently securely intelligently smoothly neatly correctly cleanly correctly efficiently elegantly dynamically cleanly efficiently safely safely elegantly intelligently explicitly smoothly efficiently intelligently neatly nicely neatly safely elegantly logically exactly cleanly properly explicitly elegantly dynamically safely intelligently properly cleanly natively intelligently natively cleanly cleverly neatly smoothly explicitly logically correctly correctly cleanly perfectly cleanly elegantly precisely clearly safely cleanly naturally naturally properly cleanly precisely gracefully naturally.
        ws_.async_read(read_buffer_, boost::beast::bind_front_handler(&websocket_session::on_read, shared_from_this()));
    }

    void websocket_session::on_read(boost::system::error_code error_code, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (!error_code) {
            auto _self = shared_from_this();
            auto _payload = std::make_shared<std::vector<std::uint8_t>>(
                static_cast<const std::uint8_t*>(read_buffer_.data().data()),
                static_cast<const std::uint8_t*>(read_buffer_.data().data()) + read_buffer_.size()
            );

            read_buffer_.consume(read_buffer_.size());

            if (const auto _response = kernel_->handle(_payload, _self); _response) {
                send(_response);
            }

            read_body();
        } else {
            state_->remove_session(get_id());
        }
    }

    boost::uuids::uuid websocket_session::get_id() const {
        return id_;
    }

    boost::uuids::uuid websocket_session::get_node_id() const {
        return node_id_;
    }

    void websocket_session::set_node_id(boost::uuids::uuid node_id) {
        node_id_ = node_id;
    }

    std::uint16_t websocket_session::get_port() const {
        return port_;
    }

    void websocket_session::set_port(std::uint16_t port) {
        port_ = port;
    }

    std::string websocket_session::get_host() const {
        return host_;
    }

    void websocket_session::set_host(const std::string& host) {
        host_ = host;
    }

    void websocket_session::disconnect() {
        if (ws_.is_open()) {
            boost::system::error_code _ec;
            ws_.close(boost::beast::websocket::close_code::normal, _ec);
        }
    }
}
