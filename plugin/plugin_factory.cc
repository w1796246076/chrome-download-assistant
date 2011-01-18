#include "stdafx.h"

#include "download_helper_plugin.h"
#include "plugin_factory.h"

PluginFactory::PluginFactory(void) {
  Plugin_Type_Item item;
  item.mime_type = "application/x-npdownload";
  item.constructor = &DownloadHelperPlugin::CreateObject;
  plugin_type_map_.insert(PluginTypeMap::value_type(item.mime_type, item));
}

PluginFactory::~PluginFactory(void) {
}

PluginBase* PluginFactory::NewPlugin(NPMIMEType pluginType) {
  PluginBase* plugin = NULL;
  PluginTypeMap::iterator iter = plugin_type_map_.find(pluginType);
  if (iter != plugin_type_map_.end())
    plugin = iter->second.constructor();

  return plugin;
}
