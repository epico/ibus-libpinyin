/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2010 Peng Wu <alexepico@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <stdio.h>
#include <glib.h>

#include "lua-plugin.h"

int main(int argc, char * argv[]){
  printf("starting test...\n");

  g_type_init();
  
  IBusEnginePlugin * plugin;
  plugin = ibus_engine_plugin_new();

  ibus_engine_plugin_load_lua_script(plugin, LUASCRIPTDIR G_DIR_SEPARATOR_S "test.lua");
  
  g_object_unref(plugin);

  printf("done.\n");
  return 0;
}
