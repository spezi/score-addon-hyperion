#include "OutputFactory.hpp"

#include <State/Widgets/AddressFragmentLineEdit.hpp>

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>

#include <Hyperion/OutputNode.hpp>

namespace Hyperion
{
class OutputSettingsWidget final : public Gfx::SharedOutputSettingsWidget
{
public:
  OutputSettingsWidget(QWidget* parent = nullptr)
      : Gfx::SharedOutputSettingsWidget{parent}
  {
    m_deviceNameEdit->setText("Hyperion Out");
    m_shmPath->setVisible(false);
    ((QLabel*)m_layout->labelForField(m_shmPath))->setVisible(false);
    
    m_host = new QLineEdit{this};
    m_host->setText("127.0.0.1");
    m_layout->addRow(tr("Host"), m_host);
    
    m_port = new QSpinBox{this};
    m_port->setRange(1, 65535);
    m_port->setValue(19400);
    m_layout->addRow(tr("Port"), m_port);
    
    m_priority = new QSpinBox{this};
    m_priority->setRange(1, 255);
    m_priority->setValue(150);
    m_layout->addRow(tr("Priority"), m_priority);
    
    m_origin = new QLineEdit{this};
    m_origin->setText("ossia score");
    m_layout->addRow(tr("Origin"), m_origin);

    setSettings(OutputFactory{}.defaultSettings());
  }

  void setSettings(const Device::DeviceSettings& settings) override
  {
    m_deviceNameEdit->setText(settings.name);

    const auto& set = settings.deviceSpecificSettings.value<OutputSettings>();
    m_host->setText(set.host);
    m_port->setValue(set.port);
    m_priority->setValue(set.priority);
    m_origin->setText(set.origin);
    m_width->setValue(set.width);
    m_height->setValue(set.height);
    m_rate->setValue(set.rate);
  }

  Device::DeviceSettings getSettings() const override
  {
    auto set = Gfx::SharedOutputSettingsWidget::getSettings();
    set.protocol = OutputFactory::static_concreteKey();

    const auto& base_s = set.deviceSpecificSettings.value<Gfx::SharedOutputSettings>();
    OutputSettings specif{
        .host = m_host->text(),
        .port = m_port->value(),
        .priority = m_priority->value(),
        .origin = m_origin->text(),
        .width = base_s.width,
        .height = base_s.height,
        .rate = base_s.rate};

    set.deviceSpecificSettings = QVariant::fromValue(std::move(specif));
    return set;
  }

  QLineEdit* m_host{};
  QSpinBox* m_port{};
  QSpinBox* m_priority{};
  QLineEdit* m_origin{};
};

Device::ProtocolSettingsWidget* OutputFactory::makeSettingsWidget()
{
  return new OutputSettingsWidget{};
}

QString OutputFactory::prettyName() const noexcept
{
  return QObject::tr("Hyperion Output");
}

QUrl OutputFactory::manual() const noexcept
{
  return QUrl("https://docs.hyperion-project.org/");
}

Device::DeviceInterface* OutputFactory::makeDevice(
    const Device::DeviceSettings& settings, const Explorer::DeviceDocumentPlugin& doc,
    const score::DocumentContext& ctx)
{
  return new OutputDevice(settings, ctx);
}

const Device::DeviceSettings& OutputFactory::defaultSettings() const noexcept
{
  static const Device::DeviceSettings settings = [&]() {
    Device::DeviceSettings s;
    s.protocol = concreteKey();
    s.name = "Hyperion Output";
    OutputSettings set;
    set.host = "127.0.0.1";
    set.port = 19400;
    set.priority = 150;
    set.origin = "ossiascore";
    set.width = 1280;
    set.height = 720;
    set.rate = 30.;
    s.deviceSpecificSettings = QVariant::fromValue(set);
    return s;
  }();
  return settings;
}

QVariant OutputFactory::makeProtocolSpecificSettings(const VisitorVariant& visitor) const
{
  return makeProtocolSpecificSettings_T<OutputSettings>(visitor);
}

void OutputFactory::serializeProtocolSpecificSettings(
    const QVariant& data, const VisitorVariant& visitor) const
{
  serializeProtocolSpecificSettings_T<OutputSettings>(data, visitor);
}
}

template <>
void DataStreamReader::read(const Hyperion::OutputSettings& n)
{
  m_stream << n.host << n.port << n.priority << n.origin;
  m_stream << n.width << n.height << n.rate;
}

template <>
void DataStreamWriter::write(Hyperion::OutputSettings& n)
{
  m_stream >> n.host >> n.port >> n.priority >> n.origin;
  m_stream >> n.width >> n.height >> n.rate;
}

template <>
void JSONReader::read(const Hyperion::OutputSettings& n)
{
  obj["Host"] = n.host;
  obj["Port"] = n.port;
  obj["Priority"] = n.priority;
  obj["Origin"] = n.origin;
  obj["Width"] = n.width;
  obj["Height"] = n.height;
  obj["Rate"] = n.rate;
}

template <>
void JSONWriter::write(Hyperion::OutputSettings& n)
{
  n.host = obj["Host"].toString();
  n.port = obj["Port"].toInt();
  n.priority = obj["Priority"].toInt();
  n.origin = obj["Origin"].toString();
  n.width = obj["Width"].toDouble();
  n.height = obj["Height"].toDouble();
  n.rate = obj["Rate"].toDouble();
}
