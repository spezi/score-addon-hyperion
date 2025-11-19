#pragma once
#include <Device/Protocol/DeviceInterface.hpp>
#include <Device/Protocol/DeviceSettings.hpp>
#include <Device/Protocol/ProtocolFactoryInterface.hpp>
#include <Device/Protocol/ProtocolSettingsWidget.hpp>

#include <Gfx/GfxDevice.hpp>
#include <Gfx/SharedOutputSettings.hpp>

#include <QLineEdit>

#include <Hyperion/OutputSettings.hpp>

namespace Hyperion
{
class gfx_protocol_base;

class OutputFactory final : public Gfx::SharedOutputProtocolFactory
{
  SCORE_CONCRETE("a7b3c5e1-d8f2-4a9b-b6e4-2c1d8f3a5b7e")
public:
  QString prettyName() const noexcept override;
  QUrl manual() const noexcept override;

  Device::DeviceInterface* makeDevice(
      const Device::DeviceSettings& settings, const Explorer::DeviceDocumentPlugin& doc,
      const score::DocumentContext& ctx) override;
  const Device::DeviceSettings& defaultSettings() const noexcept override;

  Device::ProtocolSettingsWidget* makeSettingsWidget() override;
  QVariant makeProtocolSpecificSettings(const VisitorVariant& visitor) const override;

  void serializeProtocolSpecificSettings(
      const QVariant& data, const VisitorVariant& visitor) const override;
};
}
