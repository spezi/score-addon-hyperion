#pragma once
#include <QString>

namespace Hyperion
{
struct OutputSettings
{
  QString host{"127.0.0.1"};
  int port{19400};
  int priority{150};
  QString origin{"ossiascore"};
  int width{};
  int height{};
  double rate{};
};
}
