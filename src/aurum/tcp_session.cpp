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
    /**
     * @brief Constructs a new TCP session using an accepted socket.
     * @param socket The connected network endpoint socket.
     * @param state The central application state managing sessions.
     */
    tcp_session::tcp_session(boost::asio::ip::tcp::socket socket,
                             std::shared_ptr<state> state) : id_(boost::uuids::random_generator()()),
                                                             state_(std::move(state)),
                                                             kernel_(std::make_shared<tcp_kernel>(state_)),
                                                             socket_(std::move(socket)) {
    }

    /**
     * @brief Initiates the asynchronous reading cycle for the session.
     */
    void tcp_session::start() {
        // Trigger the initial read operation to fetch the message header.
        read_header();
    }

    /**
     * @brief Thread-safely queues a binary message for transmission.
     * @param message A shared pointer to the payload bytes vector.
     */
    void tcp_session::send(std::shared_ptr<const std::vector<std::uint8_t>> message) {
        // Post the write request to the socket's executor to guarantee thread safety.
        boost::asio::post(
            // Access the underlying IO executor from the socket.
            socket_.get_executor(),
            // Capture the session shared pointer to extend lifecycle and move the payload message.
            [this, _self = shared_from_this(), _message = std::move(message)]() {
                // Call the internal serialization dispatcher on the executor strand.
                on_send(std::move(_message));
            }
        );
    }

    /**
     * @brief Internal dispatch method posting write operations serialized inside the strand/executor loop.
     * @param message The payload to add to the internal buffer sequence.
     */
    void tcp_session::on_send(std::shared_ptr<const std::vector<std::uint8_t>> message) {
        // Push the new message payload to the end of the write queue.
        queue_.push_back(std::move(message));

        // If the queue size is larger than 1, an async write operation is already pending.
        if (queue_.size() > 1) {
            // Return immediately without calling async_write again to avoid overlapping operations.
            return;
        }

        // Start an asynchronous write operation with the first message in the queue.
        boost::asio::async_write(
            // The active TCP network socket instance.
            socket_,
            // Provide a boost buffer mapping the entire front vector contents.
            boost::asio::buffer(*queue_.front()),
            // Attach a callback to process write completion logic.
            [this, _self = shared_from_this()](const boost::system::error_code& error_code, std::size_t bytes_transferred) {
                // Route completion status to the internal callback handler.
                on_write(error_code, bytes_transferred);
            }
        );
    }

    /**
     * @brief Callback invoked when an async write cycle is completed.
     * @param error_code The Boost system error status of the operation.
     * @param bytes_transferred The number of physical bytes flushed into the socket interface.
     */
    void tcp_session::on_write(boost::system::error_code error_code, std::size_t bytes_transferred) {
        // Inform compiler that bytes_transferred is intentionally unused here to prevent warnings.
        boost::ignore_unused(bytes_transferred);

        // Check if an error was emitted during the physical write attempt.
        if (error_code) {
            // Ask the state object to gracefully destroy this broken network session instance.
            state_->remove_session(get_id());
            // Return to stop further execution.
            return;
        }

        // Drop the first queue element that has now been successfully transmitted.
        queue_.erase(queue_.begin());

        // Check if there are remaining pending payloads sitting in the queue.
        if (!queue_.empty()) {
            // Dispatch a subsequent asynchronous write for the next item in the line.
            boost::asio::async_write(
                // Target the active socket connection again.
                socket_,
                // Map the newly-exposed front of the queue buffer container.
                boost::asio::buffer(*queue_.front()),
                // Pass a new completion token extending session lifetime.
                [this, _self = shared_from_this()](const boost::system::error_code& ec, std::size_t bt) {
                    // Loop recursively invoking the write completion dispatcher.
                    on_write(ec, bt);
                }
            );
        }
    }

    /**
     * @brief Gets the unique 16-byte identifier of the session.
     * @return The UUID assigned during initialization.
     */
    boost::uuids::uuid tcp_session::get_id() const {
        // Return the underlying boost UUID structure.
        return id_;
    }

    /**
     * @brief Initiates an asynchronous read targeting the 4-byte frame header limit.
     */
    void tcp_session::read_header() {
        // Safely cache a reference to the active session object lifecycle.
        auto _self = shared_from_this();
        // Request the IO context to asynchronously fetch exactly the 4-byte length header.
        async_read(socket_, boost::asio::buffer(&header_length_, sizeof(header_length_)),
                   [this, _self](const boost::system::error_code &error_code, std::size_t bytes_transferred) {
                       // Suppress the unused variable warning for transferred byte count.
                       boost::ignore_unused(bytes_transferred);
                       // Convert the received 4 bytes from little-endian back to the host architecture format.
                       boost::endian::little_to_native_inplace(header_length_);

                       // Ensure that no network issues occurred during the read block.
                       if (!error_code) {
                           // Trigger the dynamic body reading chain passing the newly received size.
                           read_body();
                       } else {
                           // Request that the application drops the active session from global management.
                           state_->remove_session(get_id());
                       }
                   });
    }

    /**
     * @brief Initiates an asynchronous read allocating the full payload size determined by the header.
     */
    void tcp_session::read_body() {
        // Track lifecycle of the active shared connection reference for async completion.
        auto _self = shared_from_this();
        // Dynamically instantiate a payload vector sized to perfectly fit the incoming data boundary length.
        auto _body = std::make_shared<std::vector<std::uint8_t> >(header_length_);
        // Submit an asynchronous read targeting the active session payload socket connection.
        async_read(socket_, boost::asio::buffer(*_body),
                   [this, _self, _body](const boost::system::error_code &error_code, std::size_t bytes_transferred) {
                       // Block compiler warnings marking transferred parameter explicitly as skipped over.
                       boost::ignore_unused(bytes_transferred);
                       // Verify that no physical connection error interrupted the body reception sequence.
                       if (!error_code) {
                           // Call the internal kernel logic parser block to process incoming buffer structures.
                           auto _response = kernel_->handle(_body, _self);

                           // If the parsing logic returns a valid resulting payload.
                           if (_response) {
                               // Request transmission sequence back over the TCP link wrapping the payload buffer structure.
                               send(_response);
                           }

                           // Trigger the read chain back towards evaluating a new 4-byte framing structure.
                           read_header();
                       } else {
                           // Tear down the active TCP connection resource freeing memory containers.
                           state_->remove_session(get_id());
                       }
                   });
    }
}
