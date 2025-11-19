#pragma once
#include <score/application/ApplicationContext.hpp>
#include <score/command/Command.hpp>
#include <score/command/CommandGeneratorMap.hpp>
#include <score/plugins/InterfaceList.hpp>
#include <score/plugins/qt_interfaces/CommandFactory_QtInterface.hpp>
#include <score/plugins/qt_interfaces/FactoryFamily_QtInterface.hpp>
#include <score/plugins/qt_interfaces/FactoryInterface_QtInterface.hpp>
#include <score/plugins/qt_interfaces/GUIApplicationPlugin_QtInterface.hpp>
#include <score/plugins/qt_interfaces/PluginRequirements_QtInterface.hpp>

#include <utility>
#include <vector>

class score_addon_hyperion final : public score::Plugin_QtInterface,
                                   public score::FactoryInterface_QtInterface
{
  SCORE_PLUGIN_METADATA(1, "b8c4e5a1-f3d2-4e6b-a9c8-7d1e3f5b2a4c")

public:
  score_addon_hyperion();
  ~score_addon_hyperion() override;

private:
  std::vector<score::InterfaceBase*> factories(
      const score::ApplicationContext& ctx,
      const score::InterfaceKey& key) const override;
};
