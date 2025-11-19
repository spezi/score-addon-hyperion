#include "score_addon_hyperion.hpp"

#include <score/plugins/FactorySetup.hpp>
#include <score/widgets/MessageBox.hpp>

#include <QTimer>

#include <Hyperion/OutputFactory.hpp>

score_addon_hyperion::score_addon_hyperion()
{
  qRegisterMetaType<Hyperion::OutputSettings>();
}

score_addon_hyperion::~score_addon_hyperion() { }

std::vector<score::InterfaceBase*> score_addon_hyperion::factories(
    const score::ApplicationContext& ctx, const score::InterfaceKey& key) const
{
  return instantiate_factories<
      score::ApplicationContext,
      FW<Device::ProtocolFactory, Hyperion::OutputFactory>>(ctx, key);
}

#include <score/plugins/PluginInstances.hpp>
SCORE_EXPORT_PLUGIN(score_addon_hyperion)
