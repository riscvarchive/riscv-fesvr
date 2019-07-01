// See LICENSE for license details.

#include "option_parser.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <utility>

option_parser_t::option_parser_t(std::initializer_list<option_parser_t::option_t>&& opts)
  : opts(std::move(opts)), helpmsg(nullptr)
{}

option_parser_t::option_parser_t(std::initializer_list<option_t>&& opts, std::function<void(void)> helpmsg)
  : opts(std::move(opts)), helpmsg(helpmsg)
{}

enum class argument_type_t
{
  character_arg,
  string_arg,
  unrecognized
};

const char* const* option_parser_t::parse(char** argv)
{
  assert(argv);
  char **app_name = argv;
  while ((++argv)[0] && argv[0][0] == '-')
  {
    char* argument = argv[0];
    bool found = false;
    for (auto it = opts.begin(); it != opts.end(); it++)
    {
      auto match = argument_type_t::unrecognized;
      if (argument[1] != '-' && it->chr && argument[1] == it->chr)
      {
        argument = &argument[2];
        match = argument_type_t::character_arg;
      }
      else if (argument[1] == '-' && !it->str.empty() && strncmp(argument+2, it->str.c_str(), it->str.length()) == 0)
      {
        argument = &argument[2 + it->str.length()];
        match = argument_type_t::string_arg;
      }
      else
      {
        continue;
      }

      if (!it->arg && !*argument)
      {
        it->func(nullptr);
      }
      else if (match == argument_type_t::character_arg && it->arg && *argument)
      {
        it->func(argument);
      }
      else if (match == argument_type_t::string_arg && it->arg && *argument == '=' && ++argument)
      {
        it->func(argument);
      }
      else
      {
        const char* option = (match == argument_type_t::character_arg) ? &it->chr : it->str.c_str();
        if (it->arg)
        {
          error("argument required for option", app_name[0], option);
        }
        else if (!it->arg)
        {
          error("no argument allowed for option", app_name[0], option);
        }
      }

      found = true;
      break;
    }

    if (!found)
      error("unrecognized option", argv[0], nullptr);
  }

  return argv;
}

void option_parser_t::error(const char* msg, const char* argv0, const char* arg)
{
  fprintf(stderr, "%s: %s %s\n", argv0, msg, arg ? arg : "");
  if (helpmsg) helpmsg();
  exit(1);
}
