#!/bin/bash
cd build && ./tests --gtest_filter=tcp_server_fixture.ConnectWebSocketSendPayloadAndDisconnect > out.log 2>&1 &
sleep 2
cat build/out.log
