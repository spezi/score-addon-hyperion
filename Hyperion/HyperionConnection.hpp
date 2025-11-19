#pragma once

#include <QString>
#include <memory>

namespace Hyperion
{
struct OutputSettings;

class HyperionConnectionImpl;

class HyperionConnection
{
public:
  HyperionConnection(const OutputSettings& settings);
  ~HyperionConnection();

  // Non-copyable
  HyperionConnection(const HyperionConnection&) = delete;
  HyperionConnection& operator=(const HyperionConnection&) = delete;

  bool isConnected() const;
  void sendImage(const uint8_t* data, int width, int height, int duration = -1);

private:
  std::unique_ptr<HyperionConnectionImpl> m_impl;
};

}
