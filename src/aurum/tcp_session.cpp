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
#include <boost/asio/write.hpp>
#include <boost/asio/post.hpp>

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

    void tcp_session::send(std::shared_ptr<const std::vector<std::uint8_t>> message) {
        boost::asio::post(
            socket_.get_executor(),
            [this, _self = shared_from_this(), _message = std::move(message)]() {
                on_send(std::move(_message));
            }
        );
    }

    void tcp_session::on_send(std::shared_ptr<const std::vector<std::uint8_t>> message) {
        queue_.push_back(std::move(message));

        if (queue_.size() > 1) {
            return;
        }

        boost::asio::async_write(
            socket_,
            boost::asio::buffer(*queue_.front()),
            [this, _self = shared_from_this()](const boost::system::error_code& error_code, std::size_t bytes_transferred) {
                on_write(error_code, bytes_transferred);
            }
        );
    }

    void tcp_session::on_write(boost::system::error_code error_code, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (error_code) {
            state_->remove_session(get_id());
            return;
        }

        queue_.erase(queue_.begin());

        if (!queue_.empty()) {
            boost::asio::async_write(
                socket_,
                boost::asio::buffer(*queue_.front()),
                [this, _self = shared_from_this()](const boost::system::error_code& ec, std::size_t bt) {
                    on_write(ec, bt);
                }
            );
        }
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
                           auto _response = kernel_->handle(_body, _self);

                           if (_response) {
                               send(_response);
                           }

                           read_header();
                       } else {
                           state_->remove_session(get_id());
                       }
                   });
    }
}
