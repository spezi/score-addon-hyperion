#include <Gfx/GfxApplicationPlugin.hpp>
#include <Gfx/GfxExecContext.hpp>
#include <Gfx/GfxParameter.hpp>
#include <Gfx/Graph/RenderList.hpp>
#include <Gfx/SharedOutputSettings.hpp>

#include <score/gfx/OpenGL.hpp>

#include <ossia/network/base/device.hpp>

#include <QOffscreenSurface>
#include <QTimer>
#include <QtGui/private/qrhigles2_p.h>

#include <Hyperion/OutputNode.hpp>
#include <Hyperion/OutputSettings.hpp>
#include <Hyperion/HyperionConnection.hpp>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Hyperion::OutputDevice)

namespace Hyperion
{
struct OutputSettings;

struct OutputNode : score::gfx::OutputNode
{
  OutputNode(const Hyperion::OutputSettings& set);
  virtual ~OutputNode();

  // Non-copyable
  OutputNode(const OutputNode&) = delete;
  OutputNode& operator=(const OutputNode&) = delete;

  Hyperion::OutputSettings m_settings;
  std::weak_ptr<score::gfx::RenderList> m_renderer{};
  QRhiTexture* m_texture{};
  QRhiTextureRenderTarget* m_renderTarget{};
  std::function<void()> m_update;
  std::shared_ptr<score::gfx::RenderState> m_renderState{};
  Gfx::InvertYRenderer* m_inv_y_renderer{};
  QRhiReadbackResult m_readback[2];
  QRhiReadbackResult* m_currentReadback{&m_readback[0]};
  std::unique_ptr<HyperionConnection> m_connection;

  void startRendering() override;
  void render() override;
  void onRendererChange() override;
  bool canRender() const override;
  void stopRendering() override;

  void setRenderer(std::shared_ptr<score::gfx::RenderList>) override;
  score::gfx::RenderList* renderer() const override;

  void createOutput(
      score::gfx::GraphicsApi graphicsApi, std::function<void()> onReady,
      std::function<void()> onUpdate, std::function<void()> onResize) override;
  void destroyOutput() override;

  std::shared_ptr<score::gfx::RenderState> renderState() const override;
  score::gfx::OutputNodeRenderer*
  createRenderer(score::gfx::RenderList& r) const noexcept override;

  Configuration configuration() const noexcept override;
};

class hyperion_output_device : public ossia::net::device_base
{
  Gfx::gfx_node_base root;

public:
  hyperion_output_device(
      const Hyperion::OutputSettings& set,
      std::unique_ptr<ossia::net::protocol_base> proto, std::string name)
      : ossia::net::device_base{std::move(proto)}
      , root{
            *this, *static_cast<Gfx::gfx_protocol_base*>(m_protocol.get()),
            new OutputNode{set}, name}
  {
  }

  // Non-copyable
  hyperion_output_device(const hyperion_output_device&) = delete;
  hyperion_output_device& operator=(const hyperion_output_device&) = delete;

