#include <gtest/gtest.h>
#include <aurum/protocol/frame_builder.hpp>

TEST(FrameBuilderTest, CanBuildRequest) {
    aurum::protocol::frame_builder builder;
    auto req_builder = builder.as_request();
    req_builder.add_ping();
    auto buffers = req_builder.get_buffers();
    EXPECT_GT(buffers.size(), 0);
}
