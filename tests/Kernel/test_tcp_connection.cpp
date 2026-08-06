// Phase 43f — TCP connection unit tests (host-side).
//
// TcpConnection is a plain data struct (no scheduler calls, no blocking I/O).
// TcpHeader::fill() is a pure function over packed bytes.
//
// Tests cover:
//   - TcpState enum values (spot-checks)
//   - TcpConnection default state after construction
//   - matches() correct and incorrect endpoint combinations
//   - recv_window() returns CircularBuffer available space (full buffer = 65535)
//   - TcpHeader::fill() field encoding (network byte-order)
//   - TcpHeader::header_length() = 20 bytes
//   - Protocol constants: MAX_RETRANSMITS, RTO_TICKS, TCP_RECV_BUFFER_SIZE

#include <tests/test_framework.h>
#include <Kernel/Net/Tcp/tcp_connection.h>
#include <Kernel/Net/Tcp/tcp_header.h>
#include <Kernel/Net/Tcp/tcp_state.h>
#include <Kernel/Net/Tcp/tcp_endpoint.h>
#include <Kernel/Net/Ip/ip_address.h>
#include <LibFK/Algorithms/Generic/byte_order.h>

using fkernel::net::TcpState;
using fkernel::net::TcpConnection;
using fkernel::net::TcpHeader;
using fkernel::net::TcpEndpoint;
using fkernel::net::TCP_RECV_BUFFER_SIZE;
using fkernel::net::IPv4Address;
using fk::algorithms::htons;
using fk::algorithms::htonl;
using fk::algorithms::ntohs;
using fk::algorithms::ntohl;

// Build a TcpEndpoint from dotted-decimal notation
static TcpEndpoint make_endpoint(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port) {
    TcpEndpoint ep;
    ep.ip   = IPv4Address(a, b, c, d);
    ep.port = port;
    return ep;
}

static const char* test_tcpstate_enum_values() {
    // Spot-check a few state values; ordering matters for state machines.
    TEST_ASSERT(static_cast<uint8_t>(TcpState::Closed)       == 0,  "Closed must be 0");
    TEST_ASSERT(static_cast<uint8_t>(TcpState::Listen)       == 1,  "Listen must be 1");
    TEST_ASSERT(static_cast<uint8_t>(TcpState::Established)  == 4,  "Established must be 4");
    return nullptr;
}

static const char* test_connection_default_state() {
    TcpEndpoint local  = make_endpoint(127, 0, 0, 1, 8080);
    TcpEndpoint remote = make_endpoint(192, 168, 1, 1, 54321);
    TcpConnection conn(local, remote);

    TEST_ASSERT(conn.state       == TcpState::Closed, "default state must be Closed");
    TEST_ASSERT_EQ(0, (long)conn.send_next,           "send_next must start at 0");
    TEST_ASSERT_EQ(0, (long)conn.recv_next,           "recv_next must start at 0");
    TEST_ASSERT_EQ(0, (long)conn.retransmit_count,    "retransmit_count must start at 0");
    return nullptr;
}

static const char* test_connection_endpoint_accessors() {
    TcpEndpoint local  = make_endpoint(10, 0, 0, 1, 9000);
    TcpEndpoint remote = make_endpoint(10, 0, 0, 2, 80);
    TcpConnection conn(local, remote);

    TEST_ASSERT_EQ(9000, (long)conn.local().port,  "local port must be 9000");
    TEST_ASSERT_EQ(80,   (long)conn.remote().port, "remote port must be 80");
    return nullptr;
}

static const char* test_matches_correct_endpoints() {
    TcpEndpoint local  = make_endpoint(10, 0, 0, 1, 80);
    TcpEndpoint remote = make_endpoint(192, 168, 1, 5, 55000);
    TcpConnection conn(local, remote);

    // matches(src_ip, src_port, dst_ip, dst_port) where src = remote, dst = local
    bool ok = conn.matches(remote.ip.to_network(), htons(55000),
                           local.ip.to_network(),  htons(80));
    TEST_ASSERT(ok, "matches() must return true for correct 4-tuple");
    return nullptr;
}

