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

#include <boost/core/ignore_unused.hpp>
#include <boost/asio/read.hpp>

#include <boost/endian/conversion.hpp>
#include <aurum/tcp_kernel.hpp>

#include <boost/uuid/random_generator.hpp>

namespace aurum {
    tcp_session::tcp_session(boost::asio::ip::tcp::socket socket,
                             std::shared_ptr<state> state) : id_(boost::uuids::random_generator()()),
                                                             state_(std::move(state)),
                                                             kernel_(std::make_shared<tcp_kernel>(state_)),
                                                             socket_(std::move(socket)) {
    }

    void tcp_session::start() {
        read_header();
    }

    boost::uuids::uuid tcp_session::get_id() const {
        return id_;
    }

    void tcp_session::read_header() {
        auto _self = shared_from_this();
        async_read(socket_, boost::asio::buffer(&header_length_, sizeof(header_length_)),
                   [this, _self](const boost::system::error_code &error_code, std::size_t bytes_transferred) {
                       boost::ignore_unused(bytes_transferred);
                       boost::endian::big_to_native_inplace(header_length_);

                       if (!error_code) {
                           read_body();
                       } else {
                           state_->remove_session(get_id());
                       }
                   });
    }

    void tcp_session::read_body() {
        auto _self = shared_from_this();
        auto _body = std::make_shared<std::vector<std::uint8_t> >(header_length_);
        async_read(socket_, boost::asio::buffer(*_body),
                   [this, _self, _body](const boost::system::error_code &error_code, std::size_t bytes_transferred) {
                       boost::ignore_unused(bytes_transferred);
                       if (!error_code) {
                           kernel_->handle(_body);

                           read_header();
                       } else {
                           state_->remove_session(get_id());
                       }
                   });
    }
}
