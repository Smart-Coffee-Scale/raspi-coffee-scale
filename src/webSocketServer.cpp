// #include "HX711.h"
#include <iostream>
#include <unistd.h>
#include <libwebsockets.h>
#include <atomic>
#include <mutex>
#include <vector>
#include <thread>
#include <chrono>
#include <signal.h>

// // Constants
// const int kDataPin = 6; // placeholder for now
// const int kClockPin = 5;
// const int kSampleTimes = 10;

// Forward declaration
static int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

// Global variables for server management
static std::atomic<bool> m_running{true};
static struct lws *m_client_wsi = nullptr; // single client connection

struct per_session_data
{
    bool ready_to_send;
};

// define protocol
static struct lws_protocols protocols[] = {
    {"websocket-protocol",
     callback_websocket,
     sizeof(struct per_session_data),
     1024,
     0, NULL, 0},
    {NULL, NULL, 0, 0}};

// send message to the single client
bool send_to_client(const std::string &message)
{
    if (!m_client_wsi)
    {
        return false;
    }

    size_t msg_len = message.length();
    unsigned char *buf = new unsigned char[LWS_PRE + msg_len];

    memcpy(&buf[LWS_PRE], message.c_str(), msg_len);

    int result = lws_write(m_client_wsi, &buf[LWS_PRE], msg_len, LWS_WRITE_TEXT);

    delete[] buf;

    if (result < 0)
    {
        std::cout << "Error sending message to client" << std::endl;
        return false;
    }

    return true;
}

static int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    struct per_session_data *sessionData = (struct per_session_data *)user;

    switch (reason)
    {
    case LWS_CALLBACK_ESTABLISHED:
        if (m_client_wsi != nullptr)
        {
            std::cout << "Warning: New client trying to connect, but we already have one. Rejecting." << std::endl;
            return -1; // Reject the connection
        }

        std::cout << "Client connected" << std::endl;
        m_client_wsi = wsi;
        sessionData->ready_to_send = true;
        break;

    case LWS_CALLBACK_CLOSED:
        std::cout << "Client disconnected" << std::endl;
        m_client_wsi = nullptr;
        break;

    case LWS_CALLBACK_RECEIVE:
    {
        std::string received_msg((char *)in, len);
        std::cout << "Received message: " << received_msg << std::endl;

        // echo a response message
        std::string echo_msg = "Echo: " + received_msg;
        if (!send_to_client(echo_msg))
        {
            std::cout << "Failed to echo message back" << std::endl;
            return -1;
        }
    }
    break;

    case LWS_CALLBACK_SERVER_WRITEABLE:
        sessionData->ready_to_send = true;
        break;

    default:
        break;
    }

    return 0;
}

// Periodically sending scale data(in a different thread)
void periodic_broadcast()
{
    while (m_running)
    {
        // send data every 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));

        if (m_running && m_client_wsi)
        {
            std::string default_data = "scale data"; // placeholder for now
            std::cout << "sending to client " << default_data << std::endl;

            if (!send_to_client(default_data))
            {
                std::cout << "failed to broadcast data to client" << std::endl;
            }
            else
            {
                std::cout << "successfully broadcast data to client" << std::endl;
            }
        }
        else if (m_running && !m_client_wsi)
        {
            std::cout << "No client connected for now, skipping broadcast" << std::endl;
        }
    }
}

// Signal handler for graceful shutdown
void signal_handler(int sig)
{
    std::cout << "\nShutting down websocket server..." << std::endl;
    m_running = false;
}

int main()
{
    // HX711 code use
    // if (gpioInitialise() < 0)
    // {
    //     std::cerr << "Failed to init pigpio" << std::endl;
    //     return 1;
    // }

    // HX711 scale(kDataPin, kClockPin);
    // scale.scaleInitialise();

    // while (true)
    // {
    //     float weight = scale.readWeight(kSampleTimes);
    //     std::cout << "Weight: " << weight << "g" << std::endl;
    //     usleep(500000); // 0.5s
    // }

    // gpioTerminate();

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = 9002;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;

    struct lws_context *context = lws_create_context(&info);
    if (!context)
    {
        std::cerr << "Failed to create libwebsockets context" << std::endl;
        return 1;
    }

    std::cout << "Single-client WebSocket server started on port " << info.port << std::endl;
    std::cout << "Waiting for client connection..." << std::endl;
    std::cout << "Will broadcast sensor data every 5 seconds once connected" << std::endl;

    // Start a separate thread for periodically broadcasting data
    std::thread broadcast_thread(periodic_broadcast);

    // Main event loop
    while (m_running)
    {
        int result = lws_service(context, 50);

        if (result < 0)
        {
            std::cerr << "lws_service failed" << std::endl;
            break;
        }
    }

    // Cleanup
    std::cout << "Cleaning up..." << std::endl;
    m_running = false;

    if (broadcast_thread.joinable())
    {
        broadcast_thread.join();
    }

    lws_context_destroy(context);

    std::cout << "Server shut down successfully" << std::endl;

    return 0;
}