  const Gfx::gfx_node_base& get_root_node() const override { return root; }
  Gfx::gfx_node_base& get_root_node() override { return root; }
};

OutputNode::OutputNode(const Hyperion::OutputSettings& set)
    : score::gfx::OutputNode{}
    , m_settings{set}
{
  input.push_back(new score::gfx::Port{this, {}, score::gfx::Types::Image, {}});
}

OutputNode::~OutputNode()
{
  m_connection.reset();
}

bool OutputNode::canRender() const
{
  return bool(m_renderState);
}

void OutputNode::startRendering() 
{
  m_connection = std::make_unique<HyperionConnection>(m_settings);
}

void OutputNode::render()
{
  if(m_update)
    m_update();

  auto renderer = m_renderer.lock();
  if(renderer && m_renderState)
  {
    auto rhi = m_renderState->rhi;
    QRhiCommandBuffer* cb{};
    if(rhi->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess)
      return;

    renderer->render(*cb);

    rhi->endOffscreenFrame();

    // Send the readback to Hyperion
    if(m_connection)
    {
      auto& readback = *m_currentReadback;
      auto width = readback.pixelSize.width();
      auto height = readback.pixelSize.height();

      if(width > 0 && height > 0 && !readback.data.isEmpty())
      {
        m_connection->sendImage(
            reinterpret_cast<const uint8_t*>(readback.data.constData()),
            width, height);
      }
    }
    
    // Swap readback buffers
    if(m_currentReadback == m_readback + 0)
      m_currentReadback = m_readback + 1;
    else
      m_currentReadback = m_readback + 0;
    
    if(m_inv_y_renderer)
      m_inv_y_renderer->updateReadback(*m_currentReadback);
  }
}

score::gfx::OutputNode::Configuration OutputNode::configuration() const noexcept
{
  return {.manualRenderingRate = 1000. / m_settings.rate};
}

void OutputNode::onRendererChange() { }

void OutputNode::stopRendering() 
{
  m_connection.reset();
}

void OutputNode::setRenderer(std::shared_ptr<score::gfx::RenderList> r)
{
  m_renderer = r;
}

score::gfx::RenderList* OutputNode::renderer() const
{
  return m_renderer.lock().get();
}

void OutputNode::createOutput(
    score::gfx::GraphicsApi graphicsApi, std::function<void()> onReady,
    std::function<void()> onUpdate, std::function<void()> onResize)
{
  m_renderState = std::make_shared<score::gfx::RenderState>();
  m_update = onUpdate;

  m_renderState->surface = QRhiGles2InitParams::newFallbackSurface();
  QRhiGles2InitParams params;
  params.fallbackSurface = m_renderState->surface;
  score::GLCapabilities caps;
  caps.setupFormat(params.format);
  m_renderState->rhi = QRhi::create(QRhi::OpenGLES2, &params, {});
  m_renderState->renderSize = QSize(m_settings.width, m_settings.height);
  m_renderState->outputSize = m_renderState->renderSize;
  m_renderState->api = score::gfx::GraphicsApi::OpenGL;
  m_renderState->version = caps.qShaderVersion;

  auto rhi = m_renderState->rhi;
  m_texture = rhi->newTexture(
      QRhiTexture::RGBA8, m_renderState->renderSize, 1,
      QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
  m_texture->create();
  m_renderTarget = rhi->newTextureRenderTarget({m_texture});
  m_renderState->renderPassDescriptor
      = m_renderTarget->newCompatibleRenderPassDescriptor();
  m_renderTarget->setRenderPassDescriptor(m_renderState->renderPassDescriptor);
  m_renderTarget->create();

  onReady();
}

void OutputNode::destroyOutput() { }

std::shared_ptr<score::gfx::RenderState> OutputNode::renderState() const
{
  return m_renderState;
}

score::gfx::OutputNodeRenderer*
OutputNode::createRenderer(score::gfx::RenderList& r) const noexcept
{
  score::gfx::TextureRenderTarget rt{
      m_texture, nullptr, nullptr, m_renderState->renderPassDescriptor, m_renderTarget};
  return const_cast<Gfx::InvertYRenderer*&>(m_inv_y_renderer) = new Gfx::InvertYRenderer{
             *this, rt, const_cast<QRhiReadbackResult&>(m_readback[0])};
}

OutputDevice::OutputDevice(
    const Device::DeviceSettings& settings, const score::DocumentContext& ctx)
    : Gfx::GfxOutputDevice{settings, ctx}
{
}

OutputDevice::~OutputDevice() { }

void OutputDevice::disconnect()
{
  GfxOutputDevice::disconnect();
  auto prev = std::move(m_dev);
  m_dev = {};
  deviceChanged(prev.get(), nullptr);
}

bool OutputDevice::reconnect()
{
  disconnect();

  try
  {
    auto plug = m_ctx.findPlugin<Gfx::DocumentPlugin>();
    if(plug)
    {
      auto set = m_settings.deviceSpecificSettings.value<Hyperion::OutputSettings>();

      m_protocol = new Gfx::gfx_protocol_base{plug->exec};
      m_dev = std::make_unique<hyperion_output_device>(
          set, std::unique_ptr<ossia::net::protocol_base>(m_protocol),
          m_settings.name.toStdString());
      deviceChanged(nullptr, m_dev.get());
    }
  }
  catch(std::exception& e)
  {
    qDebug() << "Could not connect: " << e.what();
  }
  catch(...)
  {
    // TODO save the reason of the non-connection.
  }

  return connected();
}
}
