/* vim:set et ts=4 sts=4:
 *
 * ibus-libpinyin - Intelligent Pinyin engine based on libpinyin for IBus
 *
 * Copyright (c) 2010 Peng Wu <alexepico@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lua-plugin.h"

void print_help(){
  printf("Usage: lua_ext_console [SCRIPT_FILE] ...\n");
  printf("Loads one or more script files then evaluates lua extension modes in an interactive shell.\n");
}

void print_interactive_help(){
  printf("i \t\t\t - lists all commands.\n");
  printf("i [COMMAND] \t\t - evaluates command without argument. \n");
  printf("i [COMMAND] [ARGUMENT] \t evaluates command with argument. \n");
  printf("g [TRIGGER_STRING] \t\t - tests a trigger string, fire trigger if hit.\n");
  printf("c \t\t\t - lists all converters.\n");
  printf("c [FUNCTION] [STRING] \t tests a converter function. \n");
  printf("quit \t\t\t - quit the shell.\n");
  printf("help \t\t\t - show this message.\n");
}

void list_all_commands(IBusEnginePlugin * plugin){
  const GArray * commands = ibus_engine_plugin_get_available_commands(plugin);
  size_t i;
  for ( i = 0; i < commands->len; ++i ){
    lua_command_t * command = &g_array_index(commands, lua_command_t, i);
    printf("%s.%s >\t", command->command_name,
           command->description);
  }
  printf("\n");
}

void list_all_converters(IBusEnginePlugin * plugin){
  const GArray * converters = ibus_engine_plugin_get_available_converters(plugin);
  size_t i;
  for ( i = 0; i < converters->len; ++i ){
    lua_converter_t * converter = &g_array_index(converters, lua_converter_t, i);
    printf("%s %s >\t", converter->lua_function_name, converter->description);
  }
  printf("\n");
}

int print_lua_call_result(IBusEnginePlugin * plugin, size_t num){
  if ( 1 == num ) {
    const lua_command_candidate_t * result = ibus_engine_plugin_get_retval(plugin);
    if (result->content)
      printf("result: %s.\n", result->content);
  }
  if ( num > 1) {
    GArray * results = ibus_engine_plugin_get_retvals(plugin);
    size_t i;
    for ( i = 0; i < results->len; ++i) {
      const lua_command_candidate_t * result = g_array_index(results, const lua_command_candidate_t *, i);
      if (result->content)
          printf("%d.%s >\t", (int)i, result->content);
      else{
          printf("%d. %s [%s]\t", (int)i, result->suggest, result->help);
      }
    }
    printf("\n");
  }
  return 0;
}

int do_lua_call(IBusEnginePlugin * plugin, const char * command_name, const char * argument){
  const lua_command_t * command;
  size_t num;

  g_return_val_if_fail(2 == strlen(command_name), 2);
  command = ibus_engine_plugin_lookup_command(plugin, command_name);
  if ( NULL == command) {
    fprintf(stderr, "command %s doesn't exist.\n", command_name);
    return 1;
  }

  num = ibus_engine_plugin_call(plugin, command->lua_function_name, argument);
  print_lua_call_result(plugin, num);
  return 0;
}

int do_simple_lua_call(IBusEnginePlugin * plugin, const char * lua_function_name, const char * string){
  int i;
  int num = ibus_engine_plugin_call(plugin, lua_function_name, string);
  g_assert(num == ibus_engine_plugin_get_n_result(plugin));
  for (i = 0; i < num ; ++i){
    gchar * str = ibus_engine_plugin_get_nth_result(plugin, i);
    printf("%d.%s >\t", i, str);
    g_free(str);
  }
  printf("\n");
  ibus_engine_plugin_clear_results(plugin);
  return 0;
}


int main(int argc, char * argv[]){
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  int i;

  if ( 1 == argc ){
    print_help();
    exit(1);
  }

  IBusEnginePlugin * plugin = ibus_engine_plugin_new();

  for ( i = 1; i < argc; ++i){
    ibus_engine_plugin_load_lua_script(plugin, argv[i]);
  }

  printf("Lua Plugin Console for ibus-libpinyin.\n");
  printf("Type ? for more information.\n");
  printf("> ");

  while ((read = getline(&line, &len, stdin)) != -1) {
    line[read - 1] = '\0';
    gchar ** strs = g_strsplit_set(line, " \t", 0);
    size_t len = g_strv_length(strs);
    switch (len){
    case 0:
      print_interactive_help();
      break;
    case 1:
      if ( 0 == strcmp("quit", strs[0]) )
        exit(EXIT_SUCCESS);
      if ( 0 == strcmp("help", strs[0]) || 0 == strcmp("?", strs[0]) )
        print_interactive_help();
      if ( 0 == strcmp("i", strs[0]) )
        list_all_commands(plugin);
      if ( 0 == strcmp("c", strs[0]) )
        list_all_converters(plugin);
      break;
    case 2:
      if ( 0 == strcmp("i", strs[0]))
        do_lua_call(plugin, strs[1], NULL);
      if ( 0 == strcmp("g", strs[0])) {
          const char * lua_function_name = NULL;
          if (ibus_engine_plugin_match_input
              (plugin, strs[1], &lua_function_name)) {
              do_simple_lua_call(plugin, lua_function_name, strs[1]);
          } else if (ibus_engine_plugin_match_candidate
                     (plugin, strs[1], &lua_function_name)) {
              do_simple_lua_call(plugin, lua_function_name, strs[1]);
          }
      }
      break;
    case 3:
      if ( 0 == strcmp("i", strs[0]))
        do_lua_call(plugin, strs[1], strs[2]);
      if ( 0 == strcmp("c", strs[0]))
        do_simple_lua_call(plugin, strs[1], strs[2]);
      break;
    default:
      fprintf(stderr, "wrong arguments.");
      break;
    }
    g_strfreev(strs);
    printf("> ");
  }

  if (line)
    free(line);

  return EXIT_SUCCESS;
}
