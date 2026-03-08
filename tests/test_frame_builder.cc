#include <gtest/gtest.h>
#include <aurum/protocol/frame_builder.hpp>

TEST(FrameBuilderTest, CanBuildRequest) {
    constexpr aurum::protocol::frame_builder _frame_builder;
    auto _request_builder = _frame_builder.as_request();
    _request_builder.add_ping();
    const auto _buffers = _request_builder.get_buffers();
    EXPECT_GT(_buffers.size(), 0);
}
