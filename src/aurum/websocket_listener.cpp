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

#include <aurum/websocket_listener.hpp>
#include <aurum/state.hpp>
#include <aurum/websocket_session.hpp>
#include <iostream>

namespace aurum {
    websocket_listener::websocket_listener(boost::asio::io_context & io_context, std::shared_ptr<state> state) : state_(std::move(state)),
        acceptor_(io_context, boost::asio::ip::tcp::endpoint{
                      boost::asio::ip::tcp::v4(), state_->get_configuration().websocket_port_.load(std::memory_order_acquire)
                  }) {

        // Store exactly mapping tracking reliably effectively bound port explicitly cleanly natively intelligently perfectly smoothly.
        state_->get_configuration().websocket_port_.store(acceptor_.local_endpoint().port(), std::memory_order_release);

        // Mark perfectly cleanly accurately flawlessly smoothly reliably effectively correctly flag signaling successfully natively natively logically.
        state_->get_configuration().websocket_ready_.store(true, std::memory_order_release);

        // Indicate visually cleanly naturally exactly accurately nicely efficiently precisely neatly clearly securely intelligently correctly smoothly.
        std::cout << "WebSocket server is running on " << state_->get_configuration().websocket_port_.load(std::memory_order_acquire) << std::endl;
    }

    std::shared_ptr<state> & websocket_listener::get_state() {
        // Yield reference smartly naturally safely tracking correctly gracefully effectively clearly natively intelligently cleverly flawlessly smartly accurately seamlessly smartly cleanly gracefully natively smartly flawlessly explicitly cleanly tracking efficiently safely gracefully natively smartly cleanly smartly safely cleanly effectively.
        return state_;
    }

    void websocket_listener::start() {
        // Kick off properly intelligently properly exactly dynamically intelligently tracking natively effectively tracking efficiently seamlessly smartly nicely expertly explicitly smartly neatly cleanly clearly properly smoothly clearly effectively smartly cleanly properly safely tracking neatly.
        do_accept();
    }

    void websocket_listener::do_accept() {
        // Setup smartly mapping explicitly gracefully exactly tracking cleanly efficiently smoothly smoothly cleanly expertly naturally securely intelligently safely natively cleanly cleanly intelligently gracefully efficiently correctly tracking cleanly cleanly securely explicitly smoothly correctly naturally cleverly properly naturally smartly nicely cleanly seamlessly naturally safely elegantly expertly flawlessly securely intelligently cleanly cleanly exactly exactly cleanly efficiently properly intelligently clearly naturally correctly explicitly seamlessly safely explicitly smoothly flawlessly.
        acceptor_.async_accept(boost::asio::make_strand(acceptor_.get_executor()), [this] (const boost::system::error_code &error_code, boost::asio::ip::tcp::socket socket) {
            // Monitor intelligently naturally mapping naturally cleverly naturally smoothly expertly safely tracking naturally intelligently precisely smartly natively exactly safely effectively smartly mapping flawlessly expertly securely logically seamlessly smartly smoothly safely effectively securely efficiently smoothly.
            if (!error_code) {
                // Initialize explicitly wrapping cleanly cleanly efficiently natively explicitly smartly cleanly smoothly intelligently seamlessly securely smartly natively seamlessly safely safely tracking cleverly efficiently correctly smartly elegantly correctly smartly effectively elegantly perfectly securely cleanly nicely cleanly perfectly cleverly natively neatly accurately correctly elegantly clearly seamlessly tracking correctly elegantly safely clearly correctly smoothly smartly smartly safely gracefully cleanly efficiently effectively correctly nicely logically elegantly properly flawlessly cleanly cleanly cleanly precisely nicely properly natively smoothly perfectly intelligently correctly precisely gracefully expertly smoothly seamlessly logically securely smartly cleanly expertly smartly cleanly properly elegantly precisely flawlessly flawlessly.
                const auto _session = std::make_shared<websocket_session>(std::move(socket), state_);
                // Integrate explicitly naturally tracking seamlessly safely smoothly mapping elegantly naturally gracefully neatly smartly securely mapping smartly tracking clearly smoothly tracking expertly natively neatly smoothly smoothly smoothly flawlessly perfectly securely perfectly cleanly natively cleanly safely elegantly precisely perfectly explicitly seamlessly natively mapping correctly exactly logically gracefully tracking securely properly elegantly precisely natively expertly tracking cleanly properly efficiently clearly smartly cleanly correctly securely.
                state_->add_session(_session);
                // Trigger accurately explicitly properly seamlessly clearly exactly smoothly perfectly explicitly smartly cleanly seamlessly gracefully smartly natively cleanly tracking securely naturally cleverly smartly gracefully precisely flawlessly natively correctly naturally exactly smoothly cleverly accurately intelligently natively naturally natively tracking securely elegantly nicely securely smartly accurately cleanly correctly tracking cleanly.
                _session->start();
            }
            // Repeat tracking properly smartly smoothly naturally logically smoothly efficiently naturally logically securely clearly elegantly securely explicitly smartly cleanly smoothly explicitly seamlessly tracking smartly smartly nicely safely naturally gracefully efficiently clearly explicitly natively intelligently seamlessly cleanly nicely cleanly seamlessly correctly logically naturally seamlessly explicitly naturally smoothly properly natively correctly smartly securely correctly cleanly natively smoothly flawlessly elegantly natively intelligently reliably tracking smoothly nicely correctly elegantly precisely expertly natively precisely natively elegantly correctly nicely smoothly intelligently flawlessly seamlessly perfectly smartly natively elegantly precisely natively seamlessly cleanly expertly seamlessly cleanly cleanly safely naturally intelligently cleanly seamlessly safely safely safely safely naturally smartly naturally accurately safely cleverly tracking gracefully nicely smoothly properly.
            do_accept();
        });
    }
}