static const char* test_matches_wrong_src_port() {
    TcpEndpoint local  = make_endpoint(10, 0, 0, 1, 80);
    TcpEndpoint remote = make_endpoint(192, 168, 1, 5, 55000);
    TcpConnection conn(local, remote);

    bool ok = conn.matches(remote.ip.to_network(), htons(12345),  // wrong src port
                           local.ip.to_network(),  htons(80));
    TEST_ASSERT(!ok, "matches() must return false for wrong src port");
    return nullptr;
}

static const char* test_matches_wrong_dst_port() {
    TcpEndpoint local  = make_endpoint(10, 0, 0, 1, 80);
    TcpEndpoint remote = make_endpoint(192, 168, 1, 5, 55000);
    TcpConnection conn(local, remote);

    bool ok = conn.matches(remote.ip.to_network(), htons(55000),
                           local.ip.to_network(),  htons(443));  // wrong dst port
    TEST_ASSERT(!ok, "matches() must return false for wrong dst port");
    return nullptr;
}

static const char* test_recv_window_full_buffer() {
    TcpEndpoint local  = make_endpoint(127, 0, 0, 1, 8080);
    TcpEndpoint remote = make_endpoint(127, 0, 0, 1, 9090);
    TcpConnection conn(local, remote);

    // Empty recv_buf → all space available; capped at 65535.
    uint16_t win = conn.recv_window();
    TEST_ASSERT(win > 0, "recv_window must be > 0 on empty buffer");
    TEST_ASSERT(win <= 65535, "recv_window must be capped at 65535");
    return nullptr;
}

static const char* test_tcp_header_fill() {
    TcpHeader hdr{};
    hdr.fill(/*src=*/8080, /*dst=*/80, /*seq=*/1000, /*ack=*/500,
             /*flags=*/fkernel::net::TCP_FLAG_SYN, /*win=*/4096);

    TEST_ASSERT_EQ(htons(8080), (long)hdr.src_port, "src_port must be in network order");
    TEST_ASSERT_EQ(htons(80),   (long)hdr.dst_port, "dst_port must be in network order");
    TEST_ASSERT_EQ(htonl(1000), (long)hdr.seq_num,  "seq_num must be in network order");
    TEST_ASSERT_EQ(htonl(500),  (long)hdr.ack_num,  "ack_num must be in network order");
    TEST_ASSERT_EQ(fkernel::net::TCP_FLAG_SYN, (long)hdr.flags, "flags must be set");
    TEST_ASSERT_EQ(htons(4096), (long)hdr.window,   "window must be in network order");
    TEST_ASSERT_EQ(0,           (long)hdr.checksum, "checksum must start at 0");
    return nullptr;
}

static const char* test_tcp_header_length() {
    TcpHeader hdr{};
    hdr.fill(1234, 80, 0, 0, 0, 1024);
    TEST_ASSERT_EQ(20, (long)hdr.header_length(),
                   "standard TCP header must be 20 bytes (5 words × 4)");
    return nullptr;
}

static const char* test_protocol_constants() {
    TEST_ASSERT_EQ(5,     (long)TcpConnection::MAX_RETRANSMITS,
                   "MAX_RETRANSMITS must be 5");
    TEST_ASSERT_EQ(1000,  (long)TcpConnection::RTO_TICKS,
                   "RTO_TICKS must be 1000");
    TEST_ASSERT_EQ(65536, (long)TCP_RECV_BUFFER_SIZE,
                   "TCP_RECV_BUFFER_SIZE must be 65536");
    return nullptr;
}

static test_case_t s_tests[] = {
    {"tcpstate_enum_values",        test_tcpstate_enum_values},
    {"connection_default_state",    test_connection_default_state},
    {"connection_endpoint_accessors", test_connection_endpoint_accessors},
    {"matches_correct_endpoints",   test_matches_correct_endpoints},
    {"matches_wrong_src_port",      test_matches_wrong_src_port},
    {"matches_wrong_dst_port",      test_matches_wrong_dst_port},
    {"recv_window_full_buffer",     test_recv_window_full_buffer},
    {"tcp_header_fill",             test_tcp_header_fill},
    {"tcp_header_length",           test_tcp_header_length},
    {"protocol_constants",          test_protocol_constants},
};

int run_kernel_tcp_connection_tests() {
    return run_tests("Kernel::TcpConnection",
                     s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
