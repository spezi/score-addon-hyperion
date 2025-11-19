// FlatBuffers implementation for Hyperion protocol
// Uses POSIX sockets directly for reliable operation in render thread

#include "HyperionConnection.hpp"
#include "OutputSettings.hpp"

#include <QDebug>

#include <flatbuffers/flatbuffers.h>
#include "hyperion_request_generated.h"

#include <vector>
#include <memory>
#include <cstring>

// POSIX socket includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace Hyperion
{

class HyperionConnectionImpl
{
public:
  explicit HyperionConnectionImpl(const OutputSettings& settings)
      : m_settings{settings}
      , m_builder{new flatbuffers::FlatBufferBuilder()}
  {
    doConnect();
  }

  ~HyperionConnectionImpl()
  {
    if(m_connected)
      sendClear();
    
    if(m_socket >= 0)
    {
      ::close(m_socket);
      m_socket = -1;
    }
    
    delete m_builder;
  }
  
  HyperionConnectionImpl(const HyperionConnectionImpl&) = delete;
  HyperionConnectionImpl& operator=(const HyperionConnectionImpl&) = delete;

  bool isConnected() const { return m_connected; }

  void doConnect()
  {
    m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if(m_socket < 0)
    {
      qWarning() << "Hyperion: Failed to create socket:" << strerror(errno);
      return;
    }
    
    // Set TCP_NODELAY for immediate sending
    int flag = 1;
    setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    
    // Set send buffer size
    int bufSize = 1024 * 1024; // 1MB
    setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, &bufSize, sizeof(bufSize));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_settings.port);
    
    if(inet_pton(AF_INET, m_settings.host.toStdString().c_str(), &addr.sin_addr) <= 0)
    {
      qWarning() << "Hyperion: Invalid address:" << m_settings.host;
      ::close(m_socket);
      m_socket = -1;
      return;
    }
    
    if(::connect(m_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
      qWarning() << "Hyperion: Failed to connect to" << m_settings.host << ":" << m_settings.port << "-" << strerror(errno);
      ::close(m_socket);
      m_socket = -1;
      return;
    }
    
    m_connected = true;
    qDebug() << "Hyperion: Connected to" << m_settings.host << ":" << m_settings.port;
    
    sendRegister();
  }

  void sendRegister()
  {
    auto origin = m_builder->CreateString(m_settings.origin.toStdString());
    auto registerReq = hyperionnet::CreateRegister(*m_builder, origin, m_settings.priority);
    auto req = hyperionnet::CreateRequest(*m_builder, hyperionnet::Command_Register, registerReq.Union());
    
    m_builder->Finish(req);
    
    qDebug() << "Hyperion: Sending Register command, size:" << m_builder->GetSize() 
             << "origin:" << m_settings.origin << "priority:" << m_settings.priority;
    
    sendFlatBuffer();
    m_builder->Clear();
  }

  void sendClear()
  {
    auto clearReq = hyperionnet::CreateClear(*m_builder, m_settings.priority);
    auto req = hyperionnet::CreateRequest(*m_builder, hyperionnet::Command_Clear, clearReq.Union());
    
    m_builder->Finish(req);
    sendFlatBuffer();
    m_builder->Clear();
  }

  void sendImage(const uint8_t* data, int width, int height, int duration)
  {
    if(!m_connected || m_socket < 0)
      return;

    if(width <= 0 || height <= 0 || !data)
      return;

    // Log first frame and then every 100th frame
    if(m_frameCount == 0 || m_frameCount % 100 == 0)
    {
      qDebug() << "Hyperion: Sending image frame" << m_frameCount << "size:" << width << "x" << height;
    }
    m_frameCount++;

    // Convert RGBA to RGB (strip alpha channel)
    size_t pixelCount = width * height;
    size_t rgbSize = pixelCount * 3;
    
    // Reuse buffer to avoid repeated allocations
    if(m_rgbBuffer.size() < rgbSize)
      m_rgbBuffer.resize(rgbSize);
    
    uint8_t* dst = m_rgbBuffer.data();
    const uint8_t* src = data;
    
    for(size_t i = 0; i < pixelCount; ++i)
    {
      dst[0] = src[0]; // R
      dst[1] = src[1]; // G
      dst[2] = src[2]; // B
      dst += 3;
      src += 4;
    }

    auto imgData = m_builder->CreateVector(m_rgbBuffer.data(), rgbSize);
    auto rawImg = hyperionnet::CreateRawImage(*m_builder, imgData, width, height);
    auto imageReq = hyperionnet::CreateImage(*m_builder, hyperionnet::ImageType_RawImage, rawImg.Union(), duration);
    auto req = hyperionnet::CreateRequest(*m_builder, hyperionnet::Command_Image, imageReq.Union());
    
    m_builder->Finish(req);
    sendFlatBuffer();
    m_builder->Clear();
  }

private:
  void sendFlatBuffer()
  {
    if(m_socket < 0)
      return;
      
    uint32_t size = m_builder->GetSize();
    
    if(size == 0)
      return;
    
    // Size prefix (4 bytes, big-endian)
    uint8_t header[4];
    header[0] = (size >> 24) & 0xFF;
    header[1] = (size >> 16) & 0xFF;
    header[2] = (size >> 8) & 0xFF;
    header[3] = size & 0xFF;
    
    // Send header
    if(!sendAll(header, 4))
    {
      handleDisconnect();
      return;
    }
    
    // Send data
    if(!sendAll(m_builder->GetBufferPointer(), size))
    {
      handleDisconnect();
      return;
    }
  }
  
  bool sendAll(const uint8_t* data, size_t len)
  {
    size_t sent = 0;
    while(sent < len)
    {
      ssize_t n = ::send(m_socket, data + sent, len - sent, MSG_NOSIGNAL);
      if(n <= 0)
      {
        if(errno == EINTR)
          continue;
        qWarning() << "Hyperion: Send failed:" << strerror(errno);
        return false;
      }
      sent += n;
    }
    return true;
  }
  
  void handleDisconnect()
  {
    if(m_connected)
    {
      qDebug() << "Hyperion: Disconnected";
      m_connected = false;
    }
    if(m_socket >= 0)
    {
      ::close(m_socket);
      m_socket = -1;
    }
  }

  OutputSettings m_settings;
  int m_socket{-1};
  bool m_connected{false};
  int m_frameCount{0};
  std::vector<uint8_t> m_rgbBuffer;
  flatbuffers::FlatBufferBuilder* m_builder;
};

// Public interface

HyperionConnection::HyperionConnection(const OutputSettings& settings)
    : m_impl{std::make_unique<HyperionConnectionImpl>(settings)}
{
}

HyperionConnection::~HyperionConnection() = default;

bool HyperionConnection::isConnected() const
{
  return m_impl->isConnected();
}

void HyperionConnection::sendImage(const uint8_t* data, int width, int height, int duration)
{
  m_impl->sendImage(data, width, height, duration);
}

}
